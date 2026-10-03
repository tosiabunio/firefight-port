#ifndef _HEADERS_H_INCLUDED
#define _HEADERS_H_INCLUDED

#pragma warning(disable : 4073)
#include <first.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <io.h>
#include <signal.h>
#include <process.h>

#ifndef _LOG_H_INCLUDED
#define EXCLUDE_LIBS
#  include "1lg.h"
#undef  EXCLUDE_LIBS
#endif

#endif