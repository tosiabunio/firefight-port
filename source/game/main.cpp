// Port entry point (SDL_main): resolves the data and preferences directories, then runs the
// original game (game_main in game.cpp, formerly WinMain).
//
//   firefight [--data <dir>] [--pref <dir>] [--headless] [--quit-after <seconds>] [switch ...]
//
//   --data <dir>      directory holding cwe.ini and the *.dir manifests. Default: "data" next
//                     to the executable, else the repository's data/ (development builds),
//                     else the current directory
//   --pref <dir>      settings, pilots and the log. Default: SDL_GetPrefPath
//   --headless        no window, video, sound or input devices (adds the engine switches
//                     headless=1 sos_none=1)
//   --quit-after <s>  act as if the window were closed after s seconds (quit_after=<ms>)
//
// Everything else is passed through as an original engine switch (debug=1, check, ...).
// The exit code is non-zero when the engine reported a critical error.

#include <compat/win32.h>
#include <1lg.h>

#include <SDL.h>

#include <filesystem>
#include <string>
#include <system_error>

#ifdef _MSC_VER
#include <crtdbg.h>
#include <stdlib.h>
#endif

int game_main(char *command_line);

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
#ifdef _MSC_VER
  use_msvc4_crt_behaviour();
#endif
  const char *data_dir = nullptr;
  const char *pref_dir = nullptr;
  std::string line = std::string("\"") + argv[0] + "\"";
  for (int i = 1; i < argc; i++)
  {
    std::string arg = argv[i];
    if (arg == "--data" && i + 1 < argc)
      data_dir = argv[++i];
    else if (arg == "--pref" && i + 1 < argc)
      pref_dir = argv[++i];
    else if (arg == "--headless")
      line += " headless=1 sos_none=1";
    else if (arg == "--quit-after" && i + 1 < argc)
      line += " quit_after=" + std::to_string((int)(atof(argv[++i]) * 1000));
    else
      line += " " + arg;
  }

  const fs::path data = data_dir ? fs::u8path(data_dir) : default_data_dir();
  std::error_code ec;
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
  return critical_error_occurred ? 1 : 0;
}
