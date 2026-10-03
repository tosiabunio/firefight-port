#ifndef __WORLD__
#define __WORLD__

class Update;

//KLASA WORLD
class World : public Heap_object
{
  friend class GameManger;
  friend class Update;
  private:
   Level level;
   int sx;
   int sy;
   int dx;
   int dy;
   char *name;
   int cls_color;
   int sub_level;
   int active_plane;
   int displayed_now;
   struct Pos
   {
     int x;
     int y;
   } posit_table[NoNet::max_users+1];
   void konvert(void);

  public:
   World(char* _name,int _sub_level,int _dx,int _dy);
   ~World(void);
   void register_builders(void);
   void cancel_builders(void);
   int get_x(void) {return posit_table[get_current_displayed()].x;}
   int get_y(void) {return posit_table[get_current_displayed()].y;}
   int get_x(int id) {check_id(id);return posit_table[id].x;}
   int get_y(int id) {check_id(id);return posit_table[id].y;}
   int get_sx(void) {return sx;}
   int get_sy(void) {return sy;}
   int get_dx(void) {return dx;}
   int get_dy(void) {return dy;}
   void check_id(int id);
   int get_active_plane(void) {return active_plane;}
   void get_objects_from_level(void);
   Level &get_level(void) {return level;}
   void update_my_xy(int id,int x,int y,int offset_x,int offset_y);
   void display(Screen& screen,int call_builders=1,int nine_plane=1);
   int get_current_displayed(void);
   int  count_plane(int type);
   Lobject *new_lobject(int plane,int *num);
   void set_current_displayed(int id=-1);
   int get_cls_color(void) {return cls_color;}
};

//UPDATE
class Update: public Heap_object
{
  protected:
   static Lobject*    lobject;
   static Parameters* params;
  public :
   static void freez(World*);
   static void realase(World* ,GameManager*);
   static void quit(void);
};
//STATISTICS
class Statistics
{
  private:
   static int  initialized;
   static int  sum[NoNet::max_users+1];
   static int  last[NoNet::max_users+1];
   static int  posit[NoNet::max_users+1];
   static int  selected[NoNet::max_users+1];
   static int  level;
   static int  compare(int id1,int id2);
   static int  kill,secret,time;
   static int  t_kill,t_secret,t_time;
   static int  MAX_SINGLELEVEL;
   static Screen* backscreen;
   static void put_singleback(void);
   static void put_netback(void);
   static int  rent;
   static int active_time,active_phs;
   static int first_time;
   static int say_who;
   static void *ptr;
  public :
   static void init(void);
   static void quit(void);
   static void reset_last(void);
   static void update(int player,int count);
   static void add(int player);
   static void sub(int player);
   static void set(int player,int count);
   static void reinit(void);
   static int  get(int player);
   static int  get_land(int player);
   static void recount(void);
   static int  X,Y,snd;

   static void Net(void);
   static void runNet(void);
   static void drawNet(Screen &screen,Tool& tool);

   static int Single(void);
   static void runSingle(void);
   static Tool tools[7];
   static void drawSingle(Screen &screen,Tool&,Tool&,Tool&);
};
//CHAT
class Chat
{
  private:
   enum {lines=35,screenline_len=78,bufline_len=256,name_len=30};
   struct LineInfo
   {
     int owners_id;
   };
   static LineInfo screenbufinfo[lines];
   static char screenbuf[lines][screenline_len];
   static char *screenlines[lines];
   static int  free_line;
   static int  first_time;
   static int  capt;

   static int  cursor_pos[NoNet::max_users+1];
   static int  display_pos[NoNet::max_users+1];
   static int  display_cursor[NoNet::max_users+1];
   static int  leftcount[NoNet::max_users+1];
   static int  rightcount[NoNet::max_users+1];
   static int  backcount[NoNet::max_users+1];
   static int  delcount[NoNet::max_users+1];
   static int  leftwordcount[NoNet::max_users+1];
   static int  rightwordcount[NoNet::max_users+1];
   static int  oper[NoNet::max_users+1];
   static int  lastcharcount[NoNet::max_users+1];
   static char lastchar[NoNet::max_users+1];

   static char namebuf[NoNet::max_users+1][name_len];
   static char lastlinebuf[NoNet::max_users+1][bufline_len];
   static char linebuf[NoNet::max_users+1][bufline_len];
   static int  terminating;
   static char *player_name(int id);
   static int  get_id(char *player_name);
   static int  session_finished(void);
   static void insert_new_line(int id,char *line);
   static void log_string(int id,char *format,...);
   static void input_string(int id);
   static void display_string(Screen &screen,int id,int x,int y,char *text);
   static void update_display_string(int id);
   static void log_users_status(void);
   static void log_text_strings(char *label);

   static int  _is_action(int value);
   static void update_actions(void);
   static int  del_char(int id);
   static int  back_char(int id);
   static int  left_char(int id);
   static int  right_char(int id);
   static int  left_word(int id);
   static int  right_word(int id);
   static int  disp_cursor(int id);
   static char auto_char(int id);

   static void serve_nocommand(int id,char *param);
   static void serve_help(int id,char *param);
   static void serve_quit(int id,char *param);
   static void serve_users(int id,char *param);
   static void serve_cls(int id,char *param);
   static void serve_query(int id,char *param);
   static void serve_invalidcommand(int id,char *param);
   static void serve_whoami(int id,char *param);

   static void run(void);
   static void draw(Screen& screen);

  public:
   static void talk(int in_game,int caller_id);
};

//NETMSG
class NetMsg
{
  private:
   enum {line_len=40,name_len=12};
   static int initialized;
   static char buffer[NoNet::max_users+1][line_len];
   static int editing[NoNet::max_users+1];
   static int disp_cursor;
  public:
   static void init(void);
   static void quit(void);
   static void run(void);
   static void draw(Screen& screen);
   static int in_use(int id=0);
};

//HEADER
class Header
{
  private:
   static int timer;
   static int phase;
   static int totalsy;
   static int linespaceing;
   static int offset;

   static float bx,by;
   static float vx,vy;
   static float mx,my;
   static int level;
   static unsigned int gtime;
   static int time;
   static int snd;
   static int press;
   static int disp;

   static int header_run(void);
   static void header_draw(Screen& screen);
   static int footer_run(void);
   static void footer_draw(Screen& screen);
   static int  first;
  public:
   static int  header(void);
   static void footer(void);
   static void ordering(void);
};

#endif
