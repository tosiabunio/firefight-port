#ifndef _1LG_H_INCLUDED
#define _1LG_H_INCLUDED

#ifndef EXCLUDE_LIBS
#ifdef _DEBUG
#pragma comment(lib,"1lgd.lib")
#pragma comment(lib,"winmm.lib")
#else
#pragma comment(lib,"1lg.lib")
#pragma comment(lib,"winmm.lib")
#endif
#endif

#ifdef _DEBUG
#define HI_DEBUG 1
#else
#define HI_DEBUG 0
#endif

#define DATE_STAMP char Comm::date_stamp[]=__DATE__ " " __TIME__

#define LOG_INFO               if(Log::info(__FILE__,__LINE__)==NULL) (void)0; else
#define LINEUNIQUE2(id,line)   lineunique_##id##_##line
#define LINEUNIQUE1(id,line)   LINEUNIQUE2(id,line)
#define LINEUNIQUE(id)         LINEUNIQUE1(id,__LINE__)
#define LINEUNIQUE_DECLARE(id) int LINEUNIQUE(id)=Comm::set_lineunique(&LINEUNIQUE(id))
#define LINEUNIQUE_LAST(id)    (*Comm::lineunique)

//----- makra do debuggingu

#define FILE_LINE                 strcpy(Log::last_file_line,Log::info(__FILE__,__LINE__))
#define FAILURE                   LOG_INFO Comm::failure1
#define FAILURE2                  LOG_INFO Comm::failure2
#define TERMINATE                 LOG_INFO Comm::terminate
#define MESSAGE                   LOG_INFO Log::write_message
#define WARNING                   LOG_INFO Log::write_warning
#define SYSINFO                   LOG_INFO Log::write_sysinfo
#define ENGINFO                   LOG_INFO Log::write_enginfo
#define CHECK(expr) if(!(expr))   {if(!Comm::assert_box(#expr,FILE_LINE)) Comm::debug_break();} else (void)0
#define CHECK_RANGE(v,rl,rh)      {if(((v)<(rl))||((v)>(rh))) FAILURE(Comm::m_outrange,#v,v,rl,rh);}
#define RPTCNT                    LINEUNIQUE_LAST(id)
#define REPEAT(count)             for(LINEUNIQUE_DECLARE(rptcnt); LINEUNIQUE(rptcnt)<(count); LINEUNIQUE(rptcnt)++)

#if HI_DEBUG

#define DBG_FAILURE               FAILURE
#define DBG_TERMINATE             TERMINATE
#define DBG_MESSAGE               MESSAGE
#define DBG_WARNING               WARNING
#define DBG_SYSINFO               SYSINFO
#define DBG_ENGINFO               ENGINFO
#define DBG_CHECK(expr)           CHECK(expr)
#define DBG_CHECK_RANGE(v,rl,rh)  CHECK_RANGE(v,rl,rh)
#define DBG_ONLY

#else

#define DBG_FAILURE               (void)
#define DBG_TERMINATE             (void)
#define DBG_MESSAGE               (void)
#define DBG_WARNING               (void)
#define DBG_SYSINFO               (void)
#define DBG_ENGINFO               (void)
#define DBG_CHECK(expr)           ((void)0)
#define DBG_CHECK_RANGE(v,rl,rh)  ((void)0)
#define DBG_ONLY                  if(1) (void)0; else

#endif

//----- log

class Log
{
public:
  static void init (char *_name="lastexec.log");

  // ponizszych metod i zmiennych NIE NALEZY uzywac
  //
  static char name[_MAX_PATH];
  static char info_file_line[], last_file_line[];
  static char *info (char *file,int line);

  static void write_message  (char *message, ...);
  static void write_warning  (char *message, ...);
  static void write_failure  (char *message, ...);
  static void write_sysinfo  (char *message, ...);
  static void write_enginfo  (char *message, ...);
  static void write_string   (char *c, int flush=0);

private:
  static void quit (void);
};

class Comm
{
public:

  static HWND      hwnd;
  static HINSTANCE hinstance;
  static int       debug;

  static void      quit_me (void (*quit_proc) (void), char *quit_id, char *ids_required);
  static void      failure (int ec);
  static void      failure1 (char *message, ...);
  static void      failure2 (int ec, char *message, ...);
  static void      terminate (char *message, ...);

  static           LRESULT CALLBACK window_proc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
  // Port: the message pump runs on SDL. It polls SDL events, runs the tick service (the
  // timers, now on the main thread) and turns SDL_QUIT into Closed. wait_messages blocks
  // until an event arrives or ms elapse.
  static void      process_messages (void);
  static void      wait_messages (unsigned ms);
  static void      (*tick_service) (void);
  static void      debug_break (void);
  static int       show_message_box (char *text, int ask);
  static char      pref_path[];        // settings, pilots and log; ends with a separator
  static int       headless;           // no window and no dialogs
  static unsigned  quit_time;          // SDL_GetTicks() after which the pump acts as if closed; 0: never

  typedef int (*wpp) (int *result, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
  static wpp       set_window_proc (wpp wp);
  static void      set_def_window_proc (WNDPROC w=NULL);

  static int       assert_box (char *expression, char *fileline);
  static int       allow_assertbox;

  static void      (*reinit_mouse) (RECT *rc, int sx, int sy);

  static char      m_outrange[];
  static int       *lineunique;
  static int       set_lineunique(int *adr) {lineunique=adr; return(0);}

  static void      begin_load_section();
  static void      end_load_section();
  static void      begin_run_section();
  static void      end_run_section();

  static int       exit_code;
  static int       standby;
  static void      (*init_info)  (char *text);
  static int       (*set_cancel_button) (int state);
  static char      date_stamp[];
  static int       production;

  static char      working_path[];
  static char      *get_working_path(void);
  static char      *get_file_path(char *name);
};

struct Failure {};
struct Closed  {};
extern int critical_error_occurred;

// The quit manager (1lg_comm.cpp) must be created before and shut down after every static
// object that uses the engine. MSVC did this with #pragma init_seg(lib), which GCC and Clang
// ignore. Every translation unit that includes this header constructs a Comm_init before its
// own statics (the iostream "nifty counter"), and the last one destroyed runs the final quits.
struct Comm_init
{
  Comm_init();
  ~Comm_init();
};
static Comm_init comm_init;

//----- formatowanie komunikatow

extern char formated_message[];
char *format_message (char *message, ...);
#define va_format_message(format) { \
  char buf[1024]; \
  va_list argptr; \
  va_start(argptr, format); \
  _vsnprintf(buf, 1023, format, argptr); \
  va_end(argptr); \
  strcpy(formated_message,buf); }

#endif