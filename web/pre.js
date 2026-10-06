// Fire Fight in the browser (phase 9): the module side of the page (web/index.html). Emscripten
// puts this at the start of firefight.js (--pre-js), so it sees the runtime: FS, IDBFS, the run
// dependencies and the game's exports.

// The preferences directory (settings, pilots, recorded demos) is SDL_GetPrefPath's, under
// /libsdl. IndexedDB keeps it: it loads before the game starts and saves after the game writes
// to it (ffPrefWritten).
Module.preRun = [].concat(Module.preRun || [], () => {
  FS.mkdir('/libsdl');
  FS.mount(IDBFS, {}, '/libsdl');
  addRunDependency('ff-pref');
  FS.syncfs(true, (error) => {
    if (error) console.warn('Fire Fight: the saved settings do not load', error);
    removeRunDependency('ff-pref');
  });
  // The soundtrack downloads in the background after the data. Until a track has arrived its
  // file is empty, and the game waits for it (CD::play).
  FS.mkdir('/music');
  for (const name of Module.ffTracks || [])
    FS.writeFile('/music/' + name, new Uint8Array(0));
});

// One track at a time, in the order of the list. When a track arrives the game starts it if it
// is waiting for it (ff_music_arrived in 1ss_song.cpp).
Module.postRun = [].concat(Module.postRun || [], async () => {
  for (const name of Module.ffTracks || []) {
    try {
      const response = await fetch('music/' + name);
      if (!response.ok)
        throw new Error(response.status + ' ' + response.statusText);
      const data = new Uint8Array(await response.arrayBuffer());
      if (ABORT || runtimeExited)
        return;
      FS.writeFile('/music/' + name, data);
      _ff_music_arrived();
    } catch (error) {
      console.warn('Fire Fight: music track ' + name + ' does not download', error);
    }
  }
});

// The game wrote to the preferences directory (browser_pref_written in compat/browser.cpp).
// Saved once the writes have settled, one save at a time, and at once when the page is hidden:
// a page may be closed at any time.
var ffPrefTimer = null;
var ffPrefSaving = false;
var ffPrefAgain = false;

function ffPrefWritten() {
  clearTimeout(ffPrefTimer);
  ffPrefTimer = setTimeout(ffPrefSave, 500);
}

function ffPrefSave() {
  clearTimeout(ffPrefTimer);
  ffPrefTimer = null;
  if (ffPrefSaving) {
    ffPrefAgain = true;
    return;
  }
  ffPrefSaving = true;
  FS.syncfs(false, (error) => {
    if (error) console.warn('Fire Fight: the settings are not saved', error);
    ffPrefSaving = false;
    if (ffPrefAgain) {
      ffPrefAgain = false;
      ffPrefSave();
    }
  });
}

addEventListener('visibilitychange', () => {
  if (document.hidden && ffPrefTimer !== null)
    ffPrefSave();
});
