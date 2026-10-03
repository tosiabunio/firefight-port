# define STRICT
# include <first.h>
# include <stdio.h>
# include <stdlib.h>
# include <stdarg.h>
# include <compat/crt.h>
# include <fcntl.h>
# include <string.h>
# include <errno.h>
# include <signal.h>
# include <compat/win32.h>

# include "1cw.h"
# include <SDL.h>

# include <1cw_strg.h>

static char mmu_area[]             = "mmu";
static char mmu_memory_required[]  = "memory_required";

static char eem_area[]             = "eem";
static char eem_queue_size[]       = "queue_size";
static char eem_timer_frequency[]  = "timer_frequency";
static char eem_debug[]            = "debug";
static char eem_net_supported[]    = "network_supported";
static char eem_check_sync[]       = "check_sync";
static char eem_compress_packets[] = "compress_packets";
static char eem_joy_dead_zone[]    = "joystick_dead_zone";

static char reg_area[]             = "registry";
static char reg_app_name[]         = "application_name";
static char reg_app_sub_name[]     = "application_subname";
                               
static char timer_area[]           = "timer";
static char timer_active[]         = "active";
static char timer_desired[]        = "desired_accuracy";
static char timer_critical[]       = "critical_accuracy";

static char log_area[]             = "log";
static char log_path[]             = "name";
                               
static char xio_area[]             = "xio";
static char xio_read_directory[]   = "active";

static char txt_area[]             = "txt";
static char txt_find_labels[]      = "find_unused_labels";
                                
static char sos_area[]             = "sos";
static char sos_mode[]             = "mode";
static char sos_mode_inactive[]    = "inactive";
static char sos_mode_safe[]        = "safe";
                                
static char spr_area[]             = "spr";
static char spr_active[]           = "active";
static char spr_hires_onecolor[]   = "hires_onecolor";
static char spr_lores_onecolor[]   = "lores_onecolor";
static char spr_collis_onecolor[]  = "collis_onecolor";
static char spr_load_hires[]       = "load_hires";
static char spr_load_lores[]       = "load_lores";
static char spr_load_collis[]      = "load_collis";
static char spr_mirror[]           = "mirror";
static char spr_lores_scale[]      = "lores_scale";
static char spr_hires_scale[]      = "hires_scale";
static char spr_collis_scale[]     = "collis_scale";
static char spr_scale_1[]          = "1x1";
static char spr_scale_2[]          = "2x1";
static char spr_scale_4[]          = "4x1";
static char spr_scale_8[]          = "8x1";
static char spr_use_320x200[]      = "use_320x200";
static char spr_use_640x400[]      = "use_640x400";

static char cwe_window_name[]      = "CWE game";
static char cwe_class_name[]       = "CWE game";
static char cwe_log_file[]         = "lastexec.log";
static char cwe_ini_file[]         = "cwe.ini";
static char cwe_debug[]            = "debug";
                                   
const  char cl_default_yes[]       = "yes";
const  char cl_default_no[]        = "no";

Text    Cwe::text;
BOOL    Cwe::application_ready=0;

static void sdl_quit (void)
{
  SDL_Quit();
}


//=============================================================================

static void *alt_alloc (unsigned int size, char const *name)
{
  void *result=malloc(size);
  if (result==NULL)
    FAILURE2(Cwe_error::not_enough_memory, "unable to allocate %d bytes for %s [alt_alloc]",size,name);
  return (result);
}
//-----------------------------------------------------------------------------
static void alt_free (void *ptr, char const *name)
{
  (void)name;
  free(ptr);
}
//=============================================================================

void Cwe::init (Cwe_param *param)
{
  try
  {
    //-------------------
    // LOG initialization
    //-------------------
    // Port: the log goes to the preferences directory (was TEMP).
    char tmp[1024];
    sprintf(tmp, "%s%s", Comm::pref_path, param->log_file?param->log_file:cwe_log_file);
    Log::init(tmp);
    //------------------------
    // Command line processing
    //------------------------
    Cmd_line::init(param->command_line);
    MESSAGE("command line: '%s'",Cmd_line::get());
    //----------------------
    // SDL initialization (port)
    //----------------------
    Comm::headless = Cmd_line::get_int("headless",0);
    int quit_after = Cmd_line::get_int("quit_after",0);
    if (SDL_Init(SDL_INIT_EVENTS|SDL_INIT_TIMER)!=0)
      FAILURE2(Cwe_error::general, "unable to initialize SDL: %s", (char*)SDL_GetError());
    Comm::quit_me(sdl_quit, "sdl", "log cmd_line");
    if (quit_after>0)
    {
      Comm::quit_time = SDL_GetTicks()+quit_after;
      MESSAGE("quitting after %d ms", quit_after);
    }
    //----------------------------
    // Begin engine initialization
    //----------------------------
    text.mem_alloc=alt_alloc;
    text.mem_free=alt_free;
    char *ini_file=param->ini_file?param->ini_file:cwe_ini_file;
    FILE *ini=fopen(ini_file,"rb");
    if(ini==NULL)
      FAILURE2(Cwe_error::ini_corrupted, "unable to load ini file");
    fclose(ini);
    try
    {
      text.dir_load(ini_file);
    }
    catch (Failure)
    {
      FAILURE2(Cwe_error::ini_corrupted, "ini file corrupted");
    }
    text.find_unused_labels=0;
    //----------------------
    // Window initialization
    //----------------------
    Comm::debug = Cmd_line::get_int(cwe_debug,0);
    if (!param->hinstance)
      FAILURE("Cwe::init - invalid Cwe::Param::hinstance parameter");
		HWND main_hwnd = param->hwnd;

    Spr::work_modes spr_work_mode;
    if(Comm::debug)
      spr_work_mode=Spr::work_debug;
    else
    {
      if(Cmd_line::get_int("spr_safe",0) == 1)
        spr_work_mode=Spr::work_safe;
      else
        spr_work_mode=Spr::work_normal;
    }
    
	  if (param->hwnd==NULL)
		{ 																										
		  WNDCLASS wc;																				
		  wc.hInstance       = param->hinstance;
		  wc.lpszClassName   = (param->class_name?param->class_name:cwe_class_name);
		  wc.lpfnWndProc     = Comm::window_proc;
		  wc.style           = 0;
      if (param->icon_handle)
  		  wc.hIcon           = param->icon_handle;
      else
  		  wc.hIcon           = LoadIcon   (NULL,IDI_WINLOGO);
		  wc.hCursor         = LoadCursor (NULL,IDC_ARROW);
		  wc.lpszMenuName    = NULL;
		  wc.cbClsExtra      = 0;
		  wc.cbWndExtra      = 0;
		  wc.hbrBackground   = NULL;
		  if (!RegisterClass (&wc))
        FAILURE2(Cwe_error::general, "unable to window class");
      text.area(reg_area);
      main_hwnd=Spr::create_window (
        spr_work_mode,
        param->hinstance,
        (param->class_name?param->class_name:cwe_class_name), 
        (param->window_name?param->window_name:text.string(reg_app_name)));
      text.endarea();
    }
    else if (!Comm::debug)
      FAILURE("Cwe::init - full screen game, user window not allowed");
    Comm::hinstance = param->hinstance;
    Comm::hwnd      = main_hwnd;
    //----------------------
    // Mmu initialization
    //----------------------
    text.area(mmu_area);
    Heap::init(text.value(mmu_memory_required));
    text.endarea();
    //----------------------
    // Txt initialization
    //----------------------
    text.area(txt_area);
    if (text.value(txt_find_labels)==0) Text::find_unused_labels_disable=1;
    text.endarea();
    //----------------------
    // Registry initialization
    //----------------------
    text.area(reg_area);
    char *sub_app = NULL;
    if (text.exist(reg_app_sub_name))
      sub_app = text.string(reg_app_sub_name);
    Registry::init(text.string(reg_app_name), sub_app);
    text.endarea();
    //----------------------
    // Xio initialization
    //----------------------
    text.area(xio_area);
    if (text.value(xio_read_directory))
      Xio::init();
    text.endarea();
    //----------------------
    // Spr initialization
    //----------------------

    text.area(spr_area);
    if (text.value(spr_active))
    {
      int honecolor=(text.value(spr_hires_onecolor)) ?Spr::hires :0;
      int lonecolor=(text.value(spr_lores_onecolor)) ?Spr::lores :0;
      int conecolor=(text.value(spr_collis_onecolor)) ?Spr::collis :0;
      Spr::onecolor=honecolor|lonecolor|conecolor;

      int htoload;
      int ltoload;
      int ctoload;
      unsigned load_mode = Registry::get_int(sec_spr, key_spr_load_mode, 0xFFFFFFFF);
      if (load_mode!=0xFFFFFFFF)
      {
        switch (load_mode)
        {
          case 1 : Spr::toload = Spr::hires|Spr::collis; break;
          case 2 : Spr::toload = Spr::lores|Spr::collis; break;
          case 3 : Spr::toload = Spr::hires|Spr::lores|Spr::collis; break;
        }
      }
      else
      {
        htoload=(text.value(spr_load_hires)) ?Spr::hires :0;
        ltoload=(text.value(spr_load_lores)) ?Spr::lores :0;
        ctoload=(text.value(spr_load_collis)) ?Spr::collis :0;
        Spr::toload=htoload|ltoload|ctoload;
      }
      Spr::mirror=text.value(spr_mirror);

      char *tmp;
      int scale;

      tmp = text.string(spr_lores_scale);
      if (strcmpi(tmp,spr_scale_1)==0)
        scale=1; 
      else if (strcmpi(tmp,spr_scale_2)==0)
        scale=2; 
      else if (strcmpi(tmp,spr_scale_4)==0)
        scale=4; 
      else if (strcmpi(tmp,spr_scale_8)==0)
        scale=8; 
      else
        FAILURE2(Cwe_error::ini_corrupted, "Cwe::init - incorrect %s parameter (%s)",spr_lores_scale,tmp); 
      Spr::lores_scale = scale;

      tmp = text.string(spr_hires_scale);
      if (strcmpi(tmp,spr_scale_1)==0)
        scale=1; 
      else if (strcmpi(tmp,spr_scale_2)==0)
        scale=2; 
      else if (strcmpi(tmp,spr_scale_4)==0)
        scale=4; 
      else if (strcmpi(tmp,spr_scale_8)==0)
        scale=8; 
      else
        FAILURE2(Cwe_error::ini_corrupted, "Cwe::init - incorrect %s parameter (%s)",spr_hires_scale,tmp); 
      Spr::hires_scale = scale;

      tmp = text.string(spr_collis_scale);
      if (strcmpi(tmp,spr_scale_1)==0)
        scale=1; 
      else if (strcmpi(tmp,spr_scale_2)==0)
        scale=2; 
      else if (strcmpi(tmp,spr_scale_4)==0)
        scale=4; 
      else if (strcmpi(tmp,spr_scale_8)==0)
        scale=8; 
      else
        FAILURE2(Cwe_error::ini_corrupted, "Cwe::init - incorrect %s parameter (%s)",spr_collis_scale,tmp); 
      Spr::collis_scale = scale;

      int use_640x400 = 0;
      unsigned ui = Registry::get_int(sec_spr, key_spr_hires_mode, 0);
      if (ui==2)
        use_640x400 = 1;
      else if (ui==0)
        use_640x400 = text.value(spr_use_640x400);

      int use_320x200 = 0;
      ui = Registry::get_int(sec_spr, key_spr_lores_mode, 0); 
      if (ui==2)
        use_320x200 = 1;
      else if (ui==0)
        use_320x200 = text.value(spr_use_320x200);

      Video::headless=Comm::headless;
      Spr::init(spr_work_mode, use_320x200, use_640x400);
    }
    text.endarea();
    //----------------------
    // Timer initialization
    //----------------------
    text.area(timer_area);
    if (text.value(timer_active))
      Timer::init(text.value(timer_desired), text.value(timer_critical));
    text.endarea();
    //----------------------
    // Sos initialization
    //----------------------
    if (Cmd_line::get_int("sos_none",0)==1)
      Sounds::init(0);
    else if (Cmd_line::get_int("sos_safe",0)==1)
      Sounds::init(1);
    else if (Registry::get_int(sec_sos, key_sos_disable_sound, 0)==1)
      Sounds::init(0);
    else if (Registry::get_int(sec_sos, key_sos_disable_dsound, 0)==1)
      Sounds::init(1);
    else 
    {
      unsigned mix_mode = Registry::get_int(sec_sos, key_sos_mixing_mode, 0xFFFFFFFF);
      unsigned vol_mode = Registry::get_int(sec_sos, key_sos_volume, 0xFFFFFFFF);
      unsigned channels = Registry::get_int(sec_sos, key_sos_channels, 8);
      if (mix_mode == 0xFFFFFFFF)
      {
        text.area(sos_area);
        if (strcmpi(text.string(sos_mode), sos_mode_inactive)==0)
          Sounds::init(0);
        if (strcmpi(text.string(sos_mode), sos_mode_safe)==0)
          Sounds::init(1);
        else
          Sounds::init(text.value(sos_mode), channels);
        text.endarea();
      }
      else
        Sounds::init(mix_mode, channels, vol_mode);
    }
    //----------------------
    // Eem initialization
    //----------------------
    text.area(eem_area);
    unsigned dz = Registry::get_int(sec_eem, key_eem_joy_dead, 0xFFFFFFFF);
    if (dz == 0xFFFFFFFF)
      dz = text.value(eem_joy_dead_zone);
    Eem::init(text.value(eem_queue_size),
              text.value(eem_timer_frequency), 
              text.value(eem_net_supported),
              text.value(eem_check_sync),
              text.value(eem_compress_packets),
              Comm::debug,
              dz);
    text.endarea();
    //----------------------------
    // End engine initialization
    //----------------------------
    text.free();
    Comm::quit_me(quit, "cwe", "log mmu registry eem xio spr cmd_line");  
  }
  catch (Failure)
  {
    FAILURE("engine initialization failed");
  }
  application_ready=1;
}
//-----------------------------------------------------------------------------
void Cwe::quit (void)
{
  application_ready=0;
}
