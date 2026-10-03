#ifndef __QMYWEAPN__
#define __QMYWEAPN__

class Wave;

//KLASA SWARMER
class Swarmer :
public virtual Object,
public virtual Posit,
public Sound,
public Move,
public Colis,
public Listmanager,
public Shadow
{
   friend class Weapon;
  private:
   enum {swarm_numb=4};
   int id;
   int number;
   int time;
   int life_time;
   int mode;
   int straight_angle;
   Wave *wave;
   LowColis *target;
  public:
   enum {INIT,SEEK,LOCKED,LOST};
   Swarmer(int _x,int _y,int _angle,int _id,int _number,int _straight_angle);
   virtual ~Swarmer(void) {}
   virtual void add_cmsg(Cmsg* message,int id);
   virtual void run(void);
};
//KLASA MYBOMB
class MyBomb :
public virtual Object,
public virtual Posit,
public LowColis,
public Visible,
public Listmanager,
public Sound
{
  private:
   int id;
   int time;
   int size;
   int flashing[NoNet::max_users+1];
   int done;
   int bigflash;
   int smlflash;
   struct Spark
   {
     short x;
     short y;
     char  phase;
   } buffer[70];
  public:
   MyBomb(int _x,int _y,int _id);
   virtual ~MyBomb(void) {}
   virtual void run(void);
   virtual void draw_object(Screen& screen,int plane);
};
//KLASA MYROCKET
class MyRocket :
public virtual Object,
public virtual Posit,
public Sound,
public Move,
public Colis,
public Listmanager,
public Visible
{
  private:
   int id;
   int life_time;
   int idle_time;
   int mode;
   Wave *wave;
  public:
   MyRocket(int _x,int _y,int _angle,int _id,int _mode=0);
   virtual ~MyRocket(void) {}
   virtual void add_cmsg(Cmsg* message,int id);
   virtual void run(void);
   virtual void draw_object(Screen& screen,int plane);
};
//CHGUN
const int MAX_CHGUNDOTT=20;
const int MAX_VDY=20;

class ChGun:
public virtual Object,
public Listmanager,
public LowColis,
public Sound,
public Visible
{
  protected:
   struct Wektor
   {
     int x,y,vx,vy,time;
     char dt;
   } body[MAX_CHGUNDOTT];
   int stx,sty;
   int angle,done,id;
   void init(int _x,int _y);
   void init_bullet(void);
  public:
   ChGun(int _x,int _y,int _angle,int _destroy,int _id);
   virtual ~ChGun(void){}
   virtual void run (void);
   virtual void draw_object(Screen&,int plane);
   static int vdy[MAX_VDY];
};
//KLASA SHOOTGUN
class ShootGun:
public virtual Object,
public virtual Posit,
public Listmanager,
public Sound,
public LowColis
{
  protected:
   int angle,done,id;
  public:
   ShootGun(int _x,int _y,int _angle,int _id);
   virtual ~ShootGun(void){}
   virtual void run (void);
};
//KLASA LASER
const int MAX_LASEROFFSET=12;
class Laser :
public virtual Object,
public virtual Posit,
public Move,
public Colis,
public Listmanager,
public Sound,
public Visible
{
  private:
   int id;
   int time;
   int sstop;
   int type;
   int x1,x2,y1,y2;
   int life_time;
   int index;
   static int  move_table[MAX_LASEROFFSET];
  public:
   enum {MY,ALIEN};
   Laser(int _x,int _y,int _vx,int _vy,int _angle,int _id);
   virtual ~Laser(void) {}
   virtual void run(void);
   virtual void add_cmsg(Cmsg* message,int id);
   virtual void draw_object(Screen& screen,int plane);
   virtual void draw_shadow(Screen& screen,int plane);
};

//KLASA WAVE
const int MAX_WAVE=50;
class Wave:
public virtual Object,
public virtual Posit,
public Sound,
public Move,
public Listmanager,
public Visible
{
  private:
    struct Wkt
    {
      short x,y;
      char phase;
    };
    int id;
    int Lx,Ly;
    int last;
    int time,sttime;
    int del;
    int addtime,type;
    Sprite*sprite;
    Wkt table[MAX_WAVE];
    void add_time(void);
    void add(void);
  public:
   Wave(int _x,int _y,int _angle,Sprite& spr,int _type=0,int _sttime=20);
   virtual ~Wave(void){}
   virtual void run(void);
   void set(int _x,int _y,int _del){x=_x;y=_y;del=_del;}
   void set(int _x,int _y) {x=_x;y=_y;}
   void kill(void) {KILLME}
   virtual void draw_object(Screen& screen,int plane);
};
//KLASA SPLASH
class Splash :
public virtual Object,
public virtual Posit,
public Sound,
public Move,
public Colis,
public Listmanager,
public Visible
{
  private:
   int id;
   int life_time;
   int mode;
  public:
   Splash(int _x,int _y,int _angle,int _id,Sprite& _sprite,int _mode=0);
   virtual ~Splash(void) {}
   virtual void add_cmsg(Cmsg* message,int id);
   virtual void run(void);
   static void fire(int);
   virtual void draw_object(Screen& screen,int plane);
};
//MYMINE
class Mymine:
public virtual Object,
public virtual Posit,
public Sound,
public Move,
public Colis,
public Listmanager,
public Shadow
{
  protected:
   int id;
   int time;
   void wall(void);
  public:
   Mymine(int _x,int _y,int _vx,int _vy,int _angle,int _id);
   virtual ~Mymine(void){}
   virtual void run(void);
   virtual void add_cmsg(Cmsg* message,int id);
};

//KLASA MINE
class NetMine :
public virtual Object,
public virtual Posit,
public Sound,
public Shadow,
public Move,
public Colis,
public Life,
public Listmanager
{
  protected:
   int flash;
   int counter;
   int range;
   int tiku;
   int smpl,t;
   int id;
   int global_time,bomb_time,nonsmp;
   int check_obj(void);
   void kill(void);
   void serv_kill(void);
  public:
   NetMine(int _x,int _y,int _id);
   virtual ~NetMine(void){}
   virtual void run(void);
   virtual void draw_object(Screen& screen,int plane);
};

//WEAPON
class Weapon :
public virtual Object,
public virtual Posit,
public Turn,
public Sound,
public Visible
{
  public:
   enum ServMode {
                   IDLE,
                   READY,
                   FIRING,
                   GOINGOFF,
                   GOINGON,
                   MORPHING,
                 } mode;
  private:
   User *myuser;
   int id;
   int weap_freq[6];
   int weap_stat;
   int weap_in_use;
   int tower_height;
   int ship_radius;
   int ship_flipper;
   int vx;
   int vy;
   int dir;
   int ship_angle;
   int morphing_phase;
   int morph_ranges[3];
   int weap1_flipper;
   int cannon_fire;
   int last_angle;
   int display_message;
   int out_of_ammo_played;
   int preffered_angle(void);
   int auto_change_weapon(void);
   int morph(void);
   void update_sprite_range(void);
   void fire_service(void);
   void time_service(void);
   void reset_time_service(void);
  public:
   Weapon(int _x,int _y,User* _user,int _id);
   void run(int _x,int _y,int _vx,int _vy,int _dir,int _angle,int _fire,Myship::ServMode _ship_mode);
   void draw_object(Screen& screen,int plane,int flash,int cloak,Tool *net_tool);
   void reset(void);
   virtual void run(void);
   virtual void draw_object(Screen& screen,int plane);
};

inline void Weapon::run(void)
{
#if HI_DEBUG
  FAILURE("Weapon::run(void) - you shouldn't have called this routine.");
#endif
}

inline void Weapon::draw_object(Screen& screen,int plane)
{
  (void)screen;
  (void)plane;
#if HI_DEBUG
  FAILURE("Weapon::draw_object(Screen& screen,int plane) - you shouldn't have called this routine.");
#endif
}

#endif



