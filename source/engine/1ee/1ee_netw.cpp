#include "1ee_hdrs.h"

//-----------------------------------------------------------------------------
// class Net constants
//-----------------------------------------------------------------------------

#define NET_VERSION 3

// Net class modes
static const int net_Initialized = 0x00000001;
static const int net_Opened      = 0x00000002;
static const int net_InGame      = 0x00000004;
static const int net_Locked      = 0x00000008;
static const int net_Debug       = 0x00000010;
static const int net_ProvListed  = 0x00000020;
static const int net_ExtraDebug  = 0x00000040;
static const int net_InfoDone    = 0x00000080;

// message types                                                                               
static const unsigned char  msg_None       = 0x00;
static const unsigned char  msg_User       = 0x01;
static const unsigned char  msg_Request    = 0x02;
static const unsigned char  msg_Resend     = 0x04;
static const unsigned char  msg_PrevResend = 0x08;
static const unsigned char  msg_Purge      = 0x10;
static const unsigned char  msg_Ping       = 0x20;

static const unsigned char  msgmask_Ignore = (msg_Request|msg_Resend|msg_Purge|msg_Ping);
//-----------------------------------------------------------------------------
// class Net static variables
//-----------------------------------------------------------------------------

HANDLE   Net::event = NULL;
unsigned Net::diag_total_out(0);
unsigned Net::diag_total_in(0);
unsigned Net::diag_resend(0);
unsigned Net::diag_max_packet(0);
unsigned Net::diag_total_size(0);
unsigned Net::diag_max_buf_usage(0);
unsigned Net::diag_trans_errors(0);
unsigned Net::diag_corrupted_packets(0);

char     Net::rec_buf[rec_buf_size];
unsigned Net::rec_size(0);
DPID     Net::rec_from(0);

Array<Net::Game>     Net::games;    
Array<Net::Provider> Net::providers;
Array<Net::Player>   Net::players;
int                  Net::games_qty(0);
int                  Net::providers_qty(0);
int                  Net::players_qty(0);
Bitflag              Net::status(0);
GUID                 Net::game_guid;
LPDIRECTPLAY         Net::driver=NULL;
int                  Net::enum_error(0);
DPSESSIONDESC        Net::current_session;
Net::Game            Net::current_game;
Net::Player          Net::current_player;
BOOL                 Net::session_OK(0);
DPCAPS               Net::dpcaps;
Array<Net::Buffer>  *Net::msgbuf;
int                  Net::enum_retries(0); 
int                  Net::enum_timeout(0); 

// {F7C26421-453C-11cf-8AE0-444553540000}
static const GUID TEST_GUID = 
{ 0xf7c26421, 0x453c, 0x11cf, { 0x8a, 0xe0, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0 } };

//-----------------------------------------------------------------------------
// Special debugging features
//-----------------------------------------------------------------------------

#define EXTRA_DEBUG 1

#if EXTRA_DEBUG 
  #define EXDBG_MESSAGE     if (status.is(net_ExtraDebug)) MESSAGE
  #define EXDBG_WARNING     if (status.is(net_ExtraDebug)) WARNING
  #define EXDBG_CHECK(expr) if (status.is(net_ExtraDebug)) CHECK(expr)
  #define EXDBG_FAILURE     if (status.is(net_ExtraDebug)) FAILURE 
  #define EXDBG_SYSINFO     if (status.is(net_ExtraDebug)) SYSINFO
  #define EXDBG_ENGINFO     if (status.is(net_ExtraDebug)) ENGINFO
#else        
  #define EXDBG_MESSAGE     (void)
  #define EXDBG_WARNING     (void)
  #define EXDBG_CHECK(expr) (void(0))
  #define EXDBG_FAILURE     (void)
  #define EXDBG_SYSINFO     (void)
  #define EXDBG_ENGINFO     (void)
#endif
//-----------------------------------------------------------------------------
// class Net private members
//-----------------------------------------------------------------------------
BOOL Net::enum_players   (DPID id, LPSTR friendly, LPSTR formal, DWORD flags, LPVOID user)
{
  (void)user;
  (void)flags;
  (void)friendly;
  if (id!=0)
  {
    if (players_qty==max_enum_players)
    {
      enum_error = res_enum_overflow;
      return (FALSE);
    }
    strncpy(players[players_qty].name, formal, player_name_len);
    players[players_qty].name[player_name_len-1]=0;
    players[players_qty].id = id;
    players_qty++;
  }
  return(TRUE);
}
//-----------------------------------------------------------------------------
BOOL Net::enum_games     (LPDPSESSIONDESC game, LPVOID user, LPDWORD timeout, DWORD flags)
{
  (void)timeout;
  if (flags==DPESC_TIMEDOUT)
  {
    if ((user!=NULL)&&(((Timeout_function)user)())&&(enum_retries>0))
    {
      enum_retries--;
      *timeout = enum_timeout;
      return(TRUE);
    }
    else
    {
      enum_error = res_enum_timeout;
      return(FALSE);
    }
  }
  else
  {
    games[games_qty].id = game->dwSession;
    strncpy (games[games_qty].name, game->szSessionName, game_name_len-1);
    memcpy(&(games[games_qty].info[0]),  &game->szUserField, 16);
    memcpy(&(games[games_qty].info[16]), &game->dwUser1,      4);
    memcpy(&(games[games_qty].info[20]), &game->dwUser2,      4);
    memcpy(&(games[games_qty].info[24]), &game->dwUser3,      4);
    games[games_qty].max_players = game->dwUser4;
    games_qty++;
    if (games_qty==max_enum_games)
    {
      enum_error = res_enum_overflow;
      return (FALSE);
    }
    else
      return(TRUE);
  }
}
//-----------------------------------------------------------------------------
BOOL Net::enum_providers (LPGUID id, LPSTR name, DWORD major_v, DWORD minor_v, LPVOID user)
{
  (void)major_v;
  (void)minor_v;
  (void)user;
  providers[providers_qty].id = *id;
  strncpy (providers[providers_qty].name, name, provider_name_len-1);
  providers_qty++;
  if (providers_qty==max_enum_providers)
  {
    enum_error = res_enum_overflow;
    return (FALSE);
  }
  else
    return(TRUE);
}
//-----------------------------------------------------------------------------
// class Net public members
//-----------------------------------------------------------------------------
void Net::quit (void)
{
  if (status.is(net_InGame))
  {
    driver->DestroyPlayer(current_player.id);
    driver->Close();
  }
  if (status.is(net_Opened))
    driver->Release();
  providers.free();
  games.free();
  players.free();
  for (int i=0; i<max_enum_players; i++)
  {
    (*msgbuf)[i].in.free();
    (*msgbuf)[i].out.free();
    (*msgbuf)[i].requests.free ();
    (*msgbuf)[i].resends.free ();
  }
  msgbuf->free();
  delete (msgbuf);
  double res_percent;
  double buf_percent;
  unsigned avg_packet;
  if (diag_total_out)
  {
    res_percent = (double)((double)(diag_resend*100)/(double)diag_total_out);    
    avg_packet  = diag_total_size/diag_total_out;
  }
  else
  {
    res_percent = 0;
    avg_packet = 0;
  }
  buf_percent = (double)((double)(diag_max_buf_usage*100)/(double)buffer_size);
  ENGINFO ("net statistics: packets received %d, packets send %d, resends %d (%05.2f%%)",
           diag_total_in, diag_total_out, diag_resend, res_percent);
  ENGINFO ("  kbytes send %d, max packet size %d, average packet size %d",
           diag_total_size/1024, diag_max_packet, avg_packet);
  ENGINFO ("  max buffer usage %d (%05.2f%%), corrupted packets %d", 
           diag_max_buf_usage, buf_percent, diag_corrupted_packets);

  status.reset(net_Initialized);
}
//-----------------------------------------------------------------------------
int Net::is_initialised ()
{
  return (status.is(net_Initialized));
}
//-----------------------------------------------------------------------------
void Net::init (int debug)
{
  DBG_CHECK(!status.is(net_Initialized));
  session_OK = TRUE;
  providers.alloc (max_enum_providers, "Net::providers");
  games.alloc     (max_enum_games,     "Net::games");
  players.alloc   (max_enum_players,   "Net::players");
  games_qty = 0;
  players_qty = 0;
  providers_qty = 0;
  driver=NULL;
  if (debug) 
    status.set(net_Debug);
  msgbuf=NEW(Array<Net::Buffer>(), "Net::msgbuf");
  msgbuf->alloc (max_enum_players, "Net::msgbuf");
  for (int i=0; i<max_enum_players; i++)
  {
    (*msgbuf)[i].in.alloc       (buffer_size,    "Net::msgbuf.in");
    (*msgbuf)[i].out.alloc      (buffer_size,    "Net::msgbuf.out");
    (*msgbuf)[i].requests.alloc (req_array_size, "Net::msgbuf.requests");
    (*msgbuf)[i].resends.alloc  (res_array_size, "Net::msgbuf.resends");
    (*msgbuf)[i].player         = 0;
    (*msgbuf)[i].last_in        = 0;
    (*msgbuf)[i].last_out       = 0;
    (*msgbuf)[i].msg_counter    = 0;
  }
  Comm::quit_me(Net::quit,"net","log mmu");
  status.set(net_Initialized);
}
//-----------------------------------------------------------------------------
typedef HRESULT (WINAPI *Create_function)    (LPGUID, LPDIRECTPLAY FAR *, IUnknown FAR *);
typedef HRESULT (WINAPI *Enumerate_function) ( LPDPENUMDPCALLBACK, LPVOID );

Create_function    create_function;
Enumerate_function enumerate_function;

static void init_dplay_dll (int init)
{
  static HINSTANCE dll_hinstance=NULL;
  if(init)
  {
    if(!dll_hinstance)
    {
      dll_hinstance = LoadLibrary("dplay.dll");
      if(dll_hinstance==NULL)
        FAILURE2(Eem_error::dp_not_installed, "direct play not installed");
      create_function=(Create_function)GetProcAddress(dll_hinstance,"DirectPlayCreate");
      if(create_function==NULL)
        FAILURE2(Eem_error::dp_general, "get DirectPlayCreate address failed");
      enumerate_function=(Enumerate_function)GetProcAddress(dll_hinstance,"DirectPlayEnumerate");
      if(enumerate_function==NULL)
        FAILURE2(Eem_error::dp_general, "get DirectPlayEnumerate address failed");
    }
  }
  else
  {
    if(dll_hinstance)
    {
      FreeLibrary(dll_hinstance);
      dll_hinstance=NULL;
    }
  }
}
//-----------------------------------------------------------------------------
void Net::open (GUID game, GUID *provider)
{
  DBG_CHECK(status.is(net_Initialized));
  DBG_CHECK(!status.is(net_Opened));
  DBG_CHECK(provider!=NULL);
  status.set(net_ExtraDebug);
  game_guid = game;
  init_dplay_dll(1);
  HRESULT hr = (*create_function)(provider, &driver, NULL);
  if (hr!=DP_OK)
    FAILURE2(Eem_error::dp_general, "Net::open - DirectPlayCreate failure. Error code %8X", (int)hr);
  status.set(net_Opened);
  if (!Comm::production)
    MESSAGE("net opened");
  if (!status.is(net_InfoDone))
  {
    dpcaps.dwSize = sizeof(dpcaps);
    hr = driver->GetCaps(&dpcaps);
    SYSINFO("direct play driver capabilities:");
    for (int i=0; i<providers_qty; i++)
    {
      if (providers[i].id==(*provider))
      {
        SYSINFO("  network provider : %s", providers[i].name);
        break;
      }
    }
    SYSINFO("  maximum buffer size : %d", (int)dpcaps.dwMaxBufferSize);
    SYSINFO("  maximum queue size : %d", (int)dpcaps.dwMaxQueueSize);
    SYSINFO("  maximum number of players : %d", (int)dpcaps.dwMaxPlayers);
    SYSINFO("  maximum baud rate : %d", (int)(dpcaps.dwHundredBaud*100));
    SYSINFO("  latency estimate in ms : %d", (int)dpcaps.dwLatency);
    SYSINFO("  flag DPCAPS_GUARANTEED : %s", (char*)(((dpcaps.dwFlags&DPCAPS_GUARANTEED)!=0)?"ON":"OFF"));
    SYSINFO("  flag DPCAPS_NAMESERVER : %s", (char*)(((dpcaps.dwFlags&DPCAPS_NAMESERVER)!=0)?"ON":"OFF"));
    SYSINFO("  flag DPCAPS_NAMESERVICE : %s", (char*)(((dpcaps.dwFlags&DPCAPS_NAMESERVICE)!=0)?"ON":"OFF"));
    status.set(net_InfoDone);
  }
}
//-----------------------------------------------------------------------------
void Net::close (void)
{
  DBG_CHECK(status.is(net_Initialized));
  DBG_CHECK(status.is(net_Opened));
  driver->Release();
  players_qty = 0;
  driver=NULL;
  status.reset(net_Opened);
  init_dplay_dll(0);
  if (!Comm::production)
    MESSAGE("net closed");
}
//-----------------------------------------------------------------------------
void Net::game_create  (int players_max, char *name, char *player_name, char* game_info, unsigned timeout)
{
  DBG_CHECK(status.is(net_Initialized));
  DBG_CHECK(status.is(net_Opened));
  DBG_CHECK(!status.is(net_InGame));
  DBG_CHECK(name!=NULL);
  DBG_CHECK(player_name!=NULL);
  DBG_CHECK(game_info!=NULL);
  DBG_CHECK(players_max > 0);
  memset(&current_session, 0, sizeof(DPSESSIONDESC));
  current_session.dwSize = sizeof(current_session);
  current_session.guidSession = game_guid;
  current_session.dwMaxPlayers = players_max;
  current_session.dwFlags = DPOPEN_CREATESESSION;
  strncpy(&current_session.szSessionName[0], name, DPSESSIONNAMELEN-1);
  memcpy(&current_session.szUserField, &(game_info[0]), 16);
  memcpy(&current_session.dwUser1,     &(game_info[16]), 4);
  memcpy(&current_session.dwUser2,     &(game_info[20]), 4);
  memcpy(&current_session.dwUser3,     &(game_info[24]), 4);
  current_session.dwUser4 = players_max;
  HRESULT hr; 
  hr = driver->Open(&current_session);
  if (hr!=DP_OK)
    FAILURE2(Eem_error::dp_general, "Net::game_create - Open failure. Error code %8X", (int)hr);
  memcpy(&current_game.info, game_info, game_info_len);
  current_game.max_players = players_max;
  current_game.id = current_session.dwSession;
  strncpy(current_game.name, name, DPSESSIONNAMELEN-1);
  char temp[DPSHORTNAMELEN];
  temp[0]=0;
  strncpy(current_player.name, player_name, player_name_len-1);
  hr = driver->CreatePlayer(&current_player.id, temp, current_player.name, &event);
  if (hr!=DP_OK)
    FAILURE2(Eem_error::dp_general, "Net::game_create - CreatePlayer failure. Error code %8X", (int)hr);
  status.set(net_InGame);
  int i;
  unsigned time_start = timeGetTime();
  int last_reported = 0;
  do
  {
    list_players();
    if (players_qty>last_reported)
    {
      Comm::init_info(format_message("%d of %d players found", players_qty, players_max));
      last_reported = players_qty;
    }
    if (players_qty>players_max)
      FAILURE("Unexpected players found on network");
    Comm::process_messages();
  } while ((players_qty!=players_max)&&((timeGetTime()-time_start)<timeout));
  if (players_qty<players_max)
    FAILURE2 (Eem_error::net_player_not_found, "Player not found");
  unsigned char info_buf[game_info_len+1];
  memcpy (&info_buf[1], game_info, game_info_len);
  info_buf[0] = (unsigned char)players_max;
  info_buf[game_info_len] = NET_VERSION;
  for (i=0; i<players_qty; i++)
    if (players[i].id!=current_player.id)
      send(info_buf, game_info_len+1, players[i].id);
  purge();   
  status.reset(net_ExtraDebug);
}
//-----------------------------------------------------------------------------
void Net::game_connect (DWORD game_id, char *player_name, unsigned timeout)
{
  DBG_CHECK(status.is(net_Initialized));
  DBG_CHECK(status.is(net_Opened));
  DBG_CHECK(!status.is(net_InGame));
  DBG_CHECK(player_name!=NULL);
  memset(&current_session, 0, sizeof(DPSESSIONDESC));
  current_session.dwSize = sizeof(DPSESSIONDESC);
  current_session.guidSession = game_guid;
  current_session.dwSession = game_id;
  current_session.dwFlags = DPOPEN_OPENSESSION;
  HRESULT hr;
  Comm::init_info("Connecting to game server");
  do
  {
    hr = driver->Open(&current_session);
  } while ((hr==DPERR_NOCONNECTION)||(hr==E_FAIL));
  if (hr==DPERR_USERCANCEL)
    throw Closed();
  if (hr!=DP_OK)
    FAILURE2(Eem_error::dp_general, "Net::game_connect - Open failure. Error code %8X", (int)hr);
  char temp[DPSHORTNAMELEN];
  temp[0]=0;
  strncpy(current_player.name, player_name, player_name_len-1);
  hr = driver->CreatePlayer(&current_player.id, temp, current_player.name, &event);
  if (hr!=DP_OK)
  {
    if (hr==DPERR_USERCANCEL)
      throw Closed();
    else
      FAILURE2(Eem_error::dp_general, "Net::game_connect - CreatePlayer failure. Error code %8X", (int)hr);
  }
  status.set(net_InGame);
  unsigned char info_buf[game_info_len+1];
  unsigned size = game_info_len+1;
  unsigned long from;
  unsigned time_start = timeGetTime();
  BOOL info_received = FALSE;
  int last_reported = 0;
  do
  {
    list_players();
    if ((players_qty > 0)&&(players_qty>last_reported))
    {
      Comm::init_info(format_message("%d players found on server", players_qty));
      last_reported = players_qty;
    }
    if (!info_received)
    {
      info_received = (receive (info_buf, &size, &from)&&(from==1));
      if (info_received)
      {
        current_game.max_players = info_buf[0];
        memcpy(current_game.info, &info_buf[1], game_info_len);
        if (info_buf[game_info_len]!=NET_VERSION)
          FAILURE2(Eem_error::net_incompatible_version, "Cannot connect to different version of the game.");
      }
    }
    Comm::process_messages();
  }
  while (((!info_received)||((unsigned)players_qty!=current_game.max_players))&&
          ((timeGetTime()-time_start)<timeout));

  if ((!info_received)||((unsigned)players_qty!=current_game.max_players))
    FAILURE2 (Eem_error::net_player_not_found, "Player not found");
  if ((unsigned)players_qty>current_game.max_players)
    FAILURE("Unexpected players found on network");
  status.reset(net_ExtraDebug);
}
//-----------------------------------------------------------------------------
void Net::game_destroy (void)
{
  DBG_CHECK(status.is(net_Initialized));
  DBG_CHECK(status.is(net_Opened));
  DBG_CHECK(status.is(net_InGame));
  HRESULT hr = driver->DestroyPlayer(current_player.id);
  if (hr!=DP_OK)
    FAILURE("Net::game_destroy - DestroyPlayer failure. Error code %8X", (int)hr);
  SleepEx(200, FALSE);
  hr = driver->Close();
  if (hr!=DP_OK)
    FAILURE("Net::game_destroy - Close failure. Error code %8X", (int)hr);
  players_qty = 0;
  games_qty = 0;

  status.reset(net_InGame);
}
//-----------------------------------------------------------------------------
Net::Game *Net::get_current_game(void)
{
  DBG_CHECK(status.is(net_Initialized));
  DBG_CHECK(status.is(net_Opened));
  DBG_CHECK(status.is(net_InGame));
  return (&current_game);
}
//-----------------------------------------------------------------------------
Net::Player *Net::get_current_player(void)
{
  DBG_CHECK(status.is(net_Initialized));
  DBG_CHECK(status.is(net_Opened));
  DBG_CHECK(status.is(net_InGame));
  return(&current_player);
}
//-----------------------------------------------------------------------------
void Net::game_lock (void)
{
  DBG_CHECK(status.is(net_Initialized));
  DBG_CHECK(status.is(net_Opened));
  DBG_CHECK(status.is(net_InGame));
  DBG_CHECK(!status.is(net_Locked));
  HRESULT hr = driver->EnableNewPlayers(FALSE);
  if (hr!=DP_OK)
    FAILURE("Net::game_lock - EnableNewPlayer failure. Error code %8X", (int)hr);
  status.set(net_Locked);
}
//-----------------------------------------------------------------------------
void Net::game_unlock (void)
{
  DBG_CHECK(status.is(net_Initialized));
  DBG_CHECK(status.is(net_Opened));
  DBG_CHECK(status.is(net_InGame));
  DBG_CHECK(status.is(net_Locked));
  HRESULT hr = driver->EnableNewPlayers(TRUE);
  if (hr!=DP_OK)
    FAILURE("Net::game_unlock - EnableNewPlayer failure. Error code %8X", (int)hr);
  status.reset(net_Locked);
}
//-----------------------------------------------------------------------------
Net::Game *Net::list_games (unsigned *count, int *result, int timeout, int retries, Timeout_function function)
{
  DBG_CHECK(status.is(net_Initialized));
  DBG_CHECK(status.is(net_Opened));
  DBG_CHECK(!status.is(net_InGame));
  games_qty=0;
  DPSESSIONDESC sd;
  memset(&sd, 0, sizeof(DPSESSIONDESC));
  sd.dwSize = sizeof(sd);
  sd.guidSession  = game_guid;
  enum_error = res_OK;
  enum_timeout = timeout;
  enum_retries = retries;
  HRESULT hr = driver->EnumSessions(&sd, (DWORD)timeout, enum_games, function, DPENUMSESSIONS_AVAILABLE);
  if (result)
  { 
    if ((hr==DP_OK)||(hr==DPERR_NOSESSIONS))
      *result = enum_error;
    else
      *result = res_enum_error;
  }
  if (count)
    *count = games_qty;
  return (games);
}
//-----------------------------------------------------------------------------
int Net::get_buf_pos (DPID player)
{
  int result = -1;
  for (int i=0; i<max_enum_players; i++)
    if ((*msgbuf)[i].player == player) 
    {
      result = i;
      break;
    }
  return (result);
}
//-----------------------------------------------------------------------------
int Net::get_player_pos (DPID player)
{
  int result = -1;
  for (int i=0; i<players_qty; i++)
    if (players[i].id == player) 
    {
      result = i;
      break;
    }
  return (result);
}
//-----------------------------------------------------------------------------
Net::Provider *Net::list_providers (unsigned *count, int *result)
{
  DBG_CHECK(status.is(net_Initialized));
  DBG_CHECK(!status.is(net_Opened));
  init_dplay_dll(1);
  HRESULT hr = DP_OK;
  enum_error = res_OK;
  if (!status.is(net_ProvListed))
  {
    providers_qty=0;
    hr = (*enumerate_function)(enum_providers, NULL);
    status.set(net_ProvListed);
  }     
  if (result)
  { 
    if (hr==DP_OK)
      *result = enum_error;
    else
      *result = res_enum_error;
  }
  if (count)
    *count = providers_qty;
  return (providers);
}
//-----------------------------------------------------------------------------
Net::Player *Net::list_players   (unsigned *count, int *result)
{
  DBG_CHECK(status.is(net_Initialized));
  DBG_CHECK(status.is(net_Opened));
  DBG_CHECK(status.is(net_InGame));
  players_qty=0;
  enum_error = res_OK;
  HRESULT hr = driver->EnumPlayers(current_session.dwSession, enum_players, NULL, 0);
  if (result)
  { 
    if ((hr==DP_OK)||(hr==DPERR_NOPLAYERS))
      *result = enum_error;
    else
      *result = res_enum_error;
  }
  if (count)
    *count = players_qty;
  for (int i=0; i<max_enum_players; i++)         // delete players which no longer exist
    if ((*msgbuf)[i].player!=0)
    {
      int pos = get_player_pos((*msgbuf)[i].player);
      if (pos == -1)
        (*msgbuf)[i].player = 0;
    }
  for (i=0; i<players_qty; i++)             // create buffer slots for new players
  {
    int pos = get_buf_pos(players[i].id);
    if (pos == -1)
    {
      pos = get_buf_pos(0);                 // find empty slot in buffer
      (*msgbuf)[pos].player      = players[i].id;
      (*msgbuf)[pos].last_in     = 0;
      (*msgbuf)[pos].last_out    = 0;
      (*msgbuf)[pos].msg_counter = 0;
      (*msgbuf)[pos].in.purge();
      (*msgbuf)[pos].out.purge();
      (*msgbuf)[pos].resends.purge();
      (*msgbuf)[pos].requests.purge();
    }
  }
  return (players);
}
//-----------------------------------------------------------------------------
void Net::api_send     (void *buffer, unsigned size, DPID to)
{
#if HI_DEBUG
  if (players_qty<2)
    FAILURE("Net::api_send - cannot send message. Only one player available on net"); 
#endif
  DBG_CHECK(to!=0);
  HRESULT hr;
  *((unsigned short*)buffer) = crc16(&(((char*)buffer)[2]), (unsigned short)(size-2));
/*
  if (rand()%30==1) 
  {
    ((unsigned char*)buffer)[5]++;
     hr = driver->Send(current_player.id, to, DPSEND_TRYONCE, buffer, size);
    ((unsigned char*)buffer)[5]--;
  }
  else
*/
  hr = driver->Send(current_player.id, to, DPSEND_TRYONCE, buffer, size);
  diag_total_out++;
  diag_total_size+=size;
  if (size>diag_max_packet)
    diag_max_packet = size;
  if (hr!=DP_OK)
  {
    if ((hr&0xFFFF0000)==0)
      DBG_MESSAGE("Net::api_send - %d messages in DP queue", (int)hr);
    else
      FAILURE("Net::api_send - Send to player %d failure. Error code %8X", (int)to, (int)hr);
  }
}
//-----------------------------------------------------------------------------
void Net::send (void *buffer, unsigned size, DPID to)
{
  DBG_CHECK(status.is(net_Initialized));
  DBG_CHECK(status.is(net_Opened));
  DBG_CHECK(status.is(net_InGame));
  DBG_CHECK(buffer!=NULL);
  DBG_CHECK(size<=max_packet_size);
  DBG_CHECK(to!=0);
  try
  {
    int pos = get_buf_pos(to);
    if ((*msgbuf)[pos].out.getfree() ==0)
      (*msgbuf)[pos].out.popempty();
    Msg *msg = (*msgbuf)[pos].out.pushempty();
    (*msgbuf)[pos].msg_counter++;
    msg->header.number = (*msgbuf)[pos].msg_counter;
    msg->header.msg_type = msg_User;
    msg->header.old_number = 0;
    msg->header.size = (unsigned char)size;
    memcpy(&msg->data, buffer, size);
//  if ((*msgbuf)[pos].msg_counter%100!=1) 
//    if (rand()%30!=1) 
      api_send(msg, msg->header.size+sizeof(Msg_header), to);
    EXDBG_MESSAGE("net::send - sending %d",msg->header.number);
  }
  catch (Failure)
  {
    FAILURE2(Eem_error::net_lost_connection, "Net::send failed"); 
  }
}
//-----------------------------------------------------------------------------
BOOL Net::api_receive (void)
{
  DPID     to       = current_player.id;
  HRESULT  hr       = DP_OK;
  BOOL     exit     = FALSE;
  BOOL     result   = FALSE;
  do 
  {
    DWORD wait_res = WaitForSingleObject(event, 0);
    if (wait_res==WAIT_TIMEOUT)
      break;
    else if (wait_res==WAIT_FAILED)
    {
      DBG_MESSAGE("Net::receive - unexpected state: WAIT_FAILED");
      break;
    }
    else if (wait_res==WAIT_ABANDONED)
    {
      DBG_MESSAGE("Net::receive - unexpected state: WAIT_ABANDONED");
      break;
    }
    else if (wait_res!=WAIT_OBJECT_0)
    {
      DBG_MESSAGE("Net::receive - unexpected state: <unknown>");
      break;
    }
    rec_size = rec_buf_size;
    hr = driver->Receive(&rec_from, &to, DPRECEIVE_TOPLAYER, rec_buf, (LPDWORD)(&rec_size));
    if (hr==DP_OK)
    {
      if (rec_from==0)
      {
        switch (((DPMSG_GENERIC*)rec_buf)->dwType)
        {
          case DPSYS_ADDPLAYER:    
            list_players();     
            if (!Comm::production)
              MESSAGE("Net::api_receive - DPSYS_ADDPLAYER processed"); 
            break;
          case DPSYS_DELETEPLAYER: 
            list_players();     
            if (!Comm::production)
              MESSAGE("Net::api_receive - DPSYS_DELETEPLAYER processed"); 
            break;
          case DPSYS_SESSIONLOST:  
            session_OK = FALSE; 
            if (!Comm::production)
              MESSAGE("Net::api_receive - DPSYS_SESSIONLOST processed");
            break;
          default: 
            if (!Comm::production)
              MESSAGE("Net::api_receive - unknown system message processed %0X", 
                      (int)(((DPMSG_GENERIC*)rec_buf)->dwType)); break;
        }
      }
      else
      {
        if (rec_size > max_packet_size)
          FAILURE("Net::api_receive - packet to large (%d)", rec_size);
        if (*((unsigned short*)rec_buf) != crc16(&(((char*)rec_buf)[2]), (unsigned short)(rec_size-2)))
        {
          diag_corrupted_packets++;
        }
        else
        {
          result = TRUE;
          diag_total_in++;
          exit = TRUE;
        }
      }
    }
    else if (hr==DPERR_BUFFERTOOSMALL)
    {
      DBG_MESSAGE("DPERR_BUFFERTOSMALL #2: from:%d  req_size:%d", rec_from, rec_size);
      diag_trans_errors++;
    }
    else if (hr==DPERR_NOMESSAGES)
    {
      DBG_MESSAGE("Net::receive - unexpected state: DPERR_NOMESSAGES");
      result = FALSE;
      exit = TRUE;
    }
    else
      FAILURE2(Eem_error::net_lost_connection, "Net::api_receive - Receive failure. Error code %8X", (int)hr);
  } while (!exit);
  return (result);
}
 //-----------------------------------------------------------------------------
BOOL Net::receive (void *buffer, unsigned *size, DPID* from)
{
  DBG_CHECK(status.is(net_Initialized));
  DBG_CHECK(status.is(net_Opened));
  DBG_CHECK(status.is(net_InGame));
  DBG_CHECK(buffer!=NULL);
  DBG_CHECK(size!=NULL);
  DBG_CHECK(from!=NULL);
  try
  {
AGAIN:
    BOOL res = api_receive();
    if (res)
    {
      int pos = get_buf_pos(rec_from);
      if (pos==-1)
      {
        DBG_MESSAGE("Message from unknown player %d received", rec_from);
        return(FALSE); 
      }
      Msg    *MSG = (Msg*)rec_buf;
      Buffer *BUF = &((*msgbuf)[pos]);
#if EXTRA_DEBUG
      if (status.is(net_ExtraDebug))
        dump_buffer(pos);
#endif
      EXDBG_MESSAGE("Net::receive - receive %d", MSG->header.number);
      resolve_requests (MSG, pos);         
      resolve_resends  (MSG, pos);         
      if ((char)(MSG->header.number - BUF->last_in) <= 0)                     // jakis dubel, 
      {
        if (!Comm::production)
          MESSAGE("Net::receive - unexpected doubled message received");
        goto AGAIN;
      }
      else if ((unsigned char)(BUF->last_in - MSG->header.number) == 1)       // jak trzeba 
      {
        if (BUF->in.getquantity()==0)
        {
          BUF->last_in++;
          if (((MSG->header.msg_type)&(msgmask_Ignore))!=0)
          {
            BUF->last_out++;
            goto AGAIN;
          }
          *size = MSG->header.size;
          *from = rec_from;
          if ((unsigned char)(MSG->header.number - BUF->last_out) != 1)
            FAILURE("Net::receive - error in net buffer, invalid packet order (%d -> %d)",
                    (int)(BUF->last_out), (int)(MSG->header.number));
          memcpy(buffer, MSG->data, MSG->header.size);
          BUF->last_out++;
          return(TRUE);
        }
        else
        {
          BUF->last_in++;
          Msg *msg = BUF->in.pushempty();
          if (BUF->in.getquantity()>(int)diag_max_buf_usage)
            diag_max_buf_usage = BUF->in.getquantity();
          memcpy(msg, rec_buf, rec_size);
          goto AGAIN;
        }
      }
      else // uwaga ! dziura.
      {
        Msg* msg;
        for (int i=BUF->last_in+1; char (i - MSG->header.number)<0; i++)
        {
          msg = BUF->in.pushempty();
          msg->header.msg_type = msg_None;
          msg->header.old_number = 0;
          msg->header.number = (unsigned char)i;
          if (BUF->requests.getfree()==0)
            FAILURE2(Eem_error::net_too_many_errors, "Net::receive - request array overflow - to many errors on network");
          BUF->requests.push((unsigned char)i);
          EXDBG_MESSAGE("Net::receive - packet %d lost ", i);
        }
        msg = BUF->in.pushempty();
        if (BUF->in.getquantity()>(int)diag_max_buf_usage)
          diag_max_buf_usage = BUF->in.getquantity();
        memcpy(msg, rec_buf, rec_size);
        BUF->last_in = msg->header.number;
        if (BUF->requests.getquantity()>0)
          request(pos);
        goto AGAIN;
      }
    }
    else // nic nie przyszlo wiec najpierw zalegle resendy a potem cos z bufora (jezeli jest)
    {
      clear_in_buffers();
      if (perform_resends())
        goto AGAIN;;

      Msg *msg;
      for (int i=0; i<max_enum_players; i++)
      {
        if ((*msgbuf)[i].in.getquantity()>0)
        {
          msg = (*msgbuf)[i].in.peek();
          if (msg->header.msg_type!=msg_None)
          {
            memcpy(buffer, msg->data, msg->header.size);
            *from = (*msgbuf)[i].player;
            *size = msg->header.size;
            if ( (unsigned char)(msg->header.number - (*msgbuf)[i].last_out) != 1)
              FAILURE("Net::receive - error in net buffer, invalid packet order (%d -> %d)",
                      (int)(*msgbuf)[i].last_out, (int)msg->header.number);
            (*msgbuf)[i].in.popempty();
            (*msgbuf)[i].last_out++;
            return(TRUE);
          }
        }
      }
      return (FALSE);
    }
  }
  catch (Failure)
  {
    FAILURE("Net::receive failed"); 
  }
}
//-----------------------------------------------------------------------------
void Net::request (int pos)
{
  try
  {
    if ((*msgbuf)[pos].out.getfree() ==0)
      (*msgbuf)[pos].out.popempty();
    Msg *msg = (*msgbuf)[pos].out.pushempty();
    (*msgbuf)[pos].msg_counter++;
    msg->header.number = (*msgbuf)[pos].msg_counter;
    msg->header.msg_type = msg_Request;
    msg->header.old_number = 0;
    msg->header.size = 0;
    int i = 0;
    while ((*msgbuf)[pos].requests.getquantity()>0)
    {
      msg->data[i]=(*msgbuf)[pos].requests.pop();
      EXDBG_MESSAGE("Net::request - requesting %d (msg %d)",
                    (int)(msg->data[i]), (int)msg->header.number);
      msg->header.size += sizeof(unsigned char);
      i++;
    }
    // CHECK_SUM
    api_send(msg, msg->header.size + sizeof(Msg_header), (*msgbuf)[pos].player);
  }
  catch (Failure)
  {
    FAILURE("Net::request failed");
  }
}
//-----------------------------------------------------------------------------
void Net::resend (unsigned char number, int pos)
{
  try
  {
    while ((*msgbuf)[pos].out.getfree()<2)
      (*msgbuf)[pos].out.popempty();
    int this_i = -1;
    int prev_i = -1;
    int i;
    for (i=(*msgbuf)[pos].out.getquantity()-1; i>=0; i--)
      if ((*msgbuf)[pos].out[i].header.number==number)
      {
        this_i = i;
        break;
      }
    if (this_i==-1)
      FAILURE2(Eem_error::net_too_many_errors, "Net::resend - message %d not found in buffer. Cannot resend.", (int)number);
    if ((((*msgbuf)[pos].out[this_i].header.msg_type)&msg_Resend)!=0)
    {
      for (i=this_i-1; i>=0; i--)
        if ((*msgbuf)[pos].out[i].header.number==(*msgbuf)[pos].out[this_i].header.old_number)
        {
          prev_i = i;
          break;
        }
      if (prev_i==-1)
        FAILURE2(Eem_error::net_too_many_errors, "Net::resend - message %d for player %d not found in buffer. Cannot resend.", 
                (int)((*msgbuf)[pos].out[this_i].header.old_number),(int)((*msgbuf)[pos].player));
    }
    Msg this_msg = (*msgbuf)[pos].out[this_i];
    if (prev_i!=-1)
    {
      Msg prev_msg = (*msgbuf)[pos].out[prev_i];
      Msg *msg     = (*msgbuf)[pos].out.pushempty();
      (*msgbuf)[pos].msg_counter++;
      msg->header.number     = (*msgbuf)[pos].msg_counter;
      msg->header.old_number = prev_msg.header.number;

      if (((prev_msg.header.msg_type)&msg_Resend)!=0)
      {
        msg->header.msg_type = (unsigned char)(prev_msg.header.msg_type|msg_Resend|msg_PrevResend);
        msg->header.size     = 0;
      }
      else
      {
        msg->header.msg_type = (unsigned char)(prev_msg.header.msg_type|msg_Resend);
        msg->header.size     = prev_msg.header.size;
        memcpy(msg->data, prev_msg.data, prev_msg.header.size);
        if (((msg->header.msg_type)&msg_Request)!=0)
          prev_msg.header.size = 0;
      }
      EXDBG_MESSAGE("Net::resend - resending %d (msg %d)", (int)msg->header.old_number, 
                    (int)msg->header.number);
      api_send(msg, msg->header.size + sizeof(Msg_header), (*msgbuf)[pos].player);
      diag_resend++;
    }
    Msg *msg = (*msgbuf)[pos].out.pushempty();
    (*msgbuf)[pos].msg_counter++;
    msg->header.number     = (*msgbuf)[pos].msg_counter;
    msg->header.old_number = this_msg.header.number;
    if (((this_msg.header.msg_type)&msg_Resend)!=0)
    {
      msg->header.msg_type = (unsigned char)(this_msg.header.msg_type|msg_Resend|msg_PrevResend);
      msg->header.size     = 0;
    }
    else
    {
      msg->header.msg_type = (unsigned char)(this_msg.header.msg_type|msg_Resend);
      msg->header.size     = this_msg.header.size;
      memcpy(msg->data, this_msg.data, this_msg.header.size);
      if (((msg->header.msg_type)&msg_Request)!=0)
        this_msg.header.size = 0;
    }
    EXDBG_MESSAGE("Net::resend - resending %d (msg %d)", (int)msg->header.old_number, 
                  (int)msg->header.number);
    api_send(msg, msg->header.size + sizeof(Msg_header), (*msgbuf)[pos].player);
    diag_resend++;
  } 
  catch (Failure)
  {
    FAILURE("Net::resend failed");
  }
}
//-----------------------------------------------------------------------------
void Net::queue_resend (unsigned char number, int pos)
{
  for (int i=0; i<(*msgbuf)[pos].out.getquantity(); i++)
  {
    if (number==(*msgbuf)[pos].out[i].header.number)
    {
      if ((((*msgbuf)[pos].out[i].header.msg_type)&(msg_Resend|msg_Request))!=0)
        (*msgbuf)[pos].resends.unpop(number);
      else
        (*msgbuf)[pos].resends.push(number);
    }
  }
}
//-----------------------------------------------------------------------------
void Net::dump_buffer(int pos)
{
  char dumpbuf[4096];
  char tempbuf[100];
  strcpy(dumpbuf, "Net::msgbuf[%d].in DUMP - ");
  for (int i=0; i<(*msgbuf)[pos].in.getquantity(); i++)
  {
    sprintf(tempbuf, " %03d",(int)((*msgbuf)[pos].in[i].header.number));
    strcat(dumpbuf, tempbuf);
  }
  DBG_MESSAGE(dumpbuf, (int)pos);
}
//-----------------------------------------------------------------------------
void Net::resolve_requests (Msg *msg, int pos)
{
  if (((msg->header.msg_type)&msg_Request)!=0) 
  {
    for (int i=0; i<msg->header.size; i++)
      queue_resend(msg->data[i], pos);
  }
}
//-----------------------------------------------------------------------------
void Net::resolve_resends (Msg *msg, int pos)
{
  if (((msg->header.msg_type)&msg_Resend)!=0)
  {
    for (int i=0; i<((*msgbuf)[pos].in.getquantity()-1); i++)
    {
      if ((*msgbuf)[pos].in[i].header.number==msg->header.old_number)
      {
        if (((msg->header.msg_type)&msg_PrevResend)!=0) 
          (*msgbuf)[pos].in[i].header.msg_type = msg_Resend;
        else
        {
          memcpy(&(*msgbuf)[pos].in[i], msg, msg->header.size+sizeof(Msg_header));
          (*msgbuf)[pos].in[i].header.msg_type &=~msg_Resend;
          (*msgbuf)[pos].in[i].header.number = msg->header.old_number;
        }
        break;
      }
    }
  }
}
//-----------------------------------------------------------------------------
BOOL Net::perform_resends (void)
{
  int res_cnt = 0;
  int miss_cnt;
  BOOL resended = FALSE;
  do
  {
    miss_cnt = 0;
    for (int i=0; i<max_enum_players; i++)
    {
      if ((*msgbuf)[i].resends.getquantity()>0)
      {
        unsigned char r = (*msgbuf)[i].resends.pop();
        resend(r, i);
        resended = TRUE;
        res_cnt++;
      }
      else
        miss_cnt++;
    }
  } while ((res_cnt<max_resends)&&(miss_cnt<max_enum_players));
  return (resended);
}
//-----------------------------------------------------------------------------
void Net::clear_in_buffers (void)
{
  for (int i=0; i<max_enum_players; i++) 
  {
    while (((*msgbuf)[i].in.getquantity()>0)&&
           ((((*msgbuf)[i].in.peek()->header.msg_type) & (msgmask_Ignore))!=0))
    {
      (*msgbuf)[i].in.popempty();
      (*msgbuf)[i].last_out++;
    }
  }
}
//-----------------------------------------------------------------------------
void Net::purge (void)
{
  try
  {
    const int purge_cnt     = 3;
    const int purge_timeout = 100;

    for (int k=0; k<purge_cnt; k++)
    {
      for (int i=0; i<max_enum_players; i++)
      {
        if (((*msgbuf)[i].player != 0)&&((*msgbuf)[i].player!=current_player.id))
        {
          if ((*msgbuf)[i].out.getfree() ==0)
            (*msgbuf)[i].out.popempty();
          Msg msg;
          (*msgbuf)[i].msg_counter++;
          msg.header.number = (*msgbuf)[i].msg_counter;
          msg.header.msg_type = msg_Purge;
          msg.header.size = 0;
          api_send(&msg, msg.header.size+sizeof(Msg_header), (*msgbuf)[i].player);
          EXDBG_MESSAGE("net::purge - sending %d",msg.header.number);
        }
      }
      SleepEx(purge_timeout, FALSE);
    }
  }
  catch (Failure)
  {
    FAILURE("Net::purge failed"); 
  }
}
//-----------------------------------------------------------------------------
void Net::ping (void)
{
  try
  {
    for (int i=0; i<max_enum_players; i++)
    {
      if (((*msgbuf)[i].player != 0)&&((*msgbuf)[i].player!=current_player.id))
      {
        if ((*msgbuf)[i].out.getfree() ==0)
          (*msgbuf)[i].out.popempty();
        Msg msg;
        (*msgbuf)[i].msg_counter++;
        msg.header.number = (*msgbuf)[i].msg_counter;
        msg.header.msg_type = msg_Ping;
        msg.header.size = 0;
        api_send(&msg, msg.header.size+sizeof(Msg_header), (*msgbuf)[i].player);
        EXDBG_MESSAGE("net::ping - sending %d",msg.header.number);
      }
    }
  }
  catch (Failure)
  {
    FAILURE("Net::ping failed"); 
  }
}
//-----------------------------------------------------------------------------

