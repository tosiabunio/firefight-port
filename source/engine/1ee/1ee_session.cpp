// Port: the session logic of the network transport (1ee_enet.h), on the links of a link layer
// (1ee_link.h): ENet directly or through a relay server, or WebSocket to a relay.
//
// Messages, reliable and in order:
//   HELLO   client -> host   type, version, name
//   WELCOME host -> client   type, the client's id
//   REFUSE  host -> client   type, reason (refuse_*)
//   START   host -> clients  type, players, has_info, info[28], count, count x (id, name)
//   DATA    any              type, from, to, the engine's message (the host forwards it)
//   LEFT    host -> clients  type, the id of a player who left
// Everything received is checked before use: length, type, and that a client's DATA carries its
// own id as the sender.

#include "1ee_link.h"
#include "1ee_relay.h"

#include <ctype.h>
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

struct Slot
{
  bool present;
  char name[name_len];
  int  link;  // the host's link to that client
};

struct Message
{
  unsigned      from;
  unsigned      size;
  unsigned char data[max_message];
};

void (*log_proc)(const char *text) = nullptr;
bool          initialised = false;
bool          active = false;      // hosting or joining
bool          is_host = false;
bool          is_started = false;
const char   *failed = nullptr;
unsigned      own_id = 0;
int           max = 0;
bool          has_info = false;
unsigned char info_bytes[info_len];
char          own_name[name_len];
unsigned      token = 0;  // tells this game apart in LAN discovery answers
char          relay_address[256];
char          relay_code[32];
unsigned      link_player[max_links];  // host: the player on each link, 0 before its HELLO
Slot          slots[max_players + 1];  // by player id
// The messages received for this player, oldest first. Plain data only: the engine's quit
// procedures (Net::quit) run after the static destructors, so nothing here may have one.
const int     inbox_size = 1024;
Message       inbox[inbox_size];
int           inbox_head = 0;
int           inbox_count = 0;

void copy_name(char *to, const char *from)
{
  strncpy(to, from ? from : "", name_len - 1);
  to[name_len - 1] = 0;
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

bool valid_link(int link)
{
  return link > 0 && link < max_links;
}

void host_receive(int link, const unsigned char *data, size_t size)
{
  unsigned id = link_player[link];
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
      link::send(link, refuse, sizeof(refuse));
      link::close(link);
      return;
    }
    for (id = 2; id <= (unsigned)max && slots[id].present; id++)
      ;
    slots[id].present = true;
    slots[id].link = link;
    char name[name_len];
    memcpy(name, data + 2, name_len);
    name[name_len - 1] = 0;
    copy_name(slots[id].name, name);
    link_player[link] = id;
    unsigned char welcome[2] = {msg_welcome, (unsigned char)id};
    link::send(link, welcome, sizeof(welcome));
    report("network: player %u (%s) joined, %d of %d", id, slots[id].name, present_count(), max);
    return;
  }
  if (data[0] == msg_data && size >= 3 && size - 3 <= max_message && id != 0 && data[1] == id)
  {
    unsigned to = data[2];
    if (to == own_id)
      deliver(id, data + 3, size - 3);
    else if (to >= 1 && to <= max_players && slots[to].present && slots[to].link)
      link::send(slots[to].link, data, size);
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

// A session code: relay_protocol::code_len of its characters, in either case.
bool valid_code(const char *code)
{
  if (strlen(code) != (size_t)relay_protocol::code_len)
    return false;
  for (const char *c = code; *c; c++)
    if (!strchr(relay_protocol::code_chars, toupper((unsigned char)*c)))
      return false;
  return true;
}

const char *bad_code = "a session code is 6 letters and digits: A-Z and 2-9, without I and O";

void begin(bool host, const char *name)
{
  is_host = host;
  is_started = false;
  failed = nullptr;
  own_id = host ? 1 : 0;
  copy_name(own_name, name);
  memset(slots, 0, sizeof(slots));
  memset(link_player, 0, sizeof(link_player));
  inbox_head = inbox_count = 0;
  active = true;
}
} // namespace

// --- called by the link layer --------------------------------------------------------------

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

void link_up(int link)
{
  if (!valid_link(link))
    return;
  link_player[link] = 0;
  if (!is_host)
  {
    unsigned char hello[2 + name_len];
    hello[0] = msg_hello;
    hello[1] = version;
    memcpy(hello + 2, own_name, name_len);
    link::send(link, hello, sizeof(hello));
  }
}

void link_message(int link, const unsigned char *data, size_t size)
{
  if (!valid_link(link) || size == 0)
    return;
  if (is_host)
    host_receive(link, data, size);
  else
    client_receive(data, size);
}

void link_down(int link)
{
  if (!valid_link(link))
    return;
  if (is_host)
  {
    unsigned id = link_player[link];
    link_player[link] = 0;
    if (id == 0 || !slots[id].present)
      return;
    slots[id].present = false;
    slots[id].link = 0;
    report("network: player %u (%s) left", id, slots[id].name);
    if (!is_started)
      return;
    unsigned char left[2] = {msg_left, (unsigned char)id};
    for (int other = 2; other <= max_players; other++)
      if (slots[other].present && slots[other].link)
        link::send(slots[other].link, left, sizeof(left));
    return;
  }
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

void link_failed(const char *why)
{
  if (!failed)
    failed = why;
}

void summary(Summary *s)
{
  s->token = token;
  s->players = max;
  s->joined = present_count();
  s->started = is_started;
  s->host = slots[1].name;
}

// --- the transport (1ee_enet.h) ------------------------------------------------------------

bool startup(void (*log_function)(const char *text))
{
  log_proc = log_function;
  if (!initialised && !link::startup())
    return false;
  initialised = true;
  return true;
}

void shutdown(void)
{
  leave();
  if (initialised)
    link::shutdown();
  initialised = false;
}

void set_relay(const char *relay, const char *code)
{
  snprintf(relay_address, sizeof(relay_address), "%s", relay ? relay : "");
  snprintf(relay_code, sizeof(relay_code), "%s", code ? code : "");
}

const char *session_code(void)
{
  return link::code();
}

bool host(unsigned short port, int players, const char *name, const unsigned char *info)
{
  if (players < 2 || players > max_players)
  {
    failed = "the number of players must be 2 to 4";
    return false;
  }
  begin(true, name);
  max = players;
  has_info = info != nullptr;
  memset(info_bytes, 0, info_len);
  if (info)
    memcpy(info_bytes, info, info_len);
  slots[1].present = true;
  copy_name(slots[1].name, name);
  token = (unsigned)time(nullptr) ^ (unsigned)(uintptr_t)&token ^ ((unsigned)clock() << 16);
  if (relay_address[0])
  {
    if (relay_code[0] && !valid_code(relay_code))
    {
      failed = bad_code;
      return false;
    }
    if (!link::relay_open(relay_address, players - 1, relay_code[0] ? relay_code : nullptr))
      return false;
    report("network: hosting a game for %d players through the relay %s", players, relay_address);
    return true;
  }
  if (!link::listen(port))
    return false;
  report("network: hosting a game for %d players on port %u", players, (unsigned)port);
  return true;
}

bool join(const char *address, unsigned short port, const char *name)
{
  begin(false, name);
  if (relay_address[0])
  {
    if (!valid_code(address))
    {
      failed = bad_code;
      return false;
    }
    report("network: joining the game %s through the relay %s", address, relay_address);
    return link::relay_enter(relay_address, address);
  }
  report("network: connecting to %s, port %u", address, (unsigned)port);
  return link::connect(address, port);
}

void poll(void)
{
  if (active)
    link::poll();
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
    if (slots[id].present && slots[id].link)
      link::send(slots[id].link, packet, start_header + count * start_entry);
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
  if (!active || size > max_message || to < 1 || to > max_players || to == own_id || !slots[to].present)
    return false;
  unsigned char packet[3 + max_message];
  packet[0] = msg_data;
  packet[1] = (unsigned char)own_id;
  packet[2] = (unsigned char)to;
  memcpy(packet + 3, data, size);
  int link = is_host ? slots[to].link : 1;  // a client has one link, to the host
  if (!link)
    return false;
  link::send(link, packet, 3 + size);
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
  if (!active)
    return;
  link::leave();
  active = is_host = is_started = false;
  own_id = 0;
  inbox_head = inbox_count = 0;
}

int discover(unsigned short port, unsigned timeout, Found *games, int max_games)
{
  return link::discover(port, timeout, games, max_games);
}
} // namespace enet_transport
