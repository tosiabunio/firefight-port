// Port entry point: hands the command line to the original WinMain (game.cpp).
//
//   firefight [--data <dir>] [--headless] [switch[=value] ...]
//
//   --data <dir>  directory holding cwe.ini and the *.dir manifests (default: the current one)
//   --headless    no window, video, sound or input devices (adds the engine switches
//                 headless=1 sos_none=1). In phase 1 the game stops after Game::init_all.
//
// Everything else is passed through as an original engine switch (debug=1, check, ...).
// The exit code is non-zero when the engine reported a critical error.

#include <compat/win32.h>

#include <string>

#ifdef _MSC_VER
#include <crtdbg.h>
#include <direct.h>
#include <stdlib.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

int WINAPI WinMain(HINSTANCE this_instance, HINSTANCE previous_instance, LPSTR command_line, int window_state);
extern int critical_error_occurred;

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

int main(int argc, char **argv)
{
#ifdef _MSC_VER
  use_msvc4_crt_behaviour();
#endif
  std::string line = std::string("\"") + argv[0] + "\"";
  std::string args;
  for (int i = 1; i < argc; i++)
  {
    std::string arg = argv[i];
    if (arg == "--data" && i + 1 < argc)
    {
      const char *dir = argv[++i];
      if (chdir(dir) != 0)
      {
        fprintf(stderr, "firefight: cannot change to data directory '%s'\n", dir);
        return 2;
      }
      continue;
    }
    if (arg == "--headless")
      arg = "headless=1 sos_none=1";
    args += (args.empty() ? "" : " ") + arg;
  }
  line += " " + args;
  ff_set_command_line(line.c_str());

  static char instance;
  WinMain(reinterpret_cast<HINSTANCE>(&instance), nullptr, &args[0], 1);
  return critical_error_occurred ? 1 : 0;
}
