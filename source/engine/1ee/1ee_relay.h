// Port: the relay server's protocol (phase 7 and 9), shared by the relay (tools/relay) and the
// game's transports: ENet on the desktop (1ee_enet.cpp), WebSocket in the browser (1ee_ws.cpp).
// Plain constants only, so the relay can include it without the engine.
//
// A relay carries sessions for players who can't reach each other directly: browsers, which have
// no UDP sockets and can't accept connections, and hosts behind NAT. Everyone connects out to the
// relay. A session has a host and up to max_clients clients; the relay forwards between the host
// and each client and knows nothing of the game. The game's own session protocol (HELLO,
// WELCOME, START, DATA, ...) runs inside, unchanged: the host still forwards between clients.
//
// Every message is one ENet packet (reliable, channel 0) or one binary WebSocket message. The
// first byte is its type.
//
//   peer -> relay
//     OPEN       type, protocol, clients, code[code_len]     open a session as its host; an
//                                                             all-zero code lets the relay pick
//     ENTER      type, protocol, code[code_len]              join a session as a client
//     TO         type, link, payload                         host: to that client
//     PASS       type, payload                               client: to the host
//     CLOSE      type, link                                  host: drop that client
//   relay -> peer
//     OPENED     type, code[code_len]                        the session is open
//     ENTERED    type                                        joined; PASS reaches the host
//     REFUSED    type, reason (refused_*)                    then the relay disconnects
//     LINK_UP    type, link                                  host: a client entered
//     FROM       type, link, payload                         host: from that client
//     LINK_DOWN  type, link                                  host: that client left
//     PASS       type, payload                               client: from the host
//
// When the host leaves, the relay disconnects its clients.

#ifndef FF_1EE_RELAY_H
#define FF_1EE_RELAY_H

namespace relay_protocol
{
  enum
  {
    version     = 1,
    code_len    = 6,
    max_clients = 7,
    max_payload = 512,

    enet_port   = 19970,  // UDP, ENet
    ws_port     = 19971,  // TCP, WebSocket (wss:// through a TLS proxy in front of it)

    msg_open = 1, msg_enter, msg_to, msg_pass, msg_close,
    msg_opened = 10, msg_entered, msg_refused, msg_link_up, msg_from, msg_link_down,

    refused_version = 1,  // another protocol version
    refused_no_session,   // no session has that code
    refused_full,         // the session has all its clients
    refused_code_taken,   // the code asked for is in use
    refused_busy,         // the relay has no room for another session or connection
    refused_bad_message   // malformed, out of turn, or too many
  };

  // The characters of a session code: capitals and digits without look-alikes (O and 0, I and 1).
  // The engine upper-cases its command line, so codes are case-insensitive.
  static const char code_chars[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
}

#endif
