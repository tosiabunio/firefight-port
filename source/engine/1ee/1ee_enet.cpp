// Port: the ENet transport behind Net (see 1ee_enet.h).
//
// Messages, all on channel 0, reliable and in order:
//   HELLO   client -> host   type, version, name
//   WELCOME host -> client   type, the client's id
//   REFUSE  host -> client   type, reason (refuse_*)
//   START   host -> clients  type, players, has_info, info[28], count, count x (id, name)
//   DATA    any              type, from, to, the engine's message (the host forwards it)
//   LEFT    host -> clients  type, the id of a player who left
// Everything received is checked before use: length, type, and that a client's DATA carries its
// own id as the sender.

#include "1ee_enet.h"

#include <enet/enet.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

namespace enet_transport
{
namespace
{
enum
{
  msg_hello = 1, msg_welcome, msg_refuse, msg_start, msg_data, msg_left
};
enum
{
  refuse_version = 1, refuse_full, refuse_started
};
const int start_header = 1 + 1 + 1 + info_len + 1;
const int start_entry  = 1 + name_len;

// LAN discovery, outside ENet's protocol (the host's intercept answers it):
//   query   "FFIGHT?", version
//   answer  "FFIGHT!", version, token (4 bytes), players, joined, started, host name
const unsigned char query_magic[7]  = {'F', 'F', 'I', 'G', 'H', 'T', '?'};
const unsigned char answer_magic[7] = {'F', 'F', 'I', 'G', 'H', 'T', '!'};
const int query_size  = 7 + 1;
const int answer_size = 7 + 1 + 4 + 1 + 1 + 1 + name_len;
const int max_found   = 16;

struct Slot
{
  bool     present;
  char     name[name_len];
  ENetPeer *peer;  // the host's link to that client
};

struct Message
{
  unsigned      from;
  unsigned      size;
  unsigned char data[max_message];
};

void (*log_proc)(const char *text) = nullptr;
bool          initialised = false;
ENetHost     *net = nullptr;
ENetPeer     *host_peer = nullptr;  // a client's link to the host
bool          is_host = false;
bool          is_started = false;
bool          connected = false;     // client: the host accepted the connection
const char   *failed = nullptr;
unsigned      own_id = 0;
int           max = 0;
bool          has_info = false;
unsigned char info_bytes[info_len];
char          own_name[name_len];
uint32_t      token = 0;  // tells this game apart in discovery answers
ENetAddress   host_address;
Slot          slots[max_players + 1];  // by player id
// The messages received for this player, oldest first. Plain data only: the engine's quit
// procedures (Net::quit) run after the static destructors, so nothing here may have one.
const int     inbox_size = 1024;
Message       inbox[inbox_size];
int           inbox_head = 0;
int           inbox_count = 0;

void report(const char *format, ...)
{
  if (!log_proc)
    return;
  char text[256];
  va_list args;
  va_start(args, format);
  vsnprintf(text, sizeof(text), format, args);
  va_end(args);
  log_proc(text);
}

void copy_name(char *to, const char *from)
{
  strncpy(to, from ? from : "", name_len - 1);
  to[name_len - 1] = 0;
}

void send_packet(ENetPeer *peer, const unsigned char *data, size_t size)
{
  ENetPacket *packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
  if (packet && enet_peer_send(peer, 0, packet) != 0)
    enet_packet_destroy(packet);
}

unsigned peer_id(ENetPeer *peer)
{
  return (unsigned)(uintptr_t)peer->data;
}

int present_count()
{
  int n = 0;
  for (int id = 1; id <= max_players; id++)
    if (slots[id].present)
      n++;
  return n;
}

void deliver(unsigned from, const unsigned char *data, size_t size)
{
  if (inbox_count == inbox_size)
  {
    failed = "too many messages waiting";
    return;
  }
  Message &m = inbox[(inbox_head + inbox_count) % inbox_size];
  m.from = from;
  m.size = (unsigned)size;
  memcpy(m.data, data, size);
  inbox_count++;
}

void host_receive(ENetPeer *peer, const unsigned char *data, size_t size)
{
  unsigned id = peer_id(peer);
  if (data[0] == msg_hello && size == 2 + name_len && id == 0)
  {
    unsigned char refuse[2] = {msg_refuse, 0};
    if (data[1] != version)
      refuse[1] = refuse_version;
    else if (is_started)
      refuse[1] = refuse_started;
    else if (present_count() >= max)
      refuse[1] = refuse_full;
    if (refuse[1])
    {
      send_packet(peer, refuse, sizeof(refuse));
      enet_peer_disconnect_later(peer, 0);
      return;
    }
    for (id = 2; id <= (unsigned)max && slots[id].present; id++)
      ;
    slots[id].present = true;
    slots[id].peer = peer;
    char name[name_len];
    memcpy(name, data + 2, name_len);
    name[name_len - 1] = 0;
    copy_name(slots[id].name, name);
    peer->data = (void *)(uintptr_t)id;
    unsigned char welcome[2] = {msg_welcome, (unsigned char)id};
    send_packet(peer, welcome, sizeof(welcome));
    report("network: player %u (%s) joined, %d of %d", id, slots[id].name, present_count(), max);
    return;
  }
  if (data[0] == msg_data && size >= 3 && size - 3 <= max_message && id != 0 && data[1] == id)
  {
    unsigned to = data[2];
    if (to == own_id)
      deliver(id, data + 3, size - 3);
    else if (to >= 1 && to <= max_players && slots[to].present && slots[to].peer)
      send_packet(slots[to].peer, data, size);
    return;
  }
  report("network: message of type %d and %u bytes from player %u ignored", data[0], (unsigned)size, id);
}

void client_receive(const unsigned char *data, size_t size)
{
  if (data[0] == msg_welcome && size == 2 && data[1] >= 2 && data[1] <= max_players)
  {
    own_id = data[1];
    report("network: joined as player %u", own_id);
    return;
  }
  if (data[0] == msg_refuse && size == 2)
  {
    failed = data[1] == refuse_version ? "the host runs a different version"
           : data[1] == refuse_full    ? "the game is full"
                                       : "the game has already started";
    return;
  }
  if (data[0] == msg_start && size >= (size_t)start_header && own_id != 0)
  {
    int count = data[start_header - 1];
    if (data[1] < 2 || data[1] > max_players || count < 1 || count > max_players ||
        size != (size_t)(start_header + count * start_entry))
      return;
    max = data[1];
    has_info = data[2] != 0;
    memcpy(info_bytes, data + 3, info_len);
    memset(slots, 0, sizeof(slots));
    for (int i = 0; i < count; i++)
    {
      const unsigned char *entry = data + start_header + i * start_entry;
      unsigned id = entry[0];
      if (id < 1 || id > (unsigned)max)
        continue;
      slots[id].present = true;
      char name[name_len];
      memcpy(name, entry + 1, name_len);
      name[name_len - 1] = 0;
      copy_name(slots[id].name, name);
    }
    is_started = slots[own_id].present;
    return;
  }
  if (data[0] == msg_data && size >= 3 && size - 3 <= max_message && data[2] == own_id &&
      data[1] >= 1 && data[1] <= max_players)
  {
    deliver(data[1], data + 3, size - 3);
    return;
  }
  if (data[0] == msg_left && size == 2 && data[1] >= 1 && data[1] <= max_players)
  {
    slots[data[1]].present = false;
    report("network: player %u left", (unsigned)data[1]);
    return;
  }
  report("network: message of type %d and %u bytes from the host ignored", data[0], (unsigned)size);
}

void host_disconnect(ENetPeer *peer)
{
  unsigned id = peer_id(peer);
  peer->data = nullptr;
  if (id == 0 || !slots[id].present)
    return;
  slots[id].present = false;
  slots[id].peer = nullptr;
  report("network: player %u (%s) left", id, slots[id].name);
  if (!is_started)
    return;
  unsigned char left[2] = {msg_left, (unsigned char)id};
  for (int other = 2; other <= max_players; other++)
    if (slots[other].present && slots[other].peer)
      send_packet(slots[other].peer, left, sizeof(left));
}

// The host's answer to a discovery query, sent from its game socket to whoever asked.
int ENET_CALLBACK answer_query(ENetHost *host, ENetEvent *)
{
  if (host->receivedDataLength != (size_t)query_size || memcmp(host->receivedData, query_magic, 7) != 0)
    return 0;
  unsigned char answer[answer_size];
  memcpy(answer, answer_magic, 7);
  answer[7] = version;
  answer[8] = (unsigned char)token;
  answer[9] = (unsigned char)(token >> 8);
  answer[10] = (unsigned char)(token >> 16);
  answer[11] = (unsigned char)(token >> 24);
  answer[12] = (unsigned char)max;
  answer[13] = (unsigned char)present_count();
  answer[14] = is_started ? 1 : 0;
  memcpy(answer + 15, slots[1].name, name_len);
  ENetBuffer buffer;
  buffer.data = answer;
  buffer.dataLength = sizeof(answer);
  enet_socket_send(host->socket, &host->receivedAddress, &buffer, 1);
  return 1;
}

void client_disconnect()
{
  if (!connected)
  {
    // The host isn't there yet: try again (Net gives up after its timeout).
    host_peer = enet_host_connect(net, &host_address, 1, 0);
    if (host_peer)
      enet_peer_timeout(host_peer, 0, 5000, 10000);
    return;
  }
  host_peer = nullptr;
  if (!is_started)
  {
    if (!failed)
      failed = "the connection to the host was lost";
    return;
  }
  // The host left: the other players are gone too.
  for (int id = 1; id <= max_players; id++)
    if (id != (int)own_id)
      slots[id].present = false;
  report("network: the host left");
}
} // namespace

bool startup(void (*log_function)(const char *text))
{
  log_proc = log_function;
  if (!initialised && enet_initialize() != 0)
    return false;
  initialised = true;
  return true;
}

void shutdown(void)
{
  leave();
  if (initialised)
    enet_deinitialize();
  initialised = false;
}

bool host(unsigned short port, int players, const char *name, const unsigned char *info)
{
  if (players < 2 || players > max_players)
  {
    failed = "the number of players must be 2 to 4";
    return false;
  }
  ENetAddress address;
  address.host = ENET_HOST_ANY;
  address.port = port;
  net = enet_host_create(&address, max_players - 1, 1, 0, 0);
  if (!net)
  {
    failed = "the network port is not free";
    return false;
  }
  is_host = true;
  max = players;
  own_id = 1;
  has_info = info != nullptr;
  memset(info_bytes, 0, info_len);
  if (info)
    memcpy(info_bytes, info, info_len);
  memset(slots, 0, sizeof(slots));
  slots[1].present = true;
  copy_name(slots[1].name, name);
  token = enet_time_get() ^ (uint32_t)time(nullptr) ^ (uint32_t)(uintptr_t)net;
  net->intercept = answer_query;
  report("network: hosting a game for %d players on port %u", players, (unsigned)port);
  return true;
}

bool join(const char *address, unsigned short port, const char *name)
{
  net = enet_host_create(nullptr, 1, 1, 0, 0);
  if (!net)
  {
    failed = "no network";
    return false;
  }
  if (enet_address_set_host(&host_address, address) != 0)
  {
    failed = "the host's address is unknown";
    return false;
  }
  host_address.port = port;
  copy_name(own_name, name);
  host_peer = enet_host_connect(net, &host_address, 1, 0);
  if (host_peer)
    enet_peer_timeout(host_peer, 0, 5000, 10000);
  report("network: connecting to %s, port %u", address, (unsigned)port);
  return host_peer != nullptr;
}

void poll(void)
{
  if (!net)
    return;
  ENetEvent event;
  while (enet_host_service(net, &event, 0) > 0)
  {
    switch (event.type)
    {
      case ENET_EVENT_TYPE_CONNECT:
        enet_peer_timeout(event.peer, 0, 5000, 10000);
        if (!is_host)
        {
          connected = true;
          unsigned char hello[2 + name_len];
          hello[0] = msg_hello;
          hello[1] = version;
          memcpy(hello + 2, own_name, name_len);
          send_packet(event.peer, hello, sizeof(hello));
        }
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        if (event.packet->dataLength > 0)
        {
          if (is_host)
            host_receive(event.peer, event.packet->data, event.packet->dataLength);
          else
            client_receive(event.packet->data, event.packet->dataLength);
        }
        enet_packet_destroy(event.packet);
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
        if (is_host)
          host_disconnect(event.peer);
        else
          client_disconnect();
        break;
      default:
        break;
    }
  }
  enet_host_flush(net);
}

int joined(void)
{
  return present_count();
}

void start(void)
{
  unsigned char packet[start_header + max_players * start_entry];
  int count = 0;
  packet[0] = msg_start;
  packet[1] = (unsigned char)max;
  packet[2] = has_info ? 1 : 0;
  memcpy(packet + 3, info_bytes, info_len);
  for (int id = 1; id <= max_players; id++)
    if (slots[id].present)
    {
      unsigned char *entry = packet + start_header + count * start_entry;
      entry[0] = (unsigned char)id;
      memcpy(entry + 1, slots[id].name, name_len);
      count++;
    }
  packet[start_header - 1] = (unsigned char)count;
  for (int id = 2; id <= max_players; id++)
    if (slots[id].present && slots[id].peer)
      send_packet(slots[id].peer, packet, start_header + count * start_entry);
  enet_host_flush(net);
  is_started = true;
}

bool started(void)
{
  return is_started;
}

const char *failure(void)
{
  return failed;
}

unsigned my_id(void)
{
  return own_id;
}

int players_max(void)
{
  return max;
}

bool game_info(unsigned char *info)
{
  memcpy(info, info_bytes, info_len);
  return has_info;
}

int roster(Player *players)
{
  int count = 0;
  for (int id = 1; id <= max_players; id++)
    if (slots[id].present)
    {
      players[count].id = id;
      memcpy(players[count].name, slots[id].name, name_len);
      count++;
    }
  return count;
}

bool send(unsigned to, const void *data, unsigned size)
{
  if (!net || size > max_message || to < 1 || to > max_players || to == own_id || !slots[to].present)
    return false;
  unsigned char packet[3 + max_message];
  packet[0] = msg_data;
  packet[1] = (unsigned char)own_id;
  packet[2] = (unsigned char)to;
  memcpy(packet + 3, data, size);
  ENetPeer *peer = is_host ? slots[to].peer : host_peer;
  if (!peer)
    return false;
  send_packet(peer, packet, 3 + size);
  enet_host_flush(net);
  return true;
}

bool receive(void *data, unsigned *size, unsigned *from)
{
  poll();
  if (inbox_count == 0)
    return false;
  const Message &m = inbox[inbox_head];
  bool fits = m.size <= *size;
  if (fits)
  {
    memcpy(data, m.data, m.size);
    *size = m.size;
    *from = m.from;
  }
  inbox_head = (inbox_head + 1) % inbox_size;
  inbox_count--;
  return fits;
}

void leave(void)
{
  if (!net)
    return;
  // Say goodbye, and give the messages a moment to go out.
  for (size_t i = 0; i < net->peerCount; i++)
    if (net->peers[i].state == ENET_PEER_STATE_CONNECTED)
      enet_peer_disconnect(&net->peers[i], 0);
  ENetEvent event;
  for (int waited = 0; waited < 200 && enet_host_service(net, &event, 10) >= 0; waited += 10)
    if (event.type == ENET_EVENT_TYPE_RECEIVE)
      enet_packet_destroy(event.packet);
  enet_host_destroy(net);
  net = nullptr;
  host_peer = nullptr;
  is_host = is_started = connected = false;
  own_id = 0;
  inbox_head = inbox_count = 0;
}

int discover(unsigned short port, unsigned timeout, Found *games, int max_games)
{
  ENetSocket sock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
  if (sock == ENET_SOCKET_NULL)
    return 0;
  enet_socket_set_option(sock, ENET_SOCKOPT_NONBLOCK, 1);
  enet_socket_set_option(sock, ENET_SOCKOPT_BROADCAST, 1);
  ENetAddress any;
  any.host = ENET_HOST_ANY;
  any.port = ENET_PORT_ANY;
  enet_socket_bind(sock, &any);

  unsigned char query[query_size];
  memcpy(query, query_magic, 7);
  query[7] = version;
  ENetAddress targets[2];
  targets[0].host = ENET_HOST_BROADCAST;
  targets[0].port = port;
  enet_address_set_host_ip(&targets[1], "127.0.0.1");
  targets[1].port = port;

  uint32_t tokens[max_found];
  int count = 0;
  enet_uint32 start = enet_time_get();
  enet_uint32 asked = 0;
  bool first = true;
  while (enet_time_get() - start < timeout)
  {
    if (first || enet_time_get() - asked >= 300)  // ask again: UDP may lose it
    {
      for (ENetAddress &target : targets)
      {
        ENetBuffer buffer;
        buffer.data = query;
        buffer.dataLength = sizeof(query);
        enet_socket_send(sock, &target, &buffer, 1);
      }
      asked = enet_time_get();
      first = false;
    }
    enet_uint32 condition = ENET_SOCKET_WAIT_RECEIVE;
    if (enet_socket_wait(sock, &condition, 50) != 0 || !(condition & ENET_SOCKET_WAIT_RECEIVE))
      continue;
    for (;;)
    {
      unsigned char answer[answer_size + 1];
      ENetBuffer buffer;
      buffer.data = answer;
      buffer.dataLength = sizeof(answer);
      ENetAddress from;
      int size = enet_socket_receive(sock, &from, &buffer, 1);
      if (size <= 0)
        break;
      if (size != answer_size || memcmp(answer, answer_magic, 7) != 0)
        continue;
      uint32_t id = answer[8] | (answer[9] << 8) | (answer[10] << 16) | ((uint32_t)answer[11] << 24);
      bool known = false;
      for (int i = 0; i < count; i++)
        known = known || tokens[i] == id;
      if (known || count >= max_found || count >= max_games)
        continue;
      Found &game = games[count];
      tokens[count] = id;
      enet_address_get_host_ip(&from, game.address, sizeof(game.address));
      game.port = from.port;
      char name[name_len];
      memcpy(name, answer + 15, name_len);
      name[name_len - 1] = 0;
      copy_name(game.host, name);
      game.players = answer[12];
      game.joined = answer[13];
      game.started = answer[14] != 0;
      game.compatible = answer[7] == version;
      count++;
    }
  }
  enet_socket_destroy(sock);
  return count;
}
} // namespace enet_transport
