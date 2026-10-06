#pragma once

// The browser build (phase 9, FF_BROWSER): what the game needs from its page (web/index.html,
// web/pre.js). Elsewhere these do nothing.

// After a frame is shown: wait for the browser's next frame, as SDL_RenderPresent with vsync
// does on the desktop. The browser shows the canvas, plays the sound and delivers input only
// while the game waits (JSPI suspends it).
void browser_next_frame();

// A startup stage: the page shows it and repaints.
void browser_status(const char *text);

// A file in the preferences directory changed. The page copies the directory to IndexedDB, which
// keeps it across visits (shortly afterwards, once for several writes).
void browser_pref_written();
