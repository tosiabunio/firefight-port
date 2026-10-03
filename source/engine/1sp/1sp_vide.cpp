#include "1sp_hdrs.h"
#include "1sp_vide.h"

//----- komunikaty -----------------------------------------------------------

static char F_cdmtwice[]  = "trying to display more then one message";
static char F_palalup[]   = "trying to load more than one palette";
static char F_invvmod[]   = "can't set video mode: invalid parameter 0x%08X";
static char F_getsbusy[]  = "get_screen failed: screen is not released";
static char F_relsbusy[]  = "release_screen failed: screen is already released";
static char F_showsbusy[] = "show_screen failed: screen is not released";
static char F_palnosrc[]  = "source of palette '%s' not exist";
static char F_loaderr[]   = "loading palette '%s' failed";

//----- stale i zmienne ------------------------------------------------------

static int screen_busy, screen_rsx, screen_rsy;
static int changing_mode=1,palette_changed;
Color virtual_pal[256];

extern char *gamma_tables [64];
static char *gamma_tab;

static unsigned char *closest_colors=NULL;
static const int cc_size=64*64*64;
struct pal_rebuild {};

static int FrameRate;
static int FrameCount;
static int FrameCount0;
static DWORD FrameTime;
static DWORD FrameTime0;

static Video_device *video_device;
static int lores_min_frame_rate=999,lores_max_frame_rate=0,lores_frame_rate=0,lores_frame_count=0;
static int hires_min_frame_rate=999,hires_max_frame_rate=0,hires_frame_rate=0,hires_frame_count=0;

//----- sprawy palety --------------------------------------------------------

extern int keep_palette;
int keep_palette=1;

void Video::update_palette (int gc)
{
  DBG_CHECK(initialized);
  DBG_CHECK((gc>=min_gamma)&&(gc<=max_gamma));
  gamma=gc;
  palette_changed++;
}

void Video::select_vpalette (int num)
{
  DBG_CHECK(initialized);
  DBG_CHECK((num>=0)&&(num<10));
  vpalette_num=num;
  palette_changed++;
}

void Video::load_palette (char *fname, int update_tsp)
{
  DBG_CHECK(initialized);
  if (closest_colors&&keep_palette)
  {
    Heap::free(closest_colors);
    closest_colors=NULL;
  }

  if(fname==NULL)
  {
    int z;
    for(z=0;z<20;z++)
      palette[10+z]=Color(z*64/20,z*64/20,z*64/20);
    for(z=0;z<6;z++)
      for(int y=0;y<6;y++)
        for(int x=0;x<6;x++)
          palette[30+x+y*6+z*6*6]=Color(x*64/6,y*64/6,z*64/6);
  }
  else
  {
    char name[1024];
    strupr(strcpy(name,fname));

    unsigned char *tmp=NULL;
    try
    {
      if(closest_colors)
        FAILURE(F_palalup);
      File::area(name);
      if((!File::volume)&&(!File::exist("source")))
        FAILURE(F_palnosrc,name);
      tmp=(unsigned char*)Heap::alloc(cc_size);
      try
      {
        int v;
        if(!File::exist("target")) throw pal_rebuild();
        File::open("target");
        File::read((char*)&v,sizeof(int));
        if(v!=pal_file_level) throw pal_rebuild();
        File::read((char*)&v,sizeof(int));
        if((!File::volume())&&((unsigned)v!=File::date("source"))) throw pal_rebuild();
        File::read((char*)&vpalette[0][0],sizeof(vpalette));
        memcpy(&palette[0],&vpalette[0][0],sizeof(palette));
        File::read(&tmp[0], cc_size);
        File::read(&tsp.conv[0], 256*256);
        File::close();
      } catch (pal_rebuild)
      {
        File::close();

        display_message("making %s",name);
        char pal[256][3];
        open_flic("source");
        for(int f=0; f<10; f++)
        {
          get_frame(tmp,pal);
          for(int i=0; i<256; i++)
            vpalette[f][i]=Color(pal[i][0],pal[i][1],pal[i][2]);
          memcpy(&palette[0],&vpalette[0][0],sizeof(palette));
        }
        close_flic();

        for(int r=0; r<64; r++)
        {
          progress_bar(r,64);
          for(int g=0; g<64; g++)
            for(int b=0; b<64; b++)
              tmp[r+g*64+b*64*64]=(unsigned char)closest(r,g,b);
        }

        closest_colors=tmp;
        if(update_tsp)
          tsp.update();
        else
          memset((char*)&tsp.conv[0], 0, 256*256);


        int v;
        File::create("target");
        v=(unsigned)pal_file_level;
        File::write((char*)&v,sizeof(int));
        v=(unsigned)File::date("source");
        File::write((char*)&v,sizeof(int));
        File::write((char*)&vpalette[0][0], sizeof(vpalette));
        File::write((char*)&tmp[0], cc_size);
        File::write((char*)&tsp.conv[0], 256*256);
        File::close();

        restore_background();
      }
      closest_colors=tmp;
      File::endarea();
    }
    catch (Failure)
    {
      if(tmp) Heap::free(tmp);
      FAILURE(F_loaderr,name);
    }
    if(!Comm::production) MESSAGE("palette %s",name);
  }

  Color::black  =Video::closest(Color::cblack);
  Color::dyellow=Video::closest(Color::cdyellow);
  Color::yellow =Video::closest(Color::cyellow);
  Color::dgreen =Video::closest(Color::cdgreen);
  Color::green  =Video::closest(Color::cgreen);
  Color::dcyan  =Video::closest(Color::cdcyan);
  Color::cyan   =Video::closest(Color::ccyan);
  Color::dblue  =Video::closest(Color::cdblue);
  Color::blue   =Video::closest(Color::cblue);
  Color::dviolet=Video::closest(Color::cdviolet);
  Color::violet =Video::closest(Color::cviolet);
  Color::dred   =Video::closest(Color::cdred);
  Color::red    =Video::closest(Color::cred);
  Color::dwhite =Video::closest(Color::cdwhite);
  Color::white  =Video::closest(Color::cwhite);

  Color::normal =Video::closest(Color::cnormal);
  Color::lighten=Video::closest(Color::clighten);
  Color::darken =Video::closest(Color::cdarken);
  Color::warning=Video::closest(Color::cwarning);
  Color::special=Video::closest(Color::cspecial);
//  palette_changed++;
}

void Video::free_paltabs (void)
{
  DBG_CHECK(initialized);
  if (closest_colors&&!keep_palette)
  {
    Heap::free(closest_colors);
    closest_colors=NULL;
  }
}

int Video::closest (Color &c)
{
  DBG_CHECK(initialized);
  if (closest_colors)
    return(closest_colors[c.rval()+c.gval()*64+c.bval()*64*64]);

  int closest=0, distance=99999, d, i;
  for (i=min_color; i<max_color; i++)
  {
    if ((d=c.distance(palette[i]))<distance)
    {
      closest=i;
      if ((distance=d)==0) return(closest);
    }
  }

  return(closest);
}

//----- metody ekranu --------------------------------------------------------

Screen *Video::get_screen (void)
{
  DBG_CHECK(initialized);

  if(current_mode!=next_mode)
  {
    DBG_MESSAGE("changing display mode...");
    changing_mode=1;

    video_device->quit(1);
    video_device=&vd_none;

    current_mode=next_mode;
    char *i_mode=NULL;

    switch(current_mode&Spr::mode_mask)
    {
      case Spr::lores:
        mode_flag=Spr::lores;
        i_mode="lores";
        break;
      case Spr::hires:
        mode_flag=Spr::hires;
        i_mode="hires";
        break;
      case Spr::collis:
        mode_flag=Spr::collis;
        i_mode="collis";
        break;
      default: FAILURE(F_invvmod,mode_flag);
    }

    int real_sx=Screen::convert(screen_sx,mode_flag);
    int real_sy=Screen::convert(screen_sy,mode_flag);
    screen_rsx=real_sx; screen_rsy=real_sy;

    video_device=&vd_sdl;  // port: was vd_ddraw (full screen) or vd_bitmap (windowed)

    // TODO(phase 4): map mouse coordinates from the window.
    {
      RECT rc;
      rc.left  =0;
      rc.right =screen_rsx;
      if(mode_flag==Spr::hires)
      {
        rc.top   =use_640x400 ?0   :0;   //?0          :40;
        rc.bottom=use_640x400 ?400 :480; //?screen_rsy :screen_rsy+40;
      }
      else
      {
        rc.top   =use_320x200 ?0   :0;   //?0          :20;
        rc.bottom=use_320x200 ?200 :240; //?screen_rsy :screen_rsy+20;
      }
      Comm::reinit_mouse(&rc,screen_sx,screen_sy);
    }

    video_device->init(real_sx,real_sy);

    update_palette();

    DBG_MESSAGE("...%s mode successfully set",
      i_mode);

    Spr::touch(mode_flag|Spr::collis);

    changing_mode=0;
  }

  if(screen_busy)
    FAILURE(F_getsbusy);
  screen_busy++;

  return(video_device->get_screen(mode_flag));
}

void Video::release_screen (void)
{
  DBG_CHECK(initialized);
  if(!screen_busy)
    FAILURE(F_relsbusy);
  screen_busy=0;

  video_device->release_screen();
}

void Video::set_mode (int mode_flag)
{
  DBG_CHECK(initialized);
  // Port: the port has no lores art, so a request for lores keeps hires.
  if((mode_flag&Spr::mode_mask)==Spr::lores)
    mode_flag=(mode_flag&~Spr::mode_mask)|Spr::hires;
  next_mode=mode_flag;
}

void Video::show_screen (void)
{
  DBG_CHECK(initialized);
  if(screen_busy)
    FAILURE(F_showsbusy);

  if(Video_device::active)
  {
    FrameCount++;
    FrameTime = timeGetTime();

    if (FrameTime - FrameTime0 > 1000)
    {
      FrameRate = (FrameCount - FrameCount0) * 1000 / (FrameTime - FrameTime0);
      FrameTime0 = FrameTime;
      FrameCount0 = FrameCount;
      frame_rate = FrameRate;

      static int fr[3]={9,99,9999};

      if((abs(fr[0]-frame_rate)<3)&&(abs(fr[1]-frame_rate)<3)&&(abs(fr[2]-frame_rate)<3))
      {
        if(mode_flag==Spr::hires)
        {
          if (frame_rate<hires_min_frame_rate) hires_min_frame_rate=frame_rate;
          if (frame_rate>hires_max_frame_rate) hires_max_frame_rate=frame_rate;
          hires_frame_rate+=frame_rate;
          hires_frame_count++;
        }
        else
        {
          if (frame_rate<lores_min_frame_rate) lores_min_frame_rate=frame_rate;
          if (frame_rate>lores_max_frame_rate) lores_max_frame_rate=frame_rate;
          lores_frame_rate+=frame_rate;
          lores_frame_count++;
        }
      }

      fr[0]=fr[1];
      fr[1]=fr[2];
      fr[2]=frame_rate;
    }

    if(palette_changed&&Comm::hwnd)
    {
      gamma_tab=gamma_tables[gamma-min_gamma];
      video_device->update_palette(vpalette[vpalette_num],gamma_tab);
      palette_changed=0;
    }

    video_device->show_screen((HDC)NULL);
  }
}

void Video::clear_screen(void)
{
  video_device->clear_screen((HDC)NULL);
}

static char last_message[1024];
static int last_progress=-1;

void Video::display_message (char *c, ...)
{
  DBG_CHECK(initialized);
  va_format_message(c);
  strcpy(last_message,formated_message);

  if(current_mode!=-1)
  {
    Screen *s=get_screen();
    s->cls(Color::black);
    s->rectangle(Color::dwhite,0,Spr::font_sy,screen_sx,1);
    s->ink=Color::white;
    s->print(0,0,last_message);
    release_screen();
    show_screen();
  }
  else
    Comm::init_info(last_message);
}

void Video::restore_background (void)
{
  DBG_CHECK(initialized);
  char msg[1000];
  sprintf(msg,"%s (DONE)",last_message);

  if(current_mode!=-1)
  {
    Screen *s=get_screen();
    s->cls(Color::black);
    s->rectangle(Color::dwhite,0,Spr::font_sy,screen_sx,1);
    s->ink=Color::white;
    s->print(0,0,msg);
    release_screen();
    show_screen();
  }
  else
    Comm::init_info(msg);

  last_progress=-1;
}

void Video::progress_bar (int current, int total)
{
  DBG_CHECK(initialized);
  int progress=9-(((current-1)*10)/total);

  if(last_progress!=progress)
  {
    char msg[1000];
    sprintf(msg,"%s (%d%%)",last_message,100-progress*10);
    last_progress=progress;

    if(current_mode!=-1)
    {
      Screen *s=get_screen();
      s->cls(Color::black);
      s->rectangle(Color::dwhite,0,Spr::font_sy,screen_sx,1);
      s->ink=Color::white;
      s->print(0,0,"%s",msg);
      release_screen();
      show_screen();
    }
    else
      Comm::init_info(msg);
  }
}

//----- inicjalizacje itp ----------------------------------------------------

static Comm::wpp old_wp;

void Video::init(void)
{
  initialized=1;
  old_wp=Comm::set_window_proc(window_proc);
  video_device=&vd_none;
  current_mode=-1;
  // Port: hires only (the original started in lores), and never upside down (the windowed
  // modes drew into a bottom-up Windows bitmap).
  next_mode=Spr::hires;
  upside_down=0;
  gamma=0;
  gamma_tab=gamma_tables[0];
  load_palette();
}

void Video::quit(void)
{
  if(initialized)
  {
    changing_mode=1;
    video_device->quit(0);
    initialized=0;

    if (closest_colors)
    {
      Heap::free(closest_colors);
      closest_colors=NULL;
    }

    if(hires_frame_count)
      ENGINFO("HIRES frame rate: min=%d, max=%d, avg=%d",
        hires_min_frame_rate,hires_max_frame_rate,hires_frame_rate/hires_frame_count);
    if(lores_frame_count)
      ENGINFO("LORES frame rate: min=%d, max=%d, avg=%d",
        lores_min_frame_rate,lores_max_frame_rate,lores_frame_rate/lores_frame_count);

  }
}

int Video::window_proc (int *result, HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
  int processed=0;
  if(initialized)
  {
    if(!old_wp(result,hwnd,message,wparam,lparam))
      switch(message)
      {
        case WM_ACTIVATEAPP:
          Video_device::active=LOWORD(wparam);
          break;
      }
  }
  return(processed);
}