#ifndef __GOBJECT__
#define __GOBJECT__

//FUNCKJE
int sign(int a);
//KLASA RAND
#define RAND (Rand::rand(FILE_LINE))
class Rand
{
  private:
   enum {size=1024};
   static int initialized;
   static int data[size];
   static int indx;
   static int locked;
   static int debug_cnt;
   struct Data
   {
     enum {place_buffer=50};
     int frame;
     int value;
     int counter;
     int indx;
     char place[place_buffer];
   };
   static NoQueue<Data> *queue;
   static int rand(void);
  public:
   static void init(void);
   static void quit(void);
   static void reinit(void);
   static void dump(void);
   static void lock(void);
   static void unlock(void);
   static int rand(char *place);
   static int curr_indx(void);
};
//KLASA KILL
#define KILLME {Kill::log(this);return;}
class Object;
class Kill
{
  private:
   static int initialized;
   static NoQueue<Object*> *queue;
  public:
   static void run(void);
   static void init(void);
   static void quit(void);
   static void log(Object* to_kill) { DBG_CHECK(initialized); queue->add(&to_kill);}
};
//KLASA FASTALLOC
class FastAlloc
{
  private:
   static int initialized;
   static Fast_heap my_heap;
   static unsigned int objects_table[2];
   static unsigned int objects_number;
  public:
   static void init(void);
   static void quit(void);
   FastAlloc(void) {}
   virtual ~FastAlloc(void) {}
   void *operator new(size_t obj_size);
   void operator delete(void *ptr,size_t obj_size);
};
//KLASA OBJECT
class Object : public FastAlloc
{
  public:
   Object(void) {}
   virtual ~Object(void) {}
   virtual void run(void)=0;
};
//KLASA POSIT
class Posit
{
   friend class Move;
   friend class Myship;
  private:
   static int initialized;
   static int vxlut[Mp::TURNQUALITY];
   static int vylut[Mp::TURNQUALITY];
   static int sinlut[Mp::TURNQUALITY];
   static int coslut[Mp::TURNQUALITY];
   static struct TagOffset
   {
     int x;
     int y;
   } offset[Mp::TURNQUALITY*(Mp::MAXOFFSET+1)];
  public:
   static inline int hypot(int dx,int dy);
   static inline int vect_len(int dx,int dy);
   static inline int x_offset(int angle,int dist);
   static inline int y_offset(int angle,int dist);
   static inline int _shiftsin(int angle);
   static inline int _shiftcos(int angle);
  private:
   int fractx;
   int fracty;
   int kstat_per_pixel;
   int counted_kstat_up;
   int counted_kstat_down;
   void _ctor(int _x,int _y,int _height,int _climb_speed);
  protected:
   int x;
   int y;
   int height;
   Posit(void) {}
   Posit(int _x,int _y,int _chl,int _climb_speed=Mp::CLIMBSPEED);
   Posit(int _x,int _y,int _chl,Op::ObjPack& _ref);
   virtual ~Posit(void) {}
   int distance(int _x,int _y) {return vect_len(_x-x,_y-y);}
   int get_chl(void) {return height>=Mp::HALFHEIGHT?Posit::UP:Posit::DOWN;}
   int is_down(void) {return height<=0;}
   int is_up(void) {return height==Mp::PIXELSBETWEENPLANES;}
   int get_up_x(void) {return x;}
   int get_up_y(void) {return y-(Mp::PIXELSBETWEENPLANES-height);}
   int get_down_x(void) {return x;}
   int get_down_y(void) {return y+height;}
   void go_up(void);
   void go_down(void);
   void move_me(int _angle,int _dist);
   int is_in(int range,int id=-1) {return is_in(range,x,y,id);}
   int is_visible(int id=-1) {return is_in(0,x,y,id);}
   int is_in_overscan(int id=-1) {return is_in_overscan(0,x,y,id);}
  public:
   enum
   {
     DOWN =0,
     UP   =1
   };
   int get_x(void) {return x;}
   int get_y(void) {return y;}
   static void init(void);
   static void quit(void);
   static int is_in(int range,int _x,int _y,int id=-1);
   static int is_visible(int _x,int _y,int id=-1) {return is_in(0,_x,_y,id);}
   static int is_in_overscan(int range,int _x,int _y,int id=-1);
};

int Posit::hypot(int dx,int dy)
{
  DBG_CHECK(initialized);
  if (dx<0) dx=-dx;
  if (dy<0) dy=-dy;
  int min=(dx<dy)?dx:dy;
  return dx+dy-(min>>1);
}

int Posit::vect_len(int dx,int dy)
{
  DBG_CHECK(initialized);
  dy=((dy*(Mp::ONEBYVYTOVXRATIO>>8))>>8);
  return hypot(dx,dy);
}

int Posit::_shiftsin(int angle)
{
  DBG_CHECK(initialized);
  DBG_CHECK(angle>=0&&angle<Mp::TURNQUALITY);
  return (sinlut[angle]);
}

int Posit::_shiftcos(int angle)
{
  DBG_CHECK(initialized);
  DBG_CHECK(angle>=0&&angle<Mp::TURNQUALITY);
  return (coslut[angle]);
}

int Posit::x_offset(int angle,int dist)
{
  DBG_CHECK(initialized);
  DBG_CHECK(angle>=0&&angle<Mp::TURNQUALITY);
  DBG_CHECK(dist>=0);
  if (dist>Mp::MAXOFFSET)
  {
    return (vxlut[angle]*dist)>>16;
  }
  else
  {
    return offset[dist*Mp::TURNQUALITY+angle].x;
  }
}

int Posit::y_offset(int angle,int dist)
{
  DBG_CHECK(initialized);
  DBG_CHECK(angle>=0&&angle<Mp::TURNQUALITY);
  DBG_CHECK(dist>=0);
  if (dist>Mp::MAXOFFSET)
  {
    return (vylut[angle]*dist)>>16;
  }
  else
  {
    return offset[dist*Mp::TURNQUALITY+angle].y;
  }
}
//KLASA TURN
class Turn : public virtual Posit
{
  private:
   int dirs;
   int restangle;
   int angles_per_dir;
   int dirs_per_angle;
   int angles_per_time;
   struct ATan
   {
     int wlt;
     int angle;
   };
   static ATan   atanlut[Mp::TURNQUALITY/4];
  protected:
   int  angle;
   Turn(void) {}
   Turn(int _angle,int _turn_speed=Mp::TURNSPEED,int _dirs=Mp::DIRNUM);
   Turn(int _angle,Op::ObjPack& ref);
   virtual ~Turn(void) {}
   int  get_dir(int ddir=0);
   int  get_dirs(void) {return dirs;}
   int  get_angle(int tox,int toy){return get_dangle(tox-x,toy-y);}
   int  get_angle(int dangle=0);
   void set_angle(int _angle) {DBG_CHECK(_angle>=0&&_angle<Mp::TURNQUALITY);angle=_angle;}
   void set_turn_speed(int _turn_speed);
   int  normalize_angle(int _angle);
   void turn_left(void);
   void turn_right(void);
   int  turn_point(int tox,int toy);
   int  turn_angle(int toangle);
   int  turn_opposite(int tox,int toy){return turn_point(x+x-tox,y+y-toy);}
   int  get_dir_angle(int current_dir=-1) {if (current_dir==-1) current_dir=get_dir();
                                           DBG_CHECK(current_dir>=0&&current_dir<dirs);
                                           return (current_dir*Mp::TURNQUALITY)/dirs;}
   int  point_r_or_l(int tox,int toy);
   int  angle_r_or_l(int toangle,int fromangle=-1);
  public:
   static int get_dangle(int dx,int dy);
   static int sub_angle(int fromangle,int toangle);
};
//KLASA MOVE
class Move : public Turn
{
  public:
   enum KickPower
   {
     WEAK,
     MEDIUM,
     STRONG,
     OHFUCK,
   };
  private:
   int max_speed;
   int strafe_speed;
   int accel_pwr;
   int brake_pwr;
   int auto_pwr;
  protected:
   int vx;
   int vy;
   Move(void) {}
   Move(int _angle,int _actual_speed,Op::ObjPack& ref);
   virtual ~Move(void) {}
  private:
   int _get_speed(void) {return Posit::vect_len(vx,vy);}
   void _set_speed(int _speed,int _angle=-1);
  protected:
   int  stopped(void) {return (vx==0&&vy==0);}
   int  get_speed(void) {return (_get_speed()*Mp::METRONQUALITY)>>16;}
   void set_speed(int _speed,int _angle=-1);
   void transform_speed(int _angle=-1);
   void go(void);
   void update_speed(void);
   void inline faster(int _angle);
   void faster(void) {faster(angle);}
   void rear_faster(void) {faster(get_angle(-Mp::HALFANGLE));}
   void brake(void);
   void auto_brake(void);
   void stop(void){vx=0;vy=0;}
   void strafe(int _angle);
   void strafe_left(void) {strafe(get_angle(-Mp::QUARTANGLE));}
   void strafe_right(void) {strafe(get_angle(Mp::QUARTANGLE));}
   void set_max_speed(int _max_speed);
   void set_strafe_speed(int _strafe_speed);
  public:
   void _kick_me(int _angle,int _accel);
   void kick_me(int _x,int _y,KickPower pow);
   void kick_me(int _angle,KickPower pow);
};

void Move::faster(int _angle)
{
  int _accel=accel_pwr*KbdStat::time();
  DBG_CHECK(_accel>=0);
  vx+=((_accel>>8)*(Posit::vxlut[_angle]>>8))/Mp::METRONQUALITY;
  vy+=((_accel>>8)*(Posit::vylut[_angle]>>8))/Mp::METRONQUALITY;
  update_speed();
}
//KLASA VISIBLE
class Visible : public virtual Posit
{
  private:
   friend class Shadow;
   static Sprite dummy;
   Sprite& sprite;
   View::Vprty prty;
   int phase;
   int uprange;
   int downrange;
   int no_sprite;
  protected:
   Visible(View::Vprty _prty);
   Visible(Sprite& _sprite,View::Vprty _prty,int _phase=0);
   virtual ~Visible(void) {}
   Phase& get_spr_phase(int _phase=-1) {if (_phase==-1) _phase=phase; DBG_CHECK(!no_sprite); DBG_CHECK(_phase>=0&&_phase<sprite.phases);
                                        return sprite[_phase];}
   Sprite& get_sprite(void)    {DBG_CHECK(!no_sprite); return sprite;}
   int  get_phases_num(void)   {DBG_CHECK(!no_sprite); return sprite.phases; }
   int  get_phase(void)        {DBG_CHECK(!no_sprite); return phase; }
   void plus_phase(void)       {DBG_CHECK(!no_sprite); if (++phase>uprange) phase=downrange;}
   void minus_phase(void)      {DBG_CHECK(!no_sprite); if (--phase<downrange) phase=uprange;}
   void update_phase(int _dir) {DBG_CHECK(!no_sprite); DBG_CHECK(_dir<=uprange-downrange);
                                DBG_CHECK(_dir>=0);phase=downrange+_dir;}
   int  is_last_phase(void)    {DBG_CHECK(!no_sprite);return (phase>=uprange);}
   int  is_first_phase(void)   {DBG_CHECK(!no_sprite); return (phase<=downrange);}
   void set_phase(int _phase)  {DBG_CHECK(!no_sprite);DBG_CHECK(_phase<=uprange);DBG_CHECK(_phase>=downrange);phase=_phase;}
   int  get_uprange(void)      {return uprange;}
   int  get_downrange(void)    {return downrange;}
   int  is_in(int range,int id=-1);
   int  is_visible(int id=-1)  {return is_in(0,id);}
   int  is_in_overscan(int id=-1);
   void inline log_to_view(int plane=-1);
   virtual int get_plane(void) {return (get_chl()==Posit::DOWN)?world->get_active_plane()-2:world->get_active_plane()-1;}
   void set_range(int _downrange,int _uprange);
   void reset_range(void);
  public:
   static int is_phase_in(int range,int _x,int _y,Phase& phase,int id);
   virtual void draw_object(Screen& screen,int plane) {(void)plane;DBG_CHECK(!no_sprite); screen.put(x,y,get_sprite(),get_phase());}
#if HI_DEBUG
   virtual void draw_shadow(Screen& screen,int plane);
#endif
};

void Visible::log_to_view(int plane)
{
  if (plane==-1)
  {
    plane=get_plane();
  }
  View::add(this,plane,prty);
}
//KLASA SHADOW
class Shadow : public Visible, public virtual Posit
{
  protected:
   Shadow(Sprite& _sprite,View::Vprty _prty,int _phase=0):Visible(_sprite,_prty,_phase) {}
   Shadow(View::Vprty _prty):Visible(_prty) {}
   virtual ~Shadow(void) {}
   virtual int get_shadow_plane(void) {return world->get_active_plane()-2;}
   int inline is_visible(int id=-1);
   void inline log_to_view(int object_plane=-1,int shadow_plane=-1);
  public:
   virtual void draw_shadow(Screen& screen,int plane);
};

int Shadow::is_visible(int id)
{
  DBG_CHECK(!no_sprite);
  DBG_CHECK(phase<sprite.phases);
  DBG_CHECK(phase>=0);
  Phase &p=sprite[phase];
  return is_phase_in(0,x,y+height+Mp::SHADOWDIST,p,id);
}

void Shadow::log_to_view(int object_plane,int shadow_plane)
{
  Visible::log_to_view(object_plane);
  if (shadow_plane==-1)
  {
    shadow_plane=get_shadow_plane();
  }
  if (get_chl()==Posit::UP)
  {
    View::add(this,shadow_plane,View::AFTERSHD);
  }
  else
  {
    View::add(this,shadow_plane,View::BEFORESHD);
  }
}
//KLASA LISTMANAGER
enum Oprty {
             OFIRST,
             OBACK,
             OWALL,
             OSTAT,
             OAOBJ,
             OMOBJ,
             OMINE,
             OMYSHIP,
             ONETMINE,
             OTELEPORT,
             OAWEAP,
             OMWEAP,
             ONEUTRAL,
             OLAST
           };

const int oprty_types=OLAST+1;

class Listmanager : public virtual Object
{
         friend class List<Listmanager,Oprty,oprty_types>;
  private:
   static int initialized;
   static List<Listmanager,Oprty,oprty_types> *list;
   Listmanager* prev;
   Listmanager* next;
   Oprty prty;
  protected:
   Listmanager(Oprty prty);
   virtual ~Listmanager(void);
  public:
   static void init(void);
   static void run(void);
   static void quit(void);
};
//KLASA IMPEXP
class Impexp
{
  protected:
   int no_in_level;
   int to_return;
   int level_num;
   int level_plane;
   Lobject *lobject;
   Impexp(int,World* _world=world);
   virtual ~Impexp(void);
   void remove_from_level(void);
   void update_posit(int _x,int _y) {lobject->y=_y;lobject->x=_x;world->get_level().update(level_num);}
};
//KLASA LIFE
class Life
{
  private:
   int life;
   int start_life;
  protected:
   Life(int _life)            {DBG_CHECK(_life>0);start_life=life=_life;}
   Life(void)                 {start_life=life=-1;}
   Life(Op::ObjPack& ref)     {DBG_CHECK(ref.life>0);start_life=life=ref.life;}
   virtual ~Life(void)        {}
   int get_life(void)         {DBG_CHECK(start_life>0);return life;}
   void increase(int dlife=1) {DBG_CHECK(start_life>0);life+=dlife;}
   void decrease(int dlife=1) {DBG_CHECK(start_life>0);life-=dlife;}
   void set_life(int dlife)   {DBG_CHECK(start_life>0);life=dlife;}
   int  life_percent(void)     { if (start_life<0) return -1;
                                else {int t= (life*100/start_life);
                                      return (t==0&&life>0)? 1:t;}
                               }
   int  is_alive(void)        {return life>0;}
};

//KLASA LOWCOLIS
#define C_MYHERO(id)           0,  id,   0,   0,   0,  id,   1,   1,   0
#define C_MYBASE(id)           0,  id,   0,   0,   0,   0,   0,   1,   0
#define C_MYRAFE(id)           0,  id,   0,   0,   0,  id,   1,   1,   0
#define C_MYSTATIC(id)         1,  id,   0,   0,   0,   0,   1,   1,   0
#define C_MYSHOT(id)           0,  id,   0,   1,   1,   0,   1,   0,   0
#define C_GOLDENEYE(id)        0,  id,   0,   0,   1,   0,   1,   0,   0
#define C_ALIENHERO            0,   0,   1,   0,   0,   0,   1,   1,   0
#define C_ALIENSHOT            0,   0,   1,   1,   1,   0,   1,   0,   0
#define C_ALIENSTATIC          1,   0,   1,   0,   0,   0,   1,   1,   0
#define C_SHIPFUCKER           0,   0,   1,   0,   0,   0,   1,   1,   0
#define C_NWALL                1,   0,   0,   0,   0,   0,   0,   0,   0
#define C_WWALL                1,   0,   0,   1,   0,   0,   0,   0,   0
#define C_KWALL                1,   0,   0,   0,   0,   0,   1,   0,   0
#define C_COLLECT              0,   0,   0,   1,   1,   0,   0,   0,   1
#define C_NEUTRAL              0,   0,   0,   1,   1,   0,   0,   0,   0

#define C_MINEBEGIN            0,   0,   0,   0,   1,   0,   0,   1,   0
#define C_MINEKILLALIEN        0,   1,   0,   0,   1,   0,   1,   1,   0
#define C_MINEKILLMY           0,   0,   1,   1,   1,   0,   1,   1,   0

#define C_NETMINEBEGIN(id)     0,   id,  0,   0,   1,   0,   1,   1,   0
#define C_NETMINEKILLMY        0,   0,   1,   0,   1,   0,   1,   1,   0

//to dodal los !!!:
#define C_MINEKILLALIENNET     0,   0,   0,   0,   1,   0,   1,   1,   0

#define C_MYMINES(id)          0,  id,   0,   1,   1,   0,   1,   0,   0
#define C_MYMINEM              0,   0,   0,   1,   1,   0,   1,   0,   0
#define C_DEST(sh1,sh2,sh3)    sh1, 0,   0,   0, sh2,   0,   0, sh3,   0

/*
                               W    M    A    W    O    M    K    K    C
                               A    Y    L    E    B    Y    I    I    O
                               L         I    A    J    S    L    L    L
                               L         E    P    T    H    L    L    L
                                         N    T    H    I    E    A    E
                                              H    R    P    R    B    C
                                              R    O              L    T
                                              O    U              E
                                              U    G
                                              G    H
                                              H
*/

#define C_CHECK_MY(ptr,id)          (ptr->MY!=id&&(!ptr->WEAPTHROUGH||ptr->WALL))
#define C_CHECK_ALIENSH(ptr)        ((!ptr->ALIEN&&!ptr->WEAPTHROUGH)||ptr->WALL)
#define C_TARGET_MY(ptr,id)         ((ptr->MY!=id&&ptr->KILLABLE)||(ptr->ALIEN&&!ptr->WEAPTHROUGH))
#define C_TARGET_MY_SHIP(ptr)       (ptr->ALIEN||ptr->MY)
#define C_CHECK_ALIEN(ptr)          (!ptr->ALIEN&&ptr->KILLER)
#define C_CHECK_RAFT(ptr,id)        (ptr->MY!=id&&ptr->KILLER)
#define C_CHECK_DESTROY(ptr)        (ptr->KILLER&&ptr->OBJTHROUGH)
#define C_MINE_BLOW(ptr)            (ptr->MYSHIP||(ptr->ALIEN&&!ptr->WEAPTHROUGH)||ptr->WALL)
#define C_MINE_CHECK(ptr)           (ptr->KILLER)
#define C_MINE_CHECKOBJ(ptr)        ((ptr->MYSHIP)||(ptr->ALIEN&&!ptr->WEAPTHROUGH))
#define C_CHECK_BASE(ptr,id)        (ptr->MY!=id&&ptr->KILLER&&ptr->OBJTHROUGH)
#define C_SWARM_LOCK_ALIEN(ptr,id)  (((ptr->MY!=id&&ptr->KILLABLE)||(ptr->ALIEN&&!ptr->WEAPTHROUGH&&ptr->KILLABLE))&&!ptr->MYSHIP)
#define C_SWARM_LOCK_MYSHIP(ptr,id) (ptr->MYSHIP!=0&&ptr->MYSHIP!=id)

struct Cmsg
{
  int WALL        :1;
  int MY          :4;
  int ALIEN       :1;
  int WEAPTHROUGH :1;
  int OBJTHROUGH  :1;
  int MYSHIP      :4;
  int KILLER      :1;
  int KILLABLE    :1;
  int COLLECT     :1;
  int destroy;
  int x,y;
  int vx,vy;
};

class LowColis
{
   friend class Debuginfo;
  private:
   static unsigned char* mem_for_cworld[NoNet::max_users+1];
   static Screen cworld[NoNet::max_users+1];
   static Cmanager cm[NoNet::max_users+1];
   static struct Msg_unit
   {
     Cmsg msg;
     int next;
   } *messages;
   static NoQueue<int> *free_msg_cells;
   static NoQueue<LowColis*> *logged_objects;
#if HI_DEBUG
   static int msg_size;
   static int msg_used;
   static int max_msg_used;
   static int max_user_used;
   static char *max_user_name;
   char *user_msg_name;
   int user_msg_used;
#endif
   int user_msg_cursor;
   static int initialized;
   static void show_cworld(int id,Screen& screen,int posx,int posy);
  protected:
   Cmsg myinfo;
   LowColis(int wall,int my,int alien,int weapthrough,int objthrough,int updnthrough,int killer,int killable,int collect,int destroy,char *user_name=NULL);
   LowColis(void) {}
   virtual ~LowColis(void);
   void write(int id,int x,int y,int vx,int vy,Sprite& sprite,int phase,int mirror=Spr::nomirror);
   void check(int id,int x,int y,int vx,int vy,Sprite& sprite,int phase,int mirror=Spr::nomirror);
   void see(int id,int x,int y,Sprite& sprite,int phase,int mirror=Spr::nomirror);
   void write(int id,int x,int y,int vx,int vy);
   void check(int id,int x,int y,int vx,int vy);
   void write_to_logged(int x,int y,int vx,int vy);
   Cmsg* see(int id,int x,int y,int copy_to_queue=1);
   Cmsg* get_cmsg(void);
   void set_power(int kill) {myinfo.destroy=kill;}
   void set_cinfo(int wall,int my,int alien,int weapthrough,int objthrough,int myship,int killer,int killable,int collect);
   void reset_cmsg(void);
   static void create_cworld(int id);
   static void delete_cworld(int id);
  public:
   virtual void add_cmsg(Cmsg* cmsg,int id);
   virtual Cmsg* respond(void) {return &myinfo;}
   static void check_id(int id);
   static void clean(void);
   static void init(void);
   static void quit(void);
   static void origin(int id,int x,int y);
   static LowColis *first_logged(void);
   static LowColis *next_logged(void);
};
//KLASA COLIS
class Colis : public LowColis, public virtual Posit
{
  protected:
   Colis(int wall,int my,int alien,int weapthrough,int objthrough,int updnthrough,int killer,int killable,int collect,int destroy,char *user_name=NULL);
   Colis(int wall,int my,int alien,int weapthrough,int objthrough,int updnthrough,int killer,int killable,int collect,Op::ObjPack& ref,char *user_name=NULL);
   Colis(void) {}
   virtual ~Colis(void) {}
   void write(int id,int vx,int vy,Sprite& sprite,int phase,int mirror=Spr::nomirror) {LowColis::write(id,x,y,vx,vy,sprite,phase,mirror);}
   void check(int id,int vx,int vy,Sprite& sprite,int phase,int mirror=Spr::nomirror) {LowColis::check(id,x,y,vx,vy,sprite,phase,mirror);}
   void see(int id,Sprite& sprite,int phase,int mirror=Spr::nomirror) {LowColis::see(id,x,y,sprite,phase,mirror);}
   void write_point(int id,int vx,int vy) {LowColis::write(id,x,y,vx,vy);}
   void check_point(int id,int vx,int vy) {LowColis::check(id,x,y,vx,vy);}
   Cmsg* see_point(int id,int copy_to_queue=1) {return LowColis::see(id,x,y,copy_to_queue);}
   int wall_angle(int edge=0,int circle_size=4);
   int wall_angle2(int circle_size=4);
};
//KLASA LINK
class Link : public virtual Posit
{
  private:
   static unsigned short* prv_link;
   int precision;
   int l_num;
  protected:
   Link(int _l_num,int precision=Mp::LINKPRECISION);
   virtual ~Link(void) {}
   void next_link(void) {check_init();l_num=world->get_level()[l_num].link;}
   void prev_link(void) {check_init();l_num=prv_link[l_num];}
   int link_x(void) {check_init();return world->get_level()[l_num].x;}
   int link_y(void) {check_init();return world->get_level()[l_num].y;}
   int in_link(void) {check_init();return in_link(x,y);}
   int in_link(int _x,int _y) {check_init();return (Posit::vect_len(_x-link_x(),_y-link_y())<precision);}
   Lobject *link_lobject(void) {return &(world->get_level()[l_num]);}
   static void check_init(void);
  public:
   static int prev_lnk(int num) {return prv_link[num];}
   static int initialized;
   static void send_impulse(int lev_num);
   static void init(void);
   static void quit(void);
};
//KLASA RADAR
class Radar;


struct Wektor : public Heap_object
{
  int _x,_y;
};

const MAX_RADARCUR_TYPE=8;

class Radar :
public virtual Posit,
public virtual Life
{
   friend class GameManager;
  private:
   static NoQueue<Radar*> *dott;
   static int R;
   static int X,Y;
   static int time,phase,ttime,lphase,ltime;
    struct GM_Rad
    {
      int    tar;
      int    id;
      Radar* pttr;
      int    type;
      int    snd;

      GM_Rad* ptr;
      GM_Rad* next;
      GM_Rad* prev;
      int prty;

    } gm_rad;

   static List<GM_Rad,int,1> *tar;
   static int range;
   static unsigned char points[32];
   static unsigned char queue[32];
   static int cur_type[MAX_RADARCUR_TYPE];
   static int last,radarON;
   static int TargetX,TargetY;
   int distance;
   int ray;
   int targeting;
   int dx,dy;
   static Radar* current;
  protected:
   Radar(int _tar,int _id=0,int _dx=0,int _dy=0);
   virtual ~Radar(void);
   void radar_log(void);
   void lock_targeting(void) {targeting=0;}
   virtual void run (void) {}
   static void put(Screen& screen,int num,int _R);
  public:
   void login(void);
   void login(Screen& screen);
   static int check_cur_type(int type);
   static int enable(void) {return radarON;}
   static void set(int _x,int _y) {X=_x;Y=_y;}
   static void set_cur_type(int* ptr,int size);
   static void init(void);
   static void done(void);
   static void compute(void);
   static void draw(Screen& screen);
   static void clean(void);
   static void get_target(int* tx,int* ty);
};
//KLASA HANDLE
#define GMANAGER     1
#define WEAPON       2
#define HDOORS       4
#define DESTROY_OBJ  8

class Handle
{
  friend class List<Handle,int,1>;
  private:
   Handle* ptr;
   Handle* next;
   Handle* prev;
   int prty;

  protected:
   int ID;
   static int initialized;
   static List<Handle,int,1> *mail_box;
   virtual void HandleEvent(int)=0;

  public:
   static void init(void);
   static void done(void);
   Handle(int _ID)        {DBG_CHECK(initialized);mail_box->add(this,0); ID=_ID;}
   virtual ~Handle(void)  {DBG_CHECK(initialized);mail_box->remove(this);}
   static void post(int _ID,int message){DBG_CHECK(initialized);
                                          for (Handle* temp=mail_box->reset();temp;temp=mail_box->get_next())
                                          if (temp->ID&_ID) temp->HandleEvent(message);}
};

//SOUND

const int MAX_WEATHER=100;
const int MAX_GM_SOUNDS=60;

class Sound : public virtual Posit
{
  protected:
   Sample* smp;
   static Sample sounds[MAX_GM_SOUNDS];
   static char   sounds_name[MAX_GM_SOUNDS][30];
   static int    sample_count;
   static void   pl(Sample* smp,int _x,int _y);
   static void   netpl(Sample* smp,int _x,int _y);
  public:
   Sound(void);
   virtual ~Sound(void){};
   static void callback(unsigned user);
   void play(Sample& _smp,int _panning=1);
   void run(void);
   static int  load_sound(char*);
   static void play(int smp,int x=Mp::SX/2,int _y=Mp::SY/2);
   static void mission_play(int smp);
   static void st_play(Sample* smp,int vol=0);
   static int  playing(int smp);
   static void stopsmp(int smp);
   static void init(void);
   static void done(void);

};

#endif































