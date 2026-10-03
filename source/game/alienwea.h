#ifndef __QAWEAPON__
#define __QAWEAPON__

//KLASA ROCKET
class Rocket :
public virtual Object,
public virtual Posit,
public Sound,
public Listmanager,
public Visible,
public Move,
public Colis
{
  private:
   Myship *ship;
   Wave *wave;
   int time,stime;
   int Sx,Sy;
   int X,Y;
   int plane;
  public:
   Rocket(int sx,int sy,int _x,int _y,int _angle,Op::ObjPack params,int force,int _plane=-1);
   virtual ~Rocket(void) {}
   virtual void run(void);
   virtual void draw_object(Screen& screen,int plane);
   virtual void add_cmsg(Cmsg* message,int id);
};

//KLASA BULLET
class Bullet :
public virtual Object,
public virtual Posit,
public Sound,
public Listmanager,
public Visible,
public Move,
public Colis
{
  protected:
    int X,Y;
    int time,sangle;
    Wave *wave;
  public:
   Bullet(int _x,int _y,int _angle,int _speed,int _force);
   virtual ~Bullet(void) {}
   virtual void run(void);
   virtual void draw_object(Screen& screen,int plane);
   virtual void add_cmsg(Cmsg* message,int id);
};
//KLASA ASPLASH
class ASplash :
public virtual Object,
public virtual Posit,
public Sound,
public Move,
public Colis,
public Listmanager,
public Visible
{
   int X,Y;
    int plane;
  public:
   ASplash(int _sx,int _sy,int _x,int _y,int _angle,Op::ObjPack&,int force,Sprite& _sprite,int _plane=0);
   virtual ~ASplash(void) {}
   virtual void run(void);
   virtual void add_cmsg(Cmsg* message,int id);
};
//PLASK
class Plask:
public virtual Object,
public Listmanager,
public LowColis,
public Visible
{
  protected:
   struct Wektor : public Heap_object
   {
     int x,y,vx,vy,time;
   } body[8];
   int x,y,angle,done;
   void init(int _x,int _y);
  public:
   Plask(int _x,int _y,int _angle,int _destroy);
   virtual ~Plask(void){}
   virtual void run (void);
   virtual void draw_object(Screen&,int plane);
};
//ALIENCHGUN
const int MAX_ALIENCHGUNBODY=25;

class AlienChgun:
public virtual Object,
public Listmanager,
public LowColis,
public Visible
{
  protected:
   struct Wektor : public Heap_object
   {
     int x,y,vx,vy,time;
   } body[MAX_ALIENCHGUNBODY];
   int x,y,angle,done;
   void init(int _x,int _y);
  public:
   AlienChgun(int _x,int _y,int _angle,int _destroy);
   virtual ~AlienChgun(void){}
   virtual void run (void);
   virtual void draw_object(Screen&,int plane);
};
//MORPH
class Morph:
public virtual Object,
public virtual Posit,
public Move,
public Colis,
public Listmanager,
public Shadow
{
  protected:
   int time,mtime,blc,type;
   int phs,rphase,bo_num;
   void wall(void);
   int shangle;
  public:
   Morph(int _x,int _y,int angle,int bo_num,int _type=1);
   virtual ~Morph(void) {}
   virtual void run(void);
   virtual void draw_object(Screen& screen,int plane);
   virtual void draw_shadow(Screen& screen,int plane);
};

#endif



