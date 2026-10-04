// Microsoft C runtime extensions used by the original sources.
//
// Replaces <io.h>, <dos.h>, <conio.h>, <share.h>, <process.h>, <crtdbg.h> and <malloc.h>.
// With MSVC the real headers are used. Elsewhere the string and path functions the original
// code still calls are implemented in crt.cpp. (File I/O moved to stdio in phase 2.)

#ifndef FF_COMPAT_CRT_H
#define FF_COMPAT_CRT_H

#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#ifdef _MSC_VER

#include <conio.h>
#include <crtdbg.h>
#include <direct.h>
#include <io.h>
#include <malloc.h>
#include <process.h>
#include <share.h>

#else

#include <unistd.h>

#define _MAX_PATH  260
#define _MAX_DRIVE 3
#define _MAX_DIR   256
#define _MAX_FNAME 256
#define _MAX_EXT   256

// String functions.
int   strcmpi(const char *a, const char *b);
int   stricmp(const char *a, const char *b);
int   strnicmp(const char *a, const char *b, size_t n);
char *strupr(char *s);
char *strlwr(char *s);
inline char *_strupr(char *s) { return strupr(s); }
inline char *_strlwr(char *s) { return strlwr(s); }
#define _vsnprintf vsnprintf
#define _snprintf  snprintf

// Path splitting. Accepts both '\' and '/' as separators; there are never drive letters.
void _splitpath(const char *path, char *drive, char *dir, char *fname, char *ext);

#endif

#include <compat/msvc4.h>

#endif
