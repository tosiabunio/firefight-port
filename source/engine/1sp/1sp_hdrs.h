#include <first.h>
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <conio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys\stat.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <ddraw.h>
#include <1lg.h>
#include <1mm.h>
#include <1io.h>

#define EXCLUDE_LIBS
#include "1sp.h"
#include "1sp_flic.h"
#undef EXCLUDE_LIBS

const int spr_file_level = 12;
const int pal_file_level = 1;
