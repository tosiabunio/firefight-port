#include <first.h>

#include <stdio.h>
#include <io.h>

#ifndef _MFC_VER
#include <windows.h>
#endif

#include <process.h>
#include <errno.h>
#include <dplay.h>

#include <1lg.h>
#include <1mm.h>
#include <1ba.h>
#include <1rg.h>
#include <1io.h>
#include <1ee.h>
#include <1sp.h>
#include <1ss.h>
#include <1cw.h>

#ifdef _MFC_VER
#include "resource.h"		
#endif

#define EXCLUDE_LIBS
#include "1regdata.h"
#undef EXCLUDE_LIBS

#ifdef _MFC_VER
#include "tools.h"
#include "rtfinfo.h"
#include "errordlg.h"
#include "loader.h"
#include "loaderDl.h"
#include "wiz_net.h"
#include "propmore.h"
#endif

#pragma comment(lib,"winmm.lib")

#define TRY_ON try {
#define TRY_OFF } catch(Failure)                  \
                  {                               \
                    AfxMessageBox("   Critical error ocured.\n"    \
                                  "   See log file for details.   "); \
                    PostQuitMessage(-1);          \
                  }                               \
                  catch(Closed)                   \
                  {                               \
                    PostQuitMessage(-1);          \
                  }
