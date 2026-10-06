#include <compat/browser.h>

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

EM_JS(void, show_status, (const char *text), {
  if (Module.ffStatus) Module.ffStatus(UTF8ToString(text));
});

EM_JS(void, first_frame, (), {
  if (Module.ffFirstFrame) Module.ffFirstFrame();
});

// ffPrefWritten is in web/pre.js.
EM_JS(void, pref_written, (), {
  ffPrefWritten();
});

void browser_next_frame()
{
  static bool shown = false;
  if (!shown)
  {
    shown = true;
    first_frame();
  }
  wait_for_frame();
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

#else

void browser_next_frame()
{
}

void browser_status(const char *)
{
}

void browser_pref_written()
{
}

#endif
