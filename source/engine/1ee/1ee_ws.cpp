// Port: the WebSocket link layer of the network transport (1ee_link.h), for the browser (phase 9).
// A browser has no UDP sockets, can't accept connections and can't broadcast, so the browser
// plays only through a relay server (1ee_relay.h): hosting opens a session there, joining enters
// one by its code. No direct connections and no LAN discovery.
//
// The browser delivers the socket's events while the game waits (JSPI or Asyncify suspends it),
// so the callbacks only queue them, and poll() handles them on the game's own stack.

#include "1ee_link.h"
#include "1ee_relay.h"

#include <compat/browser.h>
#include <emscripten/websocket.h>

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace enet_transport
{
namespace link
{
namespace
{
namespace rp = relay_protocol;

enum Mode { mode_none, mode_relay_host, mode_relay_client };
enum Kind { event_open, event_message, event_close };

struct Event
{
  Kind          kind;
  unsigned      size;
  unsigned char data[2 + rp::max_payload];
};

// Plain data only (see Net::quit).
const int  queue_size = 512;
Event      queue[queue_size];
int        queue_head = 0;
int        queue_count = 0;
bool       queue_overflow = false;

EMSCRIPTEN_WEBSOCKET_T socket = 0;
Mode       mode = mode_none;
bool       is_open = false;
bool       entered = false;
bool       retry = false;           // enter again after retry_at: the session isn't open yet
double     retry_at = 0;
char       url[512];
char       wanted_code[rp::code_len + 1];
int        relay_clients = 0;
char       opened_code[rp::code_len + 1];
bool       relay_link[max_links];   // host: the links that are up

void push(Kind kind, const void *data, unsigned size)
{
  if (queue_count == queue_size || size > sizeof(queue[0].data))
  {
    queue_overflow = true;
    return;
  }
  Event &e = queue[(queue_head + queue_count) % queue_size];
  e.kind = kind;
  e.size = size;
  if (size)
    memcpy(e.data, data, size);
  queue_count++;
}

EM_BOOL on_open(int, const EmscriptenWebSocketOpenEvent *, void *)
{
  push(event_open, nullptr, 0);
  return EM_TRUE;
}

EM_BOOL on_message(int, const EmscriptenWebSocketMessageEvent *e, void *)
{
  if (!e->isText && e->numBytes > 0)
    push(event_message, e->data, e->numBytes);
  return EM_TRUE;
}

EM_BOOL on_close(int, const EmscriptenWebSocketCloseEvent *, void *)
{
  push(event_close, nullptr, 0);
  return EM_TRUE;
}

EM_BOOL on_error(int, const EmscriptenWebSocketErrorEvent *, void *)
{
  return EM_TRUE;  // a close event follows
}

bool starts_with(const char *text, const char *prefix)  // ignoring case
{
  for (; *prefix; text++, prefix++)
    if (tolower((unsigned char)*text) != *prefix)
      return false;
  return true;
}

void close_socket()
{
  if (socket > 0)
  {
    emscripten_websocket_close(socket, 1000, "");
    emscripten_websocket_delete(socket);
  }
  socket = 0;
  is_open = false;
}

bool open_socket()
{
  close_socket();
  queue_head = queue_count = 0;
  EmscriptenWebSocketCreateAttributes attributes;
  emscripten_websocket_init_create_attributes(&attributes);
  attributes.url = url;
  attributes.protocols = nullptr;
  attributes.createOnMainThread = EM_TRUE;
  socket = emscripten_websocket_new(&attributes);
  if (socket <= 0)
  {
    socket = 0;
    link_failed("the relay server's address is not a WebSocket URL");
    return false;
  }
  emscripten_websocket_set_onopen_callback(socket, nullptr, on_open);
  emscripten_websocket_set_onmessage_callback(socket, nullptr, on_message);
  emscripten_websocket_set_onclose_callback(socket, nullptr, on_close);
  emscripten_websocket_set_onerror_callback(socket, nullptr, on_error);
  return true;
}

void send_raw(const unsigned char *data, size_t size)
{
  if (socket > 0 && is_open)
    emscripten_websocket_send_binary(socket, (void *)data, (uint32_t)size);
}

// "wss://..." or "ws://..." as given; otherwise a host name, reached as wss://<host>/.
void make_url(const char *relay)
{
  if (starts_with(relay, "ws://") || starts_with(relay, "wss://"))
    snprintf(url, sizeof(url), "%s", relay);
  else
    snprintf(url, sizeof(url), "wss://%s/", relay);
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

void opened()
{
  is_open = true;
  if (mode == mode_relay_host)
  {
    unsigned char open[3 + rp::code_len] = {rp::msg_open, rp::version, (unsigned char)relay_clients};
    memcpy(open + 3, wanted_code, rp::code_len);
    send_raw(open, sizeof(open));
  }
  else
  {
    unsigned char enter[2 + rp::code_len] = {rp::msg_enter, rp::version};
    memcpy(enter + 2, wanted_code, rp::code_len);
    send_raw(enter, sizeof(enter));
  }
}

void receive(const unsigned char *data, size_t size)
{
  int type = data[0];
  if (type == rp::msg_refused && size == 2)
  {
    if (mode == mode_relay_client && data[1] == rp::refused_no_session)
    {
      // Perhaps not open yet: ask again in a second (Net gives up after its timeout).
      retry = true;
      retry_at = emscripten_get_now() + 1000;
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
      char text[80];
      snprintf(text, sizeof(text), "Session code %s: waiting for the other players", opened_code);
      browser_status(text);
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

void closed()
{
  bool was_open = is_open;
  close_socket();
  if (mode == mode_relay_client && !entered)
  {
    // The relay can't be reached yet, or the session isn't open: try again in a second.
    if (!retry)
    {
      retry = true;
      retry_at = emscripten_get_now() + 1000;
    }
    return;
  }
  if (mode == mode_relay_client)
  {
    link_down(1);
    return;
  }
  if (!entered)
  {
    link_failed(was_open ? "the relay server closed the connection" : "the relay server does not answer");
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

void reset()
{
  close_socket();
  mode = mode_none;
  entered = retry = false;
  queue_head = queue_count = 0;
  queue_overflow = false;
  opened_code[0] = 0;
  memset(relay_link, 0, sizeof(relay_link));
}
} // namespace

bool startup(void)
{
  return emscripten_websocket_is_supported() != 0;
}

void shutdown(void)
{
  reset();
}

bool listen(unsigned short)
{
  link_failed("a browser can't host directly: host through a relay server (--relay)");
  return false;
}

bool connect(const char *, unsigned short)
{
  link_failed("a browser can't join directly: join through a relay server (--relay)");
  return false;
}

bool relay_open(const char *relay, int clients, const char *code)
{
  reset();
  make_url(relay);
  memset(wanted_code, 0, sizeof(wanted_code));
  if (code)
    for (int i = 0; i < rp::code_len && code[i]; i++)
      wanted_code[i] = (char)toupper((unsigned char)code[i]);
  relay_clients = clients;
  mode = mode_relay_host;
  return open_socket();
}

bool relay_enter(const char *relay, const char *code)
{
  reset();
  if (strlen(code) != (size_t)rp::code_len)
  {
    link_failed("a session code has 6 characters");
    return false;
  }
  make_url(relay);
  for (int i = 0; i < rp::code_len; i++)
    wanted_code[i] = (char)toupper((unsigned char)code[i]);
  wanted_code[rp::code_len] = 0;
  mode = mode_relay_client;
  char text[80];
  snprintf(text, sizeof(text), "Joining the game %s", wanted_code);
  browser_status(text);
  return open_socket();
}

const char *code(void)
{
  return opened_code;
}

void send(int link, const unsigned char *data, size_t size)
{
  if (size > (size_t)rp::max_payload || !entered)
    return;
  if (mode == mode_relay_host && link > 0 && link < max_links && relay_link[link])
  {
    unsigned char packet[2 + rp::max_payload] = {rp::msg_to, (unsigned char)link};
    memcpy(packet + 2, data, size);
    send_raw(packet, 2 + size);
  }
  else if (mode == mode_relay_client)
  {
    unsigned char packet[1 + rp::max_payload] = {rp::msg_pass};
    memcpy(packet + 1, data, size);
    send_raw(packet, 1 + size);
  }
}

void close(int link)
{
  if (mode == mode_relay_host && link > 0 && link < max_links && relay_link[link])
  {
    unsigned char packet[2] = {rp::msg_close, (unsigned char)link};
    send_raw(packet, sizeof(packet));
  }
}

void poll(void)
{
  if (mode == mode_none)
    return;
  if (retry && socket == 0 && emscripten_get_now() >= retry_at)
  {
    retry = false;
    open_socket();
  }
  while (queue_count > 0)
  {
    // A copy: handling it may suspend the game (browser_status), and the callbacks may then
    // reuse the slot.
    Event e = queue[queue_head];
    queue_head = (queue_head + 1) % queue_size;
    queue_count--;
    if (e.kind == event_open)
      opened();
    else if (e.kind == event_message)
      receive(e.data, e.size);
    else
      closed();
    if (socket == 0)
      break;  // closed: what's left belongs to the old socket
  }
  if (queue_overflow)
  {
    queue_overflow = false;
    link_failed("too many network messages waiting");
  }
}

void leave(void)
{
  reset();
}

int discover(unsigned short, unsigned, Found *, int)
{
  return 0;  // no LAN discovery in a browser
}
} // namespace link
} // namespace enet_transport
