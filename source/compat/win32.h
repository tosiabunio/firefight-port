// Stand-ins for the Win32, DirectX 3 and multimedia APIs the original engine was written against.
//
// Phase 1 of the port (docs/porting-plan.md) made the original code compile and link on every
// platform against these stand-ins; phases 2-7 rewrite the drivers on SDL2 and remove them.
// This header replaces <windows.h>, <windowsx.h>, <commctrl.h> and <mmsystem.h> on all
// platforms, Windows included, so every build sees the same stubs. Phase 3 removed the window,
// GDI and DirectDraw parts, phase 6 DirectSound, MCI, the mixer and wave-out, and phase 7
// DirectPlay (only its player id type, DPID, is left). What remains is types and constants, and
// a few calls with an obvious portable meaning, implemented for real in win32.cpp: the clocks
// (GetTickCount, timeGetTime), GetUserName, SleepEx and VirtualAlloc.
// Only what the original sources use is declared. Layouts follow the Win32 SDK where the code
// depends on them; integer types have their Win32 widths (DWORD and LONG are 32-bit everywhere).
// Each driver that is rewritten on SDL stops including this header; it goes away with the last.

#ifndef FF_COMPAT_WIN32_H
#define FF_COMPAT_WIN32_H

#include <stddef.h>
#include <stdint.h>

#include <compat/crt.h>

// --- calling conventions and decorations ----------------------------------------------------
#define WINAPI
#define CALLBACK
#define PASCAL
#define APIENTRY
#define FAR
#define NEAR
#ifndef STRICT
#define STRICT
#endif

// --- basic types ----------------------------------------------------------------------------
typedef int            BOOL;
typedef unsigned char  BYTE;
typedef uint16_t       WORD;
typedef uint32_t       DWORD;
typedef int32_t        LONG;
typedef uint32_t       ULONG;
typedef unsigned int   UINT;
typedef short          SHORT;
typedef char           CHAR;
typedef int32_t        HRESULT;
typedef uintptr_t      WPARAM;
typedef intptr_t       LPARAM;
typedef intptr_t       LRESULT;
typedef uintptr_t      DWORD_PTR;
typedef char          *LPSTR;
typedef const char    *LPCSTR;
typedef const char    *LPCTSTR;
typedef void          *LPVOID;
typedef BYTE          *LPBYTE;
typedef DWORD         *LPDWORD;
typedef LONG          *LPLONG;
typedef void          *HANDLE;
typedef UINT           MMRESULT;
typedef DWORD          FOURCC;

#define FF_DECLARE_HANDLE(name) typedef struct name##__ *name
FF_DECLARE_HANDLE(HWND);
FF_DECLARE_HANDLE(HINSTANCE);
FF_DECLARE_HANDLE(HDC);
FF_DECLARE_HANDLE(HICON);
FF_DECLARE_HANDLE(HCURSOR);
FF_DECLARE_HANDLE(HBRUSH);
FF_DECLARE_HANDLE(HPALETTE);
FF_DECLARE_HANDLE(HMENU);
FF_DECLARE_HANDLE(HKEY);
#undef FF_DECLARE_HANDLE
typedef HINSTANCE  HMODULE;
typedef void      *HGDIOBJ;

typedef LRESULT (CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef BOOL    (CALLBACK *DLGPROC)(HWND, UINT, WPARAM, LPARAM);

#define TRUE  1
#define FALSE 0

#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#define MAX_COMPUTERNAME_LENGTH 15

#define LOWORD(l) ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l) ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w) ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w) ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))
#define MAKELONG(a, b) ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define MAKELPARAM(l, h) ((LPARAM)(DWORD)MAKELONG(l, h))
#define MAKEINTRESOURCE(i) ((LPSTR)((uintptr_t)((WORD)(i))))

// --- HRESULT and error codes ------------------------------------------------------------------
#define S_OK      ((HRESULT)0)
#define E_FAIL    ((HRESULT)0x80004005L)
#define FAILED(hr)    (((HRESULT)(hr)) < 0)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define ERROR_SUCCESS 0L
#define ERROR_FILE_NOT_FOUND 2L

struct GUID
{
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t  Data4[8];
};
typedef GUID *LPGUID;
inline bool operator==(const GUID &a, const GUID &b) { return memcmp(&a, &b, sizeof(GUID)) == 0; }
inline bool operator!=(const GUID &a, const GUID &b) { return !(a == b); }
typedef const GUID &REFGUID;
typedef const GUID &REFIID;

// --- geometry, messages, windows --------------------------------------------------------------
struct RECT  { LONG left, top, right, bottom; };
struct POINT { LONG x, y; };
typedef RECT *LPRECT;

struct MSG
{
  HWND   hwnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
  DWORD  time;
  POINT  pt;
};

struct WNDCLASS
{
  UINT      style;
  WNDPROC   lpfnWndProc;
  int       cbClsExtra;
  int       cbWndExtra;
  HINSTANCE hInstance;
  HICON     hIcon;
  HCURSOR   hCursor;
  HBRUSH    hbrBackground;
  LPCSTR    lpszMenuName;
  LPCSTR    lpszClassName;
};

struct PAINTSTRUCT
{
  HDC  hdc;
  BOOL fErase;
  RECT rcPaint;
  BOOL fRestore;
  BOOL fIncUpdate;
  BYTE rgbReserved[32];
};

#define WM_CLOSE             0x0010
#define WM_PAINT             0x000F
#define WM_SETTEXT           0x000C
#define WM_ACTIVATEAPP       0x001C
#define WM_WINDOWPOSCHANGED  0x0047
#define WM_DISPLAYCHANGE     0x007E
#define WM_COMMAND           0x0111
#define WM_TIMER             0x0113
#define BN_CLICKED           0
#define WM_QUERYNEWPALETTE   0x030F
#define WM_PALETTECHANGED    0x0311

#define WS_OVERLAPPED   0x00000000L
#define WS_POPUP        0x80000000L
#define WS_THICKFRAME   0x00040000L
#define WS_EX_TOPMOST   0x00000008L
#define CW_USEDEFAULT   ((int)0x80000000)
#define GWL_STYLE       (-16)
#define GWL_ID          (-12)
#define SW_MINIMIZE     6
#define SW_RESTORE      9
#define SWP_NOMOVE      0x0002
#define SWP_NOZORDER    0x0004
#define SWP_NOACTIVATE  0x0010
#define HWND_DESKTOP    ((HWND)0)
#define HWND_TOPMOST    ((HWND)-1)
#define SM_CXSCREEN     0
#define SM_CYSCREEN     1
#define PM_REMOVE       0x0001
#define MB_OK           0x00000000L
#define MB_YESNOCANCEL  0x00000003L
#define MB_ICONQUESTION 0x00000020L
#define MB_ICONINFORMATION 0x00000040L
#define IDOK     1
#define IDCANCEL 2
#define IDYES    6
#define IDNO     7
#define MB_TASKMODAL    0x00002000L
#define IDI_WINLOGO     MAKEINTRESOURCE(32517)
#define IDC_ARROW       MAKEINTRESOURCE(32512)
#define BLACK_BRUSH     4
#define PBM_SETRANGE    (0x0400 + 1)
#define PBM_SETPOS      (0x0400 + 2)


// --- GDI ---------------------------------------------------------------------------------------
struct RGBQUAD      { BYTE rgbBlue, rgbGreen, rgbRed, rgbReserved; };
struct PALETTEENTRY { BYTE peRed, peGreen, peBlue, peFlags; };
typedef PALETTEENTRY *LPPALETTEENTRY;
struct LOGPALETTE
{
  WORD         palVersion;
  WORD         palNumEntries;
  PALETTEENTRY palPalEntry[1];
};
struct BITMAPINFOHEADER
{
  DWORD biSize;
  LONG  biWidth;
  LONG  biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG  biXPelsPerMeter;
  LONG  biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
};
struct BITMAPINFO
{
  BITMAPINFOHEADER bmiHeader;
  RGBQUAD          bmiColors[1];
};
#define BI_RGB         0L
#define DIB_RGB_COLORS 0
#define SRCCOPY        0x00CC0020L
#define PC_NOCOLLAPSE  0x04
#define NUMCOLORS      24

// --- system, memory, files ----------------------------------------------------------------------
struct MEMORYSTATUS
{
  DWORD dwLength;
  DWORD dwMemoryLoad;
  DWORD dwTotalPhys;
  DWORD dwAvailPhys;
  DWORD dwTotalPageFile;
  DWORD dwAvailPageFile;
  DWORD dwTotalVirtual;
  DWORD dwAvailVirtual;
};
#define MEM_COMMIT     0x1000
#define MEM_RESERVE    0x2000
#define MEM_DECOMMIT   0x4000
#define MEM_RELEASE    0x8000
#define PAGE_READWRITE 0x04
#define DRIVE_CDROM    5
#define SYNCHRONIZE    0x00100000L
#define WAIT_OBJECT_0  0x00000000L
#define WAIT_ABANDONED 0x00000080L
#define WAIT_TIMEOUT   0x00000102L
#define WAIT_FAILED    0xFFFFFFFFL
#define INFINITE       0xFFFFFFFF

// --- multimedia: timer, the sample format, the mixer lines RegData names ----------------------

struct WAVEFORMATEX
{
  WORD  wFormatTag;
  WORD  nChannels;
  DWORD nSamplesPerSec;
  DWORD nAvgBytesPerSec;
  WORD  nBlockAlign;
  WORD  wBitsPerSample;
  WORD  cbSize;
};
typedef WAVEFORMATEX *LPWAVEFORMATEX;
#define WAVE_FORMAT_PCM 1

#define mmioFOURCC(c0, c1, c2, c3) \
  ((DWORD)(BYTE)(c0) | ((DWORD)(BYTE)(c1) << 8) | ((DWORD)(BYTE)(c2) << 16) | ((DWORD)(BYTE)(c3) << 24))
#define FOURCC_RIFF mmioFOURCC('R', 'I', 'F', 'F')

#define MIXERLINE_COMPONENTTYPE_SRC_FIRST       0x00001000L
#define MIXERLINE_COMPONENTTYPE_SRC_LINE        (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 2)
#define MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 5)
#define MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 7)

// --- DirectPlay: the player id Net still uses --------------------------------------------------
typedef DWORD DPID;

// --- functions (stubs, see win32.cpp) -----------------------------------------------------------
// kernel
DWORD   GetTickCount();
DWORD   GetLastError();
DWORD   GetCurrentThreadId();
UINT    GetDriveType(LPCSTR root);
BOOL    GetUserName(LPSTR buffer, LPDWORD size);
LPVOID  VirtualAlloc(LPVOID address, size_t size, DWORD type, DWORD protect);
BOOL    VirtualFree(LPVOID address, size_t size, DWORD type);
DWORD   WaitForSingleObject(HANDLE handle, DWORD milliseconds);
DWORD   SleepEx(DWORD milliseconds, BOOL alertable);
// windows and messages
BOOL    DestroyWindow(HWND hwnd);
LONG    GetWindowLong(HWND hwnd, int index);
HICON   LoadIcon(HINSTANCE instance, LPCSTR name);
void    PostQuitMessage(int exit_code);
LRESULT DefWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
// multimedia
DWORD    timeGetTime();

#endif
