// Port: on Windows the game is a GUI program, so starting it from Explorer opens no console
// window. Started from a terminal, it attaches to that terminal's console, so the log echo
// (stderr) still shows there, unless its output is already going somewhere (a pipe or a file,
// as in the tests). Kept apart from the game's headers: <windows.h> clashes with compat/win32.h.

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>

static bool handle_in_use(DWORD which)
{
  HANDLE handle = GetStdHandle(which);
  return handle != NULL && handle != INVALID_HANDLE_VALUE && GetFileType(handle) != FILE_TYPE_UNKNOWN;
}

void attach_parent_console()
{
  if (handle_in_use(STD_ERROR_HANDLE) || !AttachConsole(ATTACH_PARENT_PROCESS))
    return;
  if (!handle_in_use(STD_OUTPUT_HANDLE))
    freopen("CONOUT$", "w", stdout);
  freopen("CONOUT$", "w", stderr);
}

#endif
