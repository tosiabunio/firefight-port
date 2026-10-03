#include "1sp_hdrs.h"

//----- komunikaty -----------------------------------------------------------

#if HI_DEBUG
static char F_colover[] = "%d,%d,%d invalid color values";
#endif

//----- stale i zmienne ------------------------------------------------------

extern int ccpower_tab[256];
extern char rgb2bw [192];

//----- operacje na kolorze --------------------------------------------------

Color::Color (int _r, int _g, int _b)
{
#if HI_DEBUG
  if ((_r<0)||(_r>63)) FAILURE(F_colover,_r,_g,_b);
  if ((_g<0)||(_g>63)) FAILURE(F_colover,_r,_g,_b);
  if ((_b<0)||(_b>63)) FAILURE(F_colover,_r,_g,_b);
#endif
  rgb=(_r&0xff)|((_g&0xff)<<8)|((_b&0xff)<<16);
}

int Color::distance (Color &c)
{
  int result;
  int c1=rgb;
  int c2=c.rgb;
  __asm
  {
    mov ecx,[c1]
    mov edx,[c2]
    xor eax,eax
    mov al,cl
    sub al,dl
    mov ebx,[ccpower_tab+eax*4]
    mov al,ch
    sub al,dh
    add ebx,[ccpower_tab+eax*4]
    ror ecx,8
    ror edx,8
    mov al,ch
    sub al,dh
    add ebx,[ccpower_tab+eax*4]
    mov [result],ebx
  }
  return(result);
}

//----- narzedzia i przezroczystosc -----------------------------------------------------

void Tool::brightness (int power)
{
  int i;
  power=(power*256)/100;
  if (power>=0)
  {
    for (i=0; i<256; i++)
    {
      int r=Video::palette[i].rval()+(Video::palette[i].rval()*power)/256;
      int g=Video::palette[i].gval()+(Video::palette[i].gval()*power)/256;
      int b=Video::palette[i].bval()+(Video::palette[i].bval()*power)/256;
      Color c=Color((r>63)?63:r, (g>63)?63:g, (b>63)?63:b);
      conv[i]=(char)Video::closest(c);
    }
  }
  else
  {
    power=-power;
    for (i=0; i<256; i++)
    {
      int r=Video::palette[i].rval()-(Video::palette[i].rval()*power)/256;
      int g=Video::palette[i].gval()-(Video::palette[i].gval()*power)/256;
      int b=Video::palette[i].bval()-(Video::palette[i].bval()*power)/256;
      Color c=Color((r<0)?0:r, (g<0)?0:g, (b<0)?0:b);
      conv[i]=(char)Video::closest(c);
    }
  }
}

void Tool::glass (Color col, int power)
{
  int i;
  power=(power*256)/100;
  for (i=0; i<256; i++)
  {
    Color c=Color(
      Video::palette[i].rval()+((col.rval()-Video::palette[i].rval())*power)/256,
      Video::palette[i].gval()+((col.gval()-Video::palette[i].gval())*power)/256,
      Video::palette[i].bval()+((col.bval()-Video::palette[i].bval())*power)/256);
    conv[i]=(char)Video::closest(c);
  }
}

void Tool::glass (Color from, Color to)
{
  int i;
  char filter[64];
  for (i=0; i<256; i+=4)
  {
    Color c=Color(
      from.rval()+((to.rval()-from.rval())*i)/256,
      from.gval()+((to.gval()-from.gval())*i)/256,
      from.bval()+((to.bval()-from.bval())*i)/256);
    filter[i/4]=(char)Video::closest(c);
  }

  for (i=0; i<256; i++)
    conv[i]=filter[rgb2bw[
      Video::palette[i].rval()+
      Video::palette[i].gval()+
      Video::palette[i].bval()]];
}

void Tool::negative (int power)
{
  int i;
  power=(power*256)/100;
  for (i=0; i<256; i++)
  {
    Color c=Color(
      Video::palette[i].rval()+(((63-Video::palette[i].rval())-Video::palette[i].rval())*power)/256,
      Video::palette[i].gval()+(((63-Video::palette[i].gval())-Video::palette[i].gval())*power)/256,
      Video::palette[i].bval()+(((63-Video::palette[i].bval())-Video::palette[i].bval())*power)/256);
    conv[i]=(char)Video::closest(c);
  }
}

void Tsp::update (void)
{
  int c,s;
  double sm=double(66)/100;
  double cm=1-sm;

  Color smt[256];
  Color cmt[256];

  for(s=0; s<256; s++)
  {
    smt[s]=Color(
      (int)(Video::palette[s].rval()*sm),
      (int)(Video::palette[s].gval()*sm),
      (int)(Video::palette[s].bval()*sm));
  }

  for(c=0; c<256; c++)
  {
    cmt[c]=Color(
      (int)(Video::palette[c].rval()*cm),
      (int)(Video::palette[c].gval()*cm),
      (int)(Video::palette[c].bval()*cm));
  }

  for(c=0; c<256; c++)
    for(s=0; s<256; s++)
    {
      int color=(*(int*)&cmt[c]+*(int*)&smt[s]);
      conv[c*256+s]=(char)Video::closest(*(Color*)&color);
    }
}

