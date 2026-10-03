#include <first.h>

#include <compat/win32.h>
#include <stdlib.h>
#include <compat/crt.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include <1lg.h>
#include <1mm.h>
#include "1io.h"

void __compress(unsigned char* p_wrk_mem,unsigned char* p_src_first,unsigned src_len,unsigned char* p_dst_first,unsigned* p_dst_len);
void __decompress(unsigned char* wrk_mem,unsigned char* p_src_first,unsigned src_len,unsigned char* p_dst_first,unsigned* p_dst_len);
