// Definitions for the Win32 stand-ins declared in win32.h.
//
// The input hooks return an inert dummy handle. DirectX is never loaded (LoadLibrary returns
// NULL), and MCI, the mixer, wave-out and joysticks report that no device is present. The clocks,
// GetUserName, SleepEx and VirtualAlloc (zero-filled) are real.

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

// Dummy objects whose addresses serve as handles.
char dummy_hook;

template <typename H>
H dummy_handle(char &object)
{
  return reinterpret_cast<H>(&object);
}

// Initialised on first use: the engine reads the clock from its own static constructors.
DWORD milliseconds_since_start()
{
  static const auto start_time = std::chrono::steady_clock::now();
  const auto elapsed = std::chrono::steady_clock::now() - start_time;
  return (DWORD)std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
}

const MMRESULT MMSYSERR_INVALHANDLE = 5;
const MMRESULT MMSYSERR_NODRIVER = 6;
const MMRESULT JOYERR_UNPLUGGED = 167;
const MCIERROR MCIERR_DEVICE_NOT_INSTALLED = 256 + 50;

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


BOOL GetClientRect(HWND, LPRECT rect)
{
  rect->left = rect->top = 0;
  rect->right = 640;
  rect->bottom = 480;
  return TRUE;
}

BOOL ClientToScreen(HWND, POINT *) { return TRUE; }

HICON   LoadIcon(HINSTANCE, LPCSTR) { return nullptr; }
int  ShowCursor(BOOL) { return 0; }
BOOL SetCursorPos(int, int) { return TRUE; }

void    PostQuitMessage(int) {}
LRESULT DefWindowProc(HWND, UINT, WPARAM, LPARAM) { return 0; }
HHOOK   SetWindowsHookEx(int, HOOKPROC, HINSTANCE, DWORD) { return dummy_handle<HHOOK>(dummy_hook); }
BOOL    UnhookWindowsHookEx(HHOOK) { return TRUE; }
LRESULT CallNextHookEx(HHOOK, int, WPARAM, LPARAM) { return 0; }
UINT    MapVirtualKey(UINT, UINT) { return 0; }

// --- GDI ----------------------------------------------------------------------------------------


// --- multimedia -----------------------------------------------------------------------------------
DWORD timeGetTime() { return milliseconds_since_start(); }



MMRESULT waveOutOpen(LPHWAVEOUT handle, UINT, const WAVEFORMATEX *, DWORD_PTR, DWORD_PTR, DWORD)
{
  if (handle)
    *handle = nullptr;
  return MMSYSERR_NODRIVER;
}
MMRESULT waveOutClose(HWAVEOUT) { return MMSYSERR_INVALHANDLE; }
MMRESULT waveOutReset(HWAVEOUT) { return MMSYSERR_INVALHANDLE; }
MMRESULT waveOutPrepareHeader(HWAVEOUT, LPWAVEHDR, UINT) { return MMSYSERR_INVALHANDLE; }
MMRESULT waveOutUnprepareHeader(HWAVEOUT, LPWAVEHDR, UINT) { return MMSYSERR_INVALHANDLE; }
MMRESULT waveOutWrite(HWAVEOUT, LPWAVEHDR, UINT) { return MMSYSERR_INVALHANDLE; }

UINT     mixerGetNumDevs() { return 0; }
MMRESULT mixerOpen(HMIXER *mixer, UINT, DWORD_PTR, DWORD_PTR, DWORD)
{
  if (mixer)
    *mixer = nullptr;
  return MMSYSERR_NODRIVER;
}
MMRESULT mixerClose(HMIXER) { return MMSYSERR_INVALHANDLE; }
MMRESULT mixerGetLineInfo(HMIXEROBJ, MIXERLINE *, DWORD) { return MMSYSERR_INVALHANDLE; }
MMRESULT mixerGetLineControls(HMIXEROBJ, MIXERLINECONTROLS *, DWORD) { return MMSYSERR_INVALHANDLE; }
MMRESULT mixerGetControlDetails(HMIXEROBJ, MIXERCONTROLDETAILS *, DWORD) { return MMSYSERR_INVALHANDLE; }
MMRESULT mixerSetControlDetails(HMIXEROBJ, MIXERCONTROLDETAILS *, DWORD) { return MMSYSERR_INVALHANDLE; }

UINT     joyGetNumDevs() { return 0; }
MMRESULT joyGetDevCaps(UINT, JOYCAPS *, UINT) { return JOYERR_UNPLUGGED; }
MMRESULT joyGetPos(UINT, JOYINFO *) { return JOYERR_UNPLUGGED; }
MMRESULT joyGetPosEx(UINT, JOYINFOEX *) { return JOYERR_UNPLUGGED; }

MCIERROR mciSendCommand(MCIDEVICEID, UINT, DWORD_PTR, DWORD_PTR) { return MCIERR_DEVICE_NOT_INSTALLED; }

BOOL mciGetErrorString(MCIERROR, LPSTR text, UINT size)
{
  if (size)
    snprintf(text, size, "MCI is not available in this build");
  return TRUE;
}
