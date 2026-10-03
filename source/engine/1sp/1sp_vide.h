enum { screen_sx=320, screen_sy=200 };

class Video_device
{
public:
  static  int     active;
  virtual void    init(int x, int y)=0;
  virtual void    quit(int changemode)=0;
  virtual void    update_palette (Color *palette, char *gamma)=0;
  virtual Screen *get_screen (int mode)=0;
  virtual void    release_screen (void)=0;
  virtual void    show_screen (HDC hdc)=0;
  virtual void    clear_screen (HDC hdc)=0;
};

class VD_none: public Video_device
{
public:
  virtual void    init(int x, int y);
  virtual void    quit(int changemode);
  virtual void    update_palette (Color *palette, char *gamma);
  virtual Screen *get_screen (int mode);
  virtual void    release_screen (void);
  virtual void    show_screen (HDC hdc);
  virtual void    clear_screen (HDC hdc);
private:
} static vd_none;

class VD_bitmap: public Video_device
{
public:
  virtual void    init(int x, int y);
  virtual void    quit(int changemode);
  virtual void    update_palette (Color *palette, char *gamma);
  virtual Screen *get_screen (int mode);
  virtual void    release_screen (void);
  virtual void    show_screen (HDC hdc);
  virtual void    clear_screen (HDC hdc);
private:
  HPALETTE hpal;
  char             *bitmap;
  BITMAPINFOHEADER *bitmap_header;
  RGBQUAD          *bitmap_colors;
  void             *bitmap_surface;
  HPALETTE          CreateIdentityPalette(RGBQUAD aRGB[], int nColors, HDC hdc);
} static vd_bitmap;

class VD_ddraw: public Video_device
{
public:
  virtual void    init(int x, int y);
  virtual void    quit(int changemode);
  virtual void    update_palette (Color *palette, char *gamma);
  virtual Screen *get_screen (int mode);
  virtual void    release_screen (void);
  virtual void    show_screen (HDC hdc);
  virtual void    clear_screen (HDC hdc);
private:
  LPDIRECTDRAW        dd_main;
  LPDIRECTDRAWSURFACE dd_primary;
  LPDIRECTDRAWSURFACE dd_secondary;
  LPDIRECTDRAWPALETTE dd_palette;
  int                 old_assertbox;
  unsigned char      *screen_buf;
  HRESULT             lock_surface(LPDIRECTDRAWSURFACE surface, DDSURFACEDESC *ddsd);
  HRESULT             unlock_surface(LPDIRECTDRAWSURFACE surface);
  HRESULT             flip_surfaces(void);
} static vd_ddraw;
