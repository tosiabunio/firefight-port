#ifndef __GMANAGER__
#define __GMANAGER__

enum Event{DESTROY=1,LAND,ARRIVE,ON,OFF,END,PHOTO,TAKE};

//STRUKTURA FIELD
struct ChType
{
  int type,what,count;
};

const MAX_FIELDTEXT =24;

class Field : public Heap_object
{
  public:
   void clean(void);
   enum {JAXON,RILEY,STACY,SHADOW};
   int   num;
   int*  type;
   int   tsize;
   int*  enableaction;
   int   enablesize;
   int*  disableaction;
   int   disablesize;
   int   count;
   int   shake_screen;
   int   init_count;
   int   radar,set_radar;
   int   del;
   int   quit;
   int   photo;
   struct NoText
   {
     char text[MAX_FIELDTEXT][40];
     char textobj[40];
     char sound[MAX_FIELDTEXT];
     int  textsize;
     int kind[MAX_FIELDTEXT];
   } text;
   int   timer,quit_timer;
   int   rebel,quit_rebel;
   int   ID;
   int   message;
   int   sample;

   ChType* chtype;
   int size;
   ChType* handle;
   int hsize;

   Field* ptr;
   Field* next;
   Field* prev;
   int prty;
};


const int MAX_SUBLEVEL=5;
class Fade;
//KLASA GAMEMANAGER
class GameManager : public Handle,public Heap_object
{
  friend class Update;
  protected:
   static int textloaded;
   static List<Field,int,1> *necessary;
   static List<Field,int,1> *bonus;
   char type[100];
   char mission[100];
   char weather_type[100];
   int  sublevel;
   static int  quit;
   static int  quit_timer;
   static Text *mission_text;
   enum{
          DISP_TYPE=1,
          SERV_TYPE=2,
          COLIS_TYPE=3,
          FRAME_DELAY=4,
          TIME_DELAY=5,
          REPEAT_COUNT=6,
          SYNC_FRAME=7,
          CHANGE_TYPE=8,
          BLOCKED=9,
       };

   void load(List<Field,int,1> *list,char* section,Text& text);
   static void prologue(void);
   void type_event(Field* ptr);
   void handle_event(Field* ptr);
   void load_chtype(Text &text,Field*);
   void load_action(Text &text,Field*,int check_point);
   void load_handle(Text &text,Field*);
   void read_action(Field* fld,Text* text,char *area);
   void read_actionsection(int** action,int* size,Text* text,char *txt,int en);
   static void active_action(Field* fld);
   int check_message(int type,Field* ptr);
   int compute_count(Field* ptr);
   void clean_list(List<Field,int,1> *list);
   void clean_up(void);
   void active_build(int* ptr,int size);
   int dx,dy;
   int net_mode;
   static int fadeing;
   static int restart;
   static int testing;
   static int all_ships;
   static int text_num;
   static int last_field;
  public:
   enum{RESTART=1,KILL};
   GameManager(int _net_mode);
   ~GameManager(void);
   char* get_typename(void) {return type;}
   void weather(Level* );
   int get_dx(void)        {return dx;}
   int get_dy(void)        {return dx;}
   int get_sublevel(void)  {return sublevel_number+1;}
   char* get_weather(void) {return weather_type;}
   void next_level(void);
   void set_quit(void)     {quit=1;}
   void set_restart(int rst)  {restart=rst;}
   int  get_restart(void)  {return restart;}
   static void set_testing(void)     {testing=1;}
   static void clear_testing(void)   {testing=0;}
   static int  get_testing(void)     {return testing;}
   void load_level(void);
   void load_events(void);
   void reload_text(void);
   static void read_all_text(void);
   int end_level(void);
   virtual void HandleEvent(int);
   int check_cur_type(int type);
   static void start(void);
   static void stop(void);
   static Text weather_text;
   static Text level_descript;
   static int code(int,int);

   static void slow_down(void);
   static void update_params(int);
   static char volume[100];
   static int  volume_number,sublevel_number;
   static Text missions[30];
   static Text types[30];
   static Text net_missions[10];
   static Text net_types[10];
   static Text game_text;
   static int  text_count;
   static void init_stuff(void);
   static void check_restart(void);
   static int  mission_object;
   static char lastobj[40];
};

#define GAMETXT GameManager::game_text.string

#endif

