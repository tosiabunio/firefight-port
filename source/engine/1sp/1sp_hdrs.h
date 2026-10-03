#include <first.h>
#include <stdio.h>
#include <stdlib.h>
#include <compat/crt.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/stat.h>
#include <compat/win32.h>

#include <1lg.h>
#include <1mm.h>
#include <1io.h>

#include "1sp.h"
#include "1sp_flic.h"

const int spr_file_level = 12;
const int pal_file_level = 1;
