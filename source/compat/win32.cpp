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

std::string command_line = "firefight";

// Initialised on first use: the engine reads the clock from its own static constructors.
DWORD milliseconds_since_start()
{
  static const auto start_time = std::chrono::steady_clock::now();
  const auto elapsed = std::chrono::steady_clock::now() - start_time;
  return (DWORD)std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
}

DWORD copy_out(const std::string &value, LPSTR buffer, DWORD size)
{
  if (value.size() + 1 > size)
    return (DWORD)value.size() + 1;
  memcpy(buffer, value.c_str(), value.size() + 1);
  return (DWORD)value.size();
}

const MMRESULT MMSYSERR_INVALHANDLE = 5;
const MMRESULT MMSYSERR_NODRIVER = 6;
const MMRESULT JOYERR_UNPLUGGED = 167;
const MCIERROR MCIERR_DEVICE_NOT_INSTALLED = 256 + 50;

} // namespace

void ff_set_command_line(const char *line)
{
  command_line = line;
}

// --- kernel -----------------------------------------------------------------------------------
DWORD GetTickCount() { return milliseconds_since_start(); }
DWORD GetLastError() { return 0; }
DWORD GetVersion() { return 0x00000004; }  // reports Windows 4.0
DWORD GetCurrentThreadId() { return 1; }
LPSTR GetCommandLine() { return &command_line[0]; }

DWORD GetEnvironmentVariable(LPCSTR name, LPSTR buffer, DWORD size)
{
  const char *value = getenv(name);
  return value ? copy_out(value, buffer, size) : 0;
}

DWORD GetTempPath(DWORD size, LPSTR buffer)
{
  const char *dir = getenv("TMPDIR");
  if (!dir || !*dir)
    dir = getenv("TEMP");
  if (!dir || !*dir)
    dir = getenv("TMP");
  std::string path = (dir && *dir) ? dir : ".";
  if (path.back() != '/' && path.back() != '\\')
    path += '/';
  return copy_out(path, buffer, size);
}

UINT GetTempFileName(LPCSTR path, LPCSTR prefix, UINT unique, LPSTR name)
{
  static UINT counter = 0;
  for (int attempt = 0; attempt < 65536; attempt++)
  {
    const UINT number = unique ? unique : ((++counter + (UINT)milliseconds_since_start()) & 0xffff);
    snprintf(name, MAX_PATH, "%s%.3s%04X.tmp", path, prefix, number);
    if (unique)
      return number;
    const int fd = _open(name, _O_CREAT | _O_EXCL | _O_WRONLY | _O_BINARY, _S_IREAD | _S_IWRITE);
    if (fd != -1)
    {
      _close(fd);
      return number;
    }
  }
  return 0;
}

DWORD GetCurrentDirectory(DWORD size, LPSTR buffer)
{
  char cwd[4096];
  if (!getcwd(cwd, sizeof(cwd)))
    return 0;
  return copy_out(cwd, buffer, size);
}

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

void GlobalMemoryStatus(MEMORYSTATUS *status)
{
  memset(status, 0, sizeof(*status));
  status->dwLength = sizeof(*status);
  status->dwTotalPhys = status->dwTotalPageFile = status->dwTotalVirtual = 0x40000000;
  status->dwAvailPhys = status->dwAvailPageFile = status->dwAvailVirtual = 0x20000000;
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
HANDLE  CreateMutex(LPVOID, BOOL, LPCSTR) { return dummy_handle<HANDLE>(dummy_mutex); }
HANDLE  OpenMutex(DWORD, BOOL, LPCSTR) { return nullptr; }
BOOL    CloseHandle(HANDLE) { return TRUE; }
DWORD   WaitForSingleObject(HANDLE, DWORD) { return WAIT_OBJECT_0; }

DWORD SleepEx(DWORD milliseconds, BOOL)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
  return 0;
}

void OutputDebugString(LPCSTR text) { fputs(text, stderr); }

void DebugBreak()
{
#ifdef _MSC_VER
  __debugbreak();
#else
  raise(SIGTRAP);
#endif
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

int MessageBox(HWND, LPCSTR text, LPCSTR caption, UINT)
{
  fprintf(stderr, "[%s] %s\n", caption ? caption : "", text ? text : "");
  return IDOK;
}

BOOL    GetMessage(MSG *, HWND, UINT, UINT) { return FALSE; }
BOOL    PeekMessage(MSG *, HWND, UINT, UINT, UINT) { return FALSE; }
BOOL    TranslateMessage(const MSG *) { return FALSE; }
LRESULT DispatchMessage(const MSG *) { return 0; }
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

// --- registry: in memory, for the length of the run ---------------------------------------------
// Keys are case-insensitive paths below a root key. A value that was never written falls back to
// launcher_defaults: values the original launcher (LOADER.EXE) always wrote before starting the
// game and for which neither the code nor cwe.ini has a default. They match RegData's defaults.
// Phase 2 replaces this with a settings file.
namespace {

struct Reg_value
{
  DWORD type;
  std::string data;
};

struct Launcher_default
{
  const char *section;
  const char *name;
  DWORD value;
};

const Launcher_default launcher_defaults[] = {
  {"spr", "hires mode", 1},  // RegData::hires_Def (640x480)
  {"spr", "lores mode", 1},  // RegData::lores_Def (320x240)
};

std::vector<std::string> reg_keys;        // handle - 0x1000 indexes this
std::map<std::string, Reg_value> reg_values;  // "<key path>\<value name>", lowercase

std::string lower(std::string s)
{
  for (char &c : s)
    c = (char)tolower((unsigned char)c);
  return s;
}

std::string reg_path(HKEY key)
{
  const uintptr_t k = reinterpret_cast<uintptr_t>(key);
  if (k >= 0x1000 && k - 0x1000 < reg_keys.size())
    return reg_keys[k - 0x1000];
  return k == reinterpret_cast<uintptr_t>(HKEY_LOCAL_MACHINE) ? "hklm" : "hkcu";
}

HKEY reg_handle(const std::string &path)
{
  for (size_t i = 0; i < reg_keys.size(); i++)
    if (reg_keys[i] == path)
      return reinterpret_cast<HKEY>(0x1000 + i);
  reg_keys.push_back(path);
  return reinterpret_cast<HKEY>(0x1000 + reg_keys.size() - 1);
}

const Reg_value *reg_find(const std::string &path, const std::string &name)
{
  auto it = reg_values.find(path + "\\" + name);
  if (it != reg_values.end())
    return &it->second;
  static Reg_value fallback;
  const std::string section = path.substr(path.rfind('\\') + 1);
  for (const Launcher_default &d : launcher_defaults)
    if (section == d.section && name == d.name)
    {
      fallback.type = REG_DWORD;
      fallback.data.assign(reinterpret_cast<const char *>(&d.value), sizeof(d.value));
      return &fallback;
    }
  return nullptr;
}

const LONG ERROR_MORE_DATA = 234;

} // namespace

LONG RegOpenKeyEx(HKEY key, LPCSTR sub_key, DWORD, REGSAM, HKEY *result)
{
  *result = reg_handle(reg_path(key) + "\\" + lower(sub_key));
  return ERROR_SUCCESS;
}

LONG RegCreateKeyEx(HKEY key, LPCSTR sub_key, DWORD, LPSTR, DWORD, REGSAM, LPVOID, HKEY *result, LPDWORD)
{
  *result = reg_handle(reg_path(key) + "\\" + lower(sub_key));
  return ERROR_SUCCESS;
}

LONG RegCloseKey(HKEY) { return ERROR_SUCCESS; }

LONG RegDeleteKey(HKEY key, LPCSTR sub_key)
{
  const std::string prefix = reg_path(key) + "\\" + lower(sub_key) + "\\";
  for (auto it = reg_values.begin(); it != reg_values.end();)
    it = (it->first.compare(0, prefix.size(), prefix) == 0) ? reg_values.erase(it) : std::next(it);
  return ERROR_SUCCESS;
}

LONG RegDeleteValue(HKEY key, LPCSTR value_name)
{
  return reg_values.erase(reg_path(key) + "\\" + lower(value_name)) ? ERROR_SUCCESS : ERROR_FILE_NOT_FOUND;
}

LONG RegQueryValueEx(HKEY key, LPCSTR value_name, LPDWORD, LPDWORD type, LPBYTE data, LPDWORD size)
{
  const Reg_value *value = reg_find(reg_path(key), lower(value_name ? value_name : ""));
  if (!value)
    return ERROR_FILE_NOT_FOUND;
  if (type)
    *type = value->type;
  const DWORD needed = (DWORD)value->data.size();
  if (data)
  {
    if (!size || *size < needed)
    {
      if (size)
        *size = needed;
      return ERROR_MORE_DATA;
    }
    memcpy(data, value->data.data(), needed);
  }
  if (size)
    *size = needed;
  return ERROR_SUCCESS;
}

LONG RegSetValueEx(HKEY key, LPCSTR value_name, DWORD, DWORD type, const BYTE *data, DWORD size)
{
  Reg_value &value = reg_values[reg_path(key) + "\\" + lower(value_name ? value_name : "")];
  value.type = type;
  value.data.assign(reinterpret_cast<const char *>(data), size);
  return ERROR_SUCCESS;
}

// --- multimedia -----------------------------------------------------------------------------------
DWORD timeGetTime() { return milliseconds_since_start(); }

MMRESULT timeGetDevCaps(TIMECAPS *caps, UINT)
{
  caps->wPeriodMin = 1;
  caps->wPeriodMax = 1000000;
  return TIMERR_NOERROR;
}

MMRESULT timeBeginPeriod(UINT) { return TIMERR_NOERROR; }
MMRESULT timeEndPeriod(UINT) { return TIMERR_NOERROR; }

// Timer events are registered but never fire; phase 2 drives the clock from the main thread.
MMRESULT timeSetEvent(UINT, UINT, LPTIMECALLBACK, DWORD_PTR, UINT)
{
  static MMRESULT next_id = 0;
  return ++next_id;
}

MMRESULT timeKillEvent(UINT) { return TIMERR_NOERROR; }

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
