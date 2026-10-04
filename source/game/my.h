#ifndef __LMYBOJECT__
#define __LMYOBJECT__

class Weapon;

//KLASA DISPINFO
class DispInfo:
public virtual Object,
public virtual Posit,
public Visible,
public Sound
{
  private:
   static int initialized;
   int id;
   User *myuser;
   int disp_flicker;
   void print_number(Screen& screen,Sprite& spr,int x,int y,int number,int color=-1);
   int number_sx(Sprite& spr,int number);
   int color_indx(int max,int curr);
  public:
   DispInfo(User *_myuser,int _id);
   virtual ~DispInfo(void) {}
   void reset(void);
   virtual void run(void);
   virtual void draw_object(Screen& screen,int plane);
};
//KLASA USER
const int MAX_USERLEVELS=20;

class User:
public Object
{
   friend class Stale_memory;  // port: reads what the original left in memory (stale.h)
  public:
   enum {weap_num=6};
   enum Invent
   {
    _EMPTY,
     MEDIKIT,
     CLOAK,
     IMMORT,
     NETMINE,
     GOLDENEYE,
    _LAST,
   };
  private:
   struct Params
   {
     int life;
     int weap_ammo[weap_num];
     int weap_in_use;
     int inventory[_LAST];
     int inv_in_use;
   } params;
   struct MaxParams
   {
     int life;
     int weap_ammo[weap_num];
   } max_values;
   int life_time;
   int life_counter;
   int life_upgrade_amount;
   int last_life;
   char buffer[256];
   void update_inv_state(void);
   void check_weap_range(int weap);
   int id;
  public:
   User(int _id);
   virtual ~User(void) {}
   void run(void);
   void reset(void);
   int get_weap_ammo(int weap);
   int get_weap_ammo_max(int weap);
   void increase_life(int dlife);
   void decrease_life(int dlife);
   void reset_lifeupgrade_timer(void);
   int get_life(void);
   int get_life_max(void);
   int get_life_percent(void);
   int is_alive(void);
   int is_full_weap(int weap);
   int is_full_life(void);
   void increase_weap_ammo(int weap,int dammo);
   void decrease_weap_ammo(int weap,int dammo);
   int get_weap_in_use(void);
   void set_weap_in_use(int weap);
   void set_full_life(void);
   void set_full_weap(void);
   void set_full_invent(void);
   int insert_item(Invent item,int count=1);
   char *get_item_descript(void);
   void next_item(void);
   void prev_item(void);
   Invent curr_item(void);
   Invent get_item(void);
   int item_numb(Invent item);
   void store_data(void);
};
//SCRCATCH
class ScrCatch :
public virtual Object,
public virtual Posit,
public Move
{
   friend class Myship;
  private:
   struct DestPoint
   {
     int x;
     int y;
   } *destination;
   int dx;
   int dy;
   ScrCatch(void);
   virtual ~ScrCatch(void);
   virtual void run(void);
   void run(int _angle);
   int get_dx(void) {return (dx>>16);}
   int get_dy(void) {return (dy>>16);}
   void reset(void);
};
//KLASA CAPSULE
class Capsule:
public virtual Object,
public virtual Posit,
public Shadow,
public Listmanager,
public Sound,
public Colis
{
  public:
   enum Type
   {
    _FIRST,
     WEAP0,
     WEAP1,
     WEAP2,
     WEAP3,
     WEAP4,
     WEAP5,
     LIFE,
     MEDIKIT,
     CLOAK,
     IMMORT,
     BRICK,
     NETMINE,
     GOLDENEYE,
     ALTERING,
     CHAOS,
    _LAST,
   };
  private:
   Type type;
   int amount;
   int power;
   int phs_changed;
   int altering;
  protected:
   int respawn_time;
   int to_respawn;
  public:
   Capsule(int _x,int _y,int _chl,Capsule::Type _type,int _power=0);
   virtual ~Capsule(void) {}
  protected:
   int _run(void);
   void collected(void);
  private:
   void adjust_range(void);
   void adjust_respawntime(void);
  public:
   static int get_amount(Capsule::Type _type,int power);
   virtual void run(void);
};
//KLASA LEVELCAPSULE
class LevelCapsule:
public Capsule,
public Impexp
{
  public:
   LevelCapsule(int _x,int _y,int _chl,int _lev_num,Capsule::Type _type,int _power=0);
   virtual ~LevelCapsule(void) {}
   virtual void run(void);
};
//KLASA BRICK
class Brick:
public virtual Object,
public virtual Posit,
public Impexp,
public Move,
public Shadow,
public Listmanager,
public Sound,
public Colis
{
  protected:
   static int initialized;
  private:
   static NoQueue<int> *queue;
   int phs_changed;
   int not_active;
   int time;
  protected:
   int _run(void);
   void wall(void);
  public:
   static void init(void);
   static void quit(void);
   static int  get_num(void);
   Brick(int _x,int _y,int _chl,int _angle,int _speed,int _level_num=-1,int _not_active=0);
   virtual ~Brick(void) {}
   virtual void run(void);
};
//KLASA SKIN
class Skin:
public virtual Object,
public virtual Posit,
public Shadow,
public Listmanager,
public Sound,
public Colis
{
   friend class Myship;
  private:
   int ship_weap[6];
   int id;
   int not_active;
  public:
   Skin(int _x,int _y,int _chl,int _id,int weap1,int weap2,int weap3,int weap4,int weap5,int weap6);
   virtual ~Skin(void) {}
  public:
   virtual void run(void);
};

class AlienSmoke;
//KLASA MYSHIP
#define SEND_DEBUG_MESSAGE if(Mp::NETDEBUGLEVEL>0) send_debug_message

class Myship:
public virtual Object,
public virtual Posit,
public Move,
public Shadow,
public Listmanager,
public Colis,
public Radar,
public Sound
{
   friend class Stale_memory;  // port: reads what the original left in memory (stale.h)
   friend class Weapon;
   friend class User;
  private:
   struct TrailData
   {
     int x;
     int y;
     int phase;
   };
   struct DebugData
   {
     enum {msg_len=256};
     int frame;
     int x;
     int y;
     int angle;
     int life;
     char msg[msg_len];
   };
   static NoQueue<DebugData> *queue[NoNet::max_users+1];
   static int debug_initialized;
   int id;
   int ship_radius;
   User *myuser;
   DispInfo *dispinfo;
   ScrCatch *scrcatch;
   Weapon *weapon;
   AlienSmoke *smoke;
   TrailData trailers[3];
   int cloak;
   int shield;
   int cloak_warn_played;
   int shield_warn_played;
   int shld_low_played;
   int shld_crit_played;
   int old_life;
   int immortal;
   int wall_through;
   int engine_flipper;
   int flash;
   int morph_strong;
   int morph_strong_phase;
   int accelerating;
   int kill_countdown;
   int kill_value;
   int kill_alreadykilled;
   int fadeing;
   int last_kill_value;
   int last_killer_id;
   int last_x;
   int last_y;
   int nocolis_x;
   int nocolis_y;
   int land_x;
   int land_y;
   int land_angle;
   int start_x;
   int start_y;
   int start_angle;
   int first_start;
   int bricks;
   int life_brick_ratio;
   int life_brick_cnt;
   int shake_time;
   int shipfuck_time;
   int shipfuck_status;
   int teleport_phase;
   int teleport_x;
   int teleport_y;
   int teleport_tox;
   int teleport_toy;
   int teleport_speed;
   int teleport_angle;
   int teleport_vx;
   int teleport_vy;
   int leaving_notified;

   void reset_all_variables(void);
   void throw_brick(void);
   void find_startplace(void);
   void life_brick_update(void);
   void reset_trailers(void);
   void update_trailers(void);
   void steering_service(void);
   void landing_service(void);
   void starting_service(void);
   void weapon_service(void);
   void inventory_service(void);
   void dispinfo_service(void);
   void teleporting_service(void);
   void smoke_service(void);
   void radar_service(void);
   void shake_service(void);
   void netcheating_service(void);
   void update_position(void);
   void check_walls_col(void);
   void check_object_col(void);
   void check_col(void);
   void check_alive(void);
   void display_service(void);
   void timer_service(void);
   void set_user_mode_params(void);
   void set_turbo_mode_params(void);

   void draw_engine(Screen &screen);
   void draw_trailers(Screen &screen);
   void draw_normal(Screen &screen);
   void draw_cloak(Screen &screen);
  public:
   enum SampleMode
   {
     SPEECH,
     INTERNAL,
     EXTERNAL,
   };
   void play_ship_sample(Sample& sample,SampleMode mode);
   static void init_debug(void);
   static void quit_debug(void);
   static void dump_debug(void);
   static void debug_service(Myship *myship,char *msg);
   enum ServMode 
   {
     USER,
     TURBO,
     LANDING,
     WAITFORSTARTSIG,
     STARTING,
     TELEPORTING,
     DYING,
   } mode;
   Myship(int x,int y,Sprite& sprite,int _id);
   virtual ~Myship(void);
   virtual void run(void);
   virtual void add_cmsg(Cmsg* message,int id);
   void send_debug_message(char *format,...);
   void landing_on(int _x,int _y,int _angle=-1);
   void landing_off(int _angle=-1);
   int get_landed(void) {return (mode==LANDING)&&is_down()&&angle==land_angle;}
   void teleporting_on(int this_x,int this_y,int that_x,int that_y);
   int get_x(void) {return x;}
   int get_y(void) {return y;}
   int get_down_x(void) {return Posit::get_down_x();}
   int get_down_y(void) {return Posit::get_down_y();}
   int get_up_x(void) {return Posit::get_up_x();}
   int get_up_y(void) {return Posit::get_up_y();}
   void set_x(int _x) {x=_x;}
   void set_y(int _y) {y=_y;}
   void set_pos(int _x,int _y) {x=_x;y=_y;}
   int get_offset_x(void) {return scrcatch->get_dx();}
   int get_offset_y(void) {return scrcatch->get_dy();}
   int get_vx(void) {return vx;}
   int get_vy(void) {return vy;}
   void set_vx(int _vx) {vx=_vx;}
   void set_vy(int _vy) {vy=_vy;}
   User *get_user(void) {return myuser;}
   void go(void);
   int is_alive(void) {return myuser->is_alive();}
   int get_id(void) {return id;}
   int get_speed(void) {return Move::get_speed();}
   int get_angle(void) {return angle;}
   int get_chl(void) {return Posit::get_chl();}
   int get_life(void) {return myuser->get_life();}
   void enable_immortal(void) {immortal=1;}
   void disable_immortal(void) {immortal=0;}
   void enable_wall_through(void) {wall_through=1;}
   void disable_wall_through(void) {wall_through=0;}
   int is_immortal(void) {return immortal;}
   int is_wall_through(void) {return wall_through;}
   int is_invisible(void) {return ((mode!=USER&&mode!=TURBO&&mode!=TELEPORTING)||cloak>0);}
   int is_teleporting(void) {return (mode==TELEPORTING);}
   int collect_capsule(Capsule::Type type,int power,int additional_info=0,int force_message=0);
   void take_skin(Skin *skin);
   int bricks_numb(void);
   int bricks_full(void);
   void start_shipfuck(void) {DBG_CHECK(!NoNet::is_network_mode());shipfuck_status=1;}
   void stop_shipfuck(void) {DBG_CHECK(!NoNet::is_network_mode());shipfuck_status=0;}
   void kick_ship(int _x,int _y);
   void kick_ship(int _angle,Move::KickPower _power);
   void shake_screen(int _time);
   void beaming_attract(int _x,int _y);
   void kill(int killer_id);
   virtual void draw_object(Screen& screen,int plane);
   virtual void draw_shadow(Screen& screen,int plane);
};
//KLASA TELEPORT
class Teleport:
public virtual Object,
public virtual Posit,
public Shadow,
public Listmanager,
public Link,
public Impexp
{
  private:
   int served[NoNet::max_users+1];
   int cool_dist(Myship* ship);
   int locked_dist(Myship* ship);
   int invisible;
   int phs_changed;
  public:
   Teleport(int _x,int _y,int _lev_num,int _invisible);
   virtual ~Teleport(void) {}
   virtual void run(void);
   virtual void draw_object(Screen& screen,int plane);
   virtual void draw_shadow(Screen& screen,int plane);
};
//KLASA LANDPLACE
class Landplace:
public virtual Object,
public virtual Posit,
public Radar,
public Visible,
public Listmanager,
public Impexp
{
  private:
   enum {READY,COME,GO,BUSY,CLEARWAIT};
   int order;
   int time;
   Myship *ship;
   int is_ship_up(void);
   int is_ship_gone(void);
   int is_ship_to_prompt(void);
  public:
   Landplace(int _x,int _y,Sprite& _sprite,int _phase,int _lev_num);
   virtual ~Landplace(void) {}
   virtual void run(void);
};
//MYCANNON

class MyCannon:
public virtual Object,
public virtual Posit,
public Turn,
public Visible,
public Colis,
public Listmanager,
public Life,
public Sound,
public Impexp
{
  protected:
   enum{GO,FIRE};
   int morph_level,mtime,sign;
   int order;
   int shtime;
   int time;
   int id;
   int passive;
   int active;
   int type,level;
   int shoot,plane,startangle;
   Myship *ship;
   void morph(void);
   void bang(void);
   void serv_kill(void);
   void service(void);
  public:
   MyCannon(int _x,int _y,Sprite& _sprite,int _level_num,int _plane,int _id,int type,int _level);
   virtual ~MyCannon(void);
   virtual void run (void);
   virtual void draw_object(Screen& screen,int plane);
};


//BASE
class Base:
public virtual Object,
public virtual Posit,
public Radar,
public Visible,
public Listmanager,
public Impexp
{
  private:
   int time;
   int morph_level;
   int sgn;
   int brick_count;
   int bcount_p_phs;
   int id;
   void build_cannon(int st);
  public:
   Base(int _x,int _y,Sprite& _sprite,int _lev_num,int _id);
   virtual ~Base(void){}
   virtual void run(void);
   virtual void draw_object(Screen& screen,int plane);
   static void impulse(int id,int bcount);
};

#endif
