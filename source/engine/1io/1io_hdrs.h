#include <first.h>

#include <windows.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include <crtdbg.h>

#include <1lg.h>
#include <1mm.h>
#define EXCLUDE_LIBS
#include "1io.h"
#undef EXCLUDE_LIBS

void __compress(unsigned char* p_wrk_mem,unsigned char* p_src_first,unsigned src_len,unsigned char* p_dst_first,unsigned* p_dst_len);
void __decompress(unsigned char* wrk_mem,unsigned char* p_src_first,unsigned src_len,unsigned char* p_dst_first,unsigned* p_dst_len);
