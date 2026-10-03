#include "1sp_hdrs.h"
#include "1sp_vide.h"

#define DDRAW_RETRY 100000

//----- komunikaty -----------------------------------------------------------

static char F_vdnotinit[] = "unable to %s: none video mode mode set";
static char F_ddproblem[] = "direct draw problem: %s (DDRVAL=%d)";

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

//----- bitmap device --------------------------------------------------------

void    VD_bitmap::init(int x, int y)
{
  bitmap=(char*)Heap::alloc(sizeof(BITMAPINFOHEADER)+256*sizeof(RGBQUAD)+x*y,"main screen");
  bitmap_header=(BITMAPINFOHEADER*)bitmap;
  bitmap_colors=(RGBQUAD*)(bitmap+sizeof(BITMAPINFOHEADER));
  bitmap_surface=(void*)(bitmap+sizeof(BITMAPINFOHEADER)+256*sizeof(RGBQUAD));

  bitmap_header->biSize=sizeof(BITMAPINFOHEADER);
  bitmap_header->biWidth=x;
  bitmap_header->biHeight=y;
  bitmap_header->biPlanes=1;
  bitmap_header->biBitCount=8;
  bitmap_header->biCompression=BI_RGB;
  bitmap_header->biSizeImage=0;
  bitmap_header->biXPelsPerMeter=0;
  bitmap_header->biYPelsPerMeter=0;
  bitmap_header->biClrUsed=0;
  bitmap_header->biClrImportant=0;
}

void    VD_bitmap::quit(int changemode)
{
  (void)changemode;
  if(!critical_error_occurred)
  {
    if (bitmap)
    {
      Heap::free(bitmap);
      bitmap=NULL;
    }
    if(hpal)
    {
      DeleteObject(hpal);
      hpal=NULL;
    }
  }
}

void    VD_bitmap::update_palette (Color *palette, char *gamma)
{
  for (int i=0; i<256; i++)
  {
    Color &c=palette[i];
    bitmap_colors[i].rgbRed      =(char)(gamma[c.rval()]*4);
    bitmap_colors[i].rgbGreen    =(char)(gamma[c.gval()]*4);
    bitmap_colors[i].rgbBlue     =(char)(gamma[c.bval()]*4);
    bitmap_colors[i].rgbReserved =0;
  }

  HDC hdc=GetDC(Comm::hwnd);
  if(hpal)
  {
    DeleteObject(hpal);
    hpal=NULL;
  }
  hpal=CreateIdentityPalette(bitmap_colors, 256, hdc);
  ReleaseDC(Comm::hwnd,hdc);
}

Screen *VD_bitmap::get_screen (int mode)
{
  unsigned char *adr=(unsigned char*)bitmap_surface;
  int sx=Screen::unconvert(bitmap_header->biWidth,mode);
  int sy=Screen::unconvert(bitmap_header->biHeight,mode);
  int llen=sx;
  last_screen.init(adr,sx,sy,sx,mode);
  return(&last_screen);
}

void    VD_bitmap::release_screen (void)
{
}

void    VD_bitmap::show_screen (HDC hdc)
{
  if (bitmap)
  {
    if(hpal)
    {
      SelectPalette(hdc,hpal,FALSE);
      RealizePalette(hdc);
    }
    int sy=last_screen.real_sy, sx=last_screen.real_sx, llen=last_screen.real_llen;
    RECT rc;
    GetClientRect(Comm::hwnd,&rc);
    
    if(Video::work_mode==Spr::work_debug)
      StretchDIBits(hdc,0,0,rc.right,rc.bottom,0,0,sx,sy,
        bitmap_surface,(BITMAPINFO*)bitmap_header,DIB_RGB_COLORS,SRCCOPY);
    else
      StretchDIBits(hdc,(rc.right-640)/2,(rc.bottom-400)/2,640,400,0,0,sx,sy,
        bitmap_surface,(BITMAPINFO*)bitmap_header,DIB_RGB_COLORS,SRCCOPY);
  }
}

void    VD_bitmap::clear_screen (HDC hdc)
{
  last_screen.cls(0);
  show_screen(hdc);
}

HPALETTE VD_bitmap::CreateIdentityPalette(RGBQUAD aRGB[], int nColors, HDC hdc)
{
	int i;
	struct {
		WORD Version;
		WORD NumberOfEntries;
		PALETTEENTRY aEntries[256];
	} Palette = {	0x300, 256 };

	int nStaticColors,nUsableColors;
	nStaticColors = 10; //GetDeviceCaps(hdc, NUMCOLORS)/2;
	nUsableColors = nColors - nStaticColors;
	GetSystemPaletteEntries(hdc, 0, 256, Palette.aEntries);

	for (i=0; i<256; i++)
	{
    if ((i>=nStaticColors)&&(i<nUsableColors))
    {
  		Palette.aEntries[i].peRed   = aRGB[i].rgbRed;
		  Palette.aEntries[i].peGreen = aRGB[i].rgbGreen;
		  Palette.aEntries[i].peBlue  = aRGB[i].rgbBlue;
      Palette.aEntries[i].peFlags = PC_NOCOLLAPSE;
    }
    else
    {
  		aRGB[i].rgbRed   = Palette.aEntries[i].peRed;
		  aRGB[i].rgbGreen = Palette.aEntries[i].peGreen;
		  aRGB[i].rgbBlue  = Palette.aEntries[i].peBlue;
      Palette.aEntries[i].peFlags = 0;
    }
	}

	return CreatePalette((LOGPALETTE *)&Palette);
}

//----- direct draw device ---------------------------------------------------

static unsigned char pixel_buf;

HRESULT VD_ddraw::lock_surface(LPDIRECTDRAWSURFACE surface, DDSURFACEDESC *ddsd)
{
  int retry = DDRAW_RETRY;
  HRESULT ddrval;
  do
  {
    ddrval=surface->Lock(NULL,ddsd,DDLOCK_SURFACEMEMORYPTR,NULL);
    if (ddrval == DDERR_SURFACELOST)
    {
      dd_primary->Restore();
      if(surface==dd_secondary)
        dd_secondary->Restore();
    }
  }
  while((ddrval!=DD_OK)&&(--retry>0));
  return(ddrval);
}

HRESULT VD_ddraw::unlock_surface(LPDIRECTDRAWSURFACE surface)
{
  int retry = DDRAW_RETRY;
  HRESULT ddrval;
  do
  {
    ddrval=surface->Unlock(NULL);
    if (ddrval == DDERR_SURFACELOST)
    {
      dd_primary->Restore();
      if(surface==dd_secondary)
        dd_secondary->Restore();
    }
  }
  while((ddrval!=DD_OK)&&(--retry>0));
  return(ddrval);
}

HRESULT VD_ddraw::flip_surfaces (void)
{
  int retry = DDRAW_RETRY;
  HRESULT ddrval;
  do
  {
    ddrval=dd_primary->Flip(NULL, DDFLIP_WAIT);
    if (ddrval == DDERR_SURFACELOST)
    {
      dd_primary->Restore();
      dd_secondary->Restore();
    }
  }
  while((ddrval!=DD_OK)&&(--retry>0));
  return(ddrval);
}

void    VD_ddraw::clear_screen(HDC hdc)
{
  (void)hdc;
  DBG_MESSAGE("clearing primary surface");

  DDSURFACEDESC	ddsd;
  memset(&ddsd,0,sizeof(ddsd));
  ddsd.dwSize = sizeof(ddsd);

  if(!dd_secondary)
  {
    if(lock_surface(dd_primary,&ddsd)==DD_OK)
    {
      int x   =ddsd.dwWidth;
      int y   =ddsd.dwHeight;
      unsigned char *adr=(unsigned char*)ddsd.lpSurface;
      int llen=ddsd.lPitch;
      Screen(adr,x,y,llen,Spr::lores).cls(0);
      unlock_surface(dd_primary);

      dd_main->WaitForVerticalBlank(DDWAITVB_BLOCKEND,NULL);
      dd_main->WaitForVerticalBlank(DDWAITVB_BLOCKEND,NULL);
    }
  }
  else
  {
    REPEAT(2)
    {
      if(lock_surface(dd_secondary,&ddsd)==DD_OK)
      {
        int x   =ddsd.dwWidth;
        int y   =ddsd.dwHeight;
        unsigned char *adr=(unsigned char*)ddsd.lpSurface;
        int llen=ddsd.lPitch;
        Screen(adr,x,y,llen,Spr::lores).cls(0);
        unlock_surface(dd_secondary);
        flip_surfaces();

        HRESULT ddrval;
        do
        {
          ddrval=dd_primary->GetFlipStatus(DDGFS_ISFLIPDONE);
        }
        while(ddrval==DDERR_WASSTILLDRAWING);
      }
    }
  }
}

HRESULT CALLBACK dd_enumcallback(LPDDSURFACEDESC ddsd, LPVOID)
{
  SYSINFO("  %dx%d %d colors", ddsd->dwWidth, ddsd->dwHeight, 1<<ddsd->ddpfPixelFormat.dwRGBBitCount);
  return(DDENUMRET_OK);
}

typedef HRESULT (WINAPI *Dd_create) (GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter);

void    VD_ddraw::init(int x, int y)
{
  old_assertbox=Comm::allow_assertbox;
  Comm::allow_assertbox=0;
  HRESULT ddrval;

  if(!dd_main)
  {
    Dd_create dd_create;

    dd_create=(Dd_create)GetProcAddress(Video::dd_hinstance,"DirectDrawCreate");
    if(dd_create==NULL)
      FAILURE2(Spr_error::ddraw_failure,"get DirectDrawCreate address failed");

    DBG_MESSAGE("creating direct draw");
    ddrval=(*dd_create)(NULL,&dd_main,NULL);
    if(ddrval!=DD_OK)
      FAILURE2(Spr_error::ddraw_failure,F_ddproblem,"can't initialize",ddrval&0xffff);

    DBG_MESSAGE("setting cooperative level");
    ddrval=dd_main->SetCooperativeLevel(Comm::hwnd,
      DDSCL_ALLOWMODEX|DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN|DDSCL_ALLOWREBOOT); 
    if(ddrval!=DD_OK)
      FAILURE2(Spr_error::ddraw_failure,F_ddproblem,"can't set cooperative level",ddrval&0xffff);

    SYSINFO("display modes supported by Direct Draw:");
    ddrval=dd_main->EnumDisplayModes(0, NULL, NULL, dd_enumcallback);
  }

  DBG_MESSAGE("setting display mode");
  if((x<=320)&&(y<=240))
  {
    if(!Video::use_320x200)
    {
      if((ddrval=dd_main->SetDisplayMode(320,240,8))!=DD_OK)
        FAILURE2(Spr_error::ddraw_cant_set_320x240,F_ddproblem,"can't set display mode 320x240x8",ddrval&0xffff);
    }
    else
    {
      if((ddrval=dd_main->SetDisplayMode(320,200,8))!=DD_OK)
        FAILURE2(Spr_error::ddraw_cant_set_320x200,F_ddproblem,"can't set display mode 320x200x8",ddrval&0xffff);
    }
  }
  else
  {
    if(!Video::use_640x400)
    {
      if((ddrval=dd_main->SetDisplayMode(640,480,8))!=DD_OK)
        FAILURE2(Spr_error::ddraw_cant_set_640x480,F_ddproblem,"can't set display mode 640x480x8",ddrval&0xffff);
    }
    else
    {
      if((ddrval=dd_main->SetDisplayMode(640,400,8))!=DD_OK)
        FAILURE2(Spr_error::ddraw_cant_set_640x400,F_ddproblem,"can't set display mode 640x400x8",ddrval&0xffff);
    }
  }

  DDSURFACEDESC	ddsd;

  memset(&ddsd,0,sizeof(ddsd));
  ddsd.dwSize = sizeof(ddsd);
  ddsd.dwFlags = DDSD_CAPS;
  ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
  DBG_MESSAGE("creating primary surface");
  ddrval=dd_main->CreateSurface(&ddsd,&dd_primary,NULL);
  if(ddrval!=DD_OK)
    FAILURE2(Spr_error::ddraw_failure,F_ddproblem,"can't create primary surface",ddrval&0xffff);

  DDSCAPS caps;
  dd_primary->GetCaps(&caps);
  if(caps.dwCaps&DDSCAPS_MODEX)
  {
    DBG_MESSAGE("modex detected - recreating primary surface");
    dd_primary->Release();

    memset(&ddsd,0,sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS|DDSD_BACKBUFFERCOUNT;
    ddsd.dwBackBufferCount = 1;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
                          DDSCAPS_FLIP |
                          DDSCAPS_COMPLEX;
    DBG_MESSAGE("creating modex primary surface");
    ddrval=dd_main->CreateSurface(&ddsd,&dd_primary,NULL);
    if(ddrval!=DD_OK)
      FAILURE2(Spr_error::ddraw_failure,F_ddproblem,"can't create primary surface",ddrval&0xffff);

    caps.dwCaps = DDSCAPS_BACKBUFFER;
    DBG_MESSAGE("creating modex secondary surface");
    ddrval=dd_primary->GetAttachedSurface(&caps, &dd_secondary);
    if(ddrval!=DD_OK)
      FAILURE2(Spr_error::ddraw_failure,F_ddproblem,"can't create secondary surface",ddrval&0xffff);
  }
  else
    dd_secondary=NULL;

  if(dd_palette==NULL)
  {
    PALETTEENTRY pe[256];
    memset(pe,0,sizeof(pe));
    DBG_MESSAGE("creating palette");
    ddrval=dd_main->CreatePalette(DDPCAPS_8BIT|DDPCAPS_ALLOW256,pe,&dd_palette,NULL);
    if(ddrval!=DD_OK)
      FAILURE2(Spr_error::ddraw_failure,F_ddproblem,"can't create palette",ddrval&0xffff);
  }

  DBG_MESSAGE("attaching palette");
  int retry = DDRAW_RETRY;
  do
  {
    ddrval=dd_primary->SetPalette(dd_palette);
    if (ddrval == DDERR_SURFACELOST)
    {
      dd_primary->Restore();
    }
    else if(ddrval!=DD_OK)
      FAILURE2(Spr_error::ddraw_failure,F_ddproblem,"can't attach palette",ddrval&0xffff);
  }
  while((ddrval!=DD_OK)&&(--retry>0));

  screen_buf=(unsigned char*)Heap::alloc(x*y);

  clear_screen((HDC)NULL);
}

void    VD_ddraw::quit(int changemode)
{
  if(dd_palette&&dd_primary&&dd_main)
    clear_screen((HDC)NULL);

  if(dd_primary)
  {
    DBG_MESSAGE("releasing primary surface");
    dd_primary->Release();
    dd_primary=NULL;
    dd_secondary=NULL;
  }

  if(!changemode)
  {
    if(dd_palette)
    {
      DBG_MESSAGE("releasing palette");
      dd_palette->Release();
      dd_palette=NULL;
    }

    if(dd_main)
    {
      DBG_MESSAGE("releasing direct draw");
      dd_main->Release();
      dd_main=NULL;
    }

    Comm::allow_assertbox=old_assertbox;
  }

  if(screen_buf)
  {
    Heap::free(screen_buf);
    screen_buf=NULL;
  }
}                                      

void    VD_ddraw::update_palette (Color *palette, char *gamma)
{
  if(dd_palette)
  {
	  struct {
		  WORD Version;
		  WORD NumberOfEntries;
		  PALETTEENTRY aEntries[256];
	  } Palette = {	0x300, 256 };

    PALETTEENTRY pe[256];

    int nStaticColors,nUsableColors;
    HDC hdc=GetDC(Comm::hwnd);
	  nStaticColors = 10; //GetDeviceCaps(hdc, NUMCOLORS)/2;
	  nUsableColors = 256 - nStaticColors;
	  GetSystemPaletteEntries(hdc, 0, 256, Palette.aEntries);
    ReleaseDC(Comm::hwnd,hdc);

    for (int i=0; i<256; i++)
	  {
      if ((i>=nStaticColors)&&(i<nUsableColors))
      {
        Color &c=palette[i];
        pe[i].peRed   =(char)(gamma[c.rval()]*4);
        pe[i].peGreen =(char)(gamma[c.gval()]*4);
        pe[i].peBlue  =(char)(gamma[c.bval()]*4);
        pe[i].peFlags =0;
      }
      else
      {
  		  pe[i].peRed   = Palette.aEntries[i].peRed;
		    pe[i].peGreen = Palette.aEntries[i].peGreen;
		    pe[i].peBlue  = Palette.aEntries[i].peBlue;
        pe[i].peFlags = 0;
      }
	  }

    DBG_MESSAGE("setting palette entries");
    dd_palette->SetEntries(0,0,256,pe);

    DBG_MESSAGE("attaching palette");
    int retry = DDRAW_RETRY;
    HRESULT ddrval;
    do
    {
      ddrval=dd_primary->SetPalette(dd_palette);
      if (ddrval == DDERR_SURFACELOST)
      {
        dd_primary->Restore();
      }
      else if(ddrval!=DD_OK)
        FAILURE2(Spr_error::ddraw_failure,F_ddproblem,"can't attach palette",ddrval&0xffff);
    }
    while((ddrval!=DD_OK)&&(--retry>0));
  }
}

Screen *VD_ddraw::get_screen (int mode)
{
  last_screen.init((unsigned char*)screen_buf,screen_sx,screen_sy,screen_sx,mode);
  return(&last_screen);
}

void    VD_ddraw::release_screen (void)
{
}

static inline void clear (Screen &scr,unsigned randx, unsigned randy)
{
  if((scr.checknc(1,            1)            !=0)||
     (scr.checknc(scr.sx-2,     1)            !=0)||
     (scr.checknc(scr.sx-2,     scr.sy-2)     !=0)||
     (scr.checknc(1,            scr.sy-2)     !=0)||
     (scr.checknc(randx%scr.sx, randy%scr.sy) !=0))
  scr.cls(0);
}
// MSVC 4 bound temporaries to Screen&.
static inline void clear (Screen &&scr,unsigned randx, unsigned randy) {clear(scr,randx,randy);}

static inline void show (DDSURFACEDESC	&ddsd)
{
  unsigned randx=timeGetTime();
  unsigned char *adr=(unsigned char*)ddsd.lpSurface;
  int sx  =Screen::unconvert(ddsd.dwWidth, Video::mode_flag);
  int sy  =Screen::unconvert(ddsd.dwHeight,Video::mode_flag);
  int llen=Screen::unconvert(ddsd.lPitch,  Video::mode_flag);
  Screen dst=Screen(adr,sx,sy,llen,Video::mode_flag);

  int mx  =(sx-screen_sx)/2;
  int my  =(sy-screen_sy)/2;
  last_screen.copy(dst,mx,my);
  unsigned randy=timeGetTime();

  if(mx>0)
  {
    if(my>0)
    {
      clear(Screen(dst,0,my,mx,sy-2*my),randx,randy);
      clear(Screen(dst,sx-mx,my,mx,sy-2*my),randx,randy);
    }
    else
    {
      clear(Screen(dst,0,0,mx,sy),randx,randy);
      clear(Screen(dst,sx-mx,0,mx,sy),randx,randy);
    }
  }

  if(my>0)
  {
    clear(Screen(dst,0,0,sx,my),randx,randy);
    clear(Screen(dst,0,sy-my,sx,my),randx,randy);
  }
}

void    VD_ddraw::show_screen (HDC hdc)
{
  (void)hdc;
  DDSURFACEDESC	ddsd;
  memset(&ddsd,0,sizeof(ddsd));
  ddsd.dwSize = sizeof(ddsd);

  if(!dd_secondary)
  {
    if(lock_surface(dd_primary,&ddsd)==DD_OK)
    {
      show(ddsd);
      unlock_surface(dd_primary);
    }
  }
  else
  {
    if(lock_surface(dd_secondary,&ddsd)==DD_OK)
    {
      show(ddsd);
      unlock_surface(dd_secondary);
      flip_surfaces();
    }
  }
}
