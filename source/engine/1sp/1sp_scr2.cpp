#include "1sp_hdrs.h"

//----- komunikaty -----------------------------------------------------------

static char F_only256[] = "unable to init cmanager '%s' for more than 256 objects";
static char F_maxman[]  = "cmanager '%s' error: buffer[%d] overflow";

//----- stale i zmienne ------------------------------------------------------

extern "C" char font_hi;
extern "C" char font_lo;

//----- manager kolizji ------------------------------------------------------

Cmanager::Cmanager(char *class_name)
{
  name=class_name;
  buf=NULL;
  pbuf=NULL;
  max=0;
}

Cmanager::~Cmanager (void)
{
  free();
}

void Cmanager::free (void)
{
  if (max)
  {
    MESSAGE("%s buffer: size %d, max used %d (%d%%)",
      name,max,max_written,(max_written*100)/max);
    if (buf) Heap::free(buf,FILE_LINE);
    if (pbuf) Heap::free(pbuf,FILE_LINE);
    buf=NULL;
    pbuf=NULL;
  }
}

void Cmanager::init(Screen &cworld, int max_objects, char *class_name)
{
  if (max_objects>256)
    FAILURE(F_only256,class_name);
  world=&cworld;
  max=max_objects;
  num=max_written=0;
  buf=(void**)Heap::alloc(max*sizeof(void*),"[spr] cmanager buf");
  pbuf=(unsigned char*)Heap::alloc(max,"[spr] cmanager pbuf");
  overflow_flag=0;
  name=class_name;
  reset();
}

void Cmanager::origin (int ox, int oy)
{
  world->origin(ox,oy);
}

void Cmanager::reset(void)
{
  world->cls(0);
  if (num>max_written)
    max_written=num;
#if HI_DEBUG
  if ((!overflow_flag)&&(((max_written*100)/max)>95))
  {
    WARNING("%s buffer is almost full",name);
    overflow_flag=1;
  }
#endif
  num=0;
}

void Cmanager::write (int x, int y, void* object, int priority)
{
  if (num+1>max)
    FAILURE (F_maxman,name,max);
  buf[num]=object;
  pbuf[num]=(unsigned char)priority;
  world->plot(num+1,x,y);
  num++;
}

void Cmanager::write (int x, int y, Sprite &s, int p, int m, void* object, int priority)
{
  if (num+1>max)
    FAILURE (F_maxman,name,max);
  buf[num]=object;
  pbuf[num]=(unsigned char)priority;
  num++;
  world->shape(x,y,s,p,m,(void*)&num);
}

void* Cmanager::check (int x, int y)
{
  unsigned char check_result=(unsigned char)world->check(x,y);
  return(check_result ?buf[check_result-1] :NULL);
}

static const maxresult=256;
static void *cm_result[maxresult+1];
static unsigned char *cm_pbuf;

static int qsort_compare (void const *a, void const *b)
{
  return (cm_pbuf[(*(unsigned char*)b)-1]-cm_pbuf[(*(unsigned char*)a)-1]);
}

void** Cmanager::check (int x, int y, Sprite &s, int p, int m)
{
  unsigned char *check_result=world->check(x,y,s,p,m);
  for (int r=0; check_result[r]; r++);
  cm_pbuf=pbuf;
  qsort(check_result,r,sizeof(char),qsort_compare);

  for (int i=0; i<r; i++)
    cm_result[i]=buf[check_result[i]-1];
  cm_result[i]=NULL;
  return(cm_result);
}

//----- print na ekranie -----------------------------------------------------

static void put_char (void *f, int c, unsigned char *p, int llen, int color)
{
  __asm
  {
    mov esi,[f]
    mov ebx,[c]
    mov edi,[p]
    mov edx,[llen]
    mov eax,[color]
    push edi
    call [esi+ebx*4]
    pop edi
  }
}

static int color_name (unsigned char *&c, char *name)
{
  unsigned char *csav=c;
  while (tolower(*c)==tolower(*name))
  {
    c++;
    name++;
    if (*name==0)
      return(1);
  }
  c=csav;
  return(0);
}

void Screen::putstr (int x, int y, unsigned char *c)
{
  int ink_sav=ink;
  void *font=&font_lo;
  if (convert(1)>1)
    font=&font_hi;
  int col=ink | (ink<<8) | (ink<<16) | (ink<<24);

  int stepx=convert(Spr::font_sx);
  x+=ox;
  y+=oy;
  int ll=Video::upside_down ?(-real_llen) :(real_llen);

  unsigned char *p=adr+(Video::upside_down ?(real_sy-convert(y)-1) :convert(y))*real_llen+convert(x);

  while (*c)
  {
    if (*c==0x0a)
    {
      x=ox;
      y+=Spr::font_sy;
      p=adr+(Video::upside_down ?(real_sy-convert(y)-1) :convert(y))*real_llen+convert(x);
      c++;
    }
    else if (*c=='<')
    {
           if (color_name(c,"<<"))         { c--; goto oops; }
      else if (color_name(c,"<black>"))    ink=Color::black;
      else if (color_name(c,"<dwhite>"))   ink=Color::dwhite;
      else if (color_name(c,"<white>"))    ink=Color::white;
      else if (color_name(c,"<dred>"))     ink=Color::dred;
      else if (color_name(c,"<red>"))      ink=Color::red;
      else if (color_name(c,"<dgreen>"))   ink=Color::dgreen;
      else if (color_name(c,"<green>"))    ink=Color::green;
      else if (color_name(c,"<dblue>"))    ink=Color::dblue;
      else if (color_name(c,"<blue>"))     ink=Color::blue;
      else if (color_name(c,"<dyellow>"))  ink=Color::dyellow;
      else if (color_name(c,"<yellow>"))   ink=Color::yellow;
      else if (color_name(c,"<dcyan>"))    ink=Color::dcyan;
      else if (color_name(c,"<cyan>"))     ink=Color::cyan;
      else if (color_name(c,"<dviolet>"))  ink=Color::dviolet;
      else if (color_name(c,"<violet>"))   ink=Color::violet;

      else if (color_name(c,"<normal>"))   ink=Color::normal;
      else if (color_name(c,"<n>"))        ink=Color::normal;
      else if (color_name(c,"<lighten>"))  ink=Color::lighten;
      else if (color_name(c,"<l>"))        ink=Color::lighten;
      else if (color_name(c,"<darken>"))   ink=Color::darken;
      else if (color_name(c,"<d>"))        ink=Color::darken;
      else if (color_name(c,"<warning>"))  ink=Color::warning;
      else if (color_name(c,"<w>"))        ink=Color::warning;
      else if (color_name(c,"<special>"))  ink=Color::special;
      else if (color_name(c,"<s>"))        ink=Color::special;
      else if (color_name(c,"<restore>"))  ink=ink_sav;
      else if (color_name(c,"<r>"))        ink=ink_sav;

      else goto oops;

      col=ink | (ink<<8) | (ink<<16) | (ink<<24);
    }
    else
    {
oops:
      if ((unsigned(x)<=unsigned(sx-Spr::font_sx)) && (unsigned(y)<=unsigned(sy-Spr::font_sy)))
        put_char(font,*c,p,ll,col);
      x+=Spr::font_sx;
      p+=stepx;
      c++;
    }
  }

  crsx=x-ox;
  crsy=y-oy;
}

void Screen::print (int x, int y, char *c, ...)
{
  char buffer[4096];
  va_list argptr;
  va_start(argptr, c);
  vsprintf(buffer, c, argptr);
  va_end(argptr);
  putstr (x, y, (unsigned char*)buffer);
}

void Screen::print (char *c, ...)
{
  char buffer[4096];
  va_list argptr;
  va_start(argptr, c);
  vsprintf(buffer, c, argptr);
  va_end(argptr);
  putstr (crsx, crsy, (unsigned char*)buffer);
}

//----- screen capture -------------------------------------------------------

static inline char bconv (int val)
{
  return ((unsigned char)(((unsigned char)(val)*0x410)/0x100));
}

void Screen::capture (void)
{
  const int hlinelen = 0x4;
  const int hlinesnum = 0x5;
  const int hpiclen = 0x8;
  short bmpname = 'B'+256*'M';
  int bmph [13] =
  {
    0x0000FE36,0x00000000,0x00000436,0x00000028,0x00000140,0x000000C8,
    0x00080001,0x00000000,0x0000FA00,0x00002E23,0x00002E23,0x00000000,0x00000000
  };
  int i,lpos,lnum,fh;
  char name [32];

  bmph [hlinelen]    = real_sx;
  bmph [hlinesnum]   = real_sy;
  bmph [hpiclen]     = (((real_sx%4)?((real_sx|3)+1):real_sx)*real_sy);

  char tmppath[_MAX_PATH];
  if(GetTempPath(sizeof(tmppath),tmppath)==0)
    sprintf(tmppath,"c:");
  i = -1;
  do
  {
    i++;
    sprintf (name, "%s\\scrn_%03d.bmp", tmppath, i);
    fh = open(name, O_RDONLY);
    close (fh);
  }
  while ((i<1000) && (fh!=-1));

  if (i<1000)
  {
    if ((fh = open(name, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, _S_IREAD | _S_IWRITE)) != -1)
    {
      write (fh, &bmpname, sizeof(bmpname));
      write (fh, bmph, sizeof(bmph));
      for (i=0; i<256; i++)
      {
        Color col;
        unsigned char b;
        col = Video::vpalette[Video::vpalette_num][i];
        b = bconv (col.bval()); write (fh, &b, 1);
        b = bconv (col.gval()); write (fh, &b, 1);
        b = bconv (col.rval()); write (fh, &b, 1);
        b = 0; write (fh, &b, 1);
      }
      if(Video::upside_down)
      {
        for (lnum=0; lnum<real_sy; lnum++)
        {
          unsigned char buf [1024*8];
          for (lpos=0; lpos<real_sx; lpos++)
            buf [lpos] = *(unsigned char*)(adr+lnum*real_llen+lpos);
          write (fh, buf, (real_sx%4)?((real_sx|3)+1):real_sx);
        }
      }
      else
      {
        for (lnum=real_sy-1; lnum>=0; lnum--)
        {
          unsigned char buf [1024*8];
          for (lpos=0; lpos<real_sx; lpos++)
            buf [lpos] = *(unsigned char*)(adr+lnum*real_llen+lpos);
          write (fh, buf, (real_sx%4)?((real_sx|3)+1):real_sx);
        }
      }
      close (fh);
    }
  }
}