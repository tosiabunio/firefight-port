// Port entry point (SDL_main): resolves the data and preferences directories, then runs the
// original game (game_main in game.cpp, formerly WinMain).
//
//   firefight [--data <dir>] [--pref <dir>] [--music <dir>] [--headless] [--sound]
//             [--quit-after <s>] [--fullscreen] [--stretch] [--shots <s>] [--fast]
//             [--shot-every <n>] [--quit-frames <n>] [--demo <name>] [--input <file>]
//             [--host <players> | --join <address>] [--port <port>] [switch ...]
//
//   --data <dir>      directory holding cwe.ini and the *.dir manifests. Default: "data" next
//                     to the executable, else the repository's data/ (development builds),
//                     else the current directory
//   --pref <dir>      settings, pilots and the log. Default: SDL_GetPrefPath
//   --music <dir>     the soundtrack, track02.flac ... Default: "music" next to the data
//                     directory
//   --headless        no window, video, sound or input devices (adds the engine switches
//                     headless=1 sos_none=1)
//   --sound           with --headless, keep the sound (drop sos_none=1); for tests with SDL's
//                     dummy or disk audio driver
//   --quit-after <s>  act as if the window were closed after s seconds (quit_after=<ms>)
//   --fullscreen      desktop full screen (fullscreen=1)
//   --stretch         show the 640x400 picture at 4:3 like a CRT (stretch=1); default: square pixels
//   --shots <s>       save the displayed frame every s seconds as frame_NNNN.bmp in the
//                     preferences directory (shots=<ms>)
//   --fast            no clock: one simulation step per frame, as fast as possible (fast=1)
//   --shot-every <n>  save every n-th frame as frame_<frame>.bmp (shot_every=<n>)
//   --quit-frames <n> act as if the window were closed after n frames (quit_frames=<n>)
//   --demo <name>     play one recorded demo (level1 ... level4C, or ! and the path of a demo
//                     file) instead of the title loop, then quit
//   --input <file>    play an input script: keys, mouse and a virtual game controller at given
//                     frames (input_script.cpp)
//   --host <players>  host a network game for 2-4 players and wait for them (net_host=<n>)
//   --join <address>  join the network game hosted at that address (net_join=<address>); lan
//                     joins the first open game found on the LAN
//   --port <port>     the network game's UDP port (net_port=<port>), default 19960
//
// Relative paths mean the current directory. Everything else is passed through as an original
// engine switch (debug=1, check, ...).
// The exit code is 1 when the engine reported a critical error. With --demo it is 3 when the
// replay went out of sync and 4 when it stopped before the end of the recording.

#include <compat/win32.h>
#include <1lg.h>
#include <1ss.h>

#include <SDL.h>

#include <filesystem>
#include <string>
#include <system_error>

#ifdef _MSC_VER
#include <crtdbg.h>
#include <stdlib.h>
#endif

int game_main(char *command_line);
bool input_script_load(const char *file);
#ifdef _WIN32
void attach_parent_console();  // win_console.cpp
#endif

// --demo's argument as given: the engine upper-cases its switches and splits them at spaces,
// which a file path can't take (Game::go).
const char *demo_argument = nullptr;

namespace fs = std::filesystem;

#ifdef _MSC_VER
// The MSVC 4 runtime returned an error for invalid arguments (Cwe::init closes cwe.ini a second
// time, for example). The modern CRT calls an invalid-parameter handler that shows a modal
// dialog in debug builds; restore the old behaviour and send CRT reports to stderr.
static void ignore_invalid_parameter(const wchar_t *, const wchar_t *, const wchar_t *, unsigned, uintptr_t)
{
}

static void use_msvc4_crt_behaviour()
{
  _set_invalid_parameter_handler(ignore_invalid_parameter);
  for (int type : {_CRT_WARN, _CRT_ERROR, _CRT_ASSERT})
  {
    _CrtSetReportMode(type, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(type, _CRTDBG_FILE_STDERR);
  }
  _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
}
#endif

static bool is_data_dir(const fs::path &dir)
{
  std::error_code ec;
  return fs::exists(dir / "cwe.ini", ec);
}

static fs::path default_data_dir()
{
  if (char *base = SDL_GetBasePath())
  {
    fs::path next_to_exe = fs::u8path(base) / "data";
    SDL_free(base);
    if (is_data_dir(next_to_exe))
      return next_to_exe;
  }
#ifdef FF_DEFAULT_DATA_DIR
  if (is_data_dir(fs::u8path(FF_DEFAULT_DATA_DIR)))
    return fs::u8path(FF_DEFAULT_DATA_DIR);
#endif
  return fs::path(".");
}

static bool set_pref_dir(const char *dir)
{
  std::string path;
  if (dir)
  {
    std::error_code ec;
    fs::create_directories(fs::u8path(dir), ec);
    path = dir;
  }
  else if (char *pref = SDL_GetPrefPath("Chaos Works", "Fire Fight"))
  {
    path = pref;
    SDL_free(pref);
  }
  else
    return false;
  if (path.empty() || (path.back() != '/' && path.back() != '\\'))
    path += '/';
  if (path.size() >= _MAX_PATH)
    return false;
  strcpy(Comm::pref_path, path.c_str());
  return true;
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
  attach_parent_console();
#endif
#ifdef _MSC_VER
  use_msvc4_crt_behaviour();
#endif
  const char *data_dir = nullptr;
  const char *pref_dir = nullptr;
  const char *music_dir = nullptr;
  const char *input_script = nullptr;
  bool headless = false, sound = false;
  std::string line = std::string("\"") + argv[0] + "\"";
  for (int i = 1; i < argc; i++)
  {
    std::string arg = argv[i];
    if (arg == "--data" && i + 1 < argc)
      data_dir = argv[++i];
    else if (arg == "--pref" && i + 1 < argc)
      pref_dir = argv[++i];
    else if (arg == "--music" && i + 1 < argc)
      music_dir = argv[++i];
    else if (arg == "--headless")
      headless = true;
    else if (arg == "--sound")
      sound = true;
    else if (arg == "--quit-after" && i + 1 < argc)
      line += " quit_after=" + std::to_string((int)(atof(argv[++i]) * 1000));
    else if (arg == "--shots" && i + 1 < argc)
      line += " shots=" + std::to_string((int)(atof(argv[++i]) * 1000));
    else if (arg == "--fast")
      line += " fast=1";
    else if (arg == "--shot-every" && i + 1 < argc)
      line += " shot_every=" + std::string(argv[++i]);
    else if (arg == "--quit-frames" && i + 1 < argc)
      line += " quit_frames=" + std::string(argv[++i]);
    else if (arg == "--demo" && i + 1 < argc)
      demo_argument = argv[++i];
    else if (arg == "--input" && i + 1 < argc)
      input_script = argv[++i];
    else if (arg == "--host" && i + 1 < argc)
      line += " net_host=" + std::string(argv[++i]);
    else if (arg == "--join" && i + 1 < argc)
      line += " net_join=" + std::string(argv[++i]);
    else if (arg == "--port" && i + 1 < argc)
      line += " net_port=" + std::string(argv[++i]);
    else if (arg == "--fullscreen")
      line += " fullscreen=1";
    else if (arg == "--stretch")
      line += " stretch=1";
    else
      line += " " + arg;
  }
  if (headless)
    line += sound ? " headless=1" : " headless=1 sos_none=1";

  // Before changing to the data directory, so a relative script path means the current one.
  if (input_script && !input_script_load(input_script))
    return 2;

  const fs::path data = data_dir ? fs::u8path(data_dir) : default_data_dir();
  std::error_code ec;
  // Relative paths on the command line mean the current directory, so make the preferences
  // directory and a demo file absolute before changing to the data directory.
  std::string pref_absolute, demo_absolute;
  if (pref_dir)
  {
    fs::path p = fs::absolute(fs::u8path(pref_dir), ec);
    if (!ec)
      pref_dir = (pref_absolute = p.u8string()).c_str();
  }
  if (demo_argument && demo_argument[0] == '!')
  {
    fs::path p = fs::absolute(fs::u8path(demo_argument + 1), ec);
    if (!ec)
      demo_argument = (demo_absolute = "!" + p.u8string()).c_str();
  }
  // The soundtrack, as an absolute path before changing to the data directory.
  fs::path music = music_dir ? fs::u8path(music_dir) : data / ".." / "music";
  music = fs::absolute(music, ec).lexically_normal();
  CD::set_directory(music.u8string().c_str());
  fs::current_path(data, ec);
  if (ec)
  {
    fprintf(stderr, "firefight: cannot change to data directory '%s'\n", data.u8string().c_str());
    return 2;
  }
  if (!set_pref_dir(pref_dir))
  {
    fprintf(stderr, "firefight: no usable preferences directory\n");
    return 2;
  }

  game_main(&line[0]);
  return critical_error_occurred ? 1 : Comm::exit_code;
}
