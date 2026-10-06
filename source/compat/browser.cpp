#include <compat/browser.h>

#include <stdlib.h>

#ifdef FF_BROWSER

#include <emscripten.h>

// The next animation frame. A hidden tab gets none, so a timer stands in (the browser slows it
// down to about once a second there).
EM_ASYNC_JS(void, wait_for_frame, (), {
  await new Promise(resolve => {
    const timer = setTimeout(resolve, 100);
    requestAnimationFrame(() => { clearTimeout(timer); resolve(); });
  });
});

// A short wait: a message to itself, which browsers don't hold back the way they do timers.
EM_ASYNC_JS(void, yield_to_browser, (), {
  await new Promise(resolve => {
    const channel = new MessageChannel();
    channel.port1.onmessage = resolve;
    channel.port2.postMessage(0);
  });
});

EM_JS(void, show_status, (const char *text), {
  if (Module.ffStatus) Module.ffStatus(UTF8ToString(text));
});

EM_JS(void, first_frame, (), {
  if (Module.ffFirstFrame) Module.ffFirstFrame();
});

EM_JS(void, set_message, (const char *text), {
  Module.ffMessage = UTF8ToString(text);
});

// ffPrefWritten is in web/pre.js.
EM_JS(void, pref_written, (), {
  ffPrefWritten();
});

static double last_wait = 0;  // when the game last let the browser run (ms)
static bool   exiting = false;

void browser_next_frame()
{
  static bool shown = false;
  if (!shown)
  {
    shown = true;
    first_frame();
  }
  if (exiting)
    return;
  wait_for_frame();
  last_wait = emscripten_get_now();
}

void browser_poll()
{
  if (exiting || emscripten_get_now() - last_wait < 10)
    return;
  yield_to_browser();
  last_wait = emscripten_get_now();
}

void browser_status(const char *text)
{
  show_status(text);
  wait_for_frame();
}

void browser_pref_written()
{
  pref_written();
}

void browser_message(const char *text)
{
  set_message(text);
}

void browser_exit(int code)
{
  exiting = true;
  exit(code);
}

#else

void browser_next_frame()
{
}

void browser_poll()
{
}

void browser_status(const char *)
{
}

void browser_pref_written()
{
}

void browser_message(const char *)
{
}

void browser_exit(int code)
{
  exit(code);
}

#endif
