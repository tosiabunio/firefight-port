#include "1sp_hdrs.h"
#include "1sp_vide.h"
#include <SDL.h>

//----- komunikaty -----------------------------------------------------------

static char F_vdnotinit[] = "unable to %s: none video mode mode set";

//----- stale i zmienne ------------------------------------------------------

static Screen last_screen;
int Video_device::active;

//----- none device ----------------------------------------------------------

void    VD_none::init(int x, int y)
{
  (void)x,y;
}

void    VD_none::quit(int changemode)
{
  (void)changemode;
}

void    VD_none::update_palette (Color *palette, char *gamma)
{
  (void)palette,gamma;
}

Screen *VD_none::get_screen (int mode)
{
  (void)mode;
  FAILURE(F_vdnotinit,"get_screen");
  return((Screen*)NULL);
}

void    VD_none::release_screen (void)
{
  FAILURE(F_vdnotinit,"release_screen");
}

void    VD_none::show_screen (HDC hdc)
{
  (void)hdc;
}

void    VD_none::clear_screen (HDC hdc)
{
  (void)hdc;
}

//----- SDL device (port) ----------------------------------------------------

static SDL_Window   *sdl_window;
static SDL_Renderer *sdl_renderer;
static SDL_Texture  *sdl_texture;

// The 20 colours Windows reserved in an 8-bit palette. The DirectDraw device kept them at
// indices 0-9 and 246-255 (GetSystemPaletteEntries), so those indices always showed these
// colours whatever the game palette said.
static const unsigned char static_colors[20][3] = {
  {  0,  0,  0}, {128,  0,  0}, {  0,128,  0}, {128,128,  0}, {  0,  0,128},
  {128,  0,128}, {  0,128,128}, {192,192,192}, {192,220,192}, {166,202,240},
  {255,251,240}, {160,160,164}, {128,128,128}, {255,  0,  0}, {  0,255,  0},
  {255,255,  0}, {  0,  0,255}, {255,  0,255}, {  0,255,255}, {255,255,255}};

static unsigned argb (int r, int g, int b)
{
  return 0xff000000u|((unsigned)r<<16)|((unsigned)g<<8)|(unsigned)b;
}

void    VD_sdl::create_window (const char *title)
{
  if(SDL_InitSubSystem(SDL_INIT_VIDEO)!=0)
    FAILURE2(Spr_error::ddraw_failure,"cannot initialize SDL video: %s",(char*)SDL_GetError());
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"nearest");
  // Twice the game resolution, or the largest whole multiple that fits on the desktop.
  int base_w=640, base_h=Video::stretch_43 ?480 :400;
  int scale=2;
  SDL_DisplayMode desktop;
  if(SDL_GetDesktopDisplayMode(0,&desktop)==0)
    while((scale>1)&&((base_w*scale>desktop.w*9/10)||(base_h*scale>desktop.h*9/10)))
      scale--;
  Uint32 flags=SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI;
  if(Video::fullscreen)
    flags|=SDL_WINDOW_FULLSCREEN_DESKTOP;
  sdl_window=SDL_CreateWindow(title,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
    base_w*scale,base_h*scale,flags);
  if(!sdl_window)
    FAILURE2(Spr_error::ddraw_failure,"cannot create window: %s",(char*)SDL_GetError());
  sdl_renderer=SDL_CreateRenderer(sdl_window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
  if(!sdl_renderer)
    sdl_renderer=SDL_CreateRenderer(sdl_window,-1,0);
  if(!sdl_renderer)
    FAILURE2(Spr_error::ddraw_failure,"cannot create renderer: %s",(char*)SDL_GetError());
  SDL_RendererInfo info;
  if(SDL_GetRendererInfo(sdl_renderer,&info)==0)
    SYSINFO("video: SDL renderer %s, %s", (char*)info.name, Video::fullscreen ?"full screen" :"window");
  // Windows activated a new window (WM_ACTIVATEAPP); until then nothing is shown.
  Video_device::active=1;
}

void    VD_sdl::init(int x, int y)
{
  width=x;
  height=y;
  screen_buf=(unsigned char*)Heap::alloc(x*y,"main screen");
  if(sdl_renderer)
  {
    sdl_texture=SDL_CreateTexture(sdl_renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,x,y);
    if(!sdl_texture)
      FAILURE2(Spr_error::ddraw_failure,"cannot create texture: %s",(char*)SDL_GetError());
  }
  for (int i=0; i<256; i++)
    argb[i]=::argb(0,0,0);
}

void    VD_sdl::quit(int changemode)
{
  if(screen_buf)
  {
    Heap::free(screen_buf);
    screen_buf=NULL;
  }
  if(sdl_texture)
  {
    SDL_DestroyTexture(sdl_texture);
    sdl_texture=NULL;
  }
  if(!changemode)
  {
    if(sdl_renderer)
    {
      SDL_DestroyRenderer(sdl_renderer);
      sdl_renderer=NULL;
    }
    if(sdl_window)
    {
      SDL_DestroyWindow(sdl_window);
      sdl_window=NULL;
      SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }
  }
}

// The palette holds 6-bit components; like the original, the output is component*4 (0..252).
void    VD_sdl::update_palette (Color *palette, char *gamma)
{
  for (int i=0; i<256; i++)
  {
    if((i>=10)&&(i<246))
    {
      Color &c=palette[i];
      argb[i]=::argb(gamma[c.rval()]*4,gamma[c.gval()]*4,gamma[c.bval()]*4);
    }
    else
    {
      const unsigned char *c=static_colors[(i<10) ?i :i-236];
      argb[i]=::argb(c[0],c[1],c[2]);
    }
  }
}

Screen *VD_sdl::get_screen (int mode)
{
  last_screen.init(screen_buf,screen_sx,screen_sy,screen_sx,mode);
  return(&last_screen);
}

void    VD_sdl::release_screen (void)
{
}

// Letterboxed: the largest whole-number scale that fits, or a fractional one when the window is
// smaller than the picture. Square pixels by default; Video::stretch_43 shows 640x400 at 4:3.
void    VD_sdl::present (void)
{
  if(!sdl_texture||!screen_buf)
    return;
  void *pixels;
  int pitch;
  if(SDL_LockTexture(sdl_texture,NULL,&pixels,&pitch)!=0)
    return;
  for (int y=0; y<height; y++)
  {
    const unsigned char *src=screen_buf+y*width;
    unsigned *dst=(unsigned*)((char*)pixels+y*pitch);
    for (int x=0; x<width; x++)
      dst[x]=argb[src[x]];
  }
  SDL_UnlockTexture(sdl_texture);

  int out_w,out_h;
  SDL_GetRendererOutputSize(sdl_renderer,&out_w,&out_h);
  double aspect_h=Video::stretch_43 ?width*3.0/4.0 :height;
  double scale=SDL_min(out_w/(double)width,out_h/aspect_h);
  if(scale>=1.0)
    scale=SDL_floor(scale);
  SDL_Rect dst;
  dst.w=(int)(width*scale);
  dst.h=(int)(aspect_h*scale);
  dst.x=(out_w-dst.w)/2;
  dst.y=(out_h-dst.h)/2;
  SDL_SetRenderDrawColor(sdl_renderer,0,0,0,255);
  SDL_RenderClear(sdl_renderer);
  SDL_RenderCopy(sdl_renderer,sdl_texture,NULL,&dst);
  SDL_RenderPresent(sdl_renderer);
}

// Saves the frame as it would be shown (after the palette) when Video::shot_interval is due.
void    VD_sdl::dump_frame (void)
{
  static Uint32 next_shot=0;
  static int shots=0;
  if(!screen_buf)
    return;
  if(Video::shot_every>0)
  {
    if(Video::frames%Video::shot_every!=0)
      return;
  }
  else
  {
    if(Video::shot_interval<=0)
      return;
    Uint32 now=SDL_GetTicks();
    if(next_shot==0)
      next_shot=now;
    if(!SDL_TICKS_PASSED(now,next_shot))
      return;
    next_shot=now+Video::shot_interval;
  }
  SDL_Surface *shot=SDL_CreateRGBSurfaceWithFormat(0,width,height,32,SDL_PIXELFORMAT_ARGB8888);
  if(!shot)
    return;
  for (int y=0; y<height; y++)
  {
    const unsigned char *src=screen_buf+y*width;
    Uint32 *dst=(Uint32*)((char*)shot->pixels+y*shot->pitch);
    for (int x=0; x<width; x++)
      dst[x]=argb[src[x]];
  }
  char name[_MAX_PATH+32];
  if(Video::shot_every>0)
    sprintf(name,"%sframe_%06d.bmp",Comm::pref_path,Video::frames);
  else
    sprintf(name,"%sframe_%04d.bmp",Comm::pref_path,shots++);
  if(SDL_SaveBMP(shot,name)==0)
    MESSAGE("frame saved: %s",name);
  SDL_FreeSurface(shot);
}

void    VD_sdl::show_screen (HDC hdc)
{
  (void)hdc;
  Video::frames++;
  dump_frame();
  present();
  if((Video::quit_frames>0)&&(Video::frames==Video::quit_frames))
  {
    SDL_Event quit;
    memset(&quit,0,sizeof(quit));
    quit.type=SDL_QUIT;
    SDL_PushEvent(&quit);
  }
}

void    VD_sdl::clear_screen (HDC hdc)
{
  (void)hdc;
  if(screen_buf)
    memset(screen_buf,0,(size_t)width*height);
  present();
}
