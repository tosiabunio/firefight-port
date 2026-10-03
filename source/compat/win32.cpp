// Definitions for the Win32 stand-ins declared in win32.h.
//
// Creation calls the engine cannot start without (window class, window, hooks, timer events,
// mutex) return inert dummy handles. DirectX is never loaded (LoadLibrary returns NULL), and
// MCI, the mixer, wave-out and joysticks report that no device is present. The registry is
// kept in memory for the run. The clocks, environment, temporary files and VirtualAlloc are real.

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
char dummy_window, dummy_dialog, dummy_hook, dummy_dc, dummy_palette, dummy_mutex;

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
BOOL RegisterClass(const WNDCLASS *) { return TRUE; }

HWND CreateWindow(LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID)
{
  return dummy_handle<HWND>(dummy_window);
}

HWND CreateWindowEx(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID)
{
  return dummy_handle<HWND>(dummy_window);
}

HWND CreateDialog(HINSTANCE, LPCSTR, HWND, DLGPROC) { return dummy_handle<HWND>(dummy_dialog); }
BOOL DestroyWindow(HWND) { return TRUE; }
BOOL ShowWindow(HWND, int) { return FALSE; }
BOOL SetForegroundWindow(HWND) { return TRUE; }
BOOL SetWindowPos(HWND, HWND, int, int, int, int, UINT) { return TRUE; }
LONG GetWindowLong(HWND, int) { return 0; }

int GetWindowText(HWND, LPSTR text, int max_count)
{
  if (max_count > 0)
    text[0] = 0;
  return 0;
}

HMENU GetMenu(HWND) { return nullptr; }
HWND  GetDlgItem(HWND, int) { return dummy_handle<HWND>(dummy_dialog); }

BOOL GetClientRect(HWND, LPRECT rect)
{
  rect->left = rect->top = 0;
  rect->right = 640;
  rect->bottom = 480;
  return TRUE;
}

BOOL ClientToScreen(HWND, POINT *) { return TRUE; }
BOOL AdjustWindowRectEx(LPRECT, DWORD, BOOL, DWORD) { return TRUE; }

BOOL SetRect(LPRECT rect, int left, int top, int right, int bottom)
{
  rect->left = left;
  rect->top = top;
  rect->right = right;
  rect->bottom = bottom;
  return TRUE;
}

BOOL InvalidateRect(HWND, const RECT *, BOOL) { return TRUE; }
int  GetSystemMetrics(int index) { return index == SM_CXSCREEN ? 640 : 480; }
HICON   LoadIcon(HINSTANCE, LPCSTR) { return nullptr; }
HCURSOR LoadCursor(HINSTANCE, LPCSTR) { return nullptr; }
int  ShowCursor(BOOL) { return 0; }
BOOL SetCursorPos(int, int) { return TRUE; }

BOOL    PostMessage(HWND, UINT, WPARAM, LPARAM) { return TRUE; }
LRESULT SendMessage(HWND, UINT, WPARAM, LPARAM) { return 0; }
void    PostQuitMessage(int) {}
LRESULT DefWindowProc(HWND, UINT, WPARAM, LPARAM) { return 0; }
HHOOK   SetWindowsHookEx(int, HOOKPROC, HINSTANCE, DWORD) { return dummy_handle<HHOOK>(dummy_hook); }
BOOL    UnhookWindowsHookEx(HHOOK) { return TRUE; }
LRESULT CallNextHookEx(HHOOK, int, WPARAM, LPARAM) { return 0; }
UINT    MapVirtualKey(UINT, UINT) { return 0; }
void    InitCommonControls() {}

// --- GDI ----------------------------------------------------------------------------------------
HDC      GetDC(HWND) { return dummy_handle<HDC>(dummy_dc); }
int      ReleaseDC(HWND, HDC) { return 1; }
HDC      BeginPaint(HWND, PAINTSTRUCT *paint) { memset(paint, 0, sizeof(*paint)); return dummy_handle<HDC>(dummy_dc); }
BOOL     EndPaint(HWND, const PAINTSTRUCT *) { return TRUE; }
HGDIOBJ  GetStockObject(int) { return nullptr; }
HGDIOBJ  SelectObject(HDC, HGDIOBJ) { return nullptr; }
BOOL     DeleteObject(HGDIOBJ) { return TRUE; }
BOOL     Rectangle(HDC, int, int, int, int) { return TRUE; }
int      GetDeviceCaps(HDC, int) { return 0; }

UINT GetSystemPaletteEntries(HDC, UINT, UINT count, LPPALETTEENTRY entries)
{
  if (entries)
    memset(entries, 0, count * sizeof(PALETTEENTRY));
  return count;
}

HPALETTE CreatePalette(const LOGPALETTE *) { return dummy_handle<HPALETTE>(dummy_palette); }
HPALETTE SelectPalette(HDC, HPALETTE, BOOL) { return nullptr; }
UINT     RealizePalette(HDC) { return 0; }
int      StretchDIBits(HDC, int, int, int, int h, int, int, int, int, const void *, const BITMAPINFO *, UINT, DWORD) { return h; }

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
