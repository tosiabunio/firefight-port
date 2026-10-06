// Port: the ENet link layer of the network transport (1ee_link.h), for the desktop. Links run
// either directly between the host and its clients (a LAN, or the internet with the host's port
// open), or through a relay server (1ee_relay.h), to which everyone connects out. Also LAN
// discovery. Everything goes on channel 0, reliable and in order.
//
// Links: directly, the host's link n is its ENet peer n-1, and a client's link to the host is 1.
// Through a relay, the host's links are the relay's link numbers, and a client's is again 1.

#include "1ee_link.h"
#include "1ee_relay.h"

#include <enet/enet.h>

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace enet_transport
{
namespace link
{
namespace
{
namespace rp = relay_protocol;

// LAN discovery, outside ENet's protocol (the host's intercept answers it):
//   query   "FFIGHT?", version
//   answer  "FFIGHT!", version, token (4 bytes), players, joined, started, host name
const unsigned char query_magic[7]  = {'F', 'F', 'I', 'G', 'H', 'T', '?'};
const unsigned char answer_magic[7] = {'F', 'F', 'I', 'G', 'H', 'T', '!'};
const int query_size  = 7 + 1;
const int answer_size = 7 + 1 + 4 + 1 + 1 + 1 + name_len;
const int max_found   = 16;

enum Mode { mode_none, mode_listen, mode_connect, mode_relay_host, mode_relay_client };

ENetHost   *net = nullptr;
Mode        mode = mode_none;
ENetPeer   *server = nullptr;      // a direct client's host, or the relay
ENetAddress server_address;
bool        connected = false;     // the server accepted the connection
bool        entered = false;       // relay: the session is open (host) or joined (client)
bool        retry = false;         // connect again at retry_at: the host, the relay or the
enet_uint32 retry_at = 0;          // session isn't there yet
char        wanted_code[rp::code_len + 1];
int         relay_clients = 0;
char        opened_code[rp::code_len + 1];
bool        relay_link[max_links]; // relay host: the links that are up

void send_packet(ENetPeer *peer, const unsigned char *data, size_t size)
{
  ENetPacket *packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
  if (packet && enet_peer_send(peer, 0, packet) != 0)
    enet_packet_destroy(packet);
}

void connect_server()
{
  server = enet_host_connect(net, &server_address, 1, 0);
  if (server)
    enet_peer_timeout(server, 0, 5000, 10000);
}

bool starts_with(const char *text, const char *prefix)  // ignoring case
{
  for (; *prefix; text++, prefix++)
    if (tolower((unsigned char)*text) != *prefix)
      return false;
  return true;
}

// "host[:port]", or a ws:// or wss:// URL (the browser's form), whose host is used with the
// relay's ENet port.
bool resolve(const char *text, unsigned short default_port, ENetAddress *address)
{
  char host[256];
  unsigned port = default_port;
  const char *start = text;
  bool url = false;
  static const char *const schemes[] = {"ws://", "wss://"};
  for (const char *scheme : schemes)
    if (starts_with(text, scheme))
    {
      start = text + strlen(scheme);
      url = true;
    }
  snprintf(host, sizeof(host), "%s", start);
  if (char *slash = strchr(host, '/'))
    *slash = 0;
  if (char *colon = strrchr(host, ':'))
  {
    *colon = 0;
    if (!url)
      port = (unsigned)atoi(colon + 1);
  }
  if (port == 0 || port > 65535 || enet_address_set_host(address, host) != 0)
    return false;
  address->port = (unsigned short)port;
  return true;
}

bool open_client_host()
{
  net = enet_host_create(nullptr, 1, 1, 0, 0);
  if (!net)
    link_failed("no network");
  return net != nullptr;
}

const char *refused_text(int reason)
{
  switch (reason)
  {
    case rp::refused_version:     return "the relay server runs another version";
    case rp::refused_no_session:  return "no game has that code";
    case rp::refused_full:        return "the game is full";
    case rp::refused_code_taken:  return "that session code is taken";
    case rp::refused_busy:        return "the relay server is busy";
    default:                      return "the relay server refused a message";
  }
}

void relay_connected()
{
  if (mode == mode_relay_host)
  {
    unsigned char open[3 + rp::code_len] = {rp::msg_open, rp::version, (unsigned char)relay_clients};
    memcpy(open + 3, wanted_code, rp::code_len);
    send_packet(server, open, sizeof(open));
  }
  else
  {
    unsigned char enter[2 + rp::code_len] = {rp::msg_enter, rp::version};
    memcpy(enter + 2, wanted_code, rp::code_len);
    send_packet(server, enter, sizeof(enter));
  }
}

void relay_receive(const unsigned char *data, size_t size)
{
  int type = data[0];
  if (type == rp::msg_refused && size == 2)
  {
    if (mode == mode_relay_client && data[1] == rp::refused_no_session)
    {
      // Perhaps not open yet: ask again in a second (Net gives up after its timeout).
      retry = true;
      retry_at = enet_time_get() + 1000;
      return;
    }
    link_failed(refused_text(data[1]));
    return;
  }
  if (mode == mode_relay_host)
  {
    if (type == rp::msg_opened && size == 1 + rp::code_len && !entered)
    {
      memcpy(opened_code, data + 1, rp::code_len);
      opened_code[rp::code_len] = 0;
      entered = true;
      report("network: session code %s", opened_code);
    }
    else if (type == rp::msg_link_up && size == 2 && data[1] > 0 && data[1] < max_links)
    {
      relay_link[data[1]] = true;
      link_up(data[1]);
    }
    else if (type == rp::msg_from && size >= 2 && data[1] > 0 && data[1] < max_links && relay_link[data[1]])
      link_message(data[1], data + 2, size - 2);
    else if (type == rp::msg_link_down && size == 2 && data[1] > 0 && data[1] < max_links && relay_link[data[1]])
    {
      relay_link[data[1]] = false;
      link_down(data[1]);
    }
    else
      report("network: relay message of type %d and %u bytes ignored", type, (unsigned)size);
    return;
  }
  if (type == rp::msg_entered && size == 1 && !entered)
  {
    entered = true;
    link_up(1);
  }
  else if (type == rp::msg_pass && size >= 1 && entered)
    link_message(1, data + 1, size - 1);
  else
    report("network: relay message of type %d and %u bytes ignored", type, (unsigned)size);
}

void server_lost()
{
  server = nullptr;
  if (mode == mode_connect || mode == mode_relay_client)
  {
    if (!connected || (mode == mode_relay_client && !entered))
    {
      // The host or the relay isn't there yet, or the session isn't open: try again.
      connected = false;
      if (!retry)
      {
        retry = true;
        retry_at = enet_time_get() + (mode == mode_connect ? 0 : 1000);
      }
      return;
    }
    link_down(1);
    return;
  }
  // The host lost the relay: every client with it.
  if (!entered)
  {
    link_failed(connected ? "the relay server closed the connection" : "the relay server does not answer");
    return;
  }
  for (int l = 1; l < max_links; l++)
    if (relay_link[l])
    {
      relay_link[l] = false;
      link_down(l);
    }
  report("network: the connection to the relay server was lost");
}

// The host's answer to a discovery query, sent from its game socket to whoever asked.
int ENET_CALLBACK answer_query(ENetHost *host, ENetEvent *)
{
  if (host->receivedDataLength != (size_t)query_size || memcmp(host->receivedData, query_magic, 7) != 0)
    return 0;
  Summary s;
  summary(&s);
  unsigned char answer[answer_size];
  memcpy(answer, answer_magic, 7);
  answer[7] = version;
  answer[8] = (unsigned char)s.token;
  answer[9] = (unsigned char)(s.token >> 8);
  answer[10] = (unsigned char)(s.token >> 16);
  answer[11] = (unsigned char)(s.token >> 24);
  answer[12] = (unsigned char)s.players;
  answer[13] = (unsigned char)s.joined;
  answer[14] = s.started ? 1 : 0;
  memset(answer + 15, 0, name_len);
  strncpy((char *)answer + 15, s.host, name_len - 1);
  ENetBuffer buffer;
  buffer.data = answer;
  buffer.dataLength = sizeof(answer);
  enet_socket_send(host->socket, &host->receivedAddress, &buffer, 1);
  return 1;
}

void reset()
{
  mode = mode_none;
  server = nullptr;
  connected = entered = retry = false;
  retry_at = 0;
  opened_code[0] = 0;
  memset(relay_link, 0, sizeof(relay_link));
}
} // namespace

bool startup(void)
{
  return enet_initialize() == 0;
}

void shutdown(void)
{
  enet_deinitialize();
}

bool listen(unsigned short port)
{
  reset();
  ENetAddress address;
  address.host = ENET_HOST_ANY;
  address.port = port;
  net = enet_host_create(&address, max_players - 1, 1, 0, 0);
  if (!net)
  {
    link_failed("the network port is not free");
    return false;
  }
  mode = mode_listen;
  net->intercept = answer_query;
  return true;
}

bool connect(const char *address, unsigned short port)
{
  reset();
  if (!open_client_host())
    return false;
  if (enet_address_set_host(&server_address, address) != 0)
  {
    link_failed("the host's address is unknown");
    return false;
  }
  server_address.port = port;
  mode = mode_connect;
  connect_server();
  return server != nullptr;
}

bool relay_open(const char *relay, int clients, const char *code)
{
  reset();
  if (!open_client_host())
    return false;
  if (!resolve(relay, rp::enet_port, &server_address))
  {
    link_failed("the relay server's address is unknown");
    return false;
  }
  memset(wanted_code, 0, sizeof(wanted_code));
  if (code)
    for (int i = 0; i < rp::code_len && code[i]; i++)
      wanted_code[i] = (char)toupper((unsigned char)code[i]);
  relay_clients = clients;
  mode = mode_relay_host;
  connect_server();
  return server != nullptr;
}

bool relay_enter(const char *relay, const char *code)
{
  reset();
  if (strlen(code) != (size_t)rp::code_len)
  {
    link_failed("a session code has 6 characters");
    return false;
  }
  if (!open_client_host())
    return false;
  if (!resolve(relay, rp::enet_port, &server_address))
  {
    link_failed("the relay server's address is unknown");
    return false;
  }
  for (int i = 0; i < rp::code_len; i++)
    wanted_code[i] = (char)toupper((unsigned char)code[i]);
  mode = mode_relay_client;
  connect_server();
  return server != nullptr;
}

const char *code(void)
{
  return opened_code;
}

void send(int link, const unsigned char *data, size_t size)
{
  if (!net || size > (size_t)rp::max_payload)
    return;
  switch (mode)
  {
    case mode_listen:
      if (link >= 1 && (size_t)link <= net->peerCount &&
          net->peers[link - 1].state == ENET_PEER_STATE_CONNECTED)
        send_packet(&net->peers[link - 1], data, size);
      break;
    case mode_connect:
      if (server && connected)
        send_packet(server, data, size);
      break;
    case mode_relay_host:
      if (server && entered && relay_link[link])
      {
        unsigned char packet[2 + rp::max_payload] = {rp::msg_to, (unsigned char)link};
        memcpy(packet + 2, data, size);
        send_packet(server, packet, 2 + size);
      }
      break;
    case mode_relay_client:
      if (server && entered)
      {
        unsigned char packet[1 + rp::max_payload] = {rp::msg_pass};
        memcpy(packet + 1, data, size);
        send_packet(server, packet, 1 + size);
      }
      break;
    default:
      break;
  }
  enet_host_flush(net);
}

void close(int link)
{
  if (!net)
    return;
  if (mode == mode_listen && link >= 1 && (size_t)link <= net->peerCount)
    enet_peer_disconnect_later(&net->peers[link - 1], 0);
  else if (mode == mode_relay_host && server && relay_link[link])
  {
    unsigned char packet[2] = {rp::msg_close, (unsigned char)link};
    send_packet(server, packet, sizeof(packet));
  }
  enet_host_flush(net);
}

void poll(void)
{
  if (!net)
    return;
  // Connect again once the server has gone (a relay disconnects after refusing).
  if (retry && !server && (int)(enet_time_get() - retry_at) >= 0)
  {
    retry = false;
    connect_server();
  }
  ENetEvent event;
  while (enet_host_service(net, &event, 0) > 0)
  {
    switch (event.type)
    {
      case ENET_EVENT_TYPE_CONNECT:
        enet_peer_timeout(event.peer, 0, 5000, 10000);
        if (mode == mode_listen)
        {
          int link = (int)(event.peer - net->peers) + 1;
          event.peer->data = (void *)(uintptr_t)link;
          link_up(link);
        }
        else
        {
          connected = true;
          if (mode == mode_connect)
            link_up(1);
          else
            relay_connected();
        }
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        if (event.packet->dataLength > 0)
        {
          if (mode == mode_listen)
          {
            int link = (int)(uintptr_t)event.peer->data;
            if (link)
              link_message(link, event.packet->data, event.packet->dataLength);
          }
          else if (mode == mode_connect)
            link_message(1, event.packet->data, event.packet->dataLength);
          else
            relay_receive(event.packet->data, event.packet->dataLength);
        }
        enet_packet_destroy(event.packet);
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
        if (mode == mode_listen)
        {
          int link = (int)(uintptr_t)event.peer->data;
          event.peer->data = nullptr;
          if (link)
            link_down(link);
        }
        else if (event.peer == server)
          server_lost();
        break;
      default:
        break;
    }
  }
  enet_host_flush(net);
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
  reset();
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
      memset(game.host, 0, name_len);
      memcpy(game.host, answer + 15, name_len - 1);
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
} // namespace link
} // namespace enet_transport
