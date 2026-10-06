#ifndef __GAME__
#define __GAME__

//KLASA GAME
class Game
{
   friend class Update;
 public:
   enum GameMode
   {
     RUNNING,
     DISPLAYING,
     OTHER,
   };
   enum ScrcaptMode
   {
     IDLE,
     FULL,
     NODISPINFO,
   };
   enum HeaderResult
   {
     PLAY,
     DEMO_PLAY,
     DEMO_RECORD,
   };
   enum DemoType
   {
     _UNKNOWN,
     PLAY_LASTSAVED,
     PLAY_RANDOM,
     PLAY_VOLNUMB,
     PLAY_FILENAME,
     RECORD_LEVELNUMB,
   };
   enum DemoVersion
   {
     DEMOVERSION=0x0001,
   };
 private:
   struct DemoInfo
   {
     int version;
     char mission[256];
     int skill; 
   };
   // Stored in demo files as raw bytes.
   static_assert(sizeof(DemoInfo)==264, "DemoInfo layout differs from the original");
  static int demo_playtype;
  static char *demo_filename;
  static int demo_number;
  static int  gamma_changed;
  static int  capt;
  static int  display_sfu;
  static int  display_fps;
  static int  display_org;
  static int  played_track;
  static int  work_mode;
  static int  timer;
  static int  syncfail;
  static int  sync;
  static int  sfu;
  static int  sfu_timer;
  static int  no_volumes;
  static int  volume_access_flags;
  static int  flicker;
  static int  last_progress;
  static int  last_volume_progress;
  static int  progress;
  static int  failure_detected;
  static int  paused;
  static void _clear_flags(void);
  static void make_csum(void);
  static void reset_sync(void) {sync=0;}
  static void loop_init(void);
  static void loop_quit(void);
  static void keyboard_service(void);
  static void add_volumes_progress(int percent);
  static void add_novolumes_progress(int percent);
  static void add_other_progress(int percent);
  static void add_end_progress(void);
  static void draw_progress(void);
  static void draw_net_connect(void);
  static void init_CD(void);
  static void quit_CD(void);
  static void pause_CD(void);
  static void unpause_CD(void);
  static void draw_sfu(Screen &screen);
  static void draw_fps(Screen &screen);
  static void draw_snail(Screen &screen);
  static void draw_paused(Screen &screen);
  static void draw_demorecord(Screen &screen);
  static void draw_demoplay(Screen &screen);
  static void play(void);
  static int  play_mission(void);  // port: see Game::play
  static void play_demo(void);
  static void record_demo(void);
  static void main_loop(void);
  static void header_net(void);
  static int  header_single(void);
  static int  last_sync(void) {return sync;}
  static void run_service(void);
  static void display_service(Screen& screen);
 public:
  static void play_song(void);
  static void play_CD(int track=-1,int repeat=1);
  static void stop_CD(void);
  static Text demo_text;
  static void init_all(HINSTANCE this_instance, HINSTANCE previous_instance, 
                   LPSTR command_line,int window_state);
  static void quit_all(void);
  static void go(void);
  static void set_demo_properties(int type,char *filename,int numb);
  static int  get_timer(void) {return timer;}
  static int  get_paused(void) {return paused;}
  static int  get_work_mode(void) {return work_mode;}
  static int  get_scrcapt_mode(void) {return capt;}
  static int  get_failure_detection(void) {return failure_detected;}
  static int  get_volume_access_flags(void) {return volume_access_flags;}
  static void raise_failure_detection(void) {failure_detected=1;}
  static void start_pause(void) {paused=1;}
  static void stop_pause(void) {paused=0;}
#ifdef SHAREWARE
  static Song mission_song;
#endif
};

#endif
