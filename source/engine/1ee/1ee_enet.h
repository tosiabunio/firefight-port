// Port: the network transport for Net (1ee_netw.cpp). The session logic (1ee_session.cpp) runs on
// a link layer (1ee_link.h): ENet on the desktop (1ee_enet.cpp), directly or through a relay
// server, and WebSocket to a relay in the browser (1ee_ws.cpp). Kept apart from the engine headers
// because <enet/enet.h> brings in the real Windows socket headers, which clash with the Win32
// stand-ins in compat/win32.h.
//
// Topology: the host relays. Clients connect to the host only; the host forwards each message to
// the player it is addressed to. Player ids are 1 (the host) and 2..players in joining order. All
// messages go on one reliable, ordered ENet channel, so the original's own resend layer is gone.
//
// Session: the host listens and polls until enough players have joined, then calls start(),
// which sends every client the roster and the game info. A client connects and polls until
// started(). After that, send() and receive() carry the engine's messages, and the roster loses
// the players who leave (a client whose host leaves is left alone).
//
// Through a relay server (1ee_relay.h, tools/relay): everyone connects out to the relay, which
// forwards between the host and each client. set_relay() before host() or join() selects it: the
// host opens a session there and gets its code (session_code()), and join() takes that code in
// place of the address. The session protocol inside is the same.

#ifndef FF_1EE_ENET_H
#define FF_1EE_ENET_H

namespace enet_transport
{
  enum
  {
    max_players = 4,
    name_len    = 52,   // Net::player_name_len
    info_len    = 28,   // Net::game_info_len
    max_message = 255,  // Net::max_packet_size
    version     = 4     // NET_VERSION: DirectPlay was 3
  };

  struct Player
  {
    unsigned id;
    char     name[name_len];
  };

  bool startup(void (*log_function)(const char *text));
  void shutdown(void);

  // relay: "host[:port]" (ENet's UDP port, relay_protocol::enet_port by default); the browser
  // also takes a ws:// or wss:// URL. code: the session code a host asks for, or nullptr for one
  // the relay picks. A null or empty relay goes back to direct connections.
  void set_relay(const char *relay, const char *code);
  const char *session_code(void);  // the hosted session's code, or "" until the relay gives it

  bool host(unsigned short port, int players, const char *name, const unsigned char *info);
  bool join(const char *address, unsigned short port, const char *name);
  void poll(void);           // services ENet: connections, the roster, relaying
  int  joined(void);         // players so far, the host included (host)
  void start(void);          // host: send the roster and the game info, lock the session
  bool started(void);
  const char *failure(void); // why the session failed, or nullptr

  unsigned my_id(void);
  int  players_max(void);
  bool game_info(unsigned char *info);  // false if the host has none
  int  roster(Player *players);         // the players present, in id order

  bool send(unsigned to, const void *data, unsigned size);
  bool receive(void *data, unsigned *size, unsigned *from);
  void leave(void);

  // LAN discovery: a client asks every host on the LAN (a UDP broadcast to the game port, and
  // 127.0.0.1 for a game on the same computer) and collects the answers for timeout ms. A host
  // answers on its game socket, from any state. Each game appears once, by its session token.
  struct Found
  {
    char           address[64];  // the host's IP address, for join()
    unsigned short port;
    char           host[name_len];
    int            joined;
    int            players;
    bool           started;
    bool           compatible;   // same version
  };
  int discover(unsigned short port, unsigned timeout, Found *games, int max);
}

#endif
