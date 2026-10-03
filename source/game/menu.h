#ifndef __MENU__
#define __MENU__

const int MAX_KEYS=100;
const int MAX_KEYS_LEN=100;

//KLASA MENU
class Item;
class User;
class Menu : public Heap_object
{
   friend class Item;
   friend class TextItem;
   friend class SubItem;
   friend class ButtItem;
   friend class RButtItem;
   friend class ZippItem;
  public:
   enum {max_items=25,recurse_depth=10};
  private:
   enum MenuType
   {
     OPTIONS,
     DISPLAY,
     SOUND,
     CONTROLS,
     QUIT,
     CHEAT,
     ABORTMISSION,
     RESTARTMISSION,
     HELP,
     REGISTER,
     GAME,
   };
   static int initialized;
   static int flushing;
   static int ignore_quit;
   static Text descript_text;
   static Sprite titles_sprite_in;
   static Sprite titles_sprite_out;
   static Tool box_tool;
   static Tool letter_in_tool;
   static Tool letter_out_tool;
   static int between_y;
   static int x;
   static int y;
   static int glass_x;
   static int glass_y;
   static int glass_sx;
   static int glass_sy;
   static int sprite_x;
   static int sprite_y;
   static Menu *recurse_buf[recurse_depth];
   static int recurse_cursor;
   static void push_menu(Menu *menu);
   static void pop_menu(void);
   static Menu *top_menu(void);
   static int prev_fx;
   static int delay_time;
   static int time_fx;
   static int enable_game_menus;
   static int register_phase;
   static int vpalette_beforeregister;
   static void draw_help(Screen &screen);
   static void draw_register(Screen &screen);
  private:
   MenuType menutype;
   int first_time;
   char *label_name;
   int size;
   int active;
   Item *items[Menu::max_items];
   struct Menuinfo : public Heap_object
   {
     public:
      unsigned char this_letter;
      unsigned char next_letter;
      int x;
      int y;
   } menuinfo[Menu::max_items];
   int title_phase;
   int freezed_keys;
  private:
   Menu(char *_label_name,MenuType _menutype);
   virtual ~Menu(void);
   char run(void);
   MenuType type(void);
   void draw(Screen& screen,int draw_active=1);
   int run_item(char letter);
   int run_item(int number);
   int run_item(void);
   Item *get_item(char letter);
   Item *get_item(int number);
   Item *get_item(void);
   void set_active(char letter);
   void set_active(int number);
   void set_return_active(char letter);
   void set_return_active(int number);
   void set_return_active(void);
   int  find_first_selectable(int dir);
   char get_active_letter(void);
   int  get_active_number(void);
   char get_letter(int number);
   int  get_number(char letter);
   void freeze_keys(void) {freezed_keys=1;}
   void unfreeze_keys(void) {freezed_keys=0;}
  private:
   static void input_device_adjust(Menu& menu);
   static void resolution_adjust(Menu& menu);
   static void detail_level_adjust(Menu& menu);
   static void gamma_adjust(Menu& menu);
   static void fx_adjust(Menu& menu);
   static void music_adjust(Menu& menu);
   static void reverse_stereo_adjust(Menu& menu);
   static void immortal_adjust(Menu& menu);
   static void serve_options(Menu& menu,char letter);
   static void serve_display(Menu& menu,char letter);
   static void serve_sound(Menu& menu,char letter);
   static void serve_controls(Menu& menu,char letter);
   static void serve_quit(Menu& menu,char letter);
   static void serve_cheat(Menu& menu,char letter);
   static void serve_abortmission(Menu& menu,char letter);
   static void serve_restartmission(Menu& menu,char letter);
   static void serve_help(Menu& menu,char letter);
   static void serve_register(Menu& menu,char letter);
   static void serve_game(Menu& menu,char letter);
   static void display_string(Screen& screen,int x,int y,char *text,int col=Color::white);
   static char key_table[2*MAX_KEYS][MAX_KEYS_LEN];
  public:
   static void init(char *_descript,char *_titles_in,char *_titles_out);
   static void done(void);
   static void serve(void);
   static void display(Screen &screen);
   static void flush(void);
   static int  in_use(void);
   static void options(int enable=0);
   static void display(void);
   static void sound(void);
   static void controls(void);
   static void quit(void);
   static void cheat(void);
   static void abortmission(void);
   static void restartmission(void);
   static void help(void);
   static void register_menu(void);
   static void game(void);
};

#endif



