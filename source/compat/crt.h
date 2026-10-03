// Microsoft C runtime extensions used by the original sources.
//
// Replaces <io.h>, <dos.h>, <conio.h>, <share.h>, <process.h>, <crtdbg.h> and <malloc.h>.
// With MSVC the real headers are used. Elsewhere the low-level I/O maps onto POSIX file
// descriptors and the remaining functions are implemented in crt.cpp.

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

// Low-level I/O on file descriptors. There is no text mode, so _O_BINARY and _O_TEXT are 0.
#define _O_RDONLY O_RDONLY
#define _O_WRONLY O_WRONLY
#define _O_RDWR   O_RDWR
#define _O_APPEND O_APPEND
#define _O_CREAT  O_CREAT
#define _O_TRUNC  O_TRUNC
#define _O_EXCL   O_EXCL
#define _O_BINARY 0
#define _O_TEXT   0
#define O_BINARY  0
#define O_TEXT    0
#define _S_IREAD  (S_IRUSR | S_IRGRP | S_IROTH)
#define _S_IWRITE (S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH)
#ifndef S_IREAD
#define S_IREAD   _S_IREAD
#endif
#ifndef S_IWRITE
#define S_IWRITE  _S_IWRITE
#endif

int  _open(const char *name, int flags, ...);
int  _creat(const char *name, int mode);
inline int  _close(int fd) { return close(fd); }
inline int  _read(int fd, void *buffer, unsigned count) { return (int)read(fd, buffer, count); }
inline int  _write(int fd, const void *buffer, unsigned count) { return (int)write(fd, buffer, count); }
inline long _lseek(int fd, long offset, int origin) { return (long)lseek(fd, offset, origin); }
long _filelength(int fd);
int  _commit(int fd);

// Directory search. Handles are small non-negative integers, so they fit the `int`
// the original code stores them in; -1 means "not found".
struct _finddata_t
{
  unsigned attrib;
  time_t   time_create;
  time_t   time_access;
  time_t   time_write;
  uint32_t size;
  char     name[260];
};
intptr_t _findfirst(const char *spec, _finddata_t *data);
int      _findnext(intptr_t handle, _finddata_t *data);
int      _findclose(intptr_t handle);

// Processes.
intptr_t _execvp(const char *file, const char *const *argv);

#endif

#endif
