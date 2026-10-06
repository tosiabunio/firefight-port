// Port: the network transport of the Emscripten build under Node.js (phase 9), which has neither
// ENet nor a WebSocket: network play reports itself unavailable. Single player never calls the
// transport (Net::init runs only for --host and --join). The browser build has its WebSocket link
// layer (1ee_ws.cpp).

#include "1ee_enet.h"

namespace enet_transport
{
namespace
{
const char *unavailable = "network play is not available in this build";
}

bool startup(void (*log_function)(const char *text))
{
  if (log_function)
    log_function(unavailable);
  return false;
}

void shutdown(void) {}
void set_relay(const char *, const char *) {}
const char *session_code(void) { return ""; }
bool host(unsigned short, int, const char *, const unsigned char *) { return false; }
bool join(const char *, unsigned short, const char *) { return false; }
void poll(void) {}
int  joined(void) { return 0; }
void start(void) {}
bool started(void) { return false; }
const char *failure(void) { return unavailable; }
unsigned my_id(void) { return 0; }
int  players_max(void) { return 0; }
bool game_info(unsigned char *) { return false; }
int  roster(Player *) { return 0; }
bool send(unsigned, const void *, unsigned) { return false; }
bool receive(void *, unsigned *, unsigned *) { return false; }
void leave(void) {}
int  discover(unsigned short, unsigned, Found *, int) { return 0; }
}
