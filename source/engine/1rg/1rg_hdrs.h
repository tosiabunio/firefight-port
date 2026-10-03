#ifndef _HEADERS_H_INCLUDED
#define _HEADERS_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>
#include <first.h>
#include <windows.h>

#ifndef _1LG_H_INCLUDED
#include <1lg.h>
#endif

#ifndef _1MM_H_INCLUDED
#include <1mm.h>
#endif

#ifndef _1BA_H_INCLUDED
#include <1ba.h>
#endif

#ifndef _1RG_H_INCLUDED
#define EXCLUDE_LIBS
#  include "1rg.h"
#undef EXCLUDE_LIBS
#endif

#endif
