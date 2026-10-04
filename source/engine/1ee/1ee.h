#ifndef _1EE_H_INCLUDED
#define _1EE_H_INCLUDED

#include <compat/win32.h>

#ifndef EXCLUDE_LIBS
#ifdef _DEBUG
#pragma comment(lib,"1eed.lib")
#else
#pragma comment(lib,"1ee.lib")
#endif																	 
#endif
#pragma comment(lib,"winmm.lib")

class Eem_net_sync_failure: public Failure {};
class Eem_demo_sync_failure: public Failure {};

//----------------------------------------------------------------------------
// Event masks
//----------------------------------------------------------------------------
#define ev_Kbd    0x01000000
#define ev_Timer  0x02000000
#define ev_Mouse  0x04000000
#define ev_Joy    0x08000000

struct Eem_error
{
  enum
  {
    general = 100,
    invalid_registry,
    error_connecting,
    dp_not_installed,
    dp_general,
    net_game_not_found,
    net_player_not_found,
    net_incompatible_version,
    net_lost_connection,
    net_too_many_errors
  };
};

//----------------------------------------------------------------------------
// Array sizes
//----------------------------------------------------------------------------
#define TIMER_ARRAY_SIZE      10           // maksymalna ilosc timerow w systemnie

#define KBD_REF_SIZE         256
#define KBD_NAMES_SIZE       256
#define KBD_ARRAY_SIZE        32           // rozmiar tablicy kontrolnej klawiatury
#define KBD_STRING_SIZE        8

#define MOUSE_REF_SIZE        15
#define MOUSE_ARRAY_SIZE       1           // rozmiar tablicy kontrolnej myszy
#define MOUSE_NAMES_SIZE       4           

#define VKEY_REF_SIZE         32
#define VKEY_ARRAY_SIZE        6

#define JOY_ARRAY_SIZE         6
#define JOY_REF_SIZE          48
#define JOY_NAMES_SIZE        48

#define EEM_VERSION   0x00020000

#define USER_BLOCK_DATA_SIZE 100
//===========================================================================
// Net
//===========================================================================

typedef BOOL(*Timeout_function)(void);

class Net
{
private:
  enum 
  {
    buffer_size     = 120,
    max_resends     = 3,
    req_array_size  = 120,
    res_array_size  = 120,
    rec_buf_size    = 4096
  };
public:
  enum 
  {
    game_name_len       = 32,
    game_info_len       = 28,
    provider_name_len   = 80, 
    player_name_len     = 52,
    max_enum_providers  = 10, 
    max_enum_players    =  4,   
    max_enum_games      = 10,
    max_packet_size    = 255
  };
  enum
  {
    res_OK              = 0,
    res_enum_overflow   = 1,
    res_enum_timeout    = 2,
    res_enum_error      = 3
  }; 
  struct Game
  {
    DWORD id;
    DWORD max_players;
    char  name[game_name_len];
    char  info[game_info_len];
  };
  struct Provider
  {
    GUID id;
    char name[provider_name_len]; 
  };
  struct Player
  {
    DPID id;
    char name[player_name_len]; 
  };
private:
#pragma pack(push, msg, 1)
  struct Msg_header
  {
    unsigned short crc16;
    unsigned char  number;
    unsigned char  old_number;
    unsigned char  size;
    unsigned char  msg_type;
  };
  struct Msg                 
  {
    Msg_header  header;
    unsigned char data[max_packet_size];
  };  
  struct Buffer
  {
    Queue<Msg>            in;
    Queue<Msg>            out;
    DPID                  player;
    unsigned char         last_in;
    unsigned char         last_out;
    unsigned char         msg_counter;    
    Queue<unsigned char>  requests;
    Queue<unsigned char>  resends;
  };
#pragma pack (pop, msg)
private:
  static HANDLE event;
  static char     rec_buf[rec_buf_size];
  static unsigned rec_size;
  static DPID     rec_from;
  static unsigned diag_total_out;
  static unsigned diag_total_in;
  static unsigned diag_resend;
  static unsigned diag_max_packet;
  static unsigned diag_total_size;
  static unsigned diag_max_buf_usage;
  static unsigned diag_trans_errors;
  static unsigned diag_corrupted_packets;
private:
  static Array<Game>     games;    
  static Array<Provider> providers;
  static Array<Player>   players;

  static Bitflag status;
  static int     games_qty;
  static int     providers_qty;
  static int     players_qty;
  static GUID    game_guid;
  static int     enum_error;
  static LPDIRECTPLAY  driver;
  static Game          current_game;
  static Player        current_player;
  static BOOL          session_OK;
  static DPCAPS        dpcaps;
  static DPSESSIONDESC current_session;
  static int           enum_retries; 
  static int           enum_timeout; 

  static Array<Buffer>   *msgbuf;

  static void quit (void);         

  static BOOL PASCAL enum_players   (DPID id, LPSTR friendly, LPSTR formal, DWORD flags, LPVOID user);
  static BOOL PASCAL enum_games     (LPDPSESSIONDESC game, LPVOID user, LPDWORD timeout, DWORD flags);
  static BOOL PASCAL enum_providers (LPGUID id, LPSTR name, DWORD major_v, DWORD minor_v, LPVOID user);

  static int get_buf_pos (DPID player);
  static int get_player_pos (DPID player);

  static void         api_send (void *buffer, unsigned size, DPID to);
  static BOOL      api_receive (void);

  static void           resend (unsigned char number, int pos);
  static void          request (int pos);
  static void     queue_resend (unsigned char number, int pos);
  static void      dump_buffer (int pos);
  static void resolve_requests (Msg *msg, int pos);
  static void  resolve_resends (Msg *msg, int pos);
  static BOOL  perform_resends (void);
  static void clear_in_buffers (void);

public:
  
  static void ping    (void);
  static void purge   (void);

  static void init (int debug);
  static void open (GUID game, GUID *provider);
  static void close (void);

  static void game_create  (int max_players, char *name, char *player_name, char* game_info, unsigned timeout);
  static void game_connect (DWORD game_id, char *player_name, unsigned timeout);
  static void game_destroy (void);
  
  static void game_lock (void);
  static void game_unlock (void);

  static void send     (void *buffer, unsigned size, DPID to);
  static BOOL receive  (void *buffer, unsigned *size, DPID *from);

  static Game     *list_games     (unsigned *count=NULL, int *result=NULL, int timeout=5000, 
                                   int retries=0, Timeout_function function=NULL);
  static Provider *list_providers (unsigned *count=NULL, int *result=NULL);
  static Player   *list_players   (unsigned *count=NULL, int *result=NULL);

  static Game     *get_current_game(void);
  static Player   *get_current_player(void);
  static inline unsigned get_players_num(void) {return (players_qty);};
  static inline BOOL     is_session_ok(void) {return (session_OK);};
  static inline int      get_max_players () {return (current_game.max_players);};
  static BOOL   is_initialised ();
};

//===========================================================================
// Joy
//===========================================================================

// Port: an SDL game controller, else a plain SDL joystick, in place of the winmm joystick API.
// It still yields the original state: 32 buttons and 6 axes read as digital directions.
class Joy
{
friend class Eem;
  static Bitflag status;
  static int      dead;
  static int      last_1;
  static int      last_2;
  static int      last_3;
  static unsigned diag_busy;
  static char *names[JOY_NAMES_SIZE];
  static void  next_read (void);
  static void  get_event (int event);
  static void  clear (void);
  static void  complete (void);
	static void  callback_joy (void);
  static void  init_ref(void);
  static void  sdl_event (const SDL_Event &event);
  static void  open_device (int index);
  static void  close_device (void);
  static void  push_state (unsigned char *joy_state);
public:
  enum
  {
    no_struck = 47,
  };
  enum
  {
    button1=0,  button2,  button3,  button4,  button5,  button6,  button7,  button8,  button9,  button10,
    button11,   button12, button13, button14, button15, button16, button17, button18, button19, button20,
    button21,   button22, button23, button24, button25, button26, button27, button28, button29, button30,
    button31,   button32,
    x_minus, x_plus,
    y_minus, y_plus,
    z_minus, z_plus,
    r_minus, r_plus,
    u_minus, u_plus,
    v_minus, v_plus,
    left   = x_minus,
    right  = x_plus,
    up     = y_minus,
    down   = y_plus,
    anybutton = 47,
    any       = 47
  };
  static void  init (int dead_percent=5);
  static void  quit (void);

  static int  is (int code);
  static int  struck (int code);
  static int  press  (int code);
  static void set_dead_zone (int dead_percent);
  static int  release (int code);
  static int  is_initialized(void);
  static char *get_key_name(int code);
  static char *get_scan_name(unsigned char scan);
  static unsigned char get_scan(int code);
  static char *get_struck_name(void);
  static unsigned char get_struck(void);
};

//===========================================================================
// Mouse
//===========================================================================

// Port: SDL mouse events in place of the WH_MOUSE hook. Coordinates are window coordinates (were
// screen coordinates), and the clip rectangle is the picture's area in the window (Video).
class Mouse
{
friend class Eem;
  static Bitflag status;
  static unsigned char ref[MOUSE_REF_SIZE];
  static char *names[MOUSE_NAMES_SIZE];
  static int   last_x;
	static int   last_y;
  static RECT  cr;
  static RECT  *clip_rect;
	static int   screen_sx;
	static int   screen_sy;
  static int   pointer_x;
  static int   pointer_y;
  static int   moved;
  static int   relative;
  static void  init_ref(void);
  static void  next_read (void);
  static void  get_event (int event);
  static void  clear (void);
  static void  complete (void);
  static void  sdl_event (const SDL_Event &event);
  static void  push_position (void);
  static void  flush (void);
  static void  update_relative (void);
public:
  enum
  {
    no_struck = 0,
  };
  enum
	{
	  anybutton=0,
		any_button=0,
		any=0,
		left,
		leftbutton,
		left_button,
		right,
		rightbutton,
		right_button,
		middle,
		middlebutton,
		middle_button,
		mid,
		midbutton,
		mid_button,
	};
  static void init (void);
  static void quit (void);
  static int  is      (int code);
  static int  struck  (int code);
  static int  press   (int code);
  static int  release (int code);
  static int  dblclk  (int code);
  static void get_xy  (int *x, int *y); 
	static void goto_xy (int x, int y);
  static int  is_initialized(void);
  static void set_ex_param (RECT *rc, int sx, int sy);
  // Port: capture the mouse (SDL relative mode) while the window has focus, for mouse steering.
  static void set_relative (int on);

  static char *get_key_name(int code);
  static char *get_struck_name(void);
  static unsigned char get_struck(void);
  static char *get_scan_name(unsigned char scan);
  static unsigned char get_scan(int code);
};

//===========================================================================
// Kbd
//===========================================================================

// Port: SDL key events in place of the WH_KEYBOARD hook, still as PC set-1 scan codes.
class Kbd
{
friend class Eem;
  static Bitflag status;
  static int string_idx;
  static unsigned diag_string_overflows;
	static int  last_key;
  static unsigned char ref[KBD_REF_SIZE][2];
  static char *names[KBD_NAMES_SIZE];
  static void init_ref(void);
  static void next_read (void);
  static void get_event (int event);
  static void clear (void);
  static void complete (void);
  static void sdl_event (const SDL_Event &event);
public:
  enum
  {
    no_struck = 0,
  };
  enum
  {
    anykey=0,
    any_key=0,
    key0='0', key1, key2, key3, key4, key5, key6, key7, key8, key9,
    keya='a', keyb, keyc, keyd, keye, keyf, keyg, keyh,
    keyi, keyj, keyk, keyl, keym, keyn, keyo, keyp, keyq,
    keyr, keys, keyt, keyu, keyv, keyw, keyx, keyy, keyz,
    f1=1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12,
    backapostrophe ='`',
    backslash      ='\\',
    backspace      ='<',
    lbracket       ='[',
    rbracket       =']',
    semicolon      =';',
    apostrophe     ='\'',
    point          ='.',
    comma          =',',
    space          =' ',
    spacebar       =' ',
    left           ='L', // num || ext
    right          ='R', // num || ext
    up             ='U', // num || ext
    down           ='D', // num || ext
    middle         ='M', // num
    tab            ='T',
    plus           ='+', // num
    minus          ='-', // num || ext
    asterisk       ='*', // num
    slash          ='/', // num || keyb
    equal          ='=',
    enter          ='E', // num || keyb
    esc            ='C', // (C od Cancel)
    shift          ='!', // l || r
    ctrl           ='^', // l || r
    alt            ='@', // l || r

    home = 128,       // num || ext            PIERWSZY BEZ ODPOWIEDNIKA
    end,              // num || ext
    pageup,           // num || ext
    pgup,             // num || ext
    pagedown,         // num || ext
    pagedn,           // num || ext
    pgdn,             // num || ext
    pgdown,           // num || ext
    ins,              // num || ext
    del,              // num || ext

    printscreen,
    prtsc,
    prtscr,
    pause,

    lshift, lalt, lctrl,
    rshift, ralt, rctrl,
    gray_plus, gray_minus,
    capslock, scrolllock, numlock,

    keyb_slash, keyb_enter, keyb_0, keyb_1, keyb_2, keyb_3,
    keyb_4, keyb_5, keyb_6, keyb_7, keyb_8, keyb_9, keyb_minus,

    ext_insert, ext_ins, ext_home, ext_pgup, ext_pageup, ext_delete, ext_del, ext_end,
    ext_pgdown, ext_pgdn, ext_pagedown, ext_pagedn, ext_up, ext_down, ext_left, ext_right,

    num_insert, num_ins, num_home, num_pgup, num_pageup, num_delete, num_del, num_end,
    num_pgdown, num_pgdn, num_pagedown, num_pagedn, num_up, num_down, num_left, num_right,
    num_middle, num_slash, num_minus, num_plus, num_enter,
    num_asterisk, num_0, num_1, num_2, num_3, num_4, num_5,
    num_6, num_7, num_8, num_9
  };
private:
   static char upper_scan (char ch);
public:
  static void init ();
  static void quit (void);
  static int  is (int code);
  static int  struck (int code);
  static int  press  (int code);
  static int  release (int code);
  static char *get_string(void);
  static int  is_initialized(void);
  static char *get_key_name(int code);
  static char *get_scan_name(unsigned char scan);
  static unsigned char get_scan(int code);
  static char *get_struck_name(void);
  static unsigned char get_struck(void);
};

//===========================================================================
// Vkey
//===========================================================================

class Vkey
{
friend class Eem;
public:
  enum
  {
    dev_Kbd   = 1,
    dev_Mouse = 2,
    dev_Joy   = 3,
    any       = 0,
    anykey    = 0,
    any_key   = 0,
  };
private:
  struct Item
  {
    int device;
    unsigned char scan;
    char *name;
  };
  static Bitflag status;
  static Array<Item> *array;
  static int quantity;
  static void next_read (void);
  static void clear (void);
  static void complete (void);
public:
  static void init ();
  static void quit (void);
  static void reset_content(void);
  static int  set(int device, unsigned char scan, char *name);
  static int  is (int code);
  static int  struck (int code);
  static int  press  (int code);
  static int  release (int code);
  static int  is_initialized(void);
  static char *get_key_name(int code);
  static char *get_scan_name(int scan);
};

//===========================================================================
// Timer
//===========================================================================

typedef void(*Timer_function)(void);
//...........................................................................
class Timer_object
{
public:
  virtual void tick(void)=0;
};
//...........................................................................
// Port: the timers run on the main thread. Comm::process_messages calls service(), which
// fires every timer that is due (was a multimedia-timer thread, timeSetEvent).
class Timer
{                            
  struct Timer_item
  {
    char *name;
    int   id;
    Timer_object   *object;
    Timer_function function;
    unsigned int   *counter;
    unsigned long long period;    // in SDL performance-counter units
    unsigned long long next_due;
  };
  static Bitflag    status;
  static int        accuracy;
  static int        last_id;
  static Timer_item timers[TIMER_ARRAY_SIZE];

  static void quit(void);
  static void fire(Timer_item &timer);
public:
  static void service(void);
  static unsigned ms_to_next(void);
  static void init(int accuracy=1, int critical_accuracy=1);
  static int add(int interval, Timer_function function, 
                               Timer_object *object, 
                               unsigned int *counter, char *name=NULL); 
  static int add(int interval, Timer_function function, char *name=NULL); 
  static int add(int interval, Timer_object *object, char *name=NULL); 
  static int add(int interval, unsigned int *counter, char *name=NULL); 
  static void kill(int timer_id);
  static void kill(char *name);
  static int is_initialized(void);
};

//===========================================================================
// Eem
//===========================================================================

class Eem
{
friend class Kbd;
friend class Mouse;
friend class Vkey;
friend class Joy;
public:
  enum
  {
    // maski do stanow
    mask_None          = 0x00000000,
    mask_All           = 0xFFFFFFFF,
    mask_Keyboard      = 0x00000001,
    mask_Mouse         = 0x00000002,
    mask_Vkey          = 0x00000004,
    mask_Joy           = 0x00000008,
  };
private:
  enum
  {
    // flagi pakietow
    flag_Server        = 0x01,       // pakiet pochodzi od servera
    // typy pakietow
    pkt_States         = 0x01,
    pkt_UserBlock      = 0x02,

    // flagi klasy Eem
    eem_AutoActive     = 0x00000001, // Eem aktywny - nie zablokowany automatycznie
    eem_Active         = 0x00000002, // Eem aktywny, czyli nie zastopowany funkcja stop
    eem_Timed          = 0x00000004, // Eem z dzielnikiem czasowym
    eem_Debug          = 0x00000008, // Tryb debug (z nieprecyzyjna synchronizacja)
    eem_Initialized    = 0x00000010, // Eem zainicjalizowany
    eem_Multi          = 0x00000020, // Eem w trybie sieciowym
    eem_Server         = 0x00000040, // Eem w trybie serwera (Net::create_game)
    eem_EventCompleted = 0x00000080, // ostatni kwant zdarzen zostal w calosci zinterpretowany
    eem_NetWait        = 0x00000100, // stan oczekiwania na event sieciowy
    eem_PktCompress    = 0x00000200, // wlaczona kompresja pakietow
    eem_CheckSync      = 0x00000400, // tryb z kontrola synchronizacji sieciowej
    eem_LastTimer      = 0x00000800, // ustawiony jezeli w trybie bez synchronizacji w ostatnim
                                     // readzie byl zasymulowany ev_Timer
    eem_AfterClear     = 0x00001000, // 
    eem_InNetActive    = 0x00002000,
    eem_UserBlock      = 0x00004000, // jest do wyslania user_block

    eem_DemoRecord     = 0x00010000,
    eem_DemoPlay       = 0x00020000,
    eem_DemoActive     = 0x00040000,

    // flagi gracza
    pl_Deleted         = 0x00000001, // gracz skasowany
    pl_UserBlock       = 0x00000002, // gracz przyslal user_block, ktory jeszcze nie zostal odebrany
  };
 #pragma pack(push, st, 1)
  struct St_joy
  {
    unsigned char prev_arr_state[JOY_ARRAY_SIZE];
    unsigned char arr_state[JOY_ARRAY_SIZE];
  };
  struct St_mouse
  {
	  unsigned char arr_state;
	  unsigned char arr_struck;
	  unsigned char arr_release;
  	unsigned char arr_dblclk;
    short  x;
    short  y;
  };                         
  struct St_kbd
  {
    unsigned char arr_state  [KBD_ARRAY_SIZE];
    unsigned char arr_struck [KBD_ARRAY_SIZE];
    unsigned char arr_release[KBD_ARRAY_SIZE];
    char string[KBD_STRING_SIZE];
  };
  struct St_vkey
  {
    unsigned char arr_state  [VKEY_ARRAY_SIZE];
    unsigned char arr_struck [VKEY_ARRAY_SIZE];
    unsigned char arr_release[VKEY_ARRAY_SIZE];
  };
  struct Player_state
  {
    unsigned char type;
    unsigned chknumber;
    unsigned user_sync;
    unsigned char flags;
    unsigned char user_flags;
    unsigned char serie;
    St_mouse mouse;
    St_kbd   kbd;
    St_vkey  vkey;
    St_joy   joy;
  };
  // Sent over the network and stored in demos as raw bytes.
  static_assert(sizeof(Player_state)==154, "Player_state layout differs from the original");
  struct User_block
  {
    unsigned char type;
    unsigned char size;
    unsigned char data[USER_BLOCK_DATA_SIZE];
  };
 #pragma pack(pop, st)
  struct Player
  {
    unsigned id;
    Bitflag  status;
    char     player_name[Net::player_name_len];
    Queue <Player_state>  states;
    Queue <unsigned>      sync_queue;
    User_block            user_block; 
  };

  static User_block user_block;
  static Bitflag status;
  static Bitflag net_mask;
  static Bitflag last_send_mask;
  static int   last_return_code;
	static DWORD thread;
  static int   *queue;
	static int   timer_id;
	static int   timer_frq;
public:
  // Port: run without the clock (--fast): one tick per read, as fast as possible, while the
  // game still counts 1/frequency seconds per tick.
  static int   untimed;
  static int   dump;     // port: input_dump=1, see dump_state
private:
  static int   queue_tail;
  static int   queue_head;
  static int   queue_size;
  static int   queue_free;
  static int   queue_last;
  static Comm::wpp old_window_proc;

  static Queue<unsigned>      *sync_queue;
  static Queue<unsigned char> *user_queue;
  static Array<Player>        *players;
  static Player_state          old_state;
  static Player_state         *new_state;
  static int             players_max;
  static int             local_player;
  static int             demo_player;
  static int             host_player;
  static int             selected_player;
  static int             server_player;
  static int             buffer_delay;
  static int             queue_delay;
  static unsigned char   serie;
  static unsigned        ping_counter;
  static unsigned        max_ping;
  static int      diag_win_queue_usage;
  static int      diag_max_win_queue_usage;

  static int   pop_event (void);
  static void  clear (void);
  static void  next_read (void);
  static void  quit(void);
  static void  callback_timer (void);	
  static void  push_event(int event);

  static void  init_multi ();

  static int   window_proc(int *presult, HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam);
  static void  sdl_event(const SDL_Event &event);
  static int   get_player_pos(unsigned player);
  static void  send_state     (void);
  static void  receive_states (void);
  static int   event_available(void);
  static void  forget_event (void);
  static void  chk_sync (void);
  static void  chk_user_sync(void);
  static void  chk_internal_sync(void);
  static void  error_dump_all(void);
  static void  error_dump_player(Player *player);
  static void  error_dump_state(Player_state *state);
  static void  dump_state(void);


  static inline Player_state *get_new();
  static inline Player_state *selected();

public:
  static void  demo_set_record_mode (char *file_name = NULL, unsigned max_size=1000000, 
                                     char *user_data = NULL, unsigned user_data_size = 0);
  static BOOL  demo_set_play_mode   (char *file_name = NULL, char *user_data = NULL, 
                                     unsigned *user_data_size = NULL);
  static BOOL  demo_is_active();
  static void  demo_terminate();

  static void user_block_send(unsigned char *buffer, unsigned size);
  static BOOL user_block_receive(unsigned char *buffer, unsigned *size);
  static BOOL user_block_available();

  static void init (int size, int frequency, int network_supported, int check_sync, 
                    int compress_packets, int debug, int joy_dead_percent);
  static void          start(void);
  static void          stop(void);
  static void          terminate(void);
  static int           read(unsigned user_sync_data = 0, unsigned char user_flags = 0);
  static unsigned char get_user_flags(void);

  static inline int is_initialized(void);
  static inline int is_network_mode (void);
  static inline int is_server(int player);

  static inline int    get_frequency(void);
  static inline DWORD  get_thread(void);
  static inline void   select (int player);
  static inline int    is_present(int player);
  static inline int    is_local(int player);
  static inline int    is_selected(int player);
  static inline int    get_players_num(void);
  static inline char  *get_player_name(int player);
  static inline void   set_net_mask (int mask);
  static inline int    get_net_mask (void);
  static void complete (void);
};


//===========================================================================
inline Eem::Player_state *Eem::get_new() 
{ 
  return (new_state);
}
//---------------------------------------------------------------------------
inline Eem::Player_state *Eem::selected() 
{
  if ((*players)[selected_player].states.getquantity()>0)
    return ((*players)[selected_player].states.peek());
  else
    return (NULL);
}
//---------------------------------------------------------------------------
inline int Eem::is_initialized(void)
{
  return (status.is(eem_Initialized));
}
//---------------------------------------------------------------------------
inline int  Eem::is_network_mode ()
{
  return (status.is(eem_Multi));
}
//---------------------------------------------------------------------------
inline int Eem::is_server(int player)
{
  if (is_local(player))
    return(status.is(eem_Server));
  else
  {
    DBG_CHECK ((server_player>=1)&&(server_player<=players_max));
    return (player==server_player);
  }
}
//---------------------------------------------------------------------------
inline int Eem::get_frequency(void)
{
  return (timer_frq);
}
//---------------------------------------------------------------------------
inline DWORD Eem::get_thread(void)
{
  return (thread);
}
//---------------------------------------------------------------------------
inline void Eem::select (int player)
{
  selected_player = player?player:local_player;
}
//---------------------------------------------------------------------------
inline int Eem::is_present(int player)
{
  return (!((*players)[player?player:local_player].status.is(pl_Deleted)));
}
//---------------------------------------------------------------------------
inline int Eem::is_local(int player)
{
  return ((player?player:local_player)==local_player);
}
//---------------------------------------------------------------------------
inline int Eem::is_selected(int player)
{
  return ((player?player:local_player)==selected_player);
}
//---------------------------------------------------------------------------
inline int Eem::get_players_num()
{
  return (players_max);
}
//---------------------------------------------------------------------------
inline char* Eem::get_player_name(int player)
{
  return ((*players)[player?player:local_player].player_name);
}
//---------------------------------------------------------------------------
inline void Eem::set_net_mask (int mask)
{
  net_mask.clear();
  net_mask.set(mask);
}
//---------------------------------------------------------------------------
inline int Eem::get_net_mask (void)
{
  return (net_mask.data);
} 
//---------------------------------------------------------------------------


#endif
