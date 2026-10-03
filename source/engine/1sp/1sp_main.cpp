#include "1sp_hdrs.h"

//----- komunikaty -----------------------------------------------------------

static char F_outrange[] = "can't access sprite %s[%d] - phase index out of range";
static char F_snotinit[] = "trying to access not initialized sprite";
static char F_cantcopy[] = "trying to clone not initialized sprite";

//----- czesc glowna sprajtow ------------------------------------------------

HWND Spr::create_window (Spr::work_modes work_mode, HINSTANCE hinstance, char *class_name, char *window_name)
{
  HWND hwnd;
  if(work_mode==work_normal)
  {
    hwnd=CreateWindowEx(WS_EX_TOPMOST,class_name,window_name,
      WS_POPUP,
      0,0,0,0,
      HWND_DESKTOP,NULL,hinstance,NULL);
  }
  else if(work_mode==work_debug)
  {
    hwnd=CreateWindow(class_name,window_name,
      WS_OVERLAPPED|WS_THICKFRAME,
      CW_USEDEFAULT,CW_USEDEFAULT,0,0,
      HWND_DESKTOP,NULL,hinstance,NULL);
  }
  else
  {
    hwnd=CreateWindowEx(WS_EX_TOPMOST,class_name,window_name,
      WS_POPUP,
      0,0,0,0,
      HWND_DESKTOP,NULL,hinstance,NULL);
  }

  CHECK(hwnd!=NULL);
  return(hwnd);
}

int Screen::convert_accelerator[5]={0,0,1,0,2};
static int rot_tab[9]={0,0,1,0,2,0,0,0,3};

void Spr::init (Spr::work_modes work_mode, int use_320x200, int use_640x400)
{
  CHECK(Comm::hwnd!=NULL);
  
  DBG_CHECK((Spr::hires==0x100)&&(Spr::lores==0x200)&&(Spr::collis==0x400)); //screen_convert_accelerator
  CHECK((Spr::hires_scale==1)||(Spr::hires_scale==2)||(Spr::hires_scale==4)||(Spr::hires_scale==8));
  CHECK((Spr::lores_scale==1)||(Spr::lores_scale==2)||(Spr::lores_scale==4)||(Spr::lores_scale==8));
  CHECK((Spr::collis_scale==1)||(Spr::collis_scale==2)||(Spr::collis_scale==4)||(Spr::collis_scale==8));
  Screen::convert_accelerator[1]=rot_tab[Spr::hires_scale];
  Screen::convert_accelerator[2]=rot_tab[Spr::lores_scale];
  Screen::convert_accelerator[4]=rot_tab[Spr::collis_scale];

  Sprite::first.next=Sprite::first.prev=&Sprite::first;

  Video::work_mode=work_mode;
  Video::use_320x200=use_320x200;
  Video::use_640x400=use_640x400;
  Video::init();
  Comm::quit_me(quit,"*spr","log mmu xio");
}

void Spr::quit (void)
{
  Video::quit();
}

extern "C" int _pconv (unsigned char *def, Tool &t);

void Sprite::convert (Tool &t)
{
  for (int i=0; i<phases; i++)
  {
    if (phase[i].hires.def[0]) {
      _pconv (phase[i].hires.def[0], t);
      if (phase[i].hires.def[1]!=phase[i].hires.def[0]) _pconv (phase[i].hires.def[1], t);
    }
    if (phase[i].lores.def[0]) {
      _pconv (phase[i].lores.def[0], t);
      if (phase[i].lores.def[1]!=phase[i].lores.def[0]) _pconv (phase[i].lores.def[1], t);
    }
    if (phase[i].collis.def[0]) {
      _pconv (phase[i].collis.def[0], t);
      if (phase[i].collis.def[1]!=phase[i].collis.def[0]) _pconv (phase[i].collis.def[1], t);
    }
  }
}

Phase &Sprite::poper (int ph)
{
  if (name==NULL)
    FAILURE (F_snotinit);
  if ((ph<0)||(ph>=phases))
    FAILURE (F_outrange,name,ph);
  return (phase[ph]);
}

Sprite::Sprite ()
{
  data=NULL;
  phase=NULL;
  name=NULL;
  rledata[0]=rledata[1]=rledata[2]=NULL;
}

void Sprite::clone (const Sprite *dst, const Sprite *src)
{
  if (src->data==NULL)
    FAILURE(F_cantcopy);
  src->data->owners++;
  memcpy((void*)dst,(void*)src,sizeof(Sprite));
}

Sprite::~Sprite ()
{
  free();
}

static char unksp[]="unknown sprite";

void Sprite::free (void)
{
  if (data)
  {
    data->owners--;
    if(data->owners==0)
    {
      data->next->prev=data->prev;
      data->prev->next=data->next;

      Heap::free(data,FILE_LINE);
      if (rledata[0])
      {
        Heap::free(rledata[0],FILE_LINE);
      }
      if (rledata[1])
      {
        Heap::free(rledata[1],FILE_LINE);
      }
      if (rledata[2])
      {
        Heap::free(rledata[2],FILE_LINE);
      }
      data=NULL;
      phase=NULL;
      name=NULL;
      rledata[0]=rledata[1]=rledata[2]=NULL;
    }
  }
}

extern "C" int _ptouch (unsigned char *def);
static char *yesno[]={"no","yes"};

void Spr::touch (int totouch)
{
  (void)totouch;
  return;
#if 0
  MESSAGE("touching all definitions (hires:%s, lores:%s, collis:%s)...",
    yesno[(totouch&Spr::hires)?1:0],
    yesno[(totouch&Spr::lores)?1:0],
    yesno[(totouch&Spr::collis)?1:0]);
  int hs=0,ls=0,cs=0;

  int snum=0;
  for (Sprite::Data *ptr=Sprite::first.next; ptr!=&Sprite::first; ptr=ptr->next)
  {
    int phases=ptr->phases;
    Phase *phase=ptr->phase;
    for (int i=0; i<phases; i++)
    {
      if ((totouch&Spr::hires)&&phase[i].hires.def[0]) {
        hs+=_ptouch (phase[i].hires.def[0]);
        if (phase[i].hires.def[1]!=phase[i].hires.def[0]) 
          hs+=_ptouch (phase[i].hires.def[1]);
      }
      if ((totouch&Spr::lores)&&phase[i].lores.def[0]) {
        ls+=_ptouch (phase[i].lores.def[0]);
        if (phase[i].lores.def[1]!=phase[i].lores.def[0]) 
          ls+=_ptouch (phase[i].lores.def[1]);
      }
      if ((totouch&Spr::collis)&&phase[i].collis.def[0]) {
        cs+=_ptouch (phase[i].collis.def[0]);
        if (phase[i].collis.def[1]!=phase[i].collis.def[0]) 
          cs+=_ptouch (phase[i].collis.def[1]);
      }
    }
    snum++;
  }
  MESSAGE("...%d bytes of %d sprites touched (h:%d l:%d c:%d)",hs+ls+cs,snum,hs,ls,cs);
#endif
}

extern "C" int _pcsum (unsigned char *def);
static int last_csum;

int Spr::calculate_csum (void)
{
  int csum=0;
  for (Sprite::Data *ptr=Sprite::first.next; ptr!=&Sprite::first; ptr=ptr->next)
  {
    int phases=ptr->phases;
    Phase *phase=ptr->phase;
    for (int i=0; i<phases; i++)
    {
      if (phase[i].hires.def[0]) {
        csum+=_pcsum (phase[i].hires.def[0]);
        if (phase[i].hires.def[1]!=phase[i].hires.def[0]) 
          csum+=_pcsum (phase[i].hires.def[1]);
      }
      if (phase[i].lores.def[0]) {
        csum+=_pcsum (phase[i].lores.def[0]);
        if (phase[i].lores.def[1]!=phase[i].lores.def[0]) 
          csum+=_pcsum (phase[i].lores.def[1]);
      }
      if (phase[i].collis.def[0]) {
        csum+=_pcsum (phase[i].collis.def[0]);
        if (phase[i].collis.def[1]!=phase[i].collis.def[0]) 
          csum+=_pcsum (phase[i].collis.def[1]);
      }
    }
  }
  return(csum);
}

void Spr::make_csum (void)
{
  MESSAGE("making csum of all sprites...");
  last_csum=calculate_csum();
  MESSAGE("...csum value %08X saved",last_csum);
}

void Spr::check_csum (void)
{
  MESSAGE("checking csum of all sprites...");
  int csum=calculate_csum();
  MESSAGE("...csum value %08X (saved value %08X)",csum,last_csum);
  CHECK(csum==last_csum);
}
                        