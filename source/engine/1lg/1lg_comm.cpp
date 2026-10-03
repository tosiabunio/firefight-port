#include "1lg_hdrs.h"
#include <atomic>
#include <new>
#pragma init_seg(lib)
#pragma comment(lib,"comctl32.lib")

static char assert_message[]="assertion (%s) failed in %s";

HWND        Comm::hwnd=NULL;
HINSTANCE   Comm::hinstance=NULL;
int         Comm::debug=0;;
int         Comm::allow_assertbox=1;
int         Comm::exit_code=0;
char       *Comm::spawn_name=NULL;
char       *default_argv[]={NULL};
char      **Comm::spawn_argv=default_argv;
int         Comm::standby=0;
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

void Comm::process_messages (void)
{
  MSG msg;
  if((!standby)||closed)
  {
    if((last_standby!=standby)&&(!production))
    {
      last_standby=standby;
      MESSAGE("active mode entered");
    }
    while (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  else
  {
    if((last_standby!=standby)&&(!production))
    {
      last_standby=standby;
      MESSAGE("stanby mode entered");
    }
    GetMessage(&msg,NULL,0,0);
    TranslateMessage(&msg);
    DispatchMessage(&msg);
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

      HWND _hwnd=GetWindowLong(Comm::hwnd,GWL_STYLE) ?Comm::hwnd :NULL;
      switch(MessageBox(_hwnd,boxmessage,"chaos works engine",MB_ICONQUESTION|MB_YESNOCANCEL))
      {
        case IDCANCEL:
          result=a_debug;
          break;
        case IDNO:
          result=a_cont;
          break;
        case IDOK:
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

static int check_dll (char *dll_name, char *function_name)
{
  int result=0;

  HINSTANCE dll_hinstance = LoadLibrary(dll_name);
  if(dll_hinstance!=NULL)
    if(GetProcAddress(dll_hinstance,function_name)!=NULL)
      result=1;

  if(dll_hinstance)
    FreeLibrary(dll_hinstance);

  return(result);
}

int Comm::check_directx (int v)
{
  int result=0;
  if((v&dplay_ok) &&(check_dll("dplay.dll", "DirectPlayCreate")))  result|=dplay_ok;
  if((v&ddraw_ok) &&(check_dll("ddraw.dll", "DirectDrawCreate")))  result|=ddraw_ok;
  if((v&dsound_ok)&&(check_dll("dsound.dll","DirectSoundCreate"))) result|=dsound_ok;
  return((result&v)==v);
}

char *Comm::get_working_path(void)
{
  return(working_path);
}

char *Comm::get_file_path(char *name)
{
  format_message("%s\\%s",working_path,name);
  return(formated_message);
}

