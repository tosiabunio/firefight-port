#include "1lg_hdrs.h"
#include <compat/browser.h>
#include <atomic>
#include <new>
#include <SDL.h>
#pragma init_seg(lib)

static char assert_message[]="assertion (%s) failed in %s";

HWND        Comm::hwnd=NULL;
HINSTANCE   Comm::hinstance=NULL;
int         Comm::debug=0;;
int         Comm::allow_assertbox=1;
int         Comm::exit_code=0;
int         Comm::standby=0;
char        Comm::pref_path[_MAX_PATH]="";
int         Comm::headless=0;
unsigned    Comm::quit_time=0;
void        (*Comm::tick_service) (void)=NULL;
void        (*Comm::input_proc) (const SDL_Event &event)=NULL;
int         Comm::production=1;
char        Comm::working_path[_MAX_PATH];

static int  default_window_proc (int*, HWND, UINT, WPARAM, LPARAM) {return(0);}
static      Comm::wpp user_window_proc=default_window_proc;

static void default_reinit_mouse (RECT *, int, int) {}
void        (*Comm::reinit_mouse) (RECT *rc, int sx, int sy)=default_reinit_mouse;

int         *Comm::lineunique;
char        Comm::m_outrange[]="out of range: %s=%d (less than %d or more than %d)";

static void default_init_info  (char *) {}
void        (*Comm::init_info) (char *text)=default_init_info;

static int  default_set_cancel_button (int state) {(void)state; return(0);}
int         (*Comm::set_cancel_button) (int)=default_set_cancel_button;

unsigned    app_starttime,load_starttime,run_starttime,in_loadsection,in_runsection;
unsigned    load_total,run_total;

inline char* timestr(unsigned start, unsigned end)
{
  unsigned dt=end-start;
  unsigned h=(dt/(1000*60*60))%60;
  unsigned m=(dt/(1000*60))%60;
  unsigned s=(dt/(1000))%60;
  unsigned x=(dt%1000)/100;
  format_message("%02d:%02d:%02d.%d",h,m,s,x);
  return(formated_message);
}

//----- quit manager ----------------------------------------------------------------------------

struct Quit_man
{
  enum { max_quits=64 };

  void (*quit_proc_tab[max_quits]) (void);
  char *quit_id_tab[max_quits];
  int  quit_prior_tab[max_quits];
  int quits;

  Quit_man ()
  {
    app_starttime=timeGetTime();
    quits=0;
  }

  ~Quit_man()
  {
    do_quit(0);
  }

  void do_quit (int prior)
  {
    if(in_loadsection) Comm::end_load_section();
    if(in_runsection) Comm::end_run_section();
    unsigned app_time=timeGetTime()-app_starttime;
                   ENGINFO("application time: %s",timestr(0,app_time));
    if(load_total) ENGINFO("data update time: %s (%d%%)",timestr(0,load_total),load_total*100/app_time);
    if(run_total)  ENGINFO("interactive time: %s (%d%%)",timestr(0,run_total),run_total*100/app_time);


    MESSAGE("performing quits...", (char*)(prior ?" (%s priority only)" :""));
    for(int i=quits-1; i>=0; i--)
      if(quit_prior_tab[i]>=prior)
      {
        MESSAGE("deinitializing %s",quit_id_tab[i]);
        try
        {
          (*quit_proc_tab[i])();
          quit_prior_tab[i]=-1;
        }
        catch(Failure)
        {
          MESSAGE("deinitialization of %s failed",quit_id_tab[i]);
        }
      }
  }

  void quit_me (void (*quit_proc) (void), char *quit_id, char *ids_required)
  {
    if(quits>=max_quits)
      FAILURE ("quit manager overflow");

    int prior=0;
    if(quit_id[0]=='*')
    {
      quit_id++;
      prior++;
    }

    if(ids_required&&strlen(ids_required))
    {
      char buf[80];
      do
      {
        buf[0]=0;
        while((*ids_required)==' ') ids_required++;
        if((sscanf(ids_required,"%s",buf)>0)&&(buf[0]!=0))
        {
          ids_required+=strlen(buf);
          int found=0;
          for(int i=0; i<quits; i++)
            if(strcmpi(quit_id_tab[i],buf)==0)
            {
              found++;
              break;
            }
          if(!found)
            FAILURE ("can't init %s because %s is not initialized",quit_id,buf);
        }
      } while(buf[0]!=0);
    }

    quit_proc_tab[quits]=quit_proc;
    quit_id_tab[quits]=quit_id;
    quit_prior_tab[quits]=prior;
    quits++;

    MESSAGE("%s initialized", quit_id, (char*)(prior ?" (%s quit priority)" :""));
  }
};

// Constructed by the first Comm_init and destroyed by the last (see 1lg.h).
static int comm_init_count;
alignas(Quit_man) static unsigned char quit_man_storage[sizeof(Quit_man)];
#define quit_man (*reinterpret_cast<Quit_man*>(quit_man_storage))

Comm_init::Comm_init()
{
  if(comm_init_count++==0)
    new (quit_man_storage) Quit_man;
}

Comm_init::~Comm_init()
{
  if(--comm_init_count==0)
    quit_man.~Quit_man();
}

void Comm::quit_me (void (*quit_proc) (void), char *quit_id, char *ids_required)
{
  quit_man.quit_me(quit_proc,quit_id,ids_required);
}

//----- procedura okna i obsluga messagow -------------------------------------------------------

static WNDPROC def_window_proc = NULL;
static int closed,cerror;
static int last_standby=-1;

// Port: SDL events are dispatched through the original window procedure chain as the Win32
// messages they replace, so the engine modules keep their message handling.
static void dispatch_sdl_event (SDL_Event &event)
{
  switch(event.type)
  {
    case SDL_QUIT:
      Comm::window_proc(Comm::hwnd,WM_CLOSE,0,0);
      break;
    case SDL_WINDOWEVENT:
      if(event.window.event==SDL_WINDOWEVENT_FOCUS_GAINED)
        Comm::window_proc(Comm::hwnd,WM_ACTIVATEAPP,TRUE,0);
      else if(event.window.event==SDL_WINDOWEVENT_FOCUS_LOST)
        Comm::window_proc(Comm::hwnd,WM_ACTIVATEAPP,FALSE,0);
      break;
  }
  if(Comm::input_proc)
    Comm::input_proc(event);
}

void Comm::process_messages (void)
{
  browser_poll();  // port: a browser delivers input and network messages only while the game waits
  SDL_Event event;
  if((!standby)||closed)
  {
    if((last_standby!=standby)&&(!production))
    {
      last_standby=standby;
      MESSAGE("active mode entered");
    }
    while (SDL_PollEvent(&event))
      dispatch_sdl_event(event);
  }
  else
  {
    if((last_standby!=standby)&&(!production))
    {
      last_standby=standby;
      MESSAGE("stanby mode entered");
    }
    // Was GetMessage: block until something happens, but keep the timers running.
    if(SDL_WaitEventTimeout(&event,100))
      dispatch_sdl_event(event);
  }
  if(tick_service)
    tick_service();
  if((quit_time!=0)&&(closed==0)&&SDL_TICKS_PASSED(SDL_GetTicks(),quit_time))
  {
    MESSAGE("quit time reached");
    closed=1;
  }
  if(cerror)
  {
    cerror=0;
    FAILURE("critical error occured in window proc");
  }
  if(closed==1)
  {
    closed=2;
    throw Closed();
  }
}

void Comm::wait_messages (unsigned ms)
{
  if(ms>0)
    SDL_WaitEventTimeout(NULL,(int)ms);
  process_messages();
}

void Comm::debug_break (void)
{
  SDL_TriggerBreakpoint();
}

// Returns 1 for "yes" (abort), 0 for "no" (continue) and -1 for "cancel" (debug); plain
// messages (ask==0) return 1. Headless runs print to stderr and answer "yes".
int Comm::show_message_box (char *text, int ask)
{
  fprintf(stderr,"[chaos works engine] %s\n",text);
  browser_message(text);  // port: the browser's page shows it when the game ends
  if(headless)
    return(1);
  static const SDL_MessageBoxButtonData buttons[]={
    {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,1,"Yes"},
    {0,0,"No"},
    {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,-1,"Cancel"}};
  static const SDL_MessageBoxButtonData ok_button[]={
    {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT|SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,1,"OK"}};
  SDL_MessageBoxData data;
  memset(&data,0,sizeof(data));
  data.flags=ask ?SDL_MESSAGEBOX_WARNING :SDL_MESSAGEBOX_INFORMATION;
  data.title="chaos works engine";
  data.message=text;
  data.numbuttons=ask ?3 :1;
  data.buttons=ask ?buttons :ok_button;
  int button=1;
  if(SDL_ShowMessageBox(&data,&button)!=0)
    return(1);
  return(button);
}

LRESULT CALLBACK Comm::window_proc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  int result=FALSE;
  if(critical_error_occurred||closed)
    result=DefWindowProc(hwnd,msg,wparam,lparam);
  else try
  {
    if(!user_window_proc(&result,hwnd,msg,wparam,lparam))
    {
      if((closed==0)&&(msg==WM_CLOSE))
      {
        MESSAGE("WM_CLOSE received");
        closed=1;
      }
      else
      {
        if (def_window_proc)
          result=def_window_proc(hwnd,msg,wparam,lparam);
        else
          result=DefWindowProc(hwnd,msg,wparam,lparam);
      }
    }
  }
  catch(Failure)
  {
    cerror++;
  }
  return(result);
}

Comm::wpp Comm::set_window_proc (Comm::wpp wp)
{
  wpp old_wp=user_window_proc;
  user_window_proc=wp;
  return(old_wp);
}

void Comm::set_def_window_proc (WNDPROC w)
{
  def_window_proc=w;
}

//----- wyjatki i assercja ----------------------------------------------------------------------

static void ce (void)
{
  if (!critical_error_occurred)
  {
    critical_error_occurred++;
    HWND _hwnd=GetWindowLong(Comm::hwnd,GWL_STYLE) ?Comm::hwnd :NULL;
    if(_hwnd) DestroyWindow(_hwnd);
  }
}

void Comm::failure (int ec)
{
  Log::write_failure("%s",formated_message);
  if(ec)
    exit_code=ec;
  ce();
  throw Failure();
}

void Comm::failure1 (char *message, ...)
{
  va_format_message(message);
  failure(0);
}

void Comm::failure2 (int ec, char *message, ...)
{
  va_format_message(message);
  failure(ec);
}

void Comm::terminate (char *message, ...)
{
  ce();

  va_format_message(message);
  Log::write_failure("TERMINATED: %s",formated_message);

  quit_man.do_quit (1);
  raise(SIGABRT);
  _exit(3);
}

static std::atomic<int> assert_busy(0);

enum {a_debug, a_cont, a_fail};

int Comm::assert_box (char *expression, char *fileline)
{
  int busy,result=a_debug;
  busy=assert_busy.exchange(1);  // was x86 asm: xchg

  if(busy)
  {
    static int first_time=1;
    if(first_time)
    {
      WARNING("assertion dialog procedure is busy - cant't display dialog box");
      first_time=0;
    }
  }
  else
  {
    if(!allow_assertbox)
      result=a_fail;
    else
    {
      char boxmessage[1024];
      format_message(assert_message,expression,fileline);
      sprintf(boxmessage,"%s\n\nAbort program? (Cancel to debug)", formated_message);

      switch(show_message_box(boxmessage,1))
      {
        case -1:
          result=a_debug;
          break;
        case 0:
          result=a_cont;
          break;
        default:
          result=a_fail;
      }
    }
  }

  assert_busy.exchange(busy);  // was x86 asm: xchg

  if(result==a_fail)
    FAILURE(assert_message,expression,fileline);

  WARNING(assert_message,expression,fileline);
  return(result);
}

void Comm::begin_load_section()
{
  MESSAGE("---> DATA UPDATE section entered");
  load_starttime=timeGetTime();
  in_loadsection=1;
}

void Comm::end_load_section()
{
  unsigned time=timeGetTime();
  load_total+=(time-load_starttime);
  MESSAGE("---> DATA UPDATE section time %s",timestr(load_starttime,time));
  in_loadsection=0;
}

void Comm::begin_run_section()
{
  MESSAGE("---> INTERACTIVE section entered");
  run_starttime=timeGetTime();
  in_runsection=1;
}

void Comm::end_run_section()
{
  unsigned time=timeGetTime();
  run_total+=(time-run_starttime);
  MESSAGE("---> INTERACTIVE section time %s",timestr(run_starttime,time));
  in_runsection=0;
}

char *Comm::get_working_path(void)
{
  return(working_path);
}

char *Comm::get_file_path(char *name)
{
  format_message("%s/%s",working_path,name);
  return(formated_message);
}

