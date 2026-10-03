// Stand-ins for the Win32, DirectX 3 and multimedia APIs the original engine was written against.
//
// Phase 1 of the port (docs/porting-plan.md): the original code has to compile and link on every
// platform with no behaviour change, before the drivers are rewritten on SDL2 in phases 2-7.
// This header replaces <windows.h>, <windowsx.h>, <commctrl.h>, <mmsystem.h>, <ddraw.h>,
// <dsound.h> and <dplay.h> on all platforms, Windows included, so every build sees the same
// stubs. The definitions live in win32.cpp:
//   - window, GDI, DirectX, MCI, mixer, wave-out and joystick calls are inert and report
//     failure or "nothing there" (LoadLibrary returns NULL, so DirectX is never created);
//   - a few calls with an obvious portable meaning are implemented for real: the clocks
//     (GetTickCount, timeGetTime), GetUserName, SleepEx and VirtualAlloc.
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
FF_DECLARE_HANDLE(HHOOK);
FF_DECLARE_HANDLE(HMIXER);
FF_DECLARE_HANDLE(HMIXEROBJ);
FF_DECLARE_HANDLE(HWAVEOUT);
#undef FF_DECLARE_HANDLE
typedef HINSTANCE  HMODULE;
typedef void      *HGDIOBJ;
typedef HWAVEOUT  *LPHWAVEOUT;

typedef LRESULT (CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef BOOL    (CALLBACK *DLGPROC)(HWND, UINT, WPARAM, LPARAM);
typedef LRESULT (CALLBACK *HOOKPROC)(int, WPARAM, LPARAM);
typedef intptr_t (WINAPI *FARPROC)(void);

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

struct MOUSEHOOKSTRUCT
{
  POINT   pt;
  HWND    hwnd;
  UINT    wHitTestCode;
  uintptr_t dwExtraInfo;
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
#define WM_LBUTTONDOWN       0x0201
#define WM_LBUTTONUP         0x0202
#define WM_LBUTTONDBLCLK     0x0203
#define WM_RBUTTONDOWN       0x0204
#define WM_RBUTTONUP         0x0205
#define WM_RBUTTONDBLCLK     0x0206
#define WM_MBUTTONDOWN       0x0207
#define WM_MBUTTONUP         0x0208
#define WM_MBUTTONDBLCLK     0x0209
#define BN_CLICKED           0
#define WM_QUERYNEWPALETTE   0x030F
#define WM_PALETTECHANGED    0x0311
#define MM_MCINOTIFY         0x03B9
#define MM_MIXM_CONTROL_CHANGE 0x3D1

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
#define WH_KEYBOARD     2
#define WH_MOUSE        7
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

// --- multimedia: timer, wave out, mixer, joystick, MCI -------------------------------------------
#define MMSYSERR_NOERROR 0
#define JOYERR_NOERROR   0
#define CALLBACK_NULL    0x00000000L
#define CALLBACK_WINDOW  0x00010000L

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
struct WAVEHDR
{
  LPSTR     lpData;
  DWORD     dwBufferLength;
  DWORD     dwBytesRecorded;
  DWORD_PTR dwUser;
  DWORD     dwFlags;
  DWORD     dwLoops;
  WAVEHDR  *lpNext;
  DWORD_PTR reserved;
};
typedef WAVEHDR *LPWAVEHDR;
#define WAVE_FORMAT_PCM 1
#define WAVE_MAPPER     ((UINT)-1)

#define mmioFOURCC(c0, c1, c2, c3) \
  ((DWORD)(BYTE)(c0) | ((DWORD)(BYTE)(c1) << 8) | ((DWORD)(BYTE)(c2) << 16) | ((DWORD)(BYTE)(c3) << 24))
#define FOURCC_RIFF mmioFOURCC('R', 'I', 'F', 'F')

#define MIXER_SHORT_NAME_CHARS 16
#define MIXER_LONG_NAME_CHARS  64
#define MAXPNAMELEN            32
struct MIXERLINE
{
  DWORD     cbStruct;
  DWORD     dwDestination;
  DWORD     dwSource;
  DWORD     dwLineID;
  DWORD     fdwLine;
  DWORD_PTR dwUser;
  DWORD     dwComponentType;
  DWORD     cChannels;
  DWORD     cConnections;
  DWORD     cControls;
  CHAR      szShortName[MIXER_SHORT_NAME_CHARS];
  CHAR      szName[MIXER_LONG_NAME_CHARS];
  struct
  {
    DWORD dwType;
    DWORD dwDeviceID;
    WORD  wMid;
    WORD  wPid;
    UINT  vDriverVersion;
    CHAR  szPname[MAXPNAMELEN];
  } Target;
};
struct MIXERCONTROL
{
  DWORD cbStruct;
  DWORD dwControlID;
  DWORD dwControlType;
  DWORD fdwControl;
  DWORD cMultipleItems;
  CHAR  szShortName[MIXER_SHORT_NAME_CHARS];
  CHAR  szName[MIXER_LONG_NAME_CHARS];
  union
  {
    struct { LONG lMinimum, lMaximum; } s1;
    struct { DWORD dwMinimum, dwMaximum; } s2;
    DWORD dwReserved[6];
  } Bounds;
  union
  {
    DWORD cSteps;
    DWORD cbCustomData;
    DWORD dwReserved[6];
  } Metrics;
};
struct MIXERLINECONTROLS
{
  DWORD cbStruct;
  DWORD dwLineID;
  union { DWORD dwControlID; DWORD dwControlType; };
  DWORD cControls;
  DWORD cbmxctrl;
  MIXERCONTROL *pamxctrl;
};
struct MIXERCONTROLDETAILS
{
  DWORD cbStruct;
  DWORD dwControlID;
  DWORD cChannels;
  union { HWND hwndOwner; DWORD cMultipleItems; };
  DWORD cbDetails;
  LPVOID paDetails;
};
struct MIXERCONTROLDETAILS_UNSIGNED { DWORD dwValue; };
#define MIXER_OBJECTF_MIXER                     0x00000000L
#define MIXER_OBJECTF_HMIXER                    0x80000000L
#define MIXER_GETLINEINFOF_COMPONENTTYPE        0x00000003L
#define MIXER_GETLINECONTROLSF_ONEBYTYPE        0x00000002L
#define MIXER_GETCONTROLDETAILSF_VALUE          0x00000000L
#define MIXER_SETCONTROLDETAILSF_VALUE          0x00000000L
#define MIXERLINE_COMPONENTTYPE_SRC_FIRST       0x00001000L
#define MIXERLINE_COMPONENTTYPE_SRC_LINE        (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 2)
#define MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 5)
#define MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 7)
#define MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT     (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 8)
#define MIXERCONTROL_CONTROLTYPE_VOLUME         0x50030001L

#define MAXPNAMELEN_JOY 32
struct JOYCAPS
{
  WORD wMid, wPid;
  CHAR szPname[MAXPNAMELEN_JOY];
  UINT wXmin, wXmax, wYmin, wYmax, wZmin, wZmax;
  UINT wNumButtons, wPeriodMin, wPeriodMax;
  UINT wRmin, wRmax, wUmin, wUmax, wVmin, wVmax;
  UINT wCaps, wMaxAxes, wNumAxes, wMaxButtons;
  CHAR szRegKey[32];
  CHAR szOEMVxD[260];
};
struct JOYINFO { UINT wXpos, wYpos, wZpos, wButtons; };
struct JOYINFOEX
{
  DWORD dwSize, dwFlags;
  DWORD dwXpos, dwYpos, dwZpos, dwRpos, dwUpos, dwVpos;
  DWORD dwButtons, dwButtonNumber, dwPOV, dwReserved1, dwReserved2;
};
#define JOYSTICKID1        0
#define JOY_RETURNX        0x00000001L
#define JOY_RETURNY        0x00000002L
#define JOY_RETURNZ        0x00000004L
#define JOY_RETURNR        0x00000008L
#define JOY_RETURNU        0x00000010L
#define JOY_RETURNV        0x00000020L
#define JOY_RETURNBUTTONS  0x00000080L
#define JOY_RETURNCENTERED 0x00000400L
#define JOY_USEDEADZONE    0x00000800L

typedef UINT MCIDEVICEID;
typedef DWORD MCIERROR;
struct MCI_GENERIC_PARMS { DWORD_PTR dwCallback; };
struct MCI_OPEN_PARMS
{
  DWORD_PTR   dwCallback;
  MCIDEVICEID wDeviceID;
  LPCSTR      lpstrDeviceType;
  LPCSTR      lpstrElementName;
  LPCSTR      lpstrAlias;
};
struct MCI_PLAY_PARMS   { DWORD_PTR dwCallback; DWORD dwFrom; DWORD dwTo; };
struct MCI_SET_PARMS    { DWORD_PTR dwCallback; DWORD dwTimeFormat; DWORD dwAudio; };
struct MCI_STATUS_PARMS { DWORD_PTR dwCallback; DWORD_PTR dwReturn; DWORD dwItem; DWORD dwTrack; };
#define MCI_OPEN                     0x0803
#define MCI_CLOSE                    0x0804
#define MCI_PLAY                     0x0806
#define MCI_STOP                     0x0808
#define MCI_SET                      0x080D
#define MCI_STATUS                   0x0814
#define MCI_NOTIFY                   0x00000001L
#define MCI_NOTIFY_SUCCESSFUL        0x0001
#define MCI_WAIT                     0x00000002L
#define MCI_FROM                     0x00000004L
#define MCI_TO                       0x00000008L
#define MCI_TRACK                    0x00000010L
#define MCI_OPEN_TYPE                0x00002000L
#define MCI_OPEN_ELEMENT             0x00000200L
#define MCI_STATUS_ITEM              0x00000100L
#define MCI_STATUS_NUMBER_OF_TRACKS  0x00000003L
#define MCI_SET_TIME_FORMAT          0x00000400L
#define MCI_FORMAT_TMSF              10
#define MCI_CDA_STATUS_TYPE_TRACK    0x00004001L
#define MCI_CDA_TRACK_AUDIO          (0x00001100L + 0)
#define MCI_MAKE_TMSF(t, m, s, f) \
  ((DWORD)(((BYTE)(t) | ((WORD)(m) << 8)) | ((DWORD)(BYTE)(s) | ((WORD)(f) << 8)) << 16))

// --- DirectDraw (DirectX 3) ---------------------------------------------------------------------
struct IUnknown
{
  virtual HRESULT QueryInterface(REFIID riid, LPVOID *ppv) = 0;
  virtual ULONG   AddRef() = 0;
  virtual ULONG   Release() = 0;
};

struct DDSCAPS { DWORD dwCaps; };
struct DDPIXELFORMAT
{
  DWORD dwSize, dwFlags, dwFourCC, dwRGBBitCount;
  DWORD dwRBitMask, dwGBitMask, dwBBitMask, dwRGBAlphaBitMask;
};
struct DDCOLORKEY { DWORD dwColorSpaceLowValue, dwColorSpaceHighValue; };
struct DDSURFACEDESC
{
  DWORD         dwSize;
  DWORD         dwFlags;
  DWORD         dwHeight;
  DWORD         dwWidth;
  LONG          lPitch;
  DWORD         dwBackBufferCount;
  DWORD         dwRefreshRate;
  DWORD         dwAlphaBitDepth;
  DWORD         dwReserved;
  LPVOID        lpSurface;
  DDCOLORKEY    ddckCKDestOverlay;
  DDCOLORKEY    ddckCKDestBlt;
  DDCOLORKEY    ddckCKSrcOverlay;
  DDCOLORKEY    ddckCKSrcBlt;
  DDPIXELFORMAT ddpfPixelFormat;
  DDSCAPS       ddsCaps;
};
typedef DDSURFACEDESC *LPDDSURFACEDESC;
struct DDCAPS { DWORD dwSize, dwCaps, dwCaps2, dwCKeyCaps, dwFXCaps, dwFXAlphaCaps, dwPalCaps; };
#define DDENUMRET_CANCEL 0
#define DDENUMRET_OK     1
typedef HRESULT (CALLBACK *LPDDENUMMODESCALLBACK)(LPDDSURFACEDESC, LPVOID);

struct IDirectDrawPalette : IUnknown
{
  virtual HRESULT SetEntries(DWORD flags, DWORD start, DWORD count, LPPALETTEENTRY entries) = 0;
};
struct IDirectDrawSurface : IUnknown
{
  virtual HRESULT Flip(IDirectDrawSurface *target, DWORD flags) = 0;
  virtual HRESULT GetAttachedSurface(DDSCAPS *caps, IDirectDrawSurface **surface) = 0;
  virtual HRESULT GetCaps(DDSCAPS *caps) = 0;
  virtual HRESULT GetFlipStatus(DWORD flags) = 0;
  virtual HRESULT Lock(LPRECT rect, LPDDSURFACEDESC desc, DWORD flags, HANDLE event) = 0;
  virtual HRESULT Unlock(LPVOID surface) = 0;
  virtual HRESULT Restore() = 0;
  virtual HRESULT SetPalette(IDirectDrawPalette *palette) = 0;
};
struct IDirectDraw : IUnknown
{
  virtual HRESULT CreatePalette(DWORD flags, LPPALETTEENTRY entries, IDirectDrawPalette **palette, IUnknown *outer) = 0;
  virtual HRESULT CreateSurface(LPDDSURFACEDESC desc, IDirectDrawSurface **surface, IUnknown *outer) = 0;
  virtual HRESULT EnumDisplayModes(DWORD flags, LPDDSURFACEDESC desc, LPVOID context, LPDDENUMMODESCALLBACK callback) = 0;
  virtual HRESULT GetCaps(DDCAPS *driver_caps, DDCAPS *hel_caps) = 0;
  virtual HRESULT SetCooperativeLevel(HWND hwnd, DWORD flags) = 0;
  virtual HRESULT SetDisplayMode(DWORD width, DWORD height, DWORD bpp) = 0;
  virtual HRESULT WaitForVerticalBlank(DWORD flags, HANDLE event) = 0;
};
typedef IDirectDraw        *LPDIRECTDRAW;
typedef IDirectDrawSurface *LPDIRECTDRAWSURFACE;
typedef IDirectDrawPalette *LPDIRECTDRAWPALETTE;

#define DD_OK                    S_OK
#define FF_DDERR(code)           ((HRESULT)(0x88760000u | (code)))
#define DDERR_GENERIC            E_FAIL
#define DDERR_SURFACELOST        FF_DDERR(450)
#define DDERR_WASSTILLDRAWING    FF_DDERR(540)
#define DDSD_CAPS                0x00000001L
#define DDSD_BACKBUFFERCOUNT     0x00000020L
#define DDSCAPS_BACKBUFFER       0x00000004L
#define DDSCAPS_COMPLEX          0x00000008L
#define DDSCAPS_FLIP             0x00000010L
#define DDSCAPS_PRIMARYSURFACE   0x00000200L
#define DDSCAPS_MODEX            0x00200000L
#define DDSCL_FULLSCREEN         0x00000001L
#define DDSCL_ALLOWREBOOT        0x00000002L
#define DDSCL_EXCLUSIVE          0x00000010L
#define DDSCL_ALLOWMODEX         0x00000040L
#define DDPCAPS_8BIT             0x00000004L
#define DDPCAPS_ALLOW256         0x00000040L
#define DDLOCK_SURFACEMEMORYPTR  0x00000000L
#define DDLOCK_WAIT              0x00000001L
#define DDFLIP_WAIT              0x00000001L
#define DDGFS_ISFLIPDONE         0x00000002L
#define DDWAITVB_BLOCKBEGIN      0x00000001L
#define DDWAITVB_BLOCKEND        0x00000004L

// --- DirectSound --------------------------------------------------------------------------------
struct DSCAPS
{
  DWORD dwSize, dwFlags, dwMinSecondarySampleRate, dwMaxSecondarySampleRate;
  DWORD dwPrimaryBuffers, dwMaxHwMixingAllBuffers, dwMaxHwMixingStaticBuffers;
  DWORD dwMaxHwMixingStreamingBuffers, dwFreeHwMixingAllBuffers, dwFreeHwMixingStaticBuffers;
  DWORD dwFreeHwMixingStreamingBuffers, dwMaxHw3DAllBuffers, dwMaxHw3DStaticBuffers;
  DWORD dwMaxHw3DStreamingBuffers, dwFreeHw3DAllBuffers, dwFreeHw3DStaticBuffers;
  DWORD dwFreeHw3DStreamingBuffers, dwTotalHwMemBytes, dwFreeHwMemBytes;
  DWORD dwMaxContigFreeHwMemBytes, dwUnlockTransferRateHwBuffers;
  DWORD dwPlayCpuOverheadSwBuffers, dwReserved1, dwReserved2;
};
struct DSBUFFERDESC
{
  DWORD          dwSize;
  DWORD          dwFlags;
  DWORD          dwBufferBytes;
  DWORD          dwReserved;
  LPWAVEFORMATEX lpwfxFormat;
};
struct IDirectSoundBuffer : IUnknown
{
  virtual HRESULT GetStatus(LPDWORD status) = 0;
  virtual HRESULT Lock(DWORD offset, DWORD bytes, LPVOID *ptr1, LPDWORD bytes1, LPVOID *ptr2, LPDWORD bytes2, DWORD flags) = 0;
  virtual HRESULT Play(DWORD reserved1, DWORD reserved2, DWORD flags) = 0;
  virtual HRESULT SetCurrentPosition(DWORD position) = 0;
  virtual HRESULT SetFormat(LPWAVEFORMATEX format) = 0;
  virtual HRESULT SetVolume(LONG volume) = 0;
  virtual HRESULT SetPan(LONG pan) = 0;
  virtual HRESULT Stop() = 0;
  virtual HRESULT Unlock(LPVOID ptr1, DWORD bytes1, LPVOID ptr2, DWORD bytes2) = 0;
  virtual HRESULT Restore() = 0;
};
struct IDirectSound : IUnknown
{
  virtual HRESULT CreateSoundBuffer(DSBUFFERDESC *desc, IDirectSoundBuffer **buffer, IUnknown *outer) = 0;
  virtual HRESULT GetCaps(DSCAPS *caps) = 0;
  virtual HRESULT SetCooperativeLevel(HWND hwnd, DWORD level) = 0;
};
typedef IDirectSound       *LPDIRECTSOUND;
typedef IDirectSoundBuffer *LPDIRECTSOUNDBUFFER;

#define DS_OK                     S_OK
#define FF_DSERR(code)            ((HRESULT)(0x88780000u | (code)))
#define DSERR_OUTOFMEMORY         ((HRESULT)0x8007000EL)
#define DSERR_BUFFERLOST          FF_DSERR(150)
#define DSSCL_EXCLUSIVE           0x00000003
#define DSBPLAY_LOOPING           0x00000001
#define DSBSTATUS_PLAYING         0x00000001
#define DSBCAPS_PRIMARYBUFFER     0x00000001
#define DSBCAPS_STATIC            0x00000002
#define DSBCAPS_LOCSOFTWARE       0x00000008
#define DSBCAPS_CTRLFREQUENCY     0x00000020
#define DSBCAPS_CTRLPAN           0x00000040
#define DSBCAPS_CTRLVOLUME        0x00000080
#define DSCAPS_PRIMARYMONO        0x00000001
#define DSCAPS_PRIMARYSTEREO      0x00000002
#define DSCAPS_PRIMARY8BIT        0x00000004
#define DSCAPS_PRIMARY16BIT       0x00000008
#define DSCAPS_CONTINUOUSRATE     0x00000010
#define DSCAPS_EMULDRIVER         0x00000020
#define DSCAPS_CERTIFIED          0x00000040
#define DSCAPS_SECONDARYMONO      0x00000100
#define DSCAPS_SECONDARYSTEREO    0x00000200
#define DSCAPS_SECONDARY8BIT      0x00000400
#define DSCAPS_SECONDARY16BIT     0x00000800

// --- DirectPlay (DirectX 3) ---------------------------------------------------------------------
typedef DWORD DPID;
typedef DPID *LPDPID;
#define DPSHORTNAMELEN   20
#define DPLONGNAMELEN    52
#define DPSESSIONNAMELEN 32
#define DPPASSWORDLEN    16
#define DPUSERRESERVED   16
struct DPSESSIONDESC
{
  DWORD dwSize;
  GUID  guidSession;
  DWORD dwSession;
  DWORD dwMaxPlayers;
  DWORD dwCurrentPlayers;
  DWORD dwFlags;
  char  szSessionName[DPSESSIONNAMELEN];
  char  szUserField[DPUSERRESERVED];
  DWORD dwReserved1;
  char  szPassword[DPPASSWORDLEN];
  DWORD dwReserved2;
  DWORD dwUser1;
  DWORD dwUser2;
  DWORD dwUser3;
  DWORD dwUser4;
};
typedef DPSESSIONDESC *LPDPSESSIONDESC;
struct DPCAPS
{
  DWORD dwSize, dwFlags, dwMaxBufferSize, dwMaxQueueSize, dwMaxPlayers;
  DWORD dwHundredBaud, dwLatency;
};
typedef DPCAPS *LPDPCAPS;
struct DPMSG_GENERIC { DWORD dwType; };
struct DPMSG_ADDPLAYER
{
  DWORD dwType, dwPlayerType;
  DPID  dpId;
  char  szLongName[DPLONGNAMELEN];
  char  szShortName[DPSHORTNAMELEN];
  DWORD dwCurrentPlayers;
};
struct DPMSG_DELETEPLAYER { DWORD dwType; DPID dpId; };

typedef BOOL (CALLBACK *LPDPENUMDPCALLBACK)(LPGUID guid, LPSTR description, DWORD major, DWORD minor, LPVOID context);
typedef BOOL (CALLBACK *LPDPENUMSESSIONSCALLBACK)(LPDPSESSIONDESC desc, LPVOID context, LPDWORD timeout, DWORD flags);
typedef BOOL (CALLBACK *LPDPENUMPLAYERSCALLBACK)(DPID id, LPSTR friendly_name, LPSTR formal_name, DWORD flags, LPVOID context);

struct IDirectPlay : IUnknown
{
  virtual HRESULT Close() = 0;
  virtual HRESULT CreatePlayer(LPDPID id, LPSTR friendly_name, LPSTR formal_name, HANDLE *event) = 0;
  virtual HRESULT DestroyPlayer(DPID id) = 0;
  virtual HRESULT EnableNewPlayers(BOOL enable) = 0;
  virtual HRESULT EnumPlayers(DWORD session, LPDPENUMPLAYERSCALLBACK callback, LPVOID context, DWORD flags) = 0;
  virtual HRESULT EnumSessions(LPDPSESSIONDESC desc, DWORD timeout, LPDPENUMSESSIONSCALLBACK callback, LPVOID context, DWORD flags) = 0;
  virtual HRESULT GetCaps(LPDPCAPS caps) = 0;
  virtual HRESULT Open(LPDPSESSIONDESC desc) = 0;
  virtual HRESULT Receive(LPDPID from, LPDPID to, DWORD flags, LPVOID data, LPDWORD size) = 0;
  virtual HRESULT Send(DPID from, DPID to, DWORD flags, LPVOID data, DWORD size) = 0;
};
typedef IDirectPlay *LPDIRECTPLAY;

#define DP_OK                    S_OK
#define FF_DPERR(code)           ((HRESULT)(0x88770000u | (code)))
#define DPERR_BUFFERTOOSMALL     FF_DPERR(30)
#define DPERR_NOCONNECTION       FF_DPERR(100)
#define DPERR_NOMESSAGES         FF_DPERR(190)
#define DPERR_NOPLAYERS          FF_DPERR(210)
#define DPERR_NOSESSIONS         FF_DPERR(220)
#define DPERR_USERCANCEL         FF_DPERR(270)
#define DPOPEN_OPENSESSION       0x00000001
#define DPOPEN_CREATESESSION     0x00000002
#define DPSEND_TRYONCE           0x00000004
#define DPRECEIVE_TOPLAYER       0x00000002
#define DPENUMSESSIONS_AVAILABLE 0x00000001
#define DPESC_TIMEDOUT           0x00000001
#define DPCAPS_NAMESERVICE       0x00000001
#define DPCAPS_NAMESERVER        0x00000002
#define DPCAPS_GUARANTEED        0x00000004
#define DPSYS_ADDPLAYER          0x0003
#define DPSYS_DELETEPLAYER       0x0005
#define DPSYS_SESSIONLOST        0x0031

// --- functions (stubs, see win32.cpp) -----------------------------------------------------------
// kernel
DWORD   GetTickCount();
DWORD   GetLastError();
DWORD   GetCurrentThreadId();
UINT    GetDriveType(LPCSTR root);
BOOL    GetUserName(LPSTR buffer, LPDWORD size);
LPVOID  VirtualAlloc(LPVOID address, size_t size, DWORD type, DWORD protect);
BOOL    VirtualFree(LPVOID address, size_t size, DWORD type);
HMODULE LoadLibrary(LPCSTR name);
BOOL    FreeLibrary(HMODULE module);
FARPROC GetProcAddress(HMODULE module, LPCSTR name);
DWORD   WaitForSingleObject(HANDLE handle, DWORD milliseconds);
DWORD   SleepEx(DWORD milliseconds, BOOL alertable);
// windows and messages
BOOL    RegisterClass(const WNDCLASS *wc);
HWND    CreateWindow(LPCSTR class_name, LPCSTR window_name, DWORD style, int x, int y, int w, int h,
                     HWND parent, HMENU menu, HINSTANCE instance, LPVOID param);
HWND    CreateWindowEx(DWORD ex_style, LPCSTR class_name, LPCSTR window_name, DWORD style, int x, int y,
                       int w, int h, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param);
HWND    CreateDialog(HINSTANCE instance, LPCSTR templ, HWND parent, DLGPROC proc);
BOOL    DestroyWindow(HWND hwnd);
BOOL    ShowWindow(HWND hwnd, int cmd);
BOOL    SetForegroundWindow(HWND hwnd);
BOOL    SetWindowPos(HWND hwnd, HWND after, int x, int y, int cx, int cy, UINT flags);
LONG    GetWindowLong(HWND hwnd, int index);
int     GetWindowText(HWND hwnd, LPSTR text, int max_count);
HMENU   GetMenu(HWND hwnd);
HWND    GetDlgItem(HWND dialog, int id);
BOOL    GetClientRect(HWND hwnd, LPRECT rect);
BOOL    ClientToScreen(HWND hwnd, POINT *point);
BOOL    AdjustWindowRectEx(LPRECT rect, DWORD style, BOOL menu, DWORD ex_style);
BOOL    SetRect(LPRECT rect, int left, int top, int right, int bottom);
BOOL    InvalidateRect(HWND hwnd, const RECT *rect, BOOL erase);
int     GetSystemMetrics(int index);
HICON   LoadIcon(HINSTANCE instance, LPCSTR name);
HCURSOR LoadCursor(HINSTANCE instance, LPCSTR name);
int     ShowCursor(BOOL show);
BOOL    SetCursorPos(int x, int y);
BOOL    PostMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT SendMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
void    PostQuitMessage(int exit_code);
LRESULT DefWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
HHOOK   SetWindowsHookEx(int id, HOOKPROC proc, HINSTANCE instance, DWORD thread_id);
BOOL    UnhookWindowsHookEx(HHOOK hook);
LRESULT CallNextHookEx(HHOOK hook, int code, WPARAM wparam, LPARAM lparam);
UINT    MapVirtualKey(UINT code, UINT map_type);
void    InitCommonControls();
#define GetWindowStyle(hwnd)   ((DWORD)GetWindowLong(hwnd, GWL_STYLE))
#define GetWindowExStyle(hwnd) ((DWORD)GetWindowLong(hwnd, -20))
// GDI
HDC      GetDC(HWND hwnd);
int      ReleaseDC(HWND hwnd, HDC hdc);
HDC      BeginPaint(HWND hwnd, PAINTSTRUCT *paint);
BOOL     EndPaint(HWND hwnd, const PAINTSTRUCT *paint);
HGDIOBJ  GetStockObject(int index);
HGDIOBJ  SelectObject(HDC hdc, HGDIOBJ object);
BOOL     DeleteObject(HGDIOBJ object);
BOOL     Rectangle(HDC hdc, int left, int top, int right, int bottom);
int      GetDeviceCaps(HDC hdc, int index);
UINT     GetSystemPaletteEntries(HDC hdc, UINT start, UINT count, LPPALETTEENTRY entries);
HPALETTE CreatePalette(const LOGPALETTE *palette);
HPALETTE SelectPalette(HDC hdc, HPALETTE palette, BOOL background);
UINT     RealizePalette(HDC hdc);
int      StretchDIBits(HDC hdc, int x, int y, int w, int h, int src_x, int src_y, int src_w, int src_h,
                       const void *bits, const BITMAPINFO *info, UINT usage, DWORD rop);
// multimedia
DWORD    timeGetTime();
MMRESULT waveOutOpen(LPHWAVEOUT handle, UINT device, const WAVEFORMATEX *format, DWORD_PTR callback,
                     DWORD_PTR instance, DWORD flags);
MMRESULT waveOutClose(HWAVEOUT handle);
MMRESULT waveOutReset(HWAVEOUT handle);
MMRESULT waveOutPrepareHeader(HWAVEOUT handle, LPWAVEHDR header, UINT size);
MMRESULT waveOutUnprepareHeader(HWAVEOUT handle, LPWAVEHDR header, UINT size);
MMRESULT waveOutWrite(HWAVEOUT handle, LPWAVEHDR header, UINT size);
UINT     mixerGetNumDevs();
MMRESULT mixerOpen(HMIXER *mixer, UINT id, DWORD_PTR callback, DWORD_PTR instance, DWORD flags);
MMRESULT mixerClose(HMIXER mixer);
MMRESULT mixerGetLineInfo(HMIXEROBJ mixer, MIXERLINE *line, DWORD flags);
MMRESULT mixerGetLineControls(HMIXEROBJ mixer, MIXERLINECONTROLS *controls, DWORD flags);
MMRESULT mixerGetControlDetails(HMIXEROBJ mixer, MIXERCONTROLDETAILS *details, DWORD flags);
MMRESULT mixerSetControlDetails(HMIXEROBJ mixer, MIXERCONTROLDETAILS *details, DWORD flags);
UINT     joyGetNumDevs();
MMRESULT joyGetDevCaps(UINT id, JOYCAPS *caps, UINT size);
MMRESULT joyGetPos(UINT id, JOYINFO *info);
MMRESULT joyGetPosEx(UINT id, JOYINFOEX *info);
MCIERROR mciSendCommand(MCIDEVICEID id, UINT msg, DWORD_PTR flags, DWORD_PTR param);
BOOL     mciGetErrorString(MCIERROR error, LPSTR text, UINT size);

#endif
