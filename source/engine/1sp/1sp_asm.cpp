// C++ versions of the TASM routines in asm/1sp_aput.asm (the fonts are in 1sp_font.cpp).
//
// Sprite definitions are run-length encoded rows. Each row starts with a 16-bit byte length
// (not counting the length itself); a non-empty row then has a 16-bit run count, and each run
// is a 16-bit skip and a 16-bit count. A positive count is followed by that many literal
// pixels, a non-positive count by one fill pixel. A row length of 0xFFFF ends the definition.
//
// _pconv, _ptouch, _pcsum, _fast_xlat and _uniput are exact ports.

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

//----- _uniput: the sprite blitter (1sp_aput.asm) ---------------------------------------------
//
// Draws (or, for collisions, scans) the RLE definition rle.def at rle.x,rle.y on the screen
// buffer rle.scr_adr. The original shares one row walker between eight modes that differ only in
// what they do with a run of literal pixels and with a run of one repeated pixel:
//
//   0 sprite   copy the pixels                         4 shcopy  copy the screen behind from `using`
//   1 shape    one colour, *(char*)using                5 tr70    dst = tsp[src*256+dst]
//   2 tool     dst = tool[src]                          6 tr30    dst = tsp[dst*256+src]
//   3 shtool   dst = tool[dst] under the sprite         7 coll    collect collision-screen values
//
// The walker below follows the register flow of the assembly, including its edge cases:
//   - only bit 1 of rle.mirror (vertical) is honoured; horizontal mirroring is ignored;
//   - an unclipped row ends at the 0xFFFF marker, a clipped one after rle.sy rows;
//   - a zero-length run is a fill in the unclipped walker but a literal in the clipped one, and
//     the tsp and collision modes visit pixel -1 for a zero-length unclipped fill (the encoder
//     never writes zero-length runs);
//   - the collision scan reads each run right to left and returns the values in reverse order of
//     discovery (they were pushed on the stack). This order feeds the collision messages.

struct RLE
{
  int x,y,sx,sy,mirror,eos;
  unsigned char *def;
  int scr_sx,scr_sy,scr_llen;
  unsigned char *scr_adr;
};
extern "C" RLE rle;
extern "C" unsigned char *cc_result;

namespace {

inline int getcount (const unsigned char *&p)
{
  short v;
  memcpy(&v,p,sizeof(v));
  p+=2;
  return(v);
}

inline bool at_end (const unsigned char *p)
{
  unsigned short v;
  memcpy(&v,p,sizeof(v));
  return(v==0xffff);
}

// Literal runs get n>0 pixels at src. Fill runs get the repeated pixel and a count n, which can be
// 0 only in the unclipped walker.
struct Op_sprite
{
  void literal (unsigned char *d, const unsigned char *s, int n) { memcpy(d,s,(size_t)n); }
  void fill (unsigned char *d, unsigned char c, int n) { if (n>0) memset(d,c,(size_t)n); }
};

struct Op_shape
{
  unsigned char color;
  void literal (unsigned char *d, const unsigned char *, int n) { memset(d,color,(size_t)n); }
  void fill (unsigned char *d, unsigned char, int n) { if (n>0) memset(d,color,(size_t)n); }
};

struct Op_tool
{
  const unsigned char *tool;
  void literal (unsigned char *d, const unsigned char *s, int n) { for (int i=0; i<n; i++) d[i]=tool[s[i]]; }
  void fill (unsigned char *d, unsigned char c, int n) { if (n>0) memset(d,tool[c],(size_t)n); }
};

struct Op_shtool
{
  const unsigned char *tool;
  void literal (unsigned char *d, const unsigned char *, int n) { for (int i=0; i<n; i++) d[i]=tool[d[i]]; }
  void fill (unsigned char *d, unsigned char, int n) { for (int i=0; i<n; i++) d[i]=tool[d[i]]; }
};

struct Op_shcopy
{
  ptrdiff_t offset;  // from the target screen to the screen being copied from
  void literal (unsigned char *d, const unsigned char *, int n) { for (int i=0; i<n; i++) d[i]=d[offset+i]; }
  void fill (unsigned char *d, unsigned char, int n) { for (int i=0; i<n; i++) d[i]=d[offset+i]; }
};

// The transparency and collision loops count down and test after the body (dec ecx / jns), so a
// zero count still visits index -1.
struct Op_tr70
{
  const unsigned char *tsp;
  void literal (unsigned char *d, const unsigned char *s, int n) { int i=n-1; do { d[i]=tsp[s[i]*256+d[i]]; } while (--i>=0); }
  void fill (unsigned char *d, unsigned char c, int n) { int i=n-1; do { d[i]=tsp[c*256+d[i]]; } while (--i>=0); }
};

struct Op_tr30
{
  const unsigned char *tsp;
  void literal (unsigned char *d, const unsigned char *s, int n) { int i=n-1; do { d[i]=tsp[d[i]*256+s[i]]; } while (--i>=0); }
  void fill (unsigned char *d, unsigned char c, int n) { int i=n-1; do { d[i]=tsp[d[i]*256+c]; } while (--i>=0); }
};

struct Op_coll
{
  unsigned char seen[256];   // cc_temp
  unsigned char stack[256];  // the values pushed, in discovery order
  int pushed;
  void scan (const unsigned char *d, int n)
  {
    int i=n-1;
    do
    {
      unsigned char v=d[i];
      if (seen[v]==0)
      {
        seen[v]=1;
        stack[pushed++]=v;
      }
    } while (--i>=0);
  }
  void literal (unsigned char *d, const unsigned char *, int n) { scan(d,n); }
  void fill (unsigned char *d, unsigned char, int n) { scan(d,n); }
};

template <class Op>
void walk (Op &op)
{
  const ptrdiff_t llen=rle.scr_llen;
  unsigned char *edi=rle.scr_adr+(ptrdiff_t)rle.y*llen+rle.x;
  const bool vmirror=(rle.mirror&2)!=0;
  if (vmirror)
    edi+=(ptrdiff_t)(rle.sy-1)*llen;
  const ptrdiff_t step=vmirror ?-llen :llen;
  const unsigned char *esi=rle.def;

  if (rle.eos==4)  // the whole sprite is on the screen
  {
    do
    {
      const unsigned char *s=esi;
      unsigned char *d=edi;
      if (getcount(s)!=0)
      {
        int blocks=getcount(s);
        do
        {
          d+=getcount(s);
          int n=getcount(s);
          if (n>0)
          {
            op.literal(d,s,n);
            s+=n;
            d+=n;
          }
          else
          {
            unsigned char c=*s++;
            n=-n;
            op.fill(d,c,n);
            d+=n;
          }
        } while (--blocks!=0);
      }
      int len=getcount(esi);
      esi+=len;
      edi+=step;
    } while (!at_end(esi));
    return;
  }

  // Clip at the top and bottom.
  if (!vmirror)
  {
    if (rle.y<0)
    {
      int rows=-rle.y;
      rle.sy-=rows;
      rle.y+=rows;
      do
      {
        int len=getcount(esi);
        esi+=len;
        edi+=llen;
      } while (--rows!=0);
    }
    int below=rle.y+rle.sy-rle.scr_sy;
    if (below>0)
      rle.sy-=below;
  }
  else
  {
    int below=rle.y+rle.sy-rle.scr_sy;
    if (below>0)
    {
      int rows=below;
      rle.sy-=rows;
      do
      {
        int len=getcount(esi);
        esi+=len;
        edi-=llen;
      } while (--rows!=0);
    }
    if (rle.y<0)
      rle.sy+=rle.y;
  }
  // Clip at the left (toskip = -pixels left of the screen) and at the right (todraw).
  int toskip=0;
  if (rle.x<0)
  {
    toskip=rle.x;
    rle.x=0;
  }
  const int todraw=rle.scr_sx-rle.x;

  do
  {
    const unsigned char *s=esi;
    unsigned char *d=edi;
    if (getcount(s)!=0)
    {
      int blocks=getcount(s);
      int edx=toskip;
      int eax=0,ecx=0;
      // Skip the runs left of the screen. Leaves eax = pixels still to skip and ecx = the run
      // count (+ literal, - fill, 0 nothing) of the first run that reaches the screen.
      do
      {
        ecx=getcount(s);
        edx+=ecx;
        if (edx>=0)
        {
          edx=-(edx-ecx);
          d+=edx;
          eax=ecx-edx;
          ecx=getcount(s);
          break;
        }
        d+=ecx;
        ecx=getcount(s);
        if (ecx>=0)
        {
          edx+=ecx;
          if (edx>=0)
          {
            edx=-(edx-ecx);
            d+=edx;
            s+=edx;
            ecx-=edx;
            eax=0;
            break;
          }
          s+=ecx;
        }
        else
        {
          ecx=-ecx;
          edx+=ecx;
          if (edx>=0)
          {
            edx=-(edx-ecx);
            d+=edx;
            ecx-=edx;
            if (ecx==0)
              s++;
            ecx=-ecx;
            eax=0;
            break;
          }
          s++;
        }
        d+=ecx;
      } while (--blocks!=0);

      if (blocks!=0)
      {
        edx=todraw;
        for (;;)
        {
          edx-=eax;
          if (edx<=0)
            break;
          d+=eax;
          if (ecx>0)
          {
            edx-=ecx;
            if (edx<=0)
            {
              op.literal(d,s,ecx+edx);
              break;
            }
            op.literal(d,s,ecx);
            s+=ecx;
            d+=ecx;
          }
          else if (ecx!=0)
          {
            unsigned char c=*s++;
            ecx=-ecx;
            edx-=ecx;
            if (edx<=0)
            {
              op.fill(d,c,ecx+edx);
              break;
            }
            op.fill(d,c,ecx);
            d+=ecx;
          }
          eax=getcount(s);
          ecx=getcount(s);
          if (--blocks==0)
            break;
        }
      }
    }
    int len=getcount(esi);
    esi+=len;
    edi+=step;
  } while (--rle.sy!=0);
}

} // namespace

extern "C" void _uniput (int doit, intptr_t uses)
{
  switch (doit)
  {
    case 0: { Op_sprite op; walk(op); break; }
    case 1: { Op_shape op={*(const unsigned char*)uses}; walk(op); break; }
    case 2: { Op_tool op={(const unsigned char*)uses}; walk(op); break; }
    case 3: { Op_shtool op={(const unsigned char*)uses}; walk(op); break; }
    case 4: { Op_shcopy op={(const unsigned char*)uses-rle.scr_adr}; walk(op); break; }
    case 5: { Op_tr70 op={(const unsigned char*)uses}; walk(op); break; }
    case 6: { Op_tr30 op={(const unsigned char*)uses}; walk(op); break; }
    case 7:
    {
      static Op_coll op;  // cc_temp is static in the original too; it is all zero between calls
      op.pushed=0;
      walk(op);
      unsigned char *out=cc_result;
      while (op.pushed>0)
      {
        unsigned char v=op.stack[--op.pushed];
        if (v&uses)
          *out++=v;
        op.seen[v]=0;
      }
      *out=0;
      break;
    }
  }
}
