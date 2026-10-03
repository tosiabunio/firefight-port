// C++ versions of the TASM routines in asm/ (1sp_aput.asm, 1sp_afhi.asm, 1sp_aflo.asm).
//
// Sprite definitions are run-length encoded rows. Each row starts with a 16-bit byte length
// (not counting the length itself); a non-empty row then has a 16-bit run count, and each run
// is a 16-bit skip and a 16-bit count. A positive count is followed by that many literal
// pixels, a non-positive count by one fill pixel. A row length of 0xFFFF ends the definition.
//
// _pconv, _ptouch, _pcsum and _fast_xlat are exact ports. _uniput (the 8-mode blitter) and the
// compiled font glyph routines are stubs until phase 3 of docs/porting-plan.md replaces them.

#include "1sp_hdrs.h"

static inline int getcount (unsigned char *&p)
{
  short v;
  memcpy(&v,p,sizeof(v));
  p+=2;
  return(v);
}

static inline int end_of_def (unsigned char *p)
{
  unsigned short v;
  memcpy(&v,p,sizeof(v));
  return(v==0xffff);
}

// Translates every pixel of a definition through t's table in place. Returns the summed
// row lengths.
extern "C" int _pconv (unsigned char *def, Tool &t)
{
  unsigned char *conv=(unsigned char*)t.conv;
  int size=0;
  unsigned char *p=def;
  do
  {
    unsigned char *row=p;
    if (getcount(p)!=0)
    {
      int runs=getcount(p);
      do
      {
        getcount(p);
        int count=getcount(p);
        if (count>0)
        {
          for (int i=0; i<count; i++)
            p[i]=conv[p[i]];
          p+=count;
        }
        else
        {
          *p=conv[*p];
          p++;
        }
      } while (--runs!=0);
    }
    p=row;
    int len=getcount(p);
    p+=len;
    size+=len;
  } while (!end_of_def(p));
  return(size);
}

// Read every byte of a definition so it is paged in. Only the size result is observable.
extern "C" int _ptouch (unsigned char *def)
{
  int size=0;
  unsigned char *p=def;
  do
  {
    int len=getcount(p);
    p+=len;
    size+=len;
  } while (!end_of_def(p));
  return(size);
}

// Checksum of a definition: every byte after each row's length word, plus the row lengths.
extern "C" int _pcsum (unsigned char *def)
{
  int csum=0;
  unsigned char *p=def;
  do
  {
    unsigned char *row=p;
    int len=getcount(p);
    for (int i=0; i<len; i++)
      csum+=*p++;
    p=row;
    len=getcount(p);
    p+=len;
    csum+=len;
  } while (!end_of_def(p));
  return(csum);
}

extern "C" void _fast_xlat (unsigned char *dst, unsigned char *tool, int len)
{
  for (int i=0; i<len; i++)
    dst[i]=tool[dst[i]];
}

// TODO(phase 3): port the blitter modes from 1sp_aput.asm.
extern "C" void _uniput (int doit, intptr_t uses)
{
  (void)doit;
  (void)uses;
}

// TODO(phase 3): the fonts were compiled into per-glyph code; convert them to bitmaps.
extern "C" char font_hi;
extern "C" char font_lo;
char font_hi;
char font_lo;

extern "C" void _font_put_char (void *font, int c, unsigned char *p, int llen, int color)
{
  (void)font;
  (void)c;
  (void)p;
  (void)llen;
  (void)color;
}
