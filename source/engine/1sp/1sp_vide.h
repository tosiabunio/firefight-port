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

// Port: the SDL device, replacing the DirectDraw (full screen) and GDI bitmap (windowed) devices.
// It owns the 8-bit framebuffer the game draws into, converts it through the palette into a
// streaming ARGB texture and draws that letterboxed in the window. Headless runs keep only the
// framebuffer. The window outlives mode changes; quit(0) closes it.
class VD_sdl: public Video_device
{
public:
  virtual void    init(int x, int y);
  virtual void    quit(int changemode);
  virtual void    update_palette (Color *palette, char *gamma);
  virtual Screen *get_screen (int mode);
  virtual void    release_screen (void);
  virtual void    show_screen (HDC hdc);
  virtual void    clear_screen (HDC hdc);
  static  void    create_window (const char *title);
private:
  void            present (void);
  void            dump_frame (void);
  unsigned char  *screen_buf;
  int             width, height;
  unsigned        argb[256];
} static vd_sdl;
