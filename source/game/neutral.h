#ifndef __QNOBJECT__
#define __QNOBJECT__

//KLASA EXPLODE
//Klasa ta jest reprezentacja eksplozji.
class Splinters;

class Explode :
public virtual Object,
public virtual Posit,
public Sound,
public Listmanager,
public Visible
{
  private:
   int step;
   int trace;
   int time;
   int back;
   Tool* tool;
   Splinters* splptr;
   int flag;
   int type;
   int phsair;
   int ratio1,ratio2;
  public:
   enum{NOLIGHT=0,NORMALL,TSP,AIR};
   enum {DISABLE=0,ENABLE};
   Explode(int _x,int _y,int _chl,int splinter,int _type,Sprite &expspr,int force,int sound=1);
   virtual ~Explode(void){}
   virtual void run(void);
   virtual void draw_object(Screen&,int plane);
};

#define E_BIG    NEW(Explode(x,y,get_chl(),8,Explode::NORMALL,Mysprites::expl_dest,Explode::ENABLE,1),gobjects_mbn)
#define E_MED    NEW(Explode(x,y,get_chl(),8,Explode::NORMALL,Mysprites::expl_dest_small,Explode::ENABLE,1),gobjects_mbn)
#define E_AIR    NEW(Explode(x,y,get_chl(),8,Explode::AIR,Mysprites::expl_air,Explode::ENABLE,1),gobjects_mbn)
#define E_SMALL  NEW(Explode(x,y,get_chl(),0,Explode::NOLIGHT,Mysprites::expl_rocket,Explode::DISABLE,1),gobjects_mbn)
#define E_SMOKE  NEW(Explode(x,y,get_chl(),0,Explode::TSP,Mysprites::expl_mine,Explode::ENABLE,1),gobjects_mbn)
#define E_ROCKET NEW(Explode(x,y,Posit::UP,0,Explode::NOLIGHT,Mysprites::expl_rocket,Explode::DISABLE,1),gobjects_mbn)

//FIRE
class Fire:
public virtual Object,
public Listmanager,
public Sound,
public Visible
{
  private:
   struct Pair
   {
     int x,y,count;
   } *explode;
   int wx,wy;
   int time;
  public:
   Fire(void);
   virtual ~Fire(void);
   virtual void run(void);
   virtual void draw_object(Screen&,int plane);
};
//KLASA BOOM
class Boom :
public virtual Object,
public virtual Posit,
public Sound,
public Listmanager,
public Visible
{
  private:
   int dtime;
  public:
   Boom(int _x,int _y,int _chl);
   virtual ~Boom(void) {}
   virtual void run(void);
};
//SPLINTERS
const int MAX_SPL=35;
const int MAX_INITSPL=200;

class Splinters :
public virtual Object,
public virtual Posit,
public Listmanager,
public Shadow
{
   friend class Stale_memory;  // port: reads what the original left in memory (stale.h)
  protected:
   int killflag;
   int counter;
   struct Spl
   {
     int x,y,h;
     short vx,vy;
     short time,vh;
     char phs,spr;
   };
   int flg;
   Spl table[MAX_SPL];
   static Spl init_table_down[MAX_INITSPL];
   static Spl init_table_up[MAX_INITSPL];
   static int initcounter;
   void add_splinter (void);
  public:
   Splinters(int _x,int _y,int _init,int chl);
   ~Splinters(void) {}
   static void init(void);
   virtual void run(void);
   virtual void draw_object(Screen&,int plane);
   virtual void draw_shadow(Screen& screen,int plane);
   void set (int _x,int _y) {x=_x;y=_y;}
   void kill (void) {killflag=1;}
};


//SPLINTER
class Splinter :
public virtual Object,
public virtual Posit,
public Move,
public Listmanager,
public Shadow
{
  protected:
   int time;
   int count;
   int stand;
  public:
   Splinter(int _x,int _y,int _chl,Sprite& _sprite,int _stand=0);
   virtual ~Splinter(void){}
   virtual void run(void);
   virtual void draw_object(Screen&,int plane);
   virtual void draw_shadow(Screen& screen,int plane);
};


//SMOKE
class Smoke:
public virtual Object,
public virtual Posit,
public Move,
public Listmanager,
public Visible
{
  protected:
   int time;
  public:
   Smoke(int _x,int _y,Sprite& _sprite);
   virtual ~Smoke(void){}
   virtual void run(void);
   virtual void draw_object(Screen&,int plane);
};
//SWITCH
class Switch :
public virtual Object,
public virtual Posit,
public Sound,
public Listmanager,
public Colis,
public Shadow,
public Impexp,
public Handle
{
  protected:
   int buttom;
   int btime;
  public:
   Switch(int _x,int _y,int _chl,Sprite& _sprite,int _level_num);
   virtual ~Switch(void){}
   virtual void run(void);
   virtual void HandleEvent(int message);
};
//UNBLOCK
class Unblock :
public virtual Object,
public virtual Posit,
public Listmanager,
public Visible,
public Colis,
public Radar,
public Impexp
{
  protected:
   int mission_obj;
   int first;
   int our;
   int change;
   int time;
   Myship* ship;
  public:
   Unblock(int _x,int _y,Sprite& sprite,int _level_num);
   virtual ~Unblock(void){}
   virtual void run(void);
};
//BACKGROUND
class Snow;

class Background :
public virtual Object,
public virtual Posit,
public Listmanager,
public Visible
{
   friend class Snow;
  protected:
   unsigned char *back;
   int xsize,ysize;
   int bx,by;
   int sx,sy;
   int wx,wy;
   float mx,my;
   void look_at(Screen &screen,int x,int y,int dx,int dy);
   void look_at(Screen &screen,int x,int y,int dx,int dy,Tool *tool);
  public:
   Background(void);
   virtual ~Background(void);
   virtual void run(void);
   virtual void draw_object(Screen&,int plane);
};
//KLASA FOG
class Fog:
public virtual Object,
public virtual Posit,
public Listmanager,
public Visible
{
  protected:
   int xsize,ysize;
   int wx,wy;
   float mx,my;
   int vx,vy,plane;
   int time,count;
   Tool *tools[30];
   int first;
  public:
   Fog(char* type,Level*);
   virtual ~Fog(void);
   virtual void run(void);
   virtual void draw_object(Screen&,int plane);
};
//NIGHT
class Night :
public virtual Object,
public Listmanager,
public Visible
{
  protected:
   Tool* tools[30];
   int  olevel;
   int  status;
   static int light_level;
   static int distance_param;
   static int light_mult;
  public:
   Night(char* type);
   virtual ~Night(void);
   virtual void run(void);
   virtual void draw_object(Screen&,int plane);
   static void add_light(int x,int y)
   {
     int t=vect_len(x-world->get_x()-Mp::SX/2,y-world->get_y()-Mp::SY/2);
     t=distance_param-t;
     if (t<0) t=0;
     light_level+=t;
   }
};
//RAIN
class Rain:
public virtual Object,
public Listmanager,
public Sound,
public Visible
{
  protected:
   int color;
   int wx,wy;
   struct Pair
   {
     int x,y,count;
   } *dott;
   int dottsize;
   struct Wkt
   {
    int x,y;
   } *move,*rnd;
   int movesize;
   int movesize2;
   int size;
   Tool* tool[2];
   int time,ltime,lflag,rand_time;
   void make_tool(void);
   void weather_service(void);
   int weather_level,ch_sign,counter;
   int weather_time,weather_change,weather_stay;
  public:
   Rain(int type);
   virtual ~Rain(void);
   virtual void run(void);
   virtual void draw_object(Screen&,int plane);

};
//SNOW
const int MAX_SNOWTOOL=32;

class Snow:
public virtual Object,
public Listmanager,
public Sound,
public Visible
{
   friend class Background;
   friend class World;
  protected:
   int color;
   int wx,wy;
   struct Pair
   {
     int x,y,count;
   } *dott;
   struct Wkt
   {
    int x,y;
   } *move,*rnd;
   int movesize;
   int movesize2;
   void weather_service(void);
   static int weather_time,weather_change;
   int time,ltime,lflag;
   static int tool_level;
   static int ch_sign;
   static int weather_stay,stay_time;
   static int weather_level;
   static int dottsize;
   static int tool_change;
   static int initialized;
   static Tool *tool[MAX_SNOWTOOL];
   static void init_tool(Text *text);
   static void draw_2plane(Screen& screen,Lobject *o);
   static void draw_snow(Screen& screen,Background* back,int x,int y);
  public:
   Snow(int);
   virtual ~Snow(void);
   virtual void run(void);
   virtual void draw_object(Screen&,int plane);
};
//CLOUDS
class Cloud:
public virtual Object,
public Listmanager,
public Visible
{
  private:
   struct Elm
   {
     int x,y,spr;
     float vx,vy;
   }* cloud;
   int overscan;
   int max,plane;
   int wx,wy;
   float mx,my;
   int count,time;
   int first;
  public:
   Cloud(char* type,Level*);
   virtual ~Cloud(void);
   virtual void run(void);
   virtual void draw_object(Screen&,int plane);
};
//DOOR
class Door:
public virtual Object,
public virtual Posit,
public Visible,
public Sound,
public Listmanager,
public Colis,
public Impexp,
public Handle
{
  private:
   int plane;
   int close;
   int blocked;
   int prepare;
   int time;
   int init;
   int unactive;
  public:
   Door(int _x,int _y,int _chl,Sprite& _sprite,int level_num,int _plane);
   virtual ~Door(void){}
   virtual void run(void);
   virtual void draw_object(Screen&,int plane);
   virtual void HandleEvent(int message);
};

class Destsmoke;

//DESTROY
class Destroy:
public virtual Object,
public virtual Posit,
public Shadow,
public Sound,
public Listmanager,
public Colis,
public Impexp,
public Handle,
public virtual Life,
public Radar
{
   friend class Stale_memory;  // port: reads what the original left in memory (stale.h)
  protected:
   int plane;
   int time;
   int blocked;
   int active;
   int fr_delay;
   int anim_phase;
   int first;
   int blow;
   int send;
   int object_mission;
   int smoketime;
   int global_timer;
   int morph_phase,morph_level,morph_time;
   int lmorph_phase1,lmorph_phase2,lmorph_level, lmorph_time;
   Destsmoke* smoke;
   int global_time;
   void servis_link(void);
  public:
   static int destcount;
   Destroy(int _x,int _y,int _chl,Sprite& _sprite,int _level_num,int _plane,int status=0);
   virtual ~Destroy(void);
   virtual void run(void);
   virtual void draw_object(Screen&,int plane);
   virtual void draw_shadow(Screen& screen,int plane);
   virtual void HandleEvent(int);
};

//MORPHDESTROY
class MorphDest:
public virtual Object,
public virtual Posit,
public Visible,
public Sound,
public Listmanager,
public Colis,
public Impexp,
public virtual Life,
public Radar
{
  protected:
   int time,mlevel;
   int object_mission;
   int plane;
   int prg,num;
  public:
   MorphDest(int _x,int _y,Sprite& _sprite,int _level_num,int _plane);
   ~MorphDest(void) {}
   virtual void run(void);
   virtual void draw_object(Screen&,int plane);
};

const int MAX_SMOKE=30;

//KLASA  ALIENSMOKE
const int MAX_ALIENWAVE=50;
class AlienSmoke:
public virtual Object,
public virtual Posit,
public Sound,
public Move,
public Listmanager,
public Visible
{
   friend class Stale_memory;  // port: reads what the original left in memory (stale.h)
  private:
    struct Wkt
    {
      short x,y,time;
      char phase;
    };
    int id;
    int Lx,Ly;
    int last;
    int time;
    int del;
    int power;
    int sstop;
    int plane;
    Sprite*sprite;
    Wkt table[MAX_ALIENWAVE];
    void add(void);
  public:
   AlienSmoke(int _x,int _y,int _angle,Sprite& spr,int _power,int _plane=-1);
   virtual ~AlienSmoke(void){}
   virtual void run(void);
   void set(int _x,int _y,int _del){x=_x;y=_y;del=_del;}
   void set(int _x,int _y) {x=_x;y=_y;}
   void set_power(int _pow) {power=_pow;}
   void stop(void) {sstop=1;}
   void start(void) {sstop=0;}
   void kill(void) {KILLME}
   virtual void draw_object(Screen& screen,int plane);
};

//Destsmoke
class Destsmoke:
public virtual Object,
public virtual Posit,
public Listmanager,
public Visible
{
   friend class Stale_memory;  // port: reads what the original left in memory (stale.h)
  protected:
   struct Wkt
   {
    short x,y;
    char level,time;
   };

   Wkt table[MAX_SMOKE];
   void add(void);
   int last,plane;
   int time,type;
  public:
   enum{BLACK=1,WHITE};
   Destsmoke(int _x,int _y,int _plane,int lev_num,int _type);
   virtual ~Destsmoke(void){}
   virtual void run(void);
   void set(int _x,int _y){x=_x;y=_y;};
   void kill(void);
   virtual void draw_object(Screen&,int plane);
};

//LEVEL_SMOKE
class LevelSmoke:
public virtual Object,
public virtual Posit,
public Listmanager,
public Impexp
{
  protected:
    int type,plane;
    Destsmoke*  big_smoke;
    AlienSmoke* small_smoke;
  public:
    enum {BIGW=1,BIGB,MEDW,MEDB,SMALL};
    LevelSmoke(int _x,int _y,int _level_num,int _type,int _plane);
    ~LevelSmoke(void) {}
    virtual void run(void);
};

//LIGHT
class Light:
public virtual Object,
public virtual Posit,
public Listmanager,
public Impexp,
public Visible
{
  protected:
   int power,plane;
  public:
   Light(int _x,int _y,int _plane,int _power,int lev_num);
   virtual ~Light(void){}
   virtual void run(void);
   virtual void draw_object(Screen&,int plane);
   static void Ray(Screen& screen,Sprite* spr,int x,int y,int power);
};
//COUNTDOWN
class Countdown :
public virtual Object,
public Listmanager,
public Visible
{
  protected:
   int time;
   int stime;
   int starttime;
   static int disable;
  public:
   Countdown(int _time);
   virtual ~Countdown(void){disable=0;}
   virtual void run(void);
   virtual void draw_object(Screen&,int plane);
   static void off(void){disable=1;}
};

//COLLECT
class Collect :
public virtual Object,
public virtual Posit,
public Visible,
public Sound,
public Listmanager,
public LowColis,
public Impexp,
public Radar
{
  protected:
   Myship* ship;
   int timer;
   int landtime;
   int ship_landing;
   int pipe_phs;
   int pipe_time;
   int coll_time;
   int Sx,Sy,snd,kill;
   int phs_target;
  public:
   Collect(int _x,int _y,Sprite& _sprite,int _level_num);
   virtual ~Collect(void){}
   virtual void run(void);
   virtual void draw_object(Screen&,int plane);
};

//KLASA WALL
class NormalWall;
class WeaponWall;
class KillerWall;
class Wall:
public virtual Object,
public Listmanager
{
  private:
   static int initialized;
   NormalWall *normal;
   WeaponWall *weapon;
   KillerWall *killer;
   void fill(int id,int x,int y,int sx,int sy);
  public:
   Wall(void);
   virtual ~Wall(void);
   virtual void add_cmsg(Cmsg* message,int id) {(void)id;(void)message;}
   virtual void run(void);
};
//KLASA NORMALWALL
class NormalWall:
public virtual Object,
public LowColis
{
   friend class Wall;
  private:
   NormalWall(void);
   virtual ~NormalWall(void) {}
   virtual void add_cmsg(Cmsg* message,int id) {(void)id;(void)message;}
   virtual void run(void);
   virtual void run(int id,int x,int y,Sprite& sprite,int phase,int mirror);
};

inline void NormalWall::run(void)
{
#if HI_DEBUG
  FAILURE("NormalWall::run(void) - you shouldn't have called this method.");
#endif
}
//KLASA WEPONWALL
//Klasa ta jest reprezentacja sciany przenikalnej dla strzalow.
class WeaponWall:
public virtual Object,
public LowColis
{
   friend class Wall;
  private:
   WeaponWall(void);
   virtual ~WeaponWall(void) {}
   virtual void add_cmsg(Cmsg* message,int id) {(void)id;(void)message;}
   virtual void run(void);
   virtual void run(int id,int x,int y,Sprite& sprite,int phase,int mirror);
};

inline void WeaponWall::run(void)
{
#if HI_DEBUG
  FAILURE("WeaponWall::run(void) - you shouldn't have called this method.");
#endif
}
//KLASA KILLERWALL
//Klasa ta jest reprezentacja sciany przenikalnej odbierajacej zywota.
class KillerWall:
public virtual Object,
public LowColis
{
   friend class Wall;
  private:
   KillerWall(void);
   virtual ~KillerWall(void) {}
   virtual void add_cmsg(Cmsg* message,int id) {(void)id;(void)message;}
   virtual void run(void);
   virtual void run(int id,int x,int y,Sprite& sprite,int phase,int mirror);
};

inline void KillerWall::run(void)
{
#if HI_DEBUG
  FAILURE("KillerWall::run(void) - you shouldn't have called this method.");
#endif
}
//KLASA FADE
class Fade
{
  private:
   enum {FADEIN,FADEOUT,IDLE,text_len=256};
   static int fade_time[NoNet::max_users+1];
   static int mode[NoNet::max_users+1];
   static int type[NoNet::max_users+1];
   static char text_buf[NoNet::max_users+1][text_len];
   static void _draw(Screen &screen,int id);
   static void _run(int id);
  public:
   enum {ONLYIN,ONLYOUT,BOTH};
   static void init(void);
   static void quit(void);
   static void start(int _type,char *text="",int id=0);
   static int  is_faded(int id=0);
   static int  in_use(int id=0);
   static void run(void);
   static void draw(Screen &screen);
};
//KLASA GENERATOR
class Generator:
public virtual Object,
public virtual Posit,
public Listmanager,
public Impexp
{
  protected:
   int time;
   int phs_time;
   int obj_count;
   int generate(void);
  public:
    Generator(int _x,int _y,Sprite& _sprite,int _level_num);
    ~Generator(void){initializing=0;}
    static int initializing;
    virtual void run(void);
};

class SplinterGen:
public virtual Object,
public virtual Posit,
public Visible,
public Sound,
public Listmanager
{
  protected:
    int time;
    int lev_num;
    int go;
  public:
    SplinterGen(int _x,int _y,Sprite& _sprite,int _level_num);
    ~SplinterGen(void){}
    virtual void run(void);
};

#define CINDERSCANNON  -6723
#define CINDERSPULSE   -6722

class Cinders:
public virtual Object,
public virtual Posit,
public Colis,
public Visible,
public Listmanager,
public Impexp
{
  protected:
   int time;
   int type;
   int plane;
   AlienSmoke* smoke;
  public:
   enum{PULSE=1,CANNON=2};
   Cinders(int _x,int _y,Sprite& _sprite,int _level_num,int type,int _plane);
   ~Cinders(void){}
   virtual void run(void);
};



#endif
