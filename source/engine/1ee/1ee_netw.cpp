#include "1ee_hdrs.h"
#include "1ee_enet.h"

// Port: Net on ENet (1ee_enet.cpp) instead of DirectPlay. The host relays for the clients, and the
// transport is reliable and ordered, so this file only sets the session up and passes the engine's
// messages through. The original's resend layer, session enumeration and providers are gone.

#define NET_VERSION enet_transport::version  // was 3 (DirectPlay)

enum
{
  net_Initialized = 0x0001,
  net_InGame      = 0x0002
};

Bitflag            Net::status;
Net::Player        Net::players[Net::max_enum_players];
int                Net::players_qty = 0;
Net::Game          Net::current_game;
Net::Player        Net::current_player;
unsigned           Net::diag_total_out = 0;
unsigned           Net::diag_total_in = 0;

static void log_line(const char *text)
{
  MESSAGE("%s", text);
}
//-----------------------------------------------------------------------------
void Net::quit (void)
{
  enet_transport::shutdown();
  ENGINFO ("net statistics: messages received %d, messages sent %d", diag_total_in, diag_total_out);
  status.reset(net_Initialized|net_InGame);
}
//-----------------------------------------------------------------------------
BOOL Net::is_initialised ()
{
  return (status.is(net_Initialized));
}
//-----------------------------------------------------------------------------
void Net::init (int debug)
{
  (void)debug;
  DBG_CHECK(!status.is(net_Initialized));
  if (!enet_transport::startup(log_line))
    FAILURE2(Eem_error::dp_not_installed, "network initialization failed (ENet)");
  memset(players, 0, sizeof(players));
  players_qty = 0;
  memset(&current_game, 0, sizeof(current_game));
  memset(&current_player, 0, sizeof(current_player));
  Comm::quit_me(Net::quit,"net","log mmu");
  status.set(net_Initialized);
}
//-----------------------------------------------------------------------------
void Net::close (void)
{
  enet_transport::shutdown();
}
//-----------------------------------------------------------------------------
// Polls the transport, keeping the window alive, until done() or the timeout (ms).
void Net::wait (unsigned timeout, int (*done)(void), int error, const char *what)
{
  unsigned start = timeGetTime();
  for (;;)
  {
    enet_transport::poll();
    if (enet_transport::failure())
      FAILURE2(Eem_error::error_connecting, "network game: %s", enet_transport::failure());
    if (done())
      return;
    if (timeGetTime()-start > timeout)
      FAILURE2(error, "network game: %s", what);
    Comm::wait_messages(10);
    Comm::process_messages();
  }
}
//-----------------------------------------------------------------------------
static int all_joined (void)
{
  return enet_transport::joined()==enet_transport::players_max();
}
static int started (void)
{
  return enet_transport::started();
}
//-----------------------------------------------------------------------------
static void fill_session (Net::Game *game, Net::Player *player, char *player_name)
{
  unsigned char info[Net::game_info_len];
  game->max_players = enet_transport::players_max();
  game->has_info = enet_transport::game_info(info);
  memcpy(game->info, info, Net::game_info_len);
  player->id = enet_transport::my_id();
  strncpy(player->name, player_name, Net::player_name_len-1);
  player->name[Net::player_name_len-1] = 0;
}
//-----------------------------------------------------------------------------
void Net::game_create (int players_max, unsigned short port, char *player_name, char *game_info,
                       unsigned timeout)
{
  DBG_CHECK(status.is(net_Initialized));
  if (!enet_transport::host(port, players_max, player_name, (unsigned char*)game_info))
    FAILURE2(Eem_error::error_connecting, "network game: %s", enet_transport::failure());
  MESSAGE("waiting for %d players", players_max-1);
  wait(timeout, all_joined, Eem_error::net_player_not_found, "not enough players joined in time");
  enet_transport::start();
  fill_session(&current_game, &current_player, player_name);
  status.set(net_InGame);
  list_players();
}
//-----------------------------------------------------------------------------
void Net::game_connect (const char *address, unsigned short port, char *player_name, unsigned timeout)
{
  DBG_CHECK(status.is(net_Initialized));
  if (!enet_transport::join(address, port, player_name))
    FAILURE2(Eem_error::error_connecting, "network game: %s", enet_transport::failure());
  wait(timeout, started, Eem_error::net_game_not_found, "no game started at that address in time");
  fill_session(&current_game, &current_player, player_name);
  status.set(net_InGame);
  list_players();
}
//-----------------------------------------------------------------------------
void Net::game_destroy (void)
{
  enet_transport::leave();
  status.reset(net_InGame);
}
//-----------------------------------------------------------------------------
Net::Player *Net::list_players (unsigned *count, int *result)
{
  enet_transport::Player roster[enet_transport::max_players];
  int n = enet_transport::roster(roster);
  memset(players, 0, sizeof(players));
  for (int i=0; i<n && i<max_enum_players; i++)
  {
    players[i].id = roster[i].id;
    memcpy(players[i].name, roster[i].name, player_name_len);
  }
  players_qty = n;
  if (count)
    *count = n;
  if (result)
    *result = res_OK;
  return (players);
}
//-----------------------------------------------------------------------------
unsigned Net::get_players_num (void)
{
  enet_transport::Player roster[enet_transport::max_players];
  return (unsigned)enet_transport::roster(roster);
}
//-----------------------------------------------------------------------------
Net::Game *Net::get_current_game (void)
{
  return (&current_game);
}
//-----------------------------------------------------------------------------
Net::Player *Net::get_current_player (void)
{
  return (&current_player);
}
//-----------------------------------------------------------------------------
// A message to a player who has just left is dropped: Eem sees the departure on its next receive.
void Net::send (void *buffer, unsigned size, DPID to)
{
  DBG_CHECK(size<=max_packet_size);
  if (enet_transport::send(to, buffer, size))
    diag_total_out++;
}
//-----------------------------------------------------------------------------
BOOL Net::receive (void *buffer, unsigned *size, DPID *from)
{
  unsigned sender = 0;
  if (!enet_transport::receive(buffer, size, &sender))
    return FALSE;
  *from = sender;
  diag_total_in++;
  return TRUE;
}
