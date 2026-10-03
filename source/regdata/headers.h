#include <first.h>

#include <stdio.h>
#include <compat/crt.h>

#ifndef _MFC_VER
#include <compat/win32.h>
#endif

#include <errno.h>

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

#include "1regdata.h"

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
