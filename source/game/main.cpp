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
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

int WINAPI WinMain(HINSTANCE this_instance, HINSTANCE previous_instance, LPSTR command_line, int window_state);
extern int critical_error_occurred;

int main(int argc, char **argv)
{
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
