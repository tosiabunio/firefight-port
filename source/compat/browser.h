#pragma once

// The browser build (phase 9, FF_BROWSER): what the game needs from its page (web/index.html,
// web/pre.js). Elsewhere these do nothing.

// After a frame is shown: wait for the browser's next frame, as SDL_RenderPresent with vsync
// does on the desktop. The browser shows the canvas, plays the sound and delivers input only
// while the game waits (JSPI suspends it).
void browser_next_frame();

// From the message pump (Comm::process_messages): lets the browser run when the game hasn't waited
// for a while, so a loop that polls for the other players still gets their messages (the browser
// delivers them only while the game waits). Frames and idle time wait anyway.
void browser_poll();

// A startup stage: the page shows it and repaints.
void browser_status(const char *text);

// Ends the game: exit(code), which runs the static destructors and the quits and tells the page.
// From here on nothing waits for the browser: the game can't be suspended inside exit().
[[noreturn]] void browser_exit(int code);

// The engine's last error message, which the page shows when the game ends.
void browser_message(const char *text);

// A file in the preferences directory changed. The page copies the directory to IndexedDB, which
// keeps it across visits (shortly afterwards, once for several writes).
void browser_pref_written();
