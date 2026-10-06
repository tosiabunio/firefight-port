#include "1ee_hdrs.h"
#include <SDL.h>

//----------------------------------------------------------------------------
// Eem constants
//----------------------------------------------------------------------------
static const int def_buffer_delay = 4;
static const int max_net_delay = 4;
static const int max_ping_delay = 30;
static const int max_allowed_win_queue_usage = 50;

static const unsigned int def_connection_timeout = 3; // (w sekundach)
static const unsigned int def_connection_retries = 40; 

#include <1cw_strg.h>
//----------------------------------------------------------------------------
// Eem static variables
//----------------------------------------------------------------------------

Eem::User_block  Eem::user_block;
unsigned char    Eem::serie(0);
DWORD            Eem::thread(0);
Bitflag          Eem::status(0);
Bitflag          Eem::net_mask(0);
Bitflag          Eem::last_send_mask(0);
int             *Eem::queue;
int              Eem::queue_tail(0);
int              Eem::queue_head(0);
int              Eem::queue_size(0);
int              Eem::queue_free(0);
int              Eem::queue_last(0);
int              Eem::queue_delay(0);
int              Eem::timer_id(0);
int              Eem::timer_frq(0);
int              Eem::untimed(0);
int              Eem::dump(0);
Comm::wpp        Eem::old_window_proc=NULL;
int              Eem::last_return_code=0;
int              Eem::players_max(0);
int              Eem::local_player(0x0FFFFFFFF);
int              Eem::demo_player(0x0FFFFFFFF);
int              Eem::host_player(0x0FFFFFFFF);
int              Eem::selected_player;
int              Eem::server_player(0x0FFFFFFFF);
int              Eem::buffer_delay(def_buffer_delay);
Array<Eem::Player> *Eem::players=NULL;
Queue<unsigned>      *Eem::sync_queue=NULL;
Queue<unsigned char> *Eem::user_queue=NULL;
Eem::Player_state  *Eem::new_state=NULL;
Eem::Player_state   Eem::old_state;
int           Eem::diag_win_queue_usage(0);
int           Eem::diag_max_win_queue_usage(0);
unsigned      Eem::ping_counter(0);
unsigned      Eem::max_ping(max_ping_delay);

//===========================================================================
// Eem
//===========================================================================
void Eem::callback_timer (void) //#T 96:05:06:21:46
{ 
  if (status.is(eem_Active|eem_AutoActive))
  {
    ping_counter++;
    Joy::callback_joy();
    Mouse::flush();  // port: the pointer position goes before the tick
    // Port: the timer runs on the main thread, so the tick is queued directly. In network
    // mode the original posted WM_TIMER to move it from the timer thread to the main thread.
    push_event(ev_Timer);
  } 
}                                       
//---------------------------------------------------------------------------
int Eem::window_proc(int *presult, HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam)
{
  int processed=0;
  if (status.is(eem_Initialized))
  {
    if (!old_window_proc(presult, hwnd, message, wparam, lparam))
    {
      switch (message)
      {
        case WM_TIMER:
          push_event(ev_Timer);
          diag_win_queue_usage--;
          processed = 1;
          break;
        case WM_ACTIVATEAPP:
          {
		        if ((BOOL)wparam) // ACTIVE ON 
            {
              if (status.is(eem_Multi))
              {
                status.set(eem_InNetActive);
                Kbd::clear();
                Mouse::clear();
                Joy::clear();
                Vkey::clear();
                clear();
              }
              else
              {
                Kbd::clear();
                Mouse::clear();
                Joy::clear();
                Vkey::clear();
                clear();
                Comm::standby = 0;
                status.set(eem_AutoActive);
              }
            }
            else              // ACTIVE OFF 
            {
              if (status.is(eem_Multi))
                status.reset(eem_InNetActive);
              else
              {
			          status.reset(eem_AutoActive);
                if (status.is(eem_Active))
                  Comm::standby = 1;
              }
            }
          }
          break;
      }
    }
  }
  return (processed); 
}
//---------------------------------------------------------------------------
// Port: the SDL input events (Comm::input_proc). The original drivers had Windows hooks.
void Eem::sdl_event (const SDL_Event &event)
{
  Kbd::sdl_event(event);
  Mouse::sdl_event(event);
  Joy::sdl_event(event);
}
//---------------------------------------------------------------------------
// Port: the session comes from the command line (main.cpp): net_host=<players> hosts a game for
// 2-4 players, net_join=<address> joins one, net_port=<port> overrides Net::default_port.
// net_relay=<relay> plays through a relay server: the host gets a session code there (net_code
// asks for one), and net_join is that code. The launcher wrote the session as registry values before ([eem] net mode, game id, provider and
// game GUIDs). The player name is still [eem] player name, else the user name, and the host's game
// rules are still [eem] game info; without it every player uses RegData's defaults.
void Eem::init_multi ()
{
  char        game_info[Net::game_info_len];
  char        player_name[Net::player_name_len]; 
  unsigned    size;
  char        address[256] = "";
  int         host_players = Cmd_line::get_int("net_host",0);
  if (const char *join = Cmd_line::get_string("net_join",NULL))  // a buffer the next call reuses
    strncpy(address, join, sizeof(address)-1);
  unsigned short port = (unsigned short)Cmd_line::get_int("net_port",Net::default_port);
  char        relay[256] = "";
  char        code[32] = "";
  if (const char *text = Cmd_line::get_string("net_relay",NULL))
    strncpy(relay, text, sizeof(relay)-1);
  if (const char *text = Cmd_line::get_string("net_code",NULL))
    strncpy(code, text, sizeof(code)-1);

  memset(player_name, 0, sizeof(player_name));
  char uname[MAX_COMPUTERNAME_LENGTH];
  DWORD i=MAX_COMPUTERNAME_LENGTH;
  if ( !GetUserName( uname, &i )) 
    strncpy( uname, "<unknown>", MAX_COMPUTERNAME_LENGTH-1);
  strncpy(player_name, Registry::get_string(sec_eem, key_eem_player_name, uname), Net::player_name_len-1);
  unsigned timeout = Registry::get_int(sec_eem, key_eem_timeout, def_connection_timeout);
  unsigned retries = Registry::get_int(sec_eem, key_eem_retries, def_connection_retries);

  Net::init (status.is(eem_Debug)?1:0);
  Net::set_relay(relay, code[0]?code:NULL);
  if (host_players)
  {
    status.set(eem_Server);
    players_max = host_players;
    size = Net::game_info_len;
    BOOL has_info = Registry::get_binary(sec_eem, key_eem_game_info, game_info, &size) &&
                    size==Net::game_info_len;
    MESSAGE("creating game");
    Net::game_create(players_max, port, player_name, has_info?game_info:NULL, timeout*1000*retries);
    MESSAGE("game created, all players connected");
  }
  else
  {
    MESSAGE("joining the network game at %s", address);
    Net::game_connect(address, port, player_name, timeout*1000*retries);
    players_max = Net::get_current_game()->max_players;
    if (Net::get_current_game()->has_info)
      Registry::set_binary(sec_eem, key_eem_game_info, Net::get_current_game()->info, Net::game_info_len);
    else
      Registry::delete_entry(sec_eem, key_eem_game_info);
    MESSAGE("game joined, all players connected");
  }
  unsigned count;
  int result;
  Net::list_players(&count, &result);
}
//---------------------------------------------------------------------------
void Eem::init (int size, int frequency, int network_supported, int check_sync, 
                int compress_packets, int debug, int joy_dead_percent)
{
  try
  {
    DBG_CHECK(!status.is(eem_Initialized));
    MESSAGE("initializing eem");
    Kbd::init();
    Mouse::init();
    if (frequency!=0)
      Joy::init(joy_dead_percent);
    Vkey::init();
    Demo_recorder::init(EEM_VERSION);
    Demo_player::init(EEM_VERSION);
    timer_frq = 0;
    queue_size=size;
    thread = GetCurrentThreadId();
    players_max = 1;

    if (debug)
      status.set(eem_Debug);
    // Port: no RLE8 packet compression (cwe.ini asks for it). Its compressor reads past its
    // source and its decompressor writes without bounds, and the packets are small anyway.
    (void)compress_packets;
    if (check_sync)
      status.set(eem_CheckSync);

    if (network_supported)
    {
      // Port: a network game is hosted or joined from the command line (init_multi); the
      // launcher chose it with [eem] play mode before.
      if (Cmd_line::get_int("net_host",0)||Cmd_line::get_string("net_join",NULL))
        status.set(eem_Multi);
    }
    if (status.is(eem_Multi))
      init_multi();
    // Port: a headless run has no window to gain focus, so in a network game it counts as active:
    // a frame still waiting for the others returns to the caller, which keeps drawing, as with
    // the focused window (WM_ACTIVATEAPP below sets eem_InNetActive for a window).
    if (status.is(eem_Multi)&&Cmd_line::get_int("headless",0))
      status.set(eem_InNetActive);
    // Nor does a window that already had the focus: its activation came while the session was
    // set up (init_multi), before Eem took messages. A browser's canvas has the focus from the
    // click that started the game.
    if (status.is(eem_Multi)&&SDL_GetKeyboardFocus())
      status.set(eem_InNetActive);

    players=NEW(Array<Eem::Player>(), "Eem::players");
    players->alloc (Net::max_enum_players+1, "Eem::players");
    players->remap(1);

    buffer_delay = def_buffer_delay;
    Net::Player *plist;
    if (status.is(eem_Multi))
    {
      unsigned count=0;
      int result = Net::res_OK;
      plist = Net::list_players(&count, &result);
    }
    int i;
    sync_queue=NEW(Queue<unsigned>(),      "Eem::sync_queue");
    sync_queue->alloc(buffer_delay*4,      "Eem::sync_queue");
    user_queue=NEW(Queue<unsigned char>(), "Eem::user_queue");
    user_queue->alloc(buffer_delay*4,      "Eem::user_queue");
    for (i=1; i<=Net::max_enum_players; i++)
    {
      (*players)[i].states.alloc (buffer_delay*4,"Eem::players.states");
      (*players)[i].sync_queue.alloc(buffer_delay*4, "Eem::player.sync_queue");
      (*players)[i].pending.alloc(buffer_delay*8, "Eem::player.pending");  // port
      if (status.is(eem_Multi))
      {
        (*players)[i].id = plist[i-1].id;
        (*players)[i].status = 0;
        strncpy((*players)[i].player_name, plist[i-1].name, Net::player_name_len-1);
        MESSAGE("player %d - id:%d name:%s",i,(int)plist[i-1].id, (char*)plist[i-1].name);
      }
      else
      {
        (*players)[i].id = 0xFFFFFFFF;
        (*players)[i].status = 0;
        strncpy((*players)[i].player_name, "local player", Net::player_name_len-1);
      }
      (*players)[i].status = 0;
    }
    if (players_max > 1)
      for (i=1; i<players_max; i++)
        for (int k=i+1; k<=players_max; k++)
          if ((*players)[k].id < (*players)[i].id)
          {
            unsigned temp    = (*players)[k].id;
            (*players)[k].id = (*players)[i].id;
            (*players)[i].id = temp;
          }
    if (status.is(eem_Multi))
    {
      for (i=1; i<=players_max; i++)
        if ((*players)[i].id==Net::get_current_player()->id)
        {
          local_player = i;
          break;
        }
    }
    else
      local_player = 1;
    host_player = local_player;
    MESSAGE ("local player - %d", (int)local_player);
    select(local_player);
    queue = (int*)Heap::alloc(sizeof(int)*queue_size, "Eem::queue");
    next_read();
    clear();
    status.set(eem_Initialized|eem_AutoActive);

    if (frequency)
    {
      timer_frq = frequency;  // game time per tick, also without the clock
      if (!untimed)
      {
        if (!Timer::is_initialized())
          FAILURE("timer not initialized [Eem::init]");
        timer_id = Timer::add(1000/frequency, callback_timer, "Eem queue timer");
        status.set(eem_Timed);
      }
    }
    ENGINFO ("eem status: queue size = %d, frequency = %d, debug %s, network %s", 
             size, frequency, debug?"on":"off", status.is(eem_Multi)?"on":"off");
    if (status.is(eem_Multi))
      ENGINFO ("  players = %d, network mode = %s", 
               players_max, status.is(eem_Server)?"server":"client");


    old_window_proc=Comm::set_window_proc(Eem::window_proc);
    Comm::input_proc=Eem::sdl_event;
    char buf[80];
    strcpy(buf,"log mmu");
    if (status.is(eem_Timed))
      strcat(buf, " timer");
    if (status.is(eem_Multi))
      strcat(buf, " net");
    Comm::quit_me(quit, "eem", buf);
  }
  catch (Failure)
  {
    FAILURE ("eem initialization error");
  }
}
//---------------------------------------------------------------------------
void Eem::quit (void)
{
  Comm::input_proc=NULL;
  Demo_recorder::quit();
  Demo_player::quit();
  Kbd::quit();
  Mouse::quit();
  if (status.is(eem_Timed))
    Joy::quit();
  Vkey::quit();
  if (status.is(eem_Timed))
  {
    timer_frq = 0;
    Timer::kill(timer_id);
  }
  Heap::free(queue);
  queue_tail = 0;
  queue_head = 0;
  queue_free = 0;
  queue_last = 0;
  for (int i=1; i<=Net::max_enum_players+1; i++)
  {
    (*players)[i].states.free();
    (*players)[i].sync_queue.free();
    (*players)[i].pending.free();  // port
  }
  players->free();
  delete(players);
  sync_queue->free();
  delete(sync_queue);
  user_queue->free();
  delete(user_queue);
  if (status.is(eem_Multi))
  {
    Net::game_destroy();
    Net::close();
  }
  status = 0;
  ENGINFO ("eem win queue usage: %d", diag_max_win_queue_usage);
  MESSAGE ("eem destructed");
}
//---------------------------------------------------------------------------
void Eem::push_event(int event)
{
  if (status.is(eem_Initialized|eem_Active|eem_AutoActive))
  {
    if (queue_free==0)
      FAILURE("local queue overflow [Eem::push_event]");
    if (event==ev_Timer)
    {
      if (((*players)[host_player].states.getquantity()+queue_delay)>=buffer_delay)
        return;
      else
        queue_delay++;
    }
    if ((event==ev_Timer)&&(queue_free<queue_size)&&(queue[queue_last]&ev_Timer))
    {
      if ((short)(queue[queue_last])<65535)
        (short)(queue[queue_last])++;
    }
    else
    {
      queue[queue_tail]=event;
      queue_last=queue_tail;
      queue_tail++;
      if (queue_tail==queue_size)
        queue_tail=0;
      queue_free--;
    }
  }
}
//---------------------------------------------------------------------------
int Eem::pop_event(void)
{
  if (queue_free==queue_size)
    return(0);
  if ((queue[queue_head]&ev_Timer)!=0)
    queue_delay--;
  if ((queue[queue_head]&ev_Timer)&&((short)(queue[queue_head])))
  {
    (short)(queue[queue_head])--;
    return(queue[queue_head]+1);
  }
  else
  {
    int pos=queue_head;
    queue_head++;
    if (queue_head==queue_size)
      queue_head=0;
    queue_free++;
    return (queue[pos]);
  }
}
//---------------------------------------------------------------------------
int Eem::read (unsigned user_sync_data, unsigned char user_flags)
{
  // Port: when the previous read found nothing to do, sleep until the next timer tick or an
  // event instead of spinning (the caller has already drawn the frame by then).
  static int previous_read_empty=0;
  if (previous_read_empty&&!last_return_code&&status.is(eem_Timed))
    Comm::wait_messages(Timer::ms_to_next());
  do
  {
    if (last_return_code)
    {
      sync_queue->push(user_sync_data);
      user_queue->push(user_flags);
    }
    do
    {
      receive_states();
      Comm::process_messages();
      Mouse::flush();  // port: see Mouse::sdl_event
      if (untimed)
        Joy::callback_joy();  // port: no timer to poll it (callback_timer)
    }
    while (!status.is(eem_Active|eem_AutoActive));
    receive_states();
    if (last_return_code)
      forget_event();
    if (status.is(eem_EventCompleted))
    {
      next_read();
      if (!status.is(eem_NetWait))             
      {
        Kbd::next_read();
        Mouse::next_read();
        Joy::next_read();
        Vkey::next_read();
        status.reset(eem_EventCompleted);
      }
    }
    int event;
    do
    {
      event=0;
      if (!status.is(eem_NetWait))
      {
        event=pop_event();
        if (event&ev_Kbd)
          Kbd::get_event(event);
        else if (event&ev_Mouse)
          Mouse::get_event(event);
        else if (event&ev_Joy)
          Joy::get_event(event);
        else if ((event == 0)&&(!status.is(eem_Timed)))
        {
          if (status.is(eem_LastTimer))
            status.reset(eem_LastTimer);
          else
          {
            status.set(eem_LastTimer);
            event = ev_Timer;
          }
        }
      }
    } while ((event!=0)&&(!(event&ev_Timer)));
    if (event&ev_Timer)
    {
      Kbd::complete();
      Mouse::complete();
      Joy::complete();
      Vkey::complete();
      complete();
    }
    last_return_code = (event_available()) &&
                        (((*players)[host_player].states.getquantity()>1) || 
                         (status.is(eem_EventCompleted)));
    if (last_return_code&&(status.in(eem_Multi|eem_DemoPlay|eem_DemoRecord)&&status.is(eem_CheckSync)))
      chk_sync();
  }
  while ((!last_return_code) && (status.is(eem_Multi)) && (!status.is(eem_InNetActive)));
  previous_read_empty=!last_return_code;
  if (last_return_code&&dump)
    dump_state();
  return(last_return_code);
}
//---------------------------------------------------------------------------
// Port: with the engine switch input_dump=1, input_states.txt in the preferences directory gets
// the local player's input state of each tick that differs from the line before, after the tick
// number: the keys (set-1 scan codes, +128 extended) held, struck (+) and released (-), the text,
// the mouse, the virtual keys and the joystick bits. It is what demos record and network play
// sends, so tests compare it across platforms (tests/check_input.cmake).
static void dump_bits (char *&p, const unsigned char *bits, int bytes, const char *prefix)
{
  for (int i=0; i<bytes*8; i++)
    if (Bit::is((unsigned char*)bits,i))
    {
      p+=sprintf(p,"%s%x",prefix,i);
      prefix=",";
    }
}

void Eem::dump_state (void)
{
  static FILE *f=NULL;
  static unsigned ticks=0;
  static char last[4096];
  ticks++;
  if (!f)
  {
    char path[_MAX_PATH];
    snprintf(path,sizeof(path),"%sinput_states.txt",Comm::pref_path);
    if (!(f=fopen(path,"w")))
    {
      dump=0;
      return;
    }
  }
  Player_state *st=(*players)[local_player].states.peek();
  char line[4096], *p=line;
  p+=sprintf(p,"flags %02x kbd",(int)st->user_flags);
  dump_bits(p,st->kbd.arr_state,KBD_ARRAY_SIZE," ");
  dump_bits(p,st->kbd.arr_struck,KBD_ARRAY_SIZE," +");
  dump_bits(p,st->kbd.arr_release,KBD_ARRAY_SIZE," -");
  p+=sprintf(p," text \"");
  for (int i=0; (i<KBD_STRING_SIZE)&&st->kbd.string[i]; i++)
    p+=sprintf(p,"%c",st->kbd.string[i]);
  p+=sprintf(p,"\" mouse %d,%d",(int)st->mouse.x,(int)st->mouse.y);
  dump_bits(p,&st->mouse.arr_state,1," ");
  dump_bits(p,&st->mouse.arr_struck,1," +");
  dump_bits(p,&st->mouse.arr_release,1," -");
  dump_bits(p,&st->mouse.arr_dblclk,1," d");
  p+=sprintf(p," vkey");
  dump_bits(p,st->vkey.arr_state,VKEY_ARRAY_SIZE," ");
  dump_bits(p,st->vkey.arr_struck,VKEY_ARRAY_SIZE," +");
  dump_bits(p,st->vkey.arr_release,VKEY_ARRAY_SIZE," -");
  p+=sprintf(p," joy");
  dump_bits(p,st->joy.arr_state,JOY_ARRAY_SIZE," ");
  dump_bits(p,st->joy.prev_arr_state,JOY_ARRAY_SIZE," prev ");
  if (strcmp(line,last))
  {
    strcpy(last,line);
    fprintf(f,"%u %s\n",ticks,line);
    fflush(f);
  }
}
//---------------------------------------------------------------------------
void Eem::next_read (void)
{
  if ((*players)[host_player].states.getquantity()<buffer_delay)
  {
    DBG_CHECK((*players)[host_player].states.getfree()>0);
    if (new_state)
      old_state = *new_state;
    else
      memset(&old_state,0, sizeof(Player_state));
    new_state = (*players)[host_player].states.pushempty();
    memset (new_state, 0, sizeof(Player_state));
    new_state->chknumber = old_state.chknumber+1;
    new_state->serie     = serie;
    if (sync_queue->getquantity()>0)
      new_state->user_sync = sync_queue->pop();
    else
      new_state->user_sync = 0;
    if (user_queue->getquantity()>0)
      new_state->user_flags = user_queue->pop();
    else
      new_state->user_flags = 0;
    if (!status.is(eem_AfterClear))
    {
      memcpy (new_state->kbd.arr_state,   old_state.kbd.arr_state,   
              KBD_ARRAY_SIZE*sizeof(new_state->kbd.arr_state[0]));
      new_state->mouse.arr_state = old_state.mouse.arr_state;
      memcpy (new_state->joy.prev_arr_state, old_state.joy.arr_state, JOY_ARRAY_SIZE);
      memcpy (new_state->joy.arr_state, old_state.joy.arr_state, JOY_ARRAY_SIZE);
    }
    else
      status.reset(eem_AfterClear);
  }
  else
    status.set(eem_NetWait);
}
//---------------------------------------------------------------------------
void Eem::start(void)
{
  DBG_CHECK(!status.is(eem_Active));
  Kbd::clear();
  Mouse::clear();
  Joy::clear();
  Vkey::clear();
  clear();
  for (int i=1; i<=players_max; i++)
  {
    (*players)[i].states.purge();
    (*players)[i].sync_queue.purge();
    memset(&((*players)[i].user_block),0,sizeof(User_block));
    (*players)[i].status.reset(pl_UserBlock);
  }
  sync_queue->purge();
  user_queue->purge();
  memset(&user_block,0,sizeof(User_block));

  if (status.is(eem_Multi))
  {
    serie++;
    apply_pending();  // port: messages that arrived before this peer started the series
  }

  host_player = 0xFFFFFFFF;
  if (status.is(eem_DemoPlay))
    host_player = demo_player;
  else
    host_player = local_player;

  status.reset(eem_EventCompleted);
  status.reset(eem_NetWait);
  status.reset(eem_LastTimer);
  status.reset(eem_UserBlock);
  status.set(eem_Active);

  if (status.in(eem_DemoRecord|eem_DemoPlay))
    status.set(eem_DemoActive);

  new_state = NULL;
  max_ping = max_ping_delay*4;
  last_return_code = 0;
  net_mask = 0;
  last_send_mask = 0;
  ping_counter = 0;
  diag_win_queue_usage = 0;

  next_read();

  if (!Comm::production)
    MESSAGE("eem started");
}
//---------------------------------------------------------------------------
void Eem::stop(void)
{
  DBG_CHECK(status.is(eem_Active));
  if (status.is(eem_DemoRecord))
  {
    Demo_recorder::close(); 
    status.reset(eem_DemoRecord);
  }
  if (status.is(eem_DemoPlay))
  {
    Demo_player::close(); 
    status.reset(eem_DemoPlay);
    players_max = 1;
    demo_player = 0xFFFFFFFF;
    (*players)[1].status.reset (pl_Deleted);
  }
  if (status.is(eem_Multi))
    Net::purge();
  status.reset(eem_Active);
  status.reset(eem_DemoActive);
  if (!Comm::production)
    MESSAGE("eem stopped");
}
//---------------------------------------------------------------------------
void Eem::terminate(void)
{
  DBG_CHECK(status.is(eem_Active));
  status.reset(eem_Active);
  MESSAGE("eem terminated - end of game");
}
//---------------------------------------------------------------------------
void Eem::complete (void)
{
  if (status.is(eem_DemoPlay|eem_DemoActive))
  {
    BOOL res = TRUE;
    for (int i=1; i<players_max; i++)  // bez ostatniego
    {
      unsigned size = sizeof(Player_state);
      Player_state *pst = (*players)[i].states.pushempty();
      res = Demo_player::play((char*)pst, &size);
      if (!res)
      {
        status.reset(eem_DemoActive);
        break;
      }
    }
    if (!res)
    {
      for (int i=1; i<players_max; i++)  // bez ostatniego
      {
        (*players)[i].status.set (pl_Deleted);
        (*players)[i].states.purge();
        Player_state *pst = (*players)[i].states.pushempty();
        memset(pst, 0, sizeof(Player_state));
      }
    }
  }
  status.set(eem_EventCompleted);
  receive_states();
  send_state();
}

//---------------------------------------------------------------------------
void Eem::forget_event (void)
{
  status.reset(eem_NetWait);
  int i;
  if (status.is(eem_DemoRecord|eem_DemoActive))
  {
    for (i=1; i<=players_max; i++)                            
    {
      unsigned size = sizeof(Player_state);
      BOOL res = Demo_recorder::record((char*)((*players)[i].states.peek()), (unsigned char)size);
      if (!res)
      {
        status.reset(eem_DemoActive);
        break;
      }
    }
  }
  for (i=1; i<=players_max; i++)
    if (is_present(i))
      (*players)[i].states.popempty();
}
//---------------------------------------------------------------------------
int Eem::event_available(void)
{
  int result = 1;
  for (int i=1; i<=players_max; i++)
    if ( is_present(i) && ((*players)[i].states.getquantity()==0))
      result = 0;
  return (result);
}
//---------------------------------------------------------------------------

void Eem::chk_user_sync(void)
{
  BOOL all_ready = TRUE;
  BOOL first     = TRUE;
  int i;
  for (i=1; i<=players_max; i++)
  {
    Player *player = &(*players)[i];
    if (is_present(i))
    {
      unsigned us = player->states.peek()->user_sync;
      if (us!=0)
      {
        if (player->sync_queue.getfree()==0)
        {
          error_dump_all();
          WARNING("Eem user synchronization failed, player %d sync queue overflow", i);
          if (status.is(eem_Multi))
            throw Eem_net_sync_failure();
          else if (status.in(eem_DemoPlay))
            throw Eem_demo_sync_failure();
          else
            throw Failure();
        }
        player->sync_queue.push (us);
      }
      if (player->sync_queue.getquantity()==0)
        all_ready = FALSE;
    }
  }
  unsigned user_sync = 0;
  if (all_ready)
  {
    for (i=1; i<=players_max; i++)
    {
      Player *player = &(*players)[i];
      if (is_present(i))
      {
        if (!first)
        {
          if (user_sync!=*(player->sync_queue.peek()))
          {
            error_dump_all();
            WARNING ("Eem user sychronization failed");
            char  buf [256];
            char dbuf [80];
            strcpy (buf, "DUMP: ");
            for (int k=1; k<=players_max; k++)
            {
              if (is_present(k))
                sprintf(dbuf, "%08X  ",(int)(*((*players)[k].sync_queue.peek())));
              else
                sprintf(dbuf, "deleted   ");
              strcat(buf, dbuf);
            }
            WARNING (buf);
            if (status.is(eem_Multi))
              throw Eem_net_sync_failure();
            else if (status.in(eem_DemoPlay))
              throw Eem_demo_sync_failure();
            else
              throw Failure();

          }
        }
        else
        {
          user_sync = *(player->sync_queue.peek());
          first = FALSE;
        }
      }
    }
    for (i=1; i<=players_max; i++)
      if (is_present(i))
        (*players)[i].sync_queue.popempty();
  }

}
//---------------------------------------------------------------------------
void Eem::chk_internal_sync(void)
{
  int i;
  BOOL first = TRUE;
  unsigned chknumber = 0;
  for (i=1; i<=players_max; i++)
  {
    Player *player = &(*players)[i];
    if (is_present(i))
    {
      if (!first)
      {                  
        if (chknumber!=player->states.peek()->chknumber)
        {
          error_dump_all();
          WARNING ("Eem internal sychronization failed, packet %d != %d", 
                   chknumber, (int)(player->states.peek()->chknumber));
          if (status.is(eem_Multi))
            throw Eem_net_sync_failure();
          else if (status.in(eem_DemoPlay))
            throw Eem_demo_sync_failure();
          else
            throw Failure();

        }
      }
      else
      {
        chknumber = player->states.peek()->chknumber;
        first = FALSE;
      }
    }
  }
}
//---------------------------------------------------------------------------
void Eem::chk_sync(void)
{
  if (status.in(eem_Multi|eem_DemoPlay|eem_DemoRecord)&&status.is(eem_CheckSync))
  {
    chk_internal_sync();
    chk_user_sync();
  }
}
//---------------------------------------------------------------------------
void Eem::clear(void)
{
  queue_head=0;
  queue_tail=0;
  queue_last=0;
  queue_delay=0;
  queue_free=queue_size;
  status.set(eem_AfterClear);
}
//===========================================================================
// supporting subroutines
//===========================================================================
int diff_byte_calcdest (unsigned size)
{
  DBG_CHECK(size>0);
  DBG_CHECK(size<256);
  int markers = size/8+1;
  if (size%8>0)
    markers++;
  return(markers);
}
//---------------------------------------------------------------------------
int Eem::get_player_pos(unsigned player)
{
  int pos = -1;
  for (int i=1; i<=players_max; i++)
    if ((*players)[i].id==player)
    {
      pos = i;
      break;
    }
  return (pos);
}
//---------------------------------------------------------------------------
void  Eem::send_state(void)
{
  if (!status.is(eem_Multi))
    return;
  unsigned char outbuf[Net::max_packet_size];
  if (status.is(eem_Server))
    new_state->flags|=flag_Server;
  Player_state pst = *new_state;
  if (last_send_mask.is(mask_Keyboard))
    memset(&old_state.kbd, 0, sizeof(old_state.kbd));
  if (last_send_mask.is(mask_Mouse))
    memset(&old_state.mouse, 0, sizeof(old_state.mouse));
  if (last_send_mask.is(mask_Joy))
    memset(&old_state.joy, 0, sizeof(old_state.joy));
  if (last_send_mask.is(mask_Vkey))
    memset(&old_state.vkey, 0, sizeof(old_state.vkey));
  if (net_mask.in(mask_All))
  {
    if (net_mask.is(mask_Keyboard))
      memset(&pst.kbd, 0, sizeof(pst.kbd));
    if (net_mask.is(mask_Mouse))
      memset(&pst.mouse, 0, sizeof(pst.mouse));
    if (net_mask.is(mask_Joy))
      memset(&pst.joy, 0, sizeof(pst.joy));
    if (net_mask.is(mask_Vkey))
      memset(&pst.vkey, 0, sizeof(pst.vkey));
  }
  last_send_mask = net_mask;
  unsigned dest_size = 0;
  if (status.is(eem_UserBlock))
  {
    DBG_MESSAGE("user block send");
    user_block.type = pkt_UserBlock;
    user_block.serie = serie;  // port
    if (status.is(eem_PktCompress))
    {
      dest_size = rle_8_compress (outbuf, (unsigned char*)(&user_block), sizeof(User_block));
      DBG_CHECK(dest_size <= Net::max_packet_size);
    }
    else
    {
      memcpy(outbuf, &user_block, sizeof(User_block));
      dest_size = sizeof(User_block);
    }
    for (int i=1; i<=players_max; i++)
      if (is_present(i) && (!is_local(i)))
        Net::send(outbuf, dest_size, (*players)[i].id);
    status.reset(eem_UserBlock);
  }
  dest_size = 0;
  pst.type = pkt_States;
  if (status.is(eem_PktCompress))
  {
    dest_size = rle_8_compress (outbuf, (unsigned char*)(&pst), sizeof(Player_state));
    DBG_CHECK(dest_size <= Net::max_packet_size);
  }
  else
  {
    memcpy(outbuf, &pst, sizeof(Player_state));
    dest_size = sizeof(Player_state);
  }
  for (int i=1; i<=players_max; i++)
    if (is_present(i) && (!is_local(i)))
      Net::send(outbuf, dest_size, (*players)[i].id);
}
//---------------------------------------------------------------------------
// Port: a message from player pos, checked before use. It is used in its own series; one of a later
// series is left for apply_pending, because a peer may start the next series first (the original's
// 300 ms purge in Eem::stop used to hide that); one of an earlier series is dropped.
int Eem::apply_message(int pos, unsigned char *msg, unsigned size)
{
  if ((msg[0]==pkt_UserBlock)&&(size==sizeof(User_block)))
  {
    signed char ahead = (signed char)(((User_block*)msg)->serie-serie);
    if (ahead>0)
      return msg_Later;
    if (ahead<0)
      return msg_Dropped;
    memcpy((unsigned char*)(&((*players)[pos].user_block)), msg, size);
    (*players)[pos].status.set(pl_UserBlock);
    DBG_MESSAGE("user block received, player %d", pos);
    return msg_Used;
  }
  if ((msg[0]==pkt_States)&&(size==sizeof(Player_state)))
  {
    Player_state &pst = *((Player_state*)(msg));
    signed char ahead = (signed char)(pst.serie-serie);
    if (ahead>0)
      return msg_Later;
    if (ahead<0)
    {
      DBG_MESSAGE ("purging old message: player:%d  msg:%d  serie:%d (current:%d)", 
                   (int)pos, (int)pst.chknumber, (int)pst.serie, (int)serie);
      return msg_Dropped;
    }
    if (((pst.flags&(flag_Server))!=0))
      server_player = pos;
    if ((*players)[pos].states.getfree()==0)
      FAILURE("buffer overflow for remote player [Eem::receive_states]");
    (*players)[pos].states.push(pst);
    return msg_Used;
  }
  WARNING("message of type %d and %d bytes from player %d ignored", (int)msg[0], (int)size, pos);
  return msg_Dropped;
}
//---------------------------------------------------------------------------
// Port: after starting a series, the messages that were waiting for it.
void Eem::apply_pending(void)
{
  for (int i=1; i<=players_max; i++)
    while ((*players)[i].pending.getquantity()>0)
    {
      Pending *p = (*players)[i].pending.peek();
      if (apply_message(i, p->data, p->size)==msg_Later)
        break;
      (*players)[i].pending.popempty();
    }
}
//---------------------------------------------------------------------------
void  Eem::receive_states(void)
{
   if (!status.is(eem_Multi))
    return;
   unsigned char inbuf[Net::max_packet_size];
  BOOL res = TRUE;
  while (res)
  {
    unsigned size = Net::max_packet_size;
    unsigned from = 0;
    res = Net::receive(inbuf,&size, (DPID*)(&from));
    if (res)
    {
      ping_counter = 0;
      max_ping = max_ping_delay;
      int pos = get_player_pos(from);
      if (pos==-1)
        FAILURE("message from unexpected player received [Eem::receive_states]");
      if (!is_present(pos))
      {
        WARNING("message from deleted player received [Eem::receive_states]");
        continue;
      }
      unsigned char dest_buf[Net::max_packet_size];
      unsigned dest_size = 0;
      if (status.is(eem_PktCompress))
        dest_size = rle_8_decompress (dest_buf, inbuf, size);
      else
      {
        memcpy(dest_buf, inbuf, size);
        dest_size = size;
      }
      // Port: a message of a later series waits in pending, and so does everything after it from
      // that player (messages arrive in order).
      Queue<Pending> &pending = (*players)[pos].pending;
      if ((pending.getquantity()>0)||(apply_message(pos, dest_buf, dest_size)==msg_Later))
      {
        if (pending.getfree()==0)
          FAILURE("too many messages of a later series [Eem::receive_states]");
        Pending *p = pending.pushempty();
        p->size = dest_size;
        memcpy(p->data, dest_buf, dest_size);
      }
    }
    else
    {
      if (ping_counter>max_ping)
      {
        Net::ping();
        ping_counter = 0;
        max_ping += max_ping;
      }
      if (Net::get_players_num()!=(unsigned)players_max)
      {
        unsigned int count;
        int result; 
        Net::Player *plist = Net::list_players(&count, &result);
        if (res!=Net::res_OK)
          FAILURE("unknown error enumerating players [Eem::receive_states]");
        for (int i=1; i<=players_max; i++)
        {
          BOOL found = FALSE;
          for (int k=0; k<(int)count; k++)
            if ((*players)[i].id==plist[k].id)
            {
              found = TRUE;
              break;
            }
          if (!found)
          {
            (*players)[i].status.set (pl_Deleted);
            (*players)[i].states.purge();
            (*players)[i].pending.purge();  // port
            Player_state *pst = (*players)[i].states.pushempty();
            memset(pst, 0, sizeof(Player_state));
            if (server_player==i)
            {
              server_player = local_player;
              status.set(eem_Server);
              for (int j=1; j<=players_max; j++)
                if ((j!=local_player)&&(!(*players)[j].status.is(pl_Deleted)))
                {
                  (*players)[j].status.set (pl_Deleted);
                  (*players)[j].states.purge();
                  (*players)[j].pending.purge();  // port
                  Player_state *pst = (*players)[j].states.pushempty();
                  memset(pst, 0, sizeof(Player_state));
                }
              break;
            }
          }
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void Eem::error_dump_all(void)
{
  MESSAGE("Dumping EEM");
  MESSAGE("-----------");
  MESSAGE("LastReturnCode:%d  NetWait:%d  AutoActive:%d  Active:%d  Server:%d",
           last_return_code,
           (int)(status.is(eem_NetWait)?1:0),
           (int)(status.is(eem_AutoActive)?1:0),
           (int)(status.is(eem_Active)?1:0),
           (int)(status.is(eem_Server)?1:0)
         );
  MESSAGE("EventCompleted:%d  PktCompress:%d  CheckSync:%d  Serie:%d",
           (int)(status.is(eem_EventCompleted)?1:0),
           (int)(status.is(eem_PktCompress)?1:0),
           (int)(status.is(eem_CheckSync)?1:0),
           (int)(serie)
         );
  MESSAGE("QueueSize:%d  QueueDelay:%d  QueueFree:%d",
           queue_size,
           queue_delay,
           queue_free
         );
  MESSAGE("PlayersMax:%d  SelectedPlayer:%d  LocalPlayer:%d  ServerPlayer:%d",
           players_max,
           selected_player,
           local_player,
           server_player);
  MESSAGE("OldState:");
  error_dump_state(&old_state);
  MESSAGE("NewState:");
  if (new_state!=NULL)
    error_dump_state(new_state);
  else
    MESSAGE("   NULL");
  MESSAGE("Players:");
  for (int i=1; i<=players_max; i++)
    error_dump_player(&((*players)[i]));
}
//---------------------------------------------------------------------------
void Eem::error_dump_player(Eem::Player *player)
{
  MESSAGE("   ID:%d  Deleted:%d  Name:%s",
                 player->id,
                 (int)(player->status.is(pl_Deleted)?1:0),
                 player->player_name
         );
  int i;
  char buf[1024];
  char tmp[256];
  strcpy(buf,"   SyncQueue: '");
  for (i=0; i<player->sync_queue.getquantity(); i++)
  {
    sprintf(tmp, "%08X  ", player->sync_queue[i]);
    strcat(buf, tmp);
  }
  strcat(buf, "'");
  MESSAGE(buf);
  MESSAGE("      States:");
  for (i=0; i<player->states.getquantity(); i++)
    error_dump_state(&(player->states[i]));
}
//---------------------------------------------------------------------------
void Eem::error_dump_state(Eem::Player_state *state)
{
  MESSAGE("         chknumber:%08d  user_sync:%08X  serie:%d user_flag:%02X", 
           state->chknumber, state->user_sync, (int)(state->serie), (int)(state->user_flags));
}
//---------------------------------------------------------------------------
unsigned char Eem::get_user_flags(void)
{
  DBG_CHECK(last_return_code!=0);
  DBG_CHECK(selected()!=NULL);
  return(selected()->user_flags);

}
//---------------------------------------------------------------------------
void  Eem::demo_set_record_mode (char *file_name, unsigned max_size, 
                                 char *user_data, unsigned user_data_size)
{
  DBG_CHECK(!status.is(eem_Active));
  Demo_recorder::open(file_name, user_data, user_data_size, store_RLE8, players_max, max_size);
  status.set(eem_DemoRecord);
}
//---------------------------------------------------------------------------

BOOL  Eem::demo_set_play_mode (char *file_name, char *user_data, 
                               unsigned *user_data_size)
{
  DBG_CHECK(!status.is(eem_Active));
  unsigned players_temp;
  if (!Demo_player::open(file_name, &players_temp, user_data, user_data_size))
    return(FALSE);
  else
  {
    players_max = players_temp+1;
    demo_player = players_max;
    status.set(eem_DemoPlay);
    return(TRUE);
  }
}
//---------------------------------------------------------------------------
BOOL Eem::demo_is_active()
{
  return(status.is(eem_DemoActive));
}
//---------------------------------------------------------------------------
void Eem::demo_terminate()
{
  status.reset(eem_DemoActive);
}
//---------------------------------------------------------------------------
void Eem::demo_progress(unsigned *played, unsigned *blocks)
{
  *played=Demo_player::closed_played;
  *blocks=Demo_player::closed_blocks;
}
//---------------------------------------------------------------------------
void Eem::user_block_send(unsigned char *buffer, unsigned size)
{
  DBG_CHECK(status.is(eem_Active));
  DBG_CHECK(buffer != NULL);
  DBG_CHECK(size<=USER_BLOCK_DATA_SIZE);
  unsigned real_size = (size<=(USER_BLOCK_DATA_SIZE))?size:(USER_BLOCK_DATA_SIZE);
  memcpy(user_block.data, buffer, real_size);
  user_block.size = (unsigned char)real_size;
  user_block.type = pkt_UserBlock;
  memcpy (&((*players)[local_player].user_block), &user_block, sizeof(User_block));
  (*players)[local_player].status.set(pl_UserBlock);
  status.set(eem_UserBlock);
}
//---------------------------------------------------------------------------
BOOL Eem::user_block_receive(unsigned char *buffer, unsigned *size)
{
  DBG_CHECK(status.is(eem_Active));
  DBG_CHECK(buffer != NULL);
  DBG_CHECK(size != NULL);
  if ((*players)[selected_player].status.is(pl_UserBlock))
  {
    *size = (*players)[selected_player].user_block.size;
    if (*size)
      memcpy(buffer, (*players)[selected_player].user_block.data, *size);
    (*players)[selected_player].status.reset(pl_UserBlock);
    return (TRUE);
  }
  else
  {
    return(FALSE);
  }
}
//---------------------------------------------------------------------------
BOOL Eem::user_block_available()
{
  DBG_CHECK(status.is(eem_Active));
  if ((*players)[selected_player].status.is(pl_UserBlock))
    return (TRUE);
  else
    return(FALSE);
}
//---------------------------------------------------------------------------









