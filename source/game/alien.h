#ifndef __ALIEN_S__
#define __ALIEN_S__

//TARAN
class Taran:
public virtual Object,
public virtual Posit,
public Move,
public Shadow,
public Colis,
public virtual Life,
public Listmanager,
public Radar,
public Sound,
public Handle,
public Impexp
{
  protected:
   enum{START,SEEK,ATTACK,WESCAPE,DISABLE,DISP};
   Myship *ship;
   int time;
   int order;
   int unblock,invisible;
   int kick,kick_angle;
   int angle_rand,type,global_time,counter;
   AlienSmoke *smoke;
   void wall(void);
   void update_posit(void);
  public:
   Taran(int _x,int _y,int lev,int _chl=Posit::DOWN,int _type=0,int _invisible=0);
   virtual ~Taran(void);
   virtual void run(void);
   virtual void HandleEvent(int);
   virtual void draw_object(Screen& screen,int plane);
   virtual void draw_shadow(Screen& screen,int plane);
};
//SIMPLE1
class Simple1:
public virtual Object,
public virtual Posit,
public Move,
public Shadow,
public Colis,
public virtual Life,
public Listmanager,
public Radar,
public Sound,
public Handle,
public Impexp
{
  protected:
   enum {START,GO,WESCAPE,DISABLE};
   int time,shtime;
   int order;
   int unblock,invisible;
   int shcount,phs1,phs2,constshcount;
   int pangle;
   int tx,ty,type;
   Myship *ship;
   AlienSmoke *smoke;
   void bang(void);
   void wall(void);
   void turn(void);
   void update_posit(void);
  public:
   enum {NORMAL,SPECIAL};
   Simple1(int _x,int _y,int lev,int _type,int _invisible=0);
   virtual ~Simple1(void);
   virtual void run(void);
   virtual void HandleEvent(int);
   virtual void draw_object(Screen& screen,int plane);
   virtual void draw_shadow(Screen& screen,int plane);
};
//SIMPLE3
class Simple3:
public virtual Object,
public virtual Posit,
public Move,
public Shadow,
public Colis,
public virtual Life,
public Listmanager,
public Radar,
public Sound,
public Handle,
public Impexp
{
  protected:
   enum {START,GO,WESCAPE,DISABLE};
   int time,shtime;
   int order;
   int unblock,invisible;
   int shcount,phs1,phs2,constshcount;
   int pangle;
   int tx,ty,type;
   Myship *ship;
   AlienSmoke *smoke;
   void bang(void);
   void wall(void);
   void turn(void);
   void update_posit(void);
  public:
   enum {NORMAL,SPECIAL};
   Simple3(int _x,int _y,int lev,int _invisible=0);
   virtual ~Simple3(void);
   virtual void run(void);
   virtual void HandleEvent(int);
   virtual void draw_object(Screen& screen,int plane);
   virtual void draw_shadow(Screen& screen,int plane);
};
//MBOSS1
class MBoss1:
public virtual Object,
public virtual Posit,
public Move,
public Shadow,
public Colis,
public virtual Life,
public Listmanager,
public Sound,
public Radar,
public Handle,
public Impexp
{
  protected:
   enum {START,GO,ATTACK,WESCAPE};
   int time,shtime,frqtime;
   int order,lorder;
   int obj_mission;
   int tx,ty;
   int pangle;
   int counter;
   int unblock;
   int power,shcounter,invisible;
   int type;
   int chgun,rocketside;
   int strafe_side;
   int appear_snd;
   AlienSmoke *smoke1;
   AlienSmoke *smoke2;
   Myship *ship;
   void wall(void);
   void turn(void);
   void app_point(void);
   void phs_turn(void);
   void kill(void);
   void bang(void);
   void bang1(void);
   void bang2(void);
   void bang3(void);
   void bang4(void);
   void update_posit(void);
   void appear(void);
  public:
   MBoss1(int _x,int _y,int lev,Sprite& _sprite,int _type,int _visible,int obj);
   virtual ~MBoss1(void){}
   virtual void run(void);
   virtual void HandleEvent(int);
   virtual void draw_object(Screen& screen,int plane);
   virtual void draw_shadow(Screen& screen,int plane);
};
//KLASA MINE
class Mine :
public virtual Object,
public virtual Posit,
public Sound,
public Shadow,
public Move,
public Colis,
public Life,
public Listmanager,
public Impexp
{
  protected:
   int flash;
   int counter;
   int range;
   int tiku;
   int smpl,t;
   int type;
   int global_time,bomb_time,nonsmp;
   void update_posit(void);
   int check_obj(void);
   void kill(void);
   void serv_kill(void);
  public:
   enum{STATIC,TRACER,MYMINE};
   Mine(int _x,int _y,int _chl,Sprite& _sprite,int _level_num,int _type);
   virtual ~Mine(void);
   virtual void run(void);
   virtual void draw_object(Screen& screen,int plane);
};

//KLASA ACANNON
class ACannon:
public virtual Object,
public virtual Posit,
public Turn,
public Visible,
public Colis,
public Listmanager,
public Life,
public Sound,
public Handle,
public Impexp
{
  protected:
   enum{GO,FIRE};
   enum{TRACE,CIRCLEL,CIRCLER,STOP};
   int order;
   int spx,spy;
   int time;
   int shtime;
   int fr_shoot;
   int shoot,plane,startangle;
   Myship *ship;
   AlienSmoke* smoke;
   Op::CannonPack* param;
   int smoketime;
   int global_time;
   void bang(void);
   void serv_kill(void);
   void service(void);
  public:
   ACannon(int _x,int _y,Sprite& _sprite,int _level_num,int _plane,Op::CannonPack *_param);
   virtual ~ACannon(void){}
   virtual void run (void);
   virtual void HandleEvent(int);
   virtual void draw_object(Screen& screen,int plane);
};

class Raft:
public virtual Object,
public virtual Posit,
public Move,
public Visible,
public Colis,
public Listmanager,
public virtual Life,
public Sound,
public Radar,
public Impexp
{
  protected:
    int time;
    int id;
  public:
   Raft(int _x,int _y,Sprite& _sprite,int _level_num,int _id);  
   ~Raft(void) {}
   virtual void run(void);
};

//KLASA BUOY
class Buoy :
public virtual Object,
public virtual Posit,
public Sound,
public Visible,
public Colis,
public Life,
public Listmanager,
public Impexp
{
  protected:
   int time;
   int check_obj(void);
   void kill(void);
   void serv_kill(void);
  public:
   Buoy(int _x,int _y,Sprite& _sprite,int _level_num,World*);
   virtual ~Buoy(void) {}
   virtual void run(void);
};

class Thief:
public virtual Object,
public virtual Posit,
public Move,
public Shadow,
public Colis,
public Listmanager,
public virtual Life,
public Sound,
public Radar,
public Impexp
{
  protected:
   int time,link;
   AlienSmoke *smoke;
  public:
   Thief(int _x,int _y,Sprite& _sprite,int _level_num);
   ~Thief(void) {}
   virtual void run(void);
};
//KLASA SHIPFUCKER
class ShipFucker
{
   static int initialized;
  public:
   static void init(void);
   static void quit(void);
   static void start(void);
   static void stop(void);
};

#endif
