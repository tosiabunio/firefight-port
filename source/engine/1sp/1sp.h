#ifndef _1SP_H_INCLUDED
#define _1SP_H_INCLUDED

#ifndef EXCLUDE_LIBS
#ifdef _DEBUG
#pragma comment(lib,"1spd.lib")
#else
#pragma comment(lib,"1sp.lib")
#endif
#endif
#pragma comment(lib,"winmm.lib")

class Spr
{
public:

  //----- stale
  enum
  {
    mirror_mask    = 0x00000003,
      nomirror     = 0x00000000,
      hmirror      = 0x00000001,
      vmirror      = 0x00000002,
      hvmirror     = 0x00000003,
    function_mask  = 0x000000f0,
      with_tool    = 0x00000010,
      with_tspsoft = 0x00000020,
      with_tsphard = 0x00000030,
      with_copy    = 0x00000040,
    mode_mask      = 0x00000f00,
      hires        = 0x00000100,
      lores        = 0x00000200,
      collis       = 0x00000400,

    font_sx = 4, font_sy = 5
  };

  //----- zmienne
  static int mirror, onecolor, toload, load_enabled, hires_scale, lores_scale, collis_scale;

  //----- inicjalizacje
  enum work_modes   {work_normal, work_debug, work_safe };
  static HWND       create_window (work_modes work_mode, HINSTANCE hinstance, char *class_name, char *window_name);
  static void       init          (work_modes work_mode, int use_320x200=0, int use_640x400=0);

  //----- metody
  static void       load_prologue (int toload=-1);
  static void       load_epilogue (int update_palette=1);
  static void       touch         (int totouch);
  static void       make_csum     (void);
  static void       check_csum    (void);

private:
  static int        calculate_csum     (void);
  static void       quit (void);
};

//--- kolory ----------------------------------------------------------------

class Color: public Heap_object
{
public:

  //----- zmienne
  static Color cblack, cdyellow, cyellow, cdgreen, cgreen, cdcyan, ccyan,
         cdblue, cblue, cdviolet, cviolet, cdred, cred, cdwhite, cwhite,
         cnormal, clighten, cdarken, cwarning, cspecial;
  static int black, dyellow, yellow, dgreen, green, dcyan, cyan,
         dblue, blue, dviolet, violet, dred, red, dwhite, white,
         normal, lighten, darken, warning, special;

  //----- inicjalizacje
  Color () { rgb=0; }
  Color (int _r, int _g, int _b);
  Color (const Color &c) { rgb=c.rgb; };
  Color &operator= (const Color &c) { rgb=c.rgb; return(*this); }

  //----- operatory
  int   operator== (Color &c) { return (c.rgb==rgb); }
  int   operator!= (Color &c) { return (c.rgb!=rgb); }

  //----- funkcje
  int rval (void) { return (*(((char*)(&rgb))+0)); }
  int gval (void) { return (*(((char*)(&rgb))+1)); }
  int bval (void) { return (*(((char*)(&rgb))+2)); }
  int distance (Color &c);

private:

  int rgb;
};

//--- narzedzia -------------------------------------------------------------

struct Tool: public Heap_object
{
  //----- zmienne
  unsigned char conv [256];

  //----- operatory
  unsigned char &operator[] (int c)  { return conv[c]; }

  //----- metody
  void brightness (int power);
  void negative   (int power);
  void glass      (Color glass_color, int power);
  void glass      (Color darkest_color, Color lightest_color);
};

struct Tsp: public Heap_object
{
  //----- zmienne
  unsigned char conv [256*256];

  //----- operatory
  unsigned char &operator[] (int c)  { return conv[c]; }
  unsigned char &operator() (int color, int scrn)  { return conv[color*256+scrn]; }

  //----- metody
  void update (void);
};

extern Tsp tsp;

//--- sprajty ---------------------------------------------------------------

#pragma pack (push,spr_phase,1)
struct Phase
{
  //----- zmienne
  short l,r,u,d;
  struct Type { short l,r,u,d,real_l,real_r,real_u,real_d; unsigned char *def[2];};
  Type hires, lores, collis;
};
#pragma pack (pop,spr_phase)

class Sprite: public Heap_object
{
public:

  //----- zmienne
  char *name;
  unsigned char *rledata[3];
  int phases,maxl,maxr,maxu,maxd,used_memory;

  //----- inicjalizacje
  Sprite ();
  ~Sprite ();
  void free();

  //----- operatory
#if HI_DEBUG
  Phase &operator[] (int ph) {return (poper(ph));}
#else
  Phase &operator[] (int ph) {return (phase[ph]);}
#endif
  Sprite (const Sprite &s) { clone(this,&s); };
  Sprite &operator= (const Sprite &s) { clone(this,&s); return(*this); }

  //----- metody
  void load (char *file_name);
  void convert (Tool &t);

private:

  friend class  Uniput;
  friend class  Spr;

#pragma pack (push,spr_data,1)
  struct Data
  {
    int           owners;
    Data          *prev,*next;
    Phase         *phase;
    int           phases;
  };
#pragma pack (pop,spr_data)
  Data *data;
  static Data first;

  Phase         *phase;
  Phase &poper (int ph);
  static void clone (const Sprite *dst, const Sprite *src);
};

//--- ekrany ----------------------------------------------------------------

class Screen: public Heap_object
{
public:

  //----- zmienne
  unsigned char *adr;
  int type,ink,crsx,crsy;
  int sx,sy,llen,ox,oy;
  int real_sx,real_sy,real_llen,real_ox,real_oy;
  int debug_origins;

  Screen *onscreen;
  int onx,ony;

  //----- inicjalizacje
  Screen () {}
  Screen (unsigned char *mem, int sx, int sy, int llen, int type=0)
    { init (mem,sx,sy,llen,type); }
  Screen (Screen &scr, int x, int y, int sx, int sy, int type=0)
    { init (scr,x,y,sx,sy,type); }
  void init (unsigned char *mem, int sx, int sy, int llen, int type=0);
  void init (Screen &scr, int x, int y, int sx, int sy, int type=0);

  //----- funkcje
  static inline int convert (int v, int t) { return((v<<1)>>convert_accelerator[t>>8]); }
  static inline int unconvert (int v, int t) { return((v<<convert_accelerator[t>>8])>>1); }
  inline int convert (int value) { return(convert(value,type));};
  static int mem_required (int llen, int sy, int type);

  //----- metody modyfikacji wlasciwosci ekranu
  void origin (int ox, int oy);
  void reinit (int type=0);
  //----- metody kopiowania ekranu
  void copy (Screen &dst, int x=0, int y=0);
  void copy (int key_color, Screen &dst, int x=0, int y=0);
  //----- metody uzywania sprajtow
  void put (int x, int y, Sprite &s,
    int p=0, int function=Spr::nomirror, void *with=NULL);
  void shape (int x, int y, Sprite &s,
    int p=0, int function=Spr::nomirror, void *with=NULL);
  unsigned char *check (int x, int y, Sprite &s,
    int p=0, int function=Spr::nomirror, int mask=0xff);
  //----- metody rysowania na ekranie
  void plot (int color, int x, int y);
  void plotnc (int color, int x, int y);
  int  check (int x, int y, int mask=0xff);
  int  checknc (int x, int y, int mask=0xff);
  void rectangle (int color,int x,int y,int sx,int sy);
  void rectangle (Tool &t, int x, int y, int sx, int sy);
  //----- metody pisania stringow
  void putstr (int x, int y, unsigned char *c);
  void print (int x, int y, char *c, ...);
  void print (char *c, ...);
  //----- metody pozostale
  void cls (int screen_color=-1);
  void capture (void);

private:
  friend class Spr;
  static int convert_accelerator[];
};

//--- manager kolizji --------------------------------------------------------

class Cmanager: public Heap_object
{
public:

  //----- zmienne
  char *name;
  Screen *world;

  //----- inicjalizacje
  Cmanager (char *class_name="cmanager");
  ~Cmanager ();
  void init (Screen &cworld, int max_objects, char *class_name="cmanager");
  void free (void);

  //----- metody zapisu i badania kolizji
  void write (int x, int y, void* object, int priority=0);
  void write (int x, int y, Sprite &s, int p, int m, void* object, int priority=0);
  void *check (int x, int y);
  void **check (int x, int y, Sprite &s, int p, int m);
  //----- metody pozostale
  void origin (int ox, int oy);
  void reset (void);

private:

  void **buf;
  unsigned char *pbuf;
  int num,max,max_written,overflow_flag;
};

//--- karta video ------------------------------------------------------------

class Video
{
public:

  //----- zmienne

  static int mode_flag, current_mode, next_mode, frame_rate;
  static int min_gamma, max_gamma, gamma;
  static int min_color, max_color;
  static int work_mode, upside_down, use_320x200, use_640x400;
  static Color palette [256], vpalette [10][256];
  static int vpalette_num;
  static HINSTANCE dd_hinstance;

  //----- metody operacji na palecie
  static void load_palette (char *name=NULL, int update_tsp=1);
  static void update_palette (int new_gamma);
  static void update_palette (void) { update_palette(gamma); }
  static void select_vpalette (int num);
  static int  closest (Color &c);
  static int  closest (int r, int g, int b) {return closest(Color(r,g,b));}
  //----- metody wspolpracy z ekranami
  static Screen *get_screen (void);
  static void release_screen (void);
  static void set_mode (int mode_flag);
  static void show_screen (void);
  static void clear_screen();
  //----- metody tylko dla bibliotek
  static void display_message (char *c, ...);
  static void restore_background (void);
  static void progress_bar (int current, int total);

private:

  friend class Spr;
  static int  initialized;
  static void init (void);
  static void quit (void);
  static void free_paltabs (void);
  static int window_proc (int *result, HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
};

struct Spr_error
{
  enum
  {
    ddraw_not_installed=300,
    ddraw_failure,
    ddraw_cant_set_320x200,
    ddraw_cant_set_320x240,
    ddraw_cant_set_640x400,
    ddraw_cant_set_640x480,
  };
};

//----------------------------------------------------------------------
// LEVEL
//----------------------------------------------------------------------

#pragma pack (push,lev_lobject,1)
class Lobject
{
public:

  /* 4 */ int            x;                     // pozycja x na poziomie
  /* 4 */ int            y;                     // pozycja y na poziomie

  /* 2 */ unsigned short sprite;                // numer sprajta
  /* 2 */ unsigned short phase;                 // faza sprajta
  /* 2 */ unsigned short mirror  : 2;           // ..............xx   lustro sprajta
  /*   */ unsigned short type    : 12;          // ..xxxxxxxxxxxx..   numer typu
  /*   */ unsigned short special : 1;           // .x..............   flaga uzytkownika
  /*   */ unsigned short active  : 1;           // x...............   flaga aktywnosci

  /* 2 */ short          status;                //
  /* 2 */ short          aux1;                  // do wykorzystania przez
  /* 2 */ short          aux2;                  // uzytkownika biblioteki

  union
  {
    struct
    {
  /* 8 */ char           data[8];
    };
    struct
    {
  /*   */ void          *pointer;
  /*   */ int            value;
    };
    struct
    {
  /*   */ short          short1;
  /*   */ short          short2;
  /*   */ short          short3;
  /*   */ short          short4;
    };
  } user;

  /* 2 */ unsigned short link;                  // link do nastepnego i poprzedniego
  /* 2 */ unsigned short reserved;              // zarezerwowane na przyszlosc
};
#pragma pack (pop,lev_lobject)

class Buildable;

class Level: public Heap_object
{
#pragma pack (push,lev_reg,1)
  struct Regnode { unsigned short next,prev; int handle; };
  struct Reginfo { unsigned short first; char maxl,maxu; };
#pragma pack (pop,lev_reg)

  Regnode *regnode;

protected:

  Lobject *object;                      // tablica elementow poziomu

public:

  int ok;
  void free (void);
  int used_memory;         

  enum
  {
    max_sx         = 8192,              // szerokosc planu
    max_sy         = 8192,              // wysokosc planu
    max_planes     = 10,                // 9 planow (0 nie uzywany)
    max_levels     = 10,                // 9 podpoziomow (0 nie uzywany)
    max_size       = 65535,             // max ilosc elementow (0 nie uzywany)
    max_sprites    = 1024,              // max ilosc grup sprajtow
    max_phases     = 1024,              // max ilosc faz w jednej grupie
    max_types      = 1024,              // max ilosc typow
    max_typename   = 24,                // max dlugosc nazwy typu
    max_sprname    = 24,                // max dlugosc nazwy sprajta
    max_buf_planes = 32,                // na tyle planow mozna spojrzec
    max_regnumx    = 64,
    max_regnumy    = 64,
    max_regnum     = max_regnumx*max_regnumy
  };

  int   size;                           // ilosc elementow poziomu
  int   defined[max_levels];            // aby uzyc trzeba zdefiniowac
  int   lloaded[max_levels];            // podpoziomy zadeklarowane do uzycia
  int   active [max_levels];            // aktywny plan dla kazdego poziomu
  int   homex  [max_levels];            // poczatek poziomu x
  int   homey  [max_levels];            // poczatek poziomu y
  Color backcolor;                      // kolor tla do cls
  int   main_level;                     // glowny poziom

  Lobject &operator[] (int obj_num)
  {
    DBG_CHECK((obj_num>0)&&(obj_num<size));
    return(object[obj_num]);
  }

  class Levsprite
  {
    friend class Level;
    Sprite sprite[max_sprites];
    Sprite &soper (int num);

  public:
    enum {if_used, loaded, always, led_only};
    unsigned char is_loaded[max_sprites];

#if HI_DEBUG
    Sprite &operator[] (int num) { return(soper(num)); }
#else
    Sprite &operator[] (int num) { return(sprite[num]); }
#endif
  };

  Levsprite sprite;                     // tablica sprajtow poziomu
  int sprites_num;                      // ilosc wczytanych sprajtow

  struct
  {
    int begin;                          // od ktorego elementu zaczyna sie plan
    int size;                           // ilosc elementow planu
    float mulx, muly;                   // mnoznik (plany szybsze i wolniejsze)
    int regsx,regsy;                    // wielkosc regionu
    int ox, oy;                         // origin
    int gridx,gridy;                    // rozmiar siatki
    int reserved;                       // ilosc elementow rezerwowanych
    Reginfo *reginfo;
  } plane [max_levels][max_planes];

  void update (int num);                // przydziela obiekt do regionu
  int  oplane (int num);                // zwraca numer planu obiektu
  int  olevel (int num);                // zwraca numer poziomu obiektu
  int  adr2num (Lobject *l) { return(int(l-object));}

  char  type      [max_types][max_typename+1]; // nazwa typu
  short status    [max_types];          // statusy dla wszystkich typow
  short aux1      [max_types];          // aux1 dla wszystkich typow
  short aux2      [max_types];          // aux2 dla wszystkich typow
  int   deftype   [max_sprites];        // domysle typy dla nowych sprajtow
  int   types_num;                      // ilosc zaladowanych typow

  int   typenum (char *type_name);
  //
  // zwraca numer typu type_name lub 0 jesli takiego typu nie ma
  // _nalezy_ zdefiniowac jako typ #0 typ, ktorego sie nie uzywa (np. UNUSED)

  Buildable *builder [max_types];
  void  build (int num, int l, int p);

  int   bregister (char *type_name, Buildable &buildable);
  //
  // rejestruje builder dla typu type_name. nie da sie zarejestrowac typu 0 !!!
  // akceptuje wildcardy (?*), zwraca numer typu lub 0 w wypadku jego braku

  void  load (char *name, char *l2load="123456789");

  char *name; // nazwa podana do load()
  char palette_name[max_sprname+1];

  void look_at (int x, int y, int sx, int sy, char *gplanes="1123456789",
    int byes=1, int oyes=1);
  //
  // pierwszy znak planes to numer poziomu, nastepne to numery planow
  // jesli w stringu wystapi '*' to nastepny znak to znowu numer poziomu
  // jesli bedzie spacja (' ') to ten plan w lbuf nie bedzie wypelniony
  // domyslnie funkcja daje wszystkie plany poziomu pierwszego
  // 'byes' decyduje czy wolac buildery, 'oyes' czy pobierac do bufora

  void look_atnb (int x, int y, int sx, int sy, char *gplanes="1123456789")
    { look_at(x,y,sx,sy,gplanes,0,1); } // daje obiekty, nie wywoluje builderow
  

  void look_atno (int x, int y, int sx, int sy, char *gplanes="1123456789")
    { look_at(x,y,sx,sy,gplanes,1,0); } // nie daje obiektow, jedynie wola buildery

#pragma pack (push,lev_plane_buf,1)
  struct plane_buf
  {
    unsigned short *object;             // numery obiektow zakonczone przez 0
    short orgx;                         // dodac do x aby putnac sprajta
    short orgy;                         // dodac do y aby putnac sprajta
    unsigned short &operator[] (int num) {return(object[num]);}
  };
#pragma pack (pop,lev_plane_buf)

  plane_buf buf[max_buf_planes];        // pobrany bufor poziomu

  Level();
  ~Level();

  static void (*progress) (int percent);
};

class Buildable: public Heap_object
{
  friend class Level;
  static int bo_mustupdate;

protected:

  // ponizsze zmienne sa poprawnie zainicjalizowane _JEDYNIE_ wewnatrz
  // metody builder() (w momencie jej wykonywania)
  //
  static Lobject *bo;           // wskaxnik do samego obiektu
  static int      bo_num;       // numer obiektu
  static Sprite  *bo_spr;       // wskaxnik do sprajta obiektu
  static int      bo_plane;     // plan na ktorym znajduje sie obiekt
  static int      bo_level;     // podpoziom na ktorym znajduje sie obiekt
  static Level   *bo_master;    // wskaxnik do poziomu zawierajacego obiekt

  inline void  bo_setx        (int val);
  inline void  bo_sety        (int val);
  inline void  bo_setsprite   (int val);
  inline void  bo_setphase    (int val);
  inline void  bo_settype     (int val);
  inline void  bo_setlink     (int val);
  inline void  bo_setstatus   (int val);
  inline void  bo_setaux1     (int val);
  inline void  bo_setaux2     (int val);
  inline void  bo_setmirror   (int val);
  inline void  bo_setspecial  (int val);
  inline void  bo_setactive   (int val);

  inline int   bo_x           (void) { return((int)bo->x); }
  inline int   bo_y           (void) { return((int)bo->y); }
  inline int   bo_sprite      (void) { return((int)bo->sprite); }
  inline int   bo_phase       (void) { return((int)bo->phase); }
  inline int   bo_type        (void) { return((int)bo->type); }
  inline int   bo_link        (void) { return((int)bo->link); }
  inline int   bo_status      (void) { return((int)bo->status); }
  inline int   bo_aux1        (void) { return((int)bo->aux1); }
  inline int   bo_aux2        (void) { return((int)bo->aux2); }
  inline int   bo_mirror      (void) { return((int)bo->mirror); }
  inline int   bo_special     (void) { return((int)bo->special); }
  inline int   bo_active      (void) { return((int)bo->active); }

  inline int   bo_phasesnum   (void) { return((int)bo_spr->phases); }
  //
  // daje ilosc faz sprajta obiektu

public:

  int bregister (char *type_name, Level &inlevel) {
    return inlevel.bregister(type_name,*this); }
  //
  // rejestracja buildera w levelu

  virtual void builder (void) {}
  //
  // level.update(bo_num) nastepuje AUTOMATYCZNIE (jedynie w razie
  // koniecznosci) po zakonczeniu obslugi buildera.

};

// ----------------------------------------------------------------------
//  dalej sa juz niezbyt istotne z punktu widzenia programisty definicje
// ----------------------------------------------------------------------

inline void Buildable::bo_setx       (int val) {
  DBG_CHECK((val>=0)&&(val<Level::max_sx));
  bo->x=val; bo_mustupdate++; }

inline void Buildable::bo_sety       (int val)  {
  DBG_CHECK((val>=0)&&(val<Level::max_sy));
  bo->y=val; bo_mustupdate++; }

inline void Buildable::bo_setsprite  (int val)   {
  DBG_CHECK((val>=0)&&(val<bo_master->sprites_num));
  bo->sprite=(unsigned short)val; bo_mustupdate++; }

inline void Buildable::bo_setphase   (int val)   {
  bo->phase=(unsigned short)val; bo_mustupdate++; }

inline void Buildable::bo_settype    (int val)   {
  DBG_CHECK((val>=0)&&(val<bo_master->types_num));
  bo->type=(unsigned short)val; }

inline void Buildable::bo_setlink    (int val)  {
  DBG_CHECK((val>0)&&(val<bo_master->size));
  bo->link=(unsigned short)val; }

inline void Buildable::bo_setstatus  (int val)  {
  bo->status=(short)val; }

inline void Buildable::bo_setaux1    (int val)  {
  bo->aux1=(short)val; }

inline void Buildable::bo_setaux2    (int val)  {
  bo->aux2=(short)val; }

inline void Buildable::bo_setmirror  (int val)   {
  DBG_CHECK((val>=0)&&(val<4));
  bo->mirror=(unsigned short)val; bo_mustupdate++; }

inline void Buildable::bo_setspecial (int val)   {
  DBG_CHECK((val>=0)&&(val<2));
  bo->special=(unsigned short)val; }

inline void Buildable::bo_setactive  (int val)   {
  DBG_CHECK((val>=0)&&(val<2));
  bo->active=(unsigned short)val; bo_mustupdate++; }

#endif
