#include "1sp_hdrs.h"

//----- komunikaty -----------------------------------------------------------

static char F_invtype[] = "invalid screen type %d, unable to %s";
static char F_invfunc[] = "invalid function flag 0x%08x, unable to %s";
static char F_nodef[]   = "unable to access sprite %s[%d%s] - %s data not loaded";

//----- stale i zmienne ------------------------------------------------------

unsigned char ccc_result[257];
extern "C" unsigned char *cc_result;
unsigned char *cc_result;

enum {UPsprite, UPshape, UPtool, UPshtool, UPshcopy, UPtr70, UPtr30, UPcoll};

extern "C" void _uniput (int doit, intptr_t uses);

struct RLE
{
  int x,y,sx,sy,mirror,eos;
  unsigned char *def;
  int scr_sx,scr_sy,scr_llen;
  unsigned char *scr_adr;
};

extern "C" RLE rle;
RLE rle;

static char *mirror_name[4]={"","h","v","hv"};

static int put_functions []=  {UPsprite,UPtool,UPtr30,UPtr70};
static int shape_functions []={UPshape,UPshtool,-1,-1,UPshcopy};

static inline int maxv (int a, int b) { return ((a)>(b))?(a):(b); }
static inline int minv (int a, int b) { return ((a)<(b))?(a):(b); }

//----- uniwersalny put sprite -----------------------------------------------

inline void uniput (Screen &scr, int doit, intptr_t uses, int x, int y, Sprite &s, int p, int m)
{
  Phase &phase=s[p];
  Phase::Type *t=NULL;
  switch(scr.type)
  {
  case Spr::collis:
    if (!phase.collis.def[m&1])
      FAILURE (F_nodef,s.name,p,mirror_name[m&3],"collis");
    t=&phase.collis;
    break;
  case Spr::lores:
    if (!phase.lores.def[m&1])
      FAILURE (F_nodef,s.name,p,mirror_name[m&3],"lores");
    t=&phase.lores;
    break;
  case Spr::hires:
    if (!phase.hires.def[m&1])
      FAILURE (F_nodef,s.name,p,mirror_name[m&3],"hires");
    t=&phase.hires;
    break;
  }

  rle.x=t->real_l+scr.convert(x)+scr.real_ox;
  rle.y=t->real_u+scr.convert(y)+scr.real_oy;
  rle.sx=t->real_r-t->real_l;
  rle.sy=t->real_d-t->real_u;
  rle.def=t->def[m&1];

  rle.mirror=m;
  if(Video::upside_down)
  {
    rle.y=scr.real_sy-rle.y-rle.sy;
    rle.mirror^=Spr::vmirror;
  }

  rle.eos=0;

  if (rle.x>=scr.real_sx) return;
  if (rle.x>=0) rle.eos++;
  if (rle.y>=scr.real_sy) return;
  if (rle.y>=0) rle.eos++;

  if (rle.x+rle.sx<=scr.real_sx) rle.eos++;
  if (rle.x+rle.sx<=0) return;
  if (rle.y+rle.sy<=scr.real_sy) rle.eos++;
  if (rle.y+rle.sy<=0) return;

  rle.scr_sx=scr.real_sx;
  rle.scr_sy=scr.real_sy;
  if((rle.scr_sx|rle.scr_sy)==0) return;

  rle.scr_llen=scr.real_llen;
  rle.scr_adr=scr.adr;

  _uniput(doit,uses); // modyfikuje pola rle!
}

void Screen::put (int x, int y, Sprite &s, int p, int function, void *with)
{
  int m=function&Spr::mirror_mask;
  int f=function>>4;
  if((f>sizeof(put_functions)/sizeof(put_functions[1]))||((f=put_functions[f])==-1))
    FAILURE(F_invfunc,function,"put sprite");
  uniput (*this, f, (intptr_t)with, x, y, s, p, m);
  if(debug_origins)
  {
    plot(Color::white,x,y);
    plot(Color::black,x+1,y);
    plot(Color::black,x-1,y);
    plot(Color::black,x,y+1);
    plot(Color::black,x,y-1);
  }
}

void Screen::shape (int x, int y, Sprite &s, int p, int function, void *with)
{
  int m=function&Spr::mirror_mask;
  int f=function>>4;
  if((f>sizeof(shape_functions)/sizeof(shape_functions[1]))||((f=shape_functions[f])==-1))
    FAILURE(F_invfunc,function,"put sprite shape");
  uniput (*this, f, (intptr_t)with, x, y, s, p, m);
  if(debug_origins)
  {
    plot(Color::white,x,y);
    plot(Color::black,x+1,y);
    plot(Color::black,x-1,y);
    plot(Color::black,x,y+1);
    plot(Color::black,x,y-1);
  }
}

unsigned char *Screen::check (int x, int y, Sprite &s, int p, int function, int mask)
{
  cc_result=ccc_result;
  *cc_result=0;

  int m=function&Spr::mirror_mask;
  uniput (*this, UPcoll, mask, x, y, s, p, m);

  return(ccc_result);
}

//----- operacje na ekranie --------------------------------------------------

void Screen::plot (int color, int x, int y)
{
  x=convert(x)+real_ox;
  y=Video::upside_down ?(real_sy-convert(y)-real_oy-convert(1)) :(convert(y)+real_oy);
  if ((x<0)||(x>=real_sx)||(y<0)||(y>=real_sy))
    return;
  if(type==Spr::hires)
  {
    short c=(short)(color|color<<8);
    unsigned char *dst=adr+y*real_llen+x;
    *(short*)(dst)=c;
    *(short*)(dst+real_llen)=c;
  }
  else
    adr[y*real_llen+x]=(unsigned char)color;
}

void Screen::plotnc (int color, int x, int y)
{
  x=convert(x)+real_ox;
  y=Video::upside_down ?(real_sy-convert(y)-real_oy-convert(1)) :(convert(y)+real_oy);
  DBG_CHECK((x>=0)&&(x<real_sx)&&(y>=0)||(y<real_sy));

  if(type==Spr::hires)
  {
    short c=(short)(color|color<<8);
    unsigned char *dst=adr+y*real_llen+x;
    *(short*)(dst)=c;
    *(short*)(dst+real_llen)=c;
  }
  else
    adr[y*real_llen+x]=(unsigned char)color;
}

int Screen::check (int x, int y, int mask)
{
  x=convert(x)+real_ox;
  y=Video::upside_down ?(real_sy-convert(y)-real_oy-1) :(convert(y)+real_oy);
  if ((x<0)||(x>=real_sx)||(y<0)||(y>=real_sy))
    return(0);
  return(adr[y*real_llen+x]&mask);
}

int Screen::checknc (int x, int y, int mask)
{
  x=convert(x)+real_ox;
  y=Video::upside_down ?(real_sy-convert(y)-real_oy-1) :(convert(y)+real_oy);
  DBG_CHECK((x>=0)&&(x<real_sx)&&(y>=0)||(y<real_sy));

  return(adr[y*real_llen+x]&mask);
}

void Screen::rectangle (int color,int x,int y,int w,int h)
{
  w=convert(w);
  h=convert(h);
  x=convert(x)+real_ox;
  y=Video::upside_down ?(real_sy-convert(y)-h-real_oy) :(convert(y)+real_oy);

  if(x<0)
  {
    w+=x;
    x=0;
  }
  if(y<0)
  {
    h+=y;
    y=0;
  }
  w=minv(real_sx-x,w);
  h=minv(real_sy-y,h);
  if ((w <= 0) || (h <= 0)) return;

  DBG_CHECK((x>=0)&&(y>=0)&&((x+w)<=real_sx)&&((y+h)<=real_sy));

  unsigned char *dst=adr+y*real_llen+x;
  for (int i=0; i<h; i++)
  {
    memset(dst,color,w);
    dst+=real_llen;
  }
}

extern "C" void _fast_xlat (unsigned char *dst,unsigned char *tool,int len);

void Screen::rectangle (Tool &t,int x,int y,int w,int h)
{
  w=convert(w);
  h=convert(h);
  x=convert(x)+real_ox;
  y=Video::upside_down ?(real_sy-convert(y)-h-real_oy) :(convert(y)+real_oy);

  if(x<0)
  {
    w+=x;
    x=0;
  }
  if(y<0)
  {
    h+=y;
    y=0;
  }
  w=minv(real_sx-x,w);
  h=minv(real_sy-y,h);
  if ((w <= 0) || (h <= 0)) return;

  DBG_CHECK((x>=0)&&(y>=0)&&((x+w)<=real_sx)&&((y+h)<=real_sy));

  unsigned char *dst=adr+y*real_llen+x;
  for (int i=0; i<h; i++)
  {
    _fast_xlat(dst,t.conv,w);
    dst+=real_llen;
  }
}

void Screen::copy (Screen &scr, int x, int y)
{
  x=scr.convert(x)+scr.real_ox;
  int l=maxv(0,x);
  int w=minv(real_sx,scr.real_sx-x);
  if(l>x) w-=(l-x);

  y=Video::upside_down ?(scr.real_sy-scr.convert(y)-scr.real_oy-real_sy) :scr.convert(y)+scr.real_oy;
  int u=maxv(0,y);
  int h=minv(real_sy,scr.real_sy-y);
  if(u>y) h-=(u-y);

  if ((w <= 0) || (h <= 0)) return;

  unsigned char *src=adr+(u-y)*real_llen+(l-x);
  unsigned char *dst=scr.adr+u*scr.real_llen+l;
  for (int i=0; i<h; i++)
  {
    memcpy(dst,src,w);  // was x86 asm: 8-byte blocks, then the remaining bytes
    src+=real_llen;
    dst+=scr.real_llen;
  }
}

void Screen::copy (int transparent, Screen &scr, int x, int y)
{
  x=scr.convert(x)+scr.real_ox;
  int l=maxv(0,x);
  int w=minv(real_sx,scr.real_sx-x);
  if(l>x) w-=(l-x);

  y=Video::upside_down ?(scr.real_sy-scr.convert(y)-scr.real_oy-real_sy) :scr.convert(y)+scr.real_oy;
  int u=maxv(0,y);
  int h=minv(real_sy,scr.real_sy-y);
  if(u>y) h-=(u-y);

  if ((w <= 0) || (h <= 0)) return;

  unsigned char *src=adr+(u-y)*real_llen+(l-x);
  unsigned char *dst=scr.adr+u*scr.real_llen+l;
  for (int i=0; i<h; i++)
  {
    for (int j=0; j<w; j++)
      if(src[j]!=transparent)
        dst[j]=src[j];
    src+=real_llen;
    dst+=scr.real_llen;
  }
}

void Screen::cls (int screen_color)
{
  if(screen_color==-1)
    screen_color=Color::black;
  unsigned char *dst=adr;
  for (int y=0; y<real_sy; y++)
  {
    memset(dst,screen_color,real_sx);
    dst+=real_llen;
  }
  crsx=crsy=0;
}

//----- wlasciwosci ekranu ---------------------------------------------------

int Screen::mem_required (int _llen, int _sy, int t)
{
  DBG_CHECK((t==Spr::hires)||(t==Spr::lores)||(t==Spr::collis));
  return(convert(_llen,t)*convert(_sy,t));
}

void Screen::origin (int _ox,int _oy)
{
  ox=_ox; oy=_oy;
  real_ox=convert(ox);
  real_oy=convert(oy);
}

static void xinit (Screen *s, int _sx, int _sy, int _llen, int _type,
  Screen *os=NULL, int _x=0, int _y=0)
{
  if (_type==0)
    if(os==NULL)
      _type=Video::mode_flag;
    else
      _type=os->type;

  if ((_type!=Spr::hires)&&(_type!=Spr::lores)&&(_type!=Spr::collis))
    FAILURE (F_invtype,_type,"initialize screen");
  s->type=_type;

  s->sx=_sx;
  s->sy=_sy;
  if(os==NULL)
    s->llen=_llen;
  else
    s->llen=s->unconvert(_llen,os->type);

  s->real_sx=s->convert(s->sx);
  s->real_sy=s->convert(s->sy);
  s->real_llen=s->convert(s->llen);

  s->crsx=s->crsy=0;
  s->origin(0,0);

  if(os==NULL)
    s->onscreen=NULL;
  else
  {
    s->onx=_x;
    s->ony=_y;
    s->onscreen=os;
    int y=Video::upside_down ?(os->real_sy-os->convert(s->ony)-s->real_sy) :os->convert(s->ony);
    s->adr=(os->adr)+(os->real_llen)*y+(os->convert(s->onx));
  }
  s->debug_origins=0;
}

void Screen::init (Screen &scr, int x, int y, int _sx, int _sy, int _type)
{
  xinit(this,_sx,_sy,scr.real_llen,_type,&scr,x,y);
}

void Screen::init (unsigned char *_adr,int _sx, int _sy, int _llen, int _type)
{
  xinit(this,_sx,_sy,_llen,_type);
  adr=_adr;
}

void Screen::reinit (int _type)
{
  int _ox=ox, _oy=oy;
  xinit(this,sx,sy,onscreen ?onscreen->real_llen :llen,_type,onscreen,onx,ony);
  origin(_ox,_oy);
}
