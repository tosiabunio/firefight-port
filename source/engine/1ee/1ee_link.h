// Port: inside the network transport (1ee_enet.h). The session logic (1ee_session.cpp: who is
// player 1..4, the roster, the host forwarding messages, the inbox) runs on links, which a link
// layer provides:
//   1ee_enet.cpp  ENet: directly between host and clients, or through a relay server (desktop)
//   1ee_ws.cpp    WebSocket to a relay server (browser)
// A link is a numbered connection: on the host, one per client; on a client, the one to the host.
// Like the transport, nothing here may have a static destructor (see Net::quit).

#ifndef FF_1EE_LINK_H
#define FF_1EE_LINK_H

#include "1ee_enet.h"

#include <stddef.h>

namespace enet_transport
{
  enum { max_links = 64 };  // link numbers are 1..max_links-1

  // The link layer.
  namespace link
  {
    bool startup(void);
    void shutdown(void);
    bool listen(unsigned short port);                           // host directly (ENet)
    bool connect(const char *address, unsigned short port);    // join directly (ENet)
    bool relay_open(const char *relay, int clients, const char *code);  // host through a relay
    bool relay_enter(const char *relay, const char *code);              // join through a relay
    const char *code(void);  // the relay's code for the hosted session, or ""
    void send(int link, const unsigned char *data, size_t size);
    void close(int link);    // after what was sent to it
    void poll(void);         // reports what happened through the calls below
    void leave(void);
    int  discover(unsigned short port, unsigned timeout, Found *games, int max);
  }

  // The session logic, called by the link layer.
  void link_up(int link);    // host: a client connected; client: the host can be reached
  void link_message(int link, const unsigned char *data, size_t size);
  void link_down(int link);
  void link_failed(const char *why);
  void report(const char *format, ...);

  // What LAN discovery answers about a hosted game.
  struct Summary
  {
    unsigned token;
    int      players;
    int      joined;
    bool     started;
    const char *host;
  };
  void summary(Summary *s);
}

#endif
