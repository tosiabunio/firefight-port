// Definitions for the Win32 stand-ins declared in win32.h.
//
// DirectX is never loaded (LoadLibrary returns NULL). The clocks, GetUserName, SleepEx and
// VirtualAlloc (zero-filled) are real.

#include <compat/win32.h>

#include <chrono>
#include <iterator>
#include <map>
#include <csignal>
#include <string>
#include <thread>
#include <vector>

#ifdef _MSC_VER
#include <direct.h>
#include <intrin.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

namespace {

// Initialised on first use: the engine reads the clock from its own static constructors.
DWORD milliseconds_since_start()
{
  static const auto start_time = std::chrono::steady_clock::now();
  const auto elapsed = std::chrono::steady_clock::now() - start_time;
  return (DWORD)std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
}

} // namespace

// --- kernel -----------------------------------------------------------------------------------
DWORD GetTickCount() { return milliseconds_since_start(); }
DWORD GetLastError() { return 0; }
DWORD GetCurrentThreadId() { return 1; }
UINT GetDriveType(LPCSTR) { return 3; }  // DRIVE_FIXED

BOOL GetUserName(LPSTR buffer, LPDWORD size)
{
  const char *user = getenv("USER");
  if (!user)
    user = getenv("USERNAME");
  const std::string value = user ? user : "player";
  if (value.size() + 1 > *size)
    return FALSE;
  memcpy(buffer, value.c_str(), value.size() + 1);
  *size = (DWORD)value.size() + 1;
  return TRUE;
}

// Committed memory is zero-filled, as on Windows. A reservation is backed straight away, so a
// later commit of the same range just returns it.
LPVOID VirtualAlloc(LPVOID address, size_t size, DWORD type, DWORD)
{
  if (address)
    return (type & MEM_COMMIT) ? address : nullptr;
  return calloc(1, size ? size : 1);
}

BOOL VirtualFree(LPVOID address, size_t, DWORD type)
{
  if (type & MEM_RELEASE)
    free(address);
  return TRUE;
}

HMODULE LoadLibrary(LPCSTR) { return nullptr; }
BOOL    FreeLibrary(HMODULE) { return TRUE; }
FARPROC GetProcAddress(HMODULE, LPCSTR) { return nullptr; }
DWORD   WaitForSingleObject(HANDLE, DWORD) { return WAIT_OBJECT_0; }

DWORD SleepEx(DWORD milliseconds, BOOL)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
  return 0;
}


// --- windows and messages -----------------------------------------------------------------------

BOOL DestroyWindow(HWND) { return TRUE; }
LONG GetWindowLong(HWND, int) { return 0; }


HICON   LoadIcon(HINSTANCE, LPCSTR) { return nullptr; }

void    PostQuitMessage(int) {}
LRESULT DefWindowProc(HWND, UINT, WPARAM, LPARAM) { return 0; }

// --- GDI ----------------------------------------------------------------------------------------


// --- multimedia -----------------------------------------------------------------------------------
DWORD timeGetTime() { return milliseconds_since_start(); }
