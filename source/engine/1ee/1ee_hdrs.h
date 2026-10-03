#ifndef _HEADERS_H_INCLUDED
#define _HEADERS_H_INCLUDED

#include <first.h>
#include <crtdbg.h>
#include <stdio.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/stat.h>

#ifndef _MFC_VER
#include <windows.h>
#endif

#ifndef _1LG_H_INCLUDED
#  include <1lg.h>
#endif

#ifndef _1MM_H_INCLUDED
#  include <1mm.h>
#endif

#ifndef _1BA_H_INCLUDED
#  include <1ba.h>
#endif

#ifndef _1RG_H_INCLUDED
#  include <1rg.h>
#endif

#ifndef _1IO_H_INCLUDED
#  include <1io.h>
#endif

#ifndef _1EE_DEMO_INCLUDED
#  include "1ee_demo.h"
#endif 

#ifndef _1EE_H_INCLUDED
#define EXCLUDE_LIBS
#  include "1ee.h"
#undef EXCLUDE_LIBS
#endif

#endif
