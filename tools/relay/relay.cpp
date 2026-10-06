// The Fire Fight relay server (phases 7 and 9): forwards game sessions between players who all
// connect out to it, desktops over ENet (UDP) and browsers over WebSocket (TCP). The protocol is
// in source/engine/1ee/1ee_relay.h. The relay knows nothing of the game: a session is a host,
// its clients and a short code.
//
//   ff_relay [--port <udp>] [--ws-port <tcp>] [--rate <n>] [--quit-after <s>] [--once]
//
//   --port <udp>       ENet's port, 19970 by default
//   --ws-port <tcp>    the WebSocket port, 19971 by default. For wss:// put a TLS proxy in front
//                      of it (Caddy, nginx); a plain HTTP request gets "ff_relay", for health checks
//   --rate <n>         the most messages a connection may send in a second, 1000 by default (the
//                      game sends about 30 a second to each other player); 0 for no limit, for
//                      tests that run the game as fast as it goes (--fast)
//   --quit-after <s>   stop after s seconds (tests)
//   --once             stop when the first session has ended and everyone has left (tests)
//
// One thread and a select loop. Everything received is untrusted: lengths, types and order are
// checked, every connection has a message rate limit, and a connection that doesn't open or
// enter a session within 10 s is dropped.

#include "1ee_relay.h"

#include <enet/enet.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <random>
#include <string>
#include <vector>

namespace rp = relay_protocol;

namespace
{
const int      max_connections = 512;
const int      max_sessions    = 256;
const unsigned hello_timeout   = 10000;  // ms to open or enter a session
int            rate_limit      = 1000;   // messages a second per connection, 0 for no limit
const size_t   ws_max_request  = 4096;
const size_t   ws_max_output   = 256 * 1024;

enum Kind { kind_free, kind_enet, kind_ws };
enum Role { role_new, role_host, role_client };

struct Connection
{
  Kind        kind = kind_free;
  Role        role = role_new;
  ENetPeer   *peer = nullptr;
  ENetSocket  socket = ENET_SOCKET_NULL;
  bool        upgraded = false;   // WebSocket: the handshake is done
  bool        closing = false;    // WebSocket: close once the output has gone
  std::string in, out;            // WebSocket buffers
  int         session = -1;
  int         link = 0;           // client: its link in the session
  enet_uint32 since = 0;
  enet_uint32 window = 0;         // rate limit: the current second and its messages
  int         count = 0;
  char        address[64] = "";
};

struct Session
{
  bool open = false;
  char code[rp::code_len + 1];
  int  host = -1;
  int  clients = 0;
  int  link[rp::max_clients + 1];  // link -> connection, or -1
};

ENetHost  *net = nullptr;
ENetSocket listener = ENET_SOCKET_NULL;
std::vector<Connection> conns(max_connections);
std::vector<Session>    sessions(max_sessions);
std::mt19937 random_codes;
bool   once = false;
bool   had_session = false;

void say(const char *format, ...)
{
  char stamp[32];
  time_t now = time(nullptr);
  strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
  char text[512];
  va_list args;
  va_start(args, format);
  vsnprintf(text, sizeof(text), format, args);
  va_end(args);
  printf("%s %s\n", stamp, text);
  fflush(stdout);
}

// --- SHA-1 and base64, for the WebSocket handshake ------------------------------------------

struct Sha1
{
  uint32_t h[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
  unsigned char block[64];
  size_t   used = 0;
  uint64_t bits = 0;

  static uint32_t rol(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }

  void compress()
  {
    uint32_t w[80];
    for (int i = 0; i < 16; i++)
      w[i] = (uint32_t)block[i * 4] << 24 | (uint32_t)block[i * 4 + 1] << 16 |
             (uint32_t)block[i * 4 + 2] << 8 | block[i * 4 + 3];
    for (int i = 16; i < 80; i++)
      w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
    for (int i = 0; i < 80; i++)
    {
      uint32_t f, k;
      if (i < 20)      { f = (b & c) | (~b & d);          k = 0x5A827999; }
      else if (i < 40) { f = b ^ c ^ d;                   k = 0x6ED9EBA1; }
      else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
      else             { f = b ^ c ^ d;                   k = 0xCA62C1D6; }
      uint32_t t = rol(a, 5) + f + e + k + w[i];
      e = d; d = c; c = rol(b, 30); b = a; a = t;
    }
    h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
  }

  void add(const void *data, size_t size)
  {
    const unsigned char *p = (const unsigned char *)data;
    bits += (uint64_t)size * 8;
    while (size--)
    {
      block[used++] = *p++;
      if (used == 64)
      {
        compress();
        used = 0;
      }
    }
  }

  void finish(unsigned char digest[20])
  {
    uint64_t total = bits;
    unsigned char pad = 0x80;
    add(&pad, 1);
    pad = 0;
    while (used != 56)
      add(&pad, 1);
    for (int i = 7; i >= 0; i--)
    {
      unsigned char byte = (unsigned char)(total >> (i * 8));
      add(&byte, 1);
    }
    for (int i = 0; i < 5; i++)
      for (int j = 0; j < 4; j++)
        digest[i * 4 + j] = (unsigned char)(h[i] >> (24 - j * 8));
  }
};

std::string base64(const unsigned char *data, size_t size)
{
  static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  for (size_t i = 0; i < size; i += 3)
  {
    uint32_t n = (uint32_t)data[i] << 16;
    if (i + 1 < size) n |= (uint32_t)data[i + 1] << 8;
    if (i + 2 < size) n |= data[i + 2];
    out += chars[(n >> 18) & 63];
    out += chars[(n >> 12) & 63];
    out += i + 1 < size ? chars[(n >> 6) & 63] : '=';
    out += i + 2 < size ? chars[n & 63] : '=';
  }
  return out;
}

// --- connections -----------------------------------------------------------------------------

void ws_frame(Connection &c, int opcode, const unsigned char *data, size_t size)
{
  unsigned char header[4] = {(unsigned char)(0x80 | opcode)};
  size_t header_size = 2;
  if (size < 126)
    header[1] = (unsigned char)size;
  else
  {
    header[1] = 126;
    header[2] = (unsigned char)(size >> 8);
    header[3] = (unsigned char)size;
    header_size = 4;
  }
  c.out.append((const char *)header, header_size);
  c.out.append((const char *)data, size);
}

void send_to(int index, const unsigned char *data, size_t size)
{
  Connection &c = conns[index];
  if (c.kind == kind_enet && c.peer)
  {
    ENetPacket *packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
    if (packet && enet_peer_send(c.peer, 0, packet) != 0)
      enet_packet_destroy(packet);
  }
  else if (c.kind == kind_ws && c.upgraded && !c.closing)
    ws_frame(c, 0x2, data, size);
}

void free_connection(int index)
{
  Connection &c = conns[index];
  if (c.kind == kind_ws && c.socket != ENET_SOCKET_NULL)
    enet_socket_destroy(c.socket);
  c = Connection();
}

void leave_session(int index);

// Ends a connection: a REFUSED first if reason isn't 0, then the session learns it's gone.
void drop(int index, int reason)
{
  Connection &c = conns[index];
  if (c.kind == kind_free)
    return;
  if (reason)
  {
    static const char *const reasons[] = {"", "another protocol version", "no such session",
                                          "the session is full", "the code is taken", "busy",
                                          "a bad message"};
    say("%s: refused, %s", c.address, reason < (int)(sizeof(reasons) / sizeof(reasons[0])) ? reasons[reason] : "?");
    unsigned char refused[2] = {rp::msg_refused, (unsigned char)reason};
    send_to(index, refused, sizeof(refused));
  }
  leave_session(index);
  if (c.kind == kind_enet)
  {
    c.peer->data = nullptr;
    enet_peer_disconnect_later(c.peer, 0);
    c = Connection();
  }
  else if (c.upgraded && !c.out.empty())
  {
    unsigned char status[2] = {0x03, 0xE8};  // 1000, normal closure
    ws_frame(c, 0x8, status, sizeof(status));
    c.closing = true;
    c.role = role_new;
  }
  else
    free_connection(index);
}

void leave_session(int index)
{
  Connection &c = conns[index];
  if (c.session < 0)
    return;
  Session &s = sessions[c.session];
  int session = c.session;
  c.session = -1;
  if (c.role == role_host)
  {
    say("session %s closed: its host left", s.code);
    s.open = false;
    for (int l = 1; l <= rp::max_clients; l++)
      if (s.link[l] >= 0)
      {
        int client = s.link[l];
        s.link[l] = -1;
        conns[client].session = -1;
        drop(client, 0);
      }
    return;
  }
  if (c.role == role_client && c.link >= 1 && c.link <= rp::max_clients && s.link[c.link] == index)
  {
    s.link[c.link] = -1;
    unsigned char down[2] = {rp::msg_link_down, (unsigned char)c.link};
    send_to(s.host, down, sizeof(down));
    say("session %s: client %d (%s) left", sessions[session].code, c.link, c.address);
  }
}

int new_connection(Kind kind, const char *address)
{
  for (int i = 0; i < max_connections; i++)
    if (conns[i].kind == kind_free)
    {
      conns[i] = Connection();
      conns[i].kind = kind;
      conns[i].since = enet_time_get();
      snprintf(conns[i].address, sizeof(conns[i].address), "%s", address);
      return i;
    }
  return -1;
}

// --- the relay protocol ----------------------------------------------------------------------

bool valid_code(const char *code)
{
  for (int i = 0; i < rp::code_len; i++)
    if (!strchr(rp::code_chars, code[i]) || !code[i])
      return false;
  return true;
}

int find_session(const char *code)
{
  for (int i = 0; i < max_sessions; i++)
    if (sessions[i].open && memcmp(sessions[i].code, code, rp::code_len) == 0)
      return i;
  return -1;
}

void open_session(int index, const unsigned char *data, size_t size)
{
  if (size >= 2 && data[1] != rp::version)
  {
    drop(index, rp::refused_version);
    return;
  }
  if (size != 3 + (size_t)rp::code_len)
  {
    drop(index, rp::refused_bad_message);
    return;
  }
  int clients = data[2];
  if (clients < 1 || clients > rp::max_clients)
  {
    drop(index, rp::refused_bad_message);
    return;
  }
  char code[rp::code_len + 1] = "";
  bool wanted = false;
  for (int i = 0; i < rp::code_len; i++)
  {
    code[i] = (char)toupper(data[3 + i]);
    wanted = wanted || data[3 + i];
  }
  if (wanted)
  {
    if (!valid_code(code))
    {
      drop(index, rp::refused_bad_message);
      return;
    }
    if (find_session(code) >= 0)
    {
      drop(index, rp::refused_code_taken);
      return;
    }
  }
  else
  {
    do
      for (int i = 0; i < rp::code_len; i++)
        code[i] = rp::code_chars[random_codes() % (sizeof(rp::code_chars) - 1)];
    while (find_session(code) >= 0);
  }
  int s = -1;
  for (int i = 0; i < max_sessions && s < 0; i++)
    if (!sessions[i].open)
      s = i;
  if (s < 0)
  {
    drop(index, rp::refused_busy);
    return;
  }
  Session &session = sessions[s];
  session.open = true;
  memcpy(session.code, code, rp::code_len + 1);
  session.host = index;
  session.clients = clients;
  for (int &l : session.link)
    l = -1;
  Connection &c = conns[index];
  c.role = role_host;
  c.session = s;
  had_session = true;
  unsigned char opened[1 + rp::code_len] = {rp::msg_opened};
  memcpy(opened + 1, code, rp::code_len);
  send_to(index, opened, sizeof(opened));
  say("session %s opened by %s for %d clients", code, c.address, clients);
}

void enter_session(int index, const unsigned char *data, size_t size)
{
  if (size >= 2 && data[1] != rp::version)
  {
    drop(index, rp::refused_version);
    return;
  }
  if (size != 2 + (size_t)rp::code_len)
  {
    drop(index, rp::refused_bad_message);
    return;
  }
  char code[rp::code_len + 1] = "";
  for (int i = 0; i < rp::code_len; i++)
    code[i] = (char)toupper(data[2 + i]);
  int s = valid_code(code) ? find_session(code) : -1;
  if (s < 0)
  {
    drop(index, rp::refused_no_session);
    return;
  }
  Session &session = sessions[s];
  int link = 0;
  for (int l = 1; l <= session.clients && !link; l++)
    if (session.link[l] < 0)
      link = l;
  if (!link)
  {
    drop(index, rp::refused_full);
    return;
  }
  Connection &c = conns[index];
  c.role = role_client;
  c.session = s;
  c.link = link;
  session.link[link] = index;
  unsigned char entered[1] = {rp::msg_entered};
  send_to(index, entered, sizeof(entered));
  unsigned char up[2] = {rp::msg_link_up, (unsigned char)link};
  send_to(session.host, up, sizeof(up));
  say("session %s: client %d (%s) entered", session.code, link, c.address);
}

void message(int index, const unsigned char *data, size_t size)
{
  Connection &c = conns[index];
  enet_uint32 now = enet_time_get();
  if (now - c.window >= 1000)
  {
    c.window = now;
    c.count = 0;
  }
  if ((rate_limit && ++c.count > rate_limit) || size == 0 || size > 3 + (size_t)rp::max_payload)
  {
    say("%s: %s, dropped", c.address, size ? "too many messages" : "an empty message");
    drop(index, rp::refused_bad_message);
    return;
  }
  int type = data[0];
  if (c.role == role_new)
  {
    if (type == rp::msg_open)
      open_session(index, data, size);
    else if (type == rp::msg_enter)
      enter_session(index, data, size);
    else
      drop(index, rp::refused_bad_message);
    return;
  }
  Session &s = sessions[c.session];
  if (c.role == role_host && type == rp::msg_to && size >= 2)
  {
    int link = data[1];
    if (link >= 1 && link <= rp::max_clients && s.link[link] >= 0)
    {
      std::vector<unsigned char> pass(size - 1);
      pass[0] = rp::msg_pass;
      memcpy(pass.data() + 1, data + 2, size - 2);
      send_to(s.link[link], pass.data(), pass.size());
    }
    return;
  }
  if (c.role == role_host && type == rp::msg_close && size == 2)
  {
    int link = data[1];
    if (link >= 1 && link <= rp::max_clients && s.link[link] >= 0)
      drop(s.link[link], 0);
    return;
  }
  if (c.role == role_client && type == rp::msg_pass)
  {
    std::vector<unsigned char> from(size + 1);
    from[0] = rp::msg_from;
    from[1] = (unsigned char)c.link;
    memcpy(from.data() + 2, data + 1, size - 1);
    send_to(s.host, from.data(), from.size());
    return;
  }
  drop(index, rp::refused_bad_message);
}

// --- WebSocket -------------------------------------------------------------------------------

bool same_text(const char *a, const char *b, size_t n)  // ignoring case
{
  for (size_t i = 0; i < n; i++)
    if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
      return false;
  return true;
}

std::string header_value(const std::string &request, const char *name)
{
  size_t at = 0;
  size_t name_len = strlen(name);
  while ((at = request.find("\r\n", at)) != std::string::npos)
  {
    at += 2;
    if (request.size() - at > name_len && request[at + name_len] == ':' &&
        same_text(request.c_str() + at, name, name_len))
    {
      size_t start = at + name_len + 1;
      size_t end = request.find("\r\n", start);
      std::string value = request.substr(start, end - start);
      while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
        value.erase(0, 1);
      while (!value.empty() && (value.back() == ' ' || value.back() == '\t'))
        value.pop_back();
      return value;
    }
  }
  return "";
}

// Returns false when the connection is to be closed.
bool ws_handshake(int index)
{
  Connection &c = conns[index];
  size_t end = c.in.find("\r\n\r\n");
  if (end == std::string::npos)
    return c.in.size() <= ws_max_request;
  std::string request = c.in.substr(0, end + 2);
  c.in.erase(0, end + 4);
  std::string key = header_value(request, "sec-websocket-key");
  if (key.empty())
  {
    // Not a WebSocket: a health check.
    c.out += "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 9\r\n"
             "Connection: close\r\n\r\nff_relay\n";
    c.closing = true;
    return true;
  }
  Sha1 sha;
  std::string accept = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  sha.add(accept.data(), accept.size());
  unsigned char digest[20];
  sha.finish(digest);
  c.out += "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
           "Sec-WebSocket-Accept: " + base64(digest, sizeof(digest)) + "\r\n\r\n";
  c.upgraded = true;
  std::string forwarded = header_value(request, "x-forwarded-for");
  if (!forwarded.empty())
    snprintf(c.address, sizeof(c.address), "%s", forwarded.c_str());
  return true;
}

// Takes the complete frames out of the input. Returns false when the connection is to be closed.
bool ws_frames(int index)
{
  for (;;)
  {
    Connection &c = conns[index];
    if (c.kind != kind_ws || c.closing)
      return true;
    const unsigned char *p = (const unsigned char *)c.in.data();
    size_t have = c.in.size();
    if (have < 2)
      return true;
    bool fin = (p[0] & 0x80) != 0;
    int opcode = p[0] & 0x0F;
    bool masked = (p[1] & 0x80) != 0;
    uint64_t size = p[1] & 0x7F;
    size_t at = 2;
    if (size == 126)
    {
      if (have < 4)
        return true;
      size = (uint64_t)p[2] << 8 | p[3];
      at = 4;
    }
    else if (size == 127)
      return false;  // nothing the game sends is that big
    if (!masked || !fin || opcode == 0 || size > 3 + (uint64_t)rp::max_payload)
      return false;  // clients mask; the game's messages are small and never fragmented
    if (have < at + 4 + size)
      return true;
    unsigned char mask[4];
    memcpy(mask, p + at, 4);
    at += 4;
    std::vector<unsigned char> payload(p + at, p + at + size);
    for (size_t i = 0; i < payload.size(); i++)
      payload[i] ^= mask[i % 4];
    c.in.erase(0, at + size);
    if (opcode == 0x2)
      message(index, payload.data(), payload.size());
    else if (opcode == 0x9)
      ws_frame(conns[index], 0xA, payload.data(), payload.size());
    else if (opcode == 0x8)
      return false;
    else if (opcode != 0xA)
      return false;  // text frames aren't the protocol
  }
}

void ws_closed(int index)
{
  if (conns[index].kind != kind_ws)
    return;
  leave_session(index);
  free_connection(index);
}

void ws_read(int index)
{
  Connection &c = conns[index];
  char buffer[4096];
  ENetBuffer b;
  b.data = buffer;
  b.dataLength = sizeof(buffer);
  int got = enet_socket_receive(c.socket, nullptr, &b, 1);
  if (got <= 0)
  {
    ws_closed(index);  // readable with nothing to read: closed
    return;
  }
  if (c.closing)
    return;
  c.in.append(buffer, (size_t)got);
  bool keep = c.upgraded ? ws_frames(index) : ws_handshake(index) && (!conns[index].upgraded || ws_frames(index));
  if (!keep)
    ws_closed(index);
}

void ws_write(int index)
{
  Connection &c = conns[index];
  ENetBuffer b;
  b.data = (void *)c.out.data();
  b.dataLength = c.out.size();
  int sent = enet_socket_send(c.socket, nullptr, &b, 1);
  if (sent < 0)
  {
    ws_closed(index);
    return;
  }
  c.out.erase(0, (size_t)sent);
  if (c.out.empty() && c.closing)
    free_connection(index);
}

// --- the loop --------------------------------------------------------------------------------

void enet_events()
{
  ENetEvent event;
  while (enet_host_service(net, &event, 0) > 0)
  {
    switch (event.type)
    {
      case ENET_EVENT_TYPE_CONNECT:
      {
        char address[64];
        enet_address_get_host_ip(&event.peer->address, address, sizeof(address));
        int index = new_connection(kind_enet, address);
        if (index < 0)
        {
          enet_peer_disconnect_now(event.peer, 0);
          break;
        }
        conns[index].peer = event.peer;
        event.peer->data = (void *)(intptr_t)(index + 1);
        enet_peer_timeout(event.peer, 0, 5000, 10000);
        break;
      }
      case ENET_EVENT_TYPE_RECEIVE:
        if (event.peer->data)
          message((int)(intptr_t)event.peer->data - 1, event.packet->data, event.packet->dataLength);
        enet_packet_destroy(event.packet);
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
        if (event.peer->data)
        {
          int index = (int)(intptr_t)event.peer->data - 1;
          event.peer->data = nullptr;
          conns[index].peer = nullptr;
          leave_session(index);
          conns[index] = Connection();
        }
        break;
      default:
        break;
    }
  }
}

int connections_in_use()
{
  int n = 0;
  for (const Connection &c : conns)
    if (c.kind != kind_free)
      n++;
  return n;
}
} // namespace

int main(int argc, char **argv)
{
  unsigned short port = rp::enet_port;
  unsigned short ws_port = rp::ws_port;
  unsigned quit_after = 0;
  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "--port") && i + 1 < argc)
      port = (unsigned short)atoi(argv[++i]);
    else if (!strcmp(argv[i], "--ws-port") && i + 1 < argc)
      ws_port = (unsigned short)atoi(argv[++i]);
    else if (!strcmp(argv[i], "--rate") && i + 1 < argc)
      rate_limit = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--quit-after") && i + 1 < argc)
      quit_after = (unsigned)atoi(argv[++i]) * 1000;
    else if (!strcmp(argv[i], "--once"))
      once = true;
    else
    {
      fprintf(stderr, "usage: ff_relay [--port <udp>] [--ws-port <tcp>] [--rate <n>] [--quit-after <s>] [--once]\n");
      return 2;
    }
  }
  random_codes.seed((unsigned)std::random_device()() ^ (unsigned)time(nullptr));
  if (enet_initialize() != 0)
  {
    fprintf(stderr, "ff_relay: ENet does not start\n");
    return 1;
  }
  ENetAddress address;
  address.host = ENET_HOST_ANY;
  address.port = port;
  net = enet_host_create(&address, max_connections, 1, 0, 0);
  if (!net)
  {
    fprintf(stderr, "ff_relay: UDP port %u is not free\n", (unsigned)port);
    return 1;
  }
  listener = enet_socket_create(ENET_SOCKET_TYPE_STREAM);
  ENetAddress ws_address;
  ws_address.host = ENET_HOST_ANY;
  ws_address.port = ws_port;
  if (listener != ENET_SOCKET_NULL)
    enet_socket_set_option(listener, ENET_SOCKOPT_REUSEADDR, 1);
  if (listener == ENET_SOCKET_NULL || enet_socket_bind(listener, &ws_address) != 0 ||
      enet_socket_listen(listener, 64) != 0)
  {
    fprintf(stderr, "ff_relay: TCP port %u is not free\n", (unsigned)ws_port);
    return 1;
  }
  enet_socket_set_option(listener, ENET_SOCKOPT_NONBLOCK, 1);
  say("relay protocol %d: ENet on UDP port %u, WebSocket on TCP port %u", rp::version,
      (unsigned)port, (unsigned)ws_port);

  enet_uint32 started = enet_time_get();
  for (;;)
  {
    if (quit_after && enet_time_get() - started >= quit_after)
    {
      say("quit time reached");
      break;
    }
    if (once && had_session && connections_in_use() == 0)
    {
      bool any = false;
      for (const Session &s : sessions)
        any = any || s.open;
      if (!any)
      {
        say("the session has ended");
        break;
      }
    }

    ENetSocketSet readable, writable;
    ENET_SOCKETSET_EMPTY(readable);
    ENET_SOCKETSET_EMPTY(writable);
    ENetSocket highest = net->socket > listener ? net->socket : listener;
    ENET_SOCKETSET_ADD(readable, net->socket);
    ENET_SOCKETSET_ADD(readable, listener);
    for (const Connection &c : conns)
      if (c.kind == kind_ws)
      {
        ENET_SOCKETSET_ADD(readable, c.socket);
        if (!c.out.empty())
          ENET_SOCKETSET_ADD(writable, c.socket);
        if (c.socket > highest)
          highest = c.socket;
      }
    enet_socketset_select(highest, &readable, &writable, 5);

    enet_events();

    if (ENET_SOCKETSET_CHECK(readable, listener))
    {
      ENetAddress from;
      ENetSocket accepted = enet_socket_accept(listener, &from);
      if (accepted != ENET_SOCKET_NULL)
      {
        char text[64];
        enet_address_get_host_ip(&from, text, sizeof(text));
        int index = new_connection(kind_ws, text);
        if (index < 0)
          enet_socket_destroy(accepted);
        else
        {
          enet_socket_set_option(accepted, ENET_SOCKOPT_NONBLOCK, 1);
          enet_socket_set_option(accepted, ENET_SOCKOPT_NODELAY, 1);
          conns[index].socket = accepted;
        }
      }
    }

    enet_uint32 now = enet_time_get();
    for (int i = 0; i < max_connections; i++)
    {
      Connection &c = conns[i];
      if (c.kind == kind_ws && ENET_SOCKETSET_CHECK(readable, c.socket))
        ws_read(i);
      if (c.kind == kind_ws && !c.out.empty() && ENET_SOCKETSET_CHECK(writable, c.socket))
        ws_write(i);
      if (c.kind == kind_ws && c.out.size() > ws_max_output)
      {
        say("%s: too slow, dropped", c.address);
        ws_closed(i);
      }
      if (c.kind != kind_free && c.role == role_new && !c.closing && now - c.since > hello_timeout)
      {
        say("%s: no session asked for in time, dropped", c.address);
        drop(i, rp::refused_bad_message);
      }
    }
    enet_host_flush(net);
  }

  for (int i = 0; i < max_connections; i++)
    if (conns[i].kind == kind_ws)
      free_connection(i);
  enet_socket_destroy(listener);
  enet_host_destroy(net);
  enet_deinitialize();
  return 0;
}
