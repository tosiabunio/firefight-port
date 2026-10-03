#ifndef _headers_H_INCLUDED
#define _headers_H_INCLUDED

#if HI_DEBUG
inline void *operator new(size_t)
{
  FAILURE("operator new(size_t size) - allocation with Windows Heap is not allowed.");
  return NULL;
}
#endif

#define _INC_MATH

#include <first.h>
#include <windows.h>
#include <windowsx.h>
#include <sys\stat.h>
#include <sys\types.h>
#include <share.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <conio.h>
#include <string.h>
#include <errno.h>
#include <iostream.h>
#include <fstream.h>
#ifndef _CWE_H_INCLUDED
#   include <1cw.h>
#endif
#include <1regdata.h>

class World;
class GameManager;
extern World *world;
extern GameManager *gamemanager;

#include "queue.h"
#include "nonet.h"
#include "game.h"
#include "kbdsta.h"
#include "params.h"
#include "list.h"
#include "view.h"
#include "build.h"
#include "world.h"
#include "gobj.h"
#include "debug.h"
#include "gman.h"
#include "msg.h"
#include "data.h"
#include "sysset.h"
#include "pilots.h"
#include "print.h"
#include "tools.h"
#include "menu.h"
#include "item.h"
#include "my.h"
#include "myweap.h"
#include "neutral.h"
#include "alien.h"
#include "alienwea.h"
#include "commctrl.h"
#include "resource.h"

class TerminateMission {};
class TerminateGame    {};

extern char *builders_mbn;
extern char *gobjects_mbn;
extern char *print_mbn;
extern char *menu_mbn;
extern char *queue_mbn;
extern char *list_mbn;
extern char *world_mbn;
extern char *gamemanager_mbn;
extern char *user_mbn;
extern char *item_mbn;
extern char *ship_mbn;
extern char *sound_mbn;

const int CONVERTED=-746584;

extern char *patch_path;

#endif

