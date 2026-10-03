#include "headers.h"

DATE_STAMP;
char *builders_mbn="builders";
char *gobjects_mbn="game objects";
char *print_mbn="print";
char *menu_mbn="menu";
char *queue_mbn="queue";
char *list_mbn="list";
char *world_mbn="world";
char *gamemanager_mbn="game manager";
char *item_mbn="item";
char *ship_mbn="ship";
char *sound_mbn="sound";

static char patch_path_buffer[_MAX_PATH];
char *patch_path=NULL;

World *world=NULL;
GameManager *gamemanager=NULL;
int Game::capt=Game::IDLE;
int Game::gamma_changed=0;
int Game::sfu=0;
int Game::sfu_timer=0;
int Game::display_sfu=0;
int Game::display_fps=0;
int Game::display_org=0;
int Game::played_track=-1;
int Game::work_mode=0;
int Game::timer=0;
int Game::syncfail=0;
int Game::sync=0;
int Game::paused=0;
int Game::no_volumes=0;
int Game::volume_access_flags=0;
int Game::progress=0;
int Game::last_progress=0;
int Game::last_volume_progress=0;
int Game::flicker=0;
int Game::failure_detected=0;
int Game::demo_playtype=Game::_UNKNOWN;
int Game::demo_number=0;
char *Game::demo_filename=NULL;
Text Game::demo_text;
#ifdef SHAREWARE
Song Game::mission_song;
#endif
static HWND idd_progress,idc_progress,idc_progress_text;
static void progress_text(char *text);
static void progress_start(int range,HINSTANCE instance);
static void progress_stop(void);
static void progress_set(int position);
static void progress(int percent);
BOOL CALLBACK progress_proc(HWND,UINT msg,WPARAM w,LPARAM l);

// Port: was WinMain; the SDL entry point (main.cpp) calls it with the full command line.
int game_main(char *command_line)
{                             
  static char instance;  // Cwe::init requires a non-null instance handle
  try
  {
    Game::init_all((HINSTANCE)&instance,NULL,command_line,1);
    Game::go();
  }
  catch (TerminateGame)    {}
  catch (Closed)           {DBG_MESSAGE("WinMain() - Closed caught.");}
  catch (TerminateMission) {DBG_MESSAGE("WinMain() - TerminateMission caught.");}
  catch (Failure)          {DBG_MESSAGE("WinMain() - Failure caught.");Game::raise_failure_detection();}
  try
  {
    Comm::allow_assertbox=0;
    Game::quit_all();
  }
  catch (Failure)          {WARNING("WinMain() - Quit procedure failed.");Game::raise_failure_detection();}
  return 0;
}

void Game::keyboard_service(void)
{
  if (KbdStat::chat_request(-1))
  {
    if (NoNet::is_network_mode())
    {
      DBG_CHECK(NoNet::is_network_mode());
      int user=KbdStat::chat_request(-1);
      if (!KbdStat::using_menu(user)&&!KbdStat::using_netmsg(user))
      {
        Chat::talk(1,user);
      }
    }
    else
    {
      MSGTXT(GAMETXT("CHAT_UNAVAIL"),NoNet::get_this_id());
    }
  }
  if (KbdStat::netmsg_request())
  {
    if (!NoNet::is_network_mode())
    {
      MSGTXT(GAMETXT("NETMSG_UNAVAIL"),NoNet::get_this_id());
    }
  }
  else if (KbdStat::last_objective()&&!Menu::in_use()&&!paused)
  {
    if (!NoNet::is_network_mode())
    {
      DBG_CHECK(!NoNet::is_network_mode());
      MManager::add_objective(GAMETXT(GameManager::lastobj));
    }
    else
    {
      MSGTXT(GAMETXT("LASTOBJECTIVE_UNAVAIL"),NoNet::get_this_id());
    }
  }
  else if (KbdStat::restart()&&!Menu::in_use())
  {
    if (!NoNet::is_network_mode())
    {
      DBG_CHECK(!NoNet::is_network_mode());
      Menu::restartmission();
    }
    else
    {
      MSGTXT(GAMETXT("RESTART_UNAVAIL"),NoNet::get_this_id());
    }
  }
  else if (KbdStat::menu_secret_on()&&(SysSet::get(SysSet::CHEATING)||KbdStat::is_demo())&&!Menu::in_use())
  {
    if (!NoNet::is_network_mode())
    {
      DBG_CHECK(!NoNet::is_network_mode());
      Menu::cheat();
    }
  }
  else if (KbdStat::menu_on()&&!KbdStat::using_menu()&&!KbdStat::using_netmsg())
  {
    Menu::options(1);
  }
  else if (KbdStat::quit())
  {
    Menu::quit();
  }
  else if (KbdStat::abort()&&!Menu::in_use())
  {
    if (!NoNet::is_network_mode())
    {
      DBG_CHECK(!NoNet::is_network_mode());
      Menu::abortmission();
    }
    else
    {
      MSGTXT(GAMETXT("ABORT_UNAVAIL"),NoNet::get_this_id());
    }
  }
  else if (KbdStat::test_objective()&&!Menu::in_use()&&!NoNet::is_network_mode())
  {
    GameManager::read_all_text();
  }
  else if (KbdStat::help()&&!Menu::in_use())
  {
    Menu::help();
  }
  else if (KbdStat::controls()&&!Menu::in_use()&&!paused)
  {
    switch (SysSet::get(SysSet::INPUT_DEVICE))
    {
      case 0: SysSet::set(SysSet::INPUT_DEVICE,1);
              MSGTXT(GAMETXT("MOUSE_ON"),NoNet::get_this_id());
              break;
      case 1: SysSet::set(SysSet::INPUT_DEVICE,2);
              MSGTXT(GAMETXT("JOYSTICK_ON"),NoNet::get_this_id());
              break;
      case 2: SysSet::set(SysSet::INPUT_DEVICE,0);
              MSGTXT(GAMETXT("KEYBOARD_ON"),NoNet::get_this_id());
              break;
     default: DBG_CHECK(0);
              break;
    }
  }
  else if (KbdStat::detail_level()&&!Menu::in_use()&&!paused)
  {
    if (SysSet::get(SysSet::DETAIL_LEVEL)) MSGTXT(GAMETXT("DETAIL_LOW"),NoNet::get_this_id());
    else MSGTXT(GAMETXT("DETAIL_HIGH"),NoNet::get_this_id());
    SysSet::set(SysSet::DETAIL_LEVEL,!SysSet::get(SysSet::DETAIL_LEVEL));
  }
  else if (KbdStat::hires()&&Mp::HIRESAVAIL&&!paused)
  {
    if (SysSet::get(SysSet::RESOLUTION)) MSGTXT(GAMETXT("HIRES_OFF"),NoNet::get_this_id());
    else MSGTXT(GAMETXT("HIRES_ON"),NoNet::get_this_id());
    SysSet::set(SysSet::RESOLUTION,!SysSet::get(SysSet::RESOLUTION));
  }
  else if (KbdStat::gamma_minus()&&!Menu::in_use()&&!paused)
  {
    int curr=SysSet::get(SysSet::GAMMA);
    if (curr>Video::min_gamma)
    {
      SysSet::set(SysSet::GAMMA,curr-1);
      gamma_changed=Mp::METRONQUALITY/2;
    }
  }

  else if (KbdStat::gamma_plus()&&!Menu::in_use()&&!paused)
  {
    int curr=SysSet::get(SysSet::GAMMA);
    if (curr<Video::max_gamma)
    {
      SysSet::set(SysSet::GAMMA,curr+1);
      gamma_changed=Mp::METRONQUALITY/2;
    }
  }
  else if (KbdStat::auto_run()&&!Menu::in_use()&&!paused)
  {
    if (SysSet::get(SysSet::AUTORUN)) MSGTXT(GAMETXT("TURBO_UNLOCKED"),NoNet::get_this_id());
    else MSGTXT(GAMETXT("TURBO_LOCKED"),NoNet::get_this_id());
    SysSet::set(SysSet::AUTORUN,!SysSet::get(SysSet::AUTORUN));
  }
  else if (KbdStat::cheating()&&!NoNet::is_network_mode()&&!Menu::in_use()&&!paused)
  {
    SysSet::set(SysSet::CHEATING,!SysSet::get(SysSet::CHEATING));
    if (SysSet::get(SysSet::CHEATING))
    {
      Sound::st_play(&Mysound::cheat_on);
    }
    else
    {
      Sound::st_play(&Mysound::cheat_off);
    }
  }
  else if (KbdStat::display_fps()&&!Menu::in_use()&&!paused)
  {
    display_fps=!display_fps;
    if (Mp::DEBUG)
    {
      display_sfu=!display_sfu;
    }
  }
  else if (KbdStat::points()&&!Menu::in_use()&&!paused)
  {
    display_org=!display_org;
  }
  if (KbdStat::captscreen_noinfo())
  {
    capt=NODISPINFO;
  }
  else if (KbdStat::captscreen())
  {
    capt=FULL;
  }
  else if (KbdStat::no_wall()&&!NoNet::is_network_mode()&&!Menu::in_use()&&!paused)
  {
    DBG_CHECK(!NoNet::is_network_mode());
    if (NoNet::get_ship()->is_wall_through())
    {
      NoNet::get_ship()->disable_wall_through();
    }
    else
    {
      NoNet::get_ship()->enable_wall_through();
    }
  }
  else
  {
    FOR_ALL_USERS(ID)
    {
      if (KbdStat::make_failure(ID))
      {
        FAILURE2(RegData::DiagnosticFailure,"User %s(%d) deliberately made faiulre.",NoNet::get_player_name(ID),ID);
      }
    }
  }
}

void Game::loop_init(void)
{                                           
  Game::work_mode=OTHER;
  if (KbdStat::is_demo())
  {
    Rand::reinit();
  }
  Rand::unlock();
  if (Mp::MEMORYDEBUG)
  {
    Heap::check(FILE_LINE);
    MESSAGE("heap usage before loading level:");
    Heap::usage(NULL);
  }
  Comm::begin_load_section();
  Game::_clear_flags();
  Game::add_other_progress(0);
  Menu::flush();
  Sound::init();
  Posit::init();
  LowColis::init();
  Kill::init();
  Listmanager::init();
  GameManager::update_params(NoNet::is_network_mode());
  if (Mp::BUILDSPRITESMODE)
  {
    MESSAGE("BUILD_SPRITES Loading sublevel %d in %s level.",gamemanager->get_sublevel(),GameManager::volume);
    MESSAGE("BUILD_SPRITES Used volumes: %s,%s%d.",GameManager::volume,GameManager::volume,gamemanager->get_sublevel());
  }
  char tmp[1024];
  if (Mp::LOADSAMPLE)
    sprintf(tmp,(no_volumes)?"update,%s,%s%d,%s%ds,params":"update,%s,%s%d,%s%ds",GameManager::volume,GameManager::volume,gamemanager->get_sublevel(),GameManager::volume,gamemanager->get_sublevel());
  else
    sprintf(tmp,(no_volumes)?"update,%s,%s%d,params":"update,%s,%s%d",GameManager::volume,GameManager::volume,gamemanager->get_sublevel());

  Game::add_other_progress(10);
  File::access_on(tmp,patch_path,volume_access_flags,&Game::add_volumes_progress);
  Game::add_novolumes_progress(10);
  gamemanager->load_level();
  Spr::load_prologue();
  world=NEW(World(GameManager::volume,gamemanager->get_sublevel(),gamemanager->get_dx(),gamemanager->get_dy()),world_mbn);
  Game::add_other_progress(10);
  Mysprites::back=&world->get_level().sprite[Mysprites::sprnum("back")];
  Game::add_novolumes_progress(10);
  gamemanager->weather(&world->get_level());
  Spr::load_epilogue(0);
  Game::add_other_progress(10);
  View::init();
  Radar::init();
  NoNet::init_resources();
  world->set_current_displayed(NoNet::get_this_id());
  Game::add_novolumes_progress(10);
  NEW(Background,gobjects_mbn);
  NEW(Wall,gobjects_mbn);
  Game::add_other_progress(10);
  MManager::init(10);
  Fade::init();
  ShipFucker::init();
  Link::init();
  Game::add_novolumes_progress(10);
  world->register_builders();
  gamemanager->load_events();
  Game::add_novolumes_progress(10);
  KbdStat::init();
  File::access_off();   
  NetMsg::init();
  Brick::init();
  Splinters::init();
  Game::add_other_progress(10);
  Comm::end_load_section();
  GameManager::init_stuff();
  reset_sync();
  Fade::start(Fade::ONLYOUT);
  Game::add_end_progress();
  Mysprites::change_pattern();
  Rand::lock();
  SManager::unlock();
  if (Mp::MEMORYDEBUG)
  {
    Heap::check(FILE_LINE);
    MESSAGE("heap usage after loading level:");
    Heap::usage(NULL);
  }
  Display::clear_vga(gamemanager->get_sublevel());
  if (NoNet::is_network_mode())
  {
    draw_net_connect();
  }
  Comm::begin_run_section();
  if (Mp::BUILDSPRITESMODE)
  {
    MESSAGE("BUILD_SPRITES Level loaded successfully - all sprites built.");
    gamemanager->next_level();
    throw TerminateMission();
  }
  if (KbdStat::is_demo())
  {
    Rand::reinit();
  }
  play_song();
}

void Game::loop_quit(void)
{
  Game::work_mode=OTHER;
  Comm::end_run_section();
  if (Mp::MEMORYDEBUG)
  {
    Heap::check(FILE_LINE);
    MESSAGE("heap usage before quiting level:");
    Heap::usage(NULL);
  }
  Rand::unlock();
  Menu::flush();
  GameManager::clear_testing();
  Brick::quit();
  NetMsg::quit();
  KbdStat::done();
  stop_CD();
  Sounds::stop_all();
  Sound::done();
  if (world!=NULL)
  {
    world->cancel_builders();
  }
  Listmanager::quit();
  Link::quit();
  Fade::quit();
  ShipFucker::quit();
  Radar::done();
  NoNet::quit_resources();
  View::quit();
  if (world!=NULL)
  {
    delete world;
    world=NULL;
  }
  Kill::quit();
  LowColis::quit();
  Posit::quit();
  Update::quit();
  if (Mp::MEMORYDEBUG)
  {
    Heap::check(FILE_LINE);
    MESSAGE("heap usage after quiting level:");
    Heap::usage(NULL);
  }
  Rand::lock();
  SManager::unlock();
  Display::clear_vga(9);
}

void Game::main_loop(void)
{
  do
  {
    while (KbdStat::run((unsigned)last_sync()))
    {
      run_service();
    }
    display_service(Display::screen());
    Display::copy2vga();
  } while (1);
}

void Game::header_net(void)
{
  Statistics::Net();
}

int Game::header_single(void)
{
  return (Statistics::Single());
}

void Game::play(void)
{
  try
  {
    Game::loop_init();
    Game::main_loop();
  }
  catch (Eem_net_sync_failure)
  {
    MESSAGE("NETWORK: Eem network synchronization failure caught.");
    if (!Mp::SYNCFAILABORT)
    {
      FOR_ALL_USERS(ID)
      {
        Statistics::set(ID,0);
      }
      KbdStat::reinitframes();
      Rand::reinit();
      MESSAGE("NETWORK: System reinitialization. Restarting game.");
      Game::loop_quit();
    }
    else
    {
      DBG_CHECK(Mp::DEBUG);
      Myship::dump_debug();
      Rand::dump();
      KbdStat::dump();
      FAILURE("NETWORK: Terminated. Leaving now.");
    }
  }
  catch (Eem_demo_sync_failure)
  {
    MESSAGE("NETWORK: Eem demo synchronization failure caught.");
    Game::loop_quit();
  }
  catch (TerminateMission)
  {
    if (!Mp::BUILDSPRITESMODE)
    {
      gamemanager->end_level();
    }
    Game::loop_quit();
  }
  catch (TerminateGame)
  {
    Game::loop_quit();
    throw;
  }
  catch (Closed)
  {
    Game::loop_quit();
    throw;
  }
}

void Game::play_demo(void)
{
  Pilot::freeze();
  DemoInfo d;
  unsigned size=sizeof(d);
  int initstatus=1;

  File::access_on("demo",patch_path,Game::get_volume_access_flags());
  if (demo_playtype==PLAY_RANDOM)
  {
    int num=timeGetTime()%demo_text.size("demo_list");
    initstatus=KbdStat::demo_init(KbdStat::PLAY_DEMO,demo_text.string("demo_list",num),(char*)&d,&size);
  }
  else if (demo_playtype==PLAY_FILENAME)
  {
    initstatus=KbdStat::demo_init(KbdStat::PLAY_DEMO,demo_filename,(char*)&d,&size);
  }
  else
  {
    initstatus=KbdStat::demo_init(KbdStat::PLAY_DEMO,NULL,(char*)&d,&size);
  }
  File::access_off();

  if (initstatus)
  {
    if (d.version!=DEMOVERSION)
    {
      MESSAGE("Playing demo failed - different version");
    }
    else
    {
      for (int i=0;i<GameManager::level_descript.size("missions_order");i++)
      {
        if (!strcmpi(d.mission,GameManager::level_descript.string("missions_order",i)))
        {
          GameManager::set_testing();
          Pilot::set_skill(d.skill);
          Pilot::set_level(i);
          play();
        }
      }
      MESSAGE("Playing demo failed - level '%s' not found",d.mission);
    }
  }
  else
  {
    KbdStat::clear_demo_mode();
    MESSAGE("Playing demo failed");
  }
  Pilot::unfreeze();
}

void Game::record_demo(void)
{
  DemoInfo d;
  unsigned size=sizeof(d);
  Pilot::freeze();
  GameManager::set_testing();
  Pilot::set_level(demo_number);
  d.version=Game::DEMOVERSION;
  strncpy(d.mission,GameManager::level_descript.string("missions_order",demo_number),256);
  d.mission[255]='\0';
  d.skill=Pilot::get_skill();
  KbdStat::demo_init(KbdStat::SAVE_DEMO,NULL,(char*)&d,&size);
  play();
  Pilot::unfreeze();
}

void Game::set_demo_properties(int type,char *filename,int numb)
{
  demo_playtype=type;
  switch (type)
  {
     case PLAY_LASTSAVED: demo_filename=NULL;
                          demo_number=0;
                          break;
        case PLAY_RANDOM: demo_filename=NULL;
                          demo_number=0;
                          break;
       case PLAY_VOLNUMB: demo_filename=NULL;
                          demo_number=numb;
                          break;
      case PLAY_FILENAME: demo_filename=filename;
                          demo_number=numb;
                          break;
   case RECORD_LEVELNUMB: demo_filename=filename;
                          demo_number=numb;
                          break;
                 default: DBG_CHECK(0);
                          break;
  }
}

void Game::run_service(void)
{
  Game::work_mode=RUNNING;
  if (!paused&&(!Menu::in_use()||NoNet::is_network_mode()))
  {
    Rand::unlock();
    timer+=KbdStat::time();
    if (Mp::DEBUG)
    {
      sfu_timer-=KbdStat::time();
      if (sfu_timer<=0)
      {
        sfu_timer=5*Mp::METRONQUALITY;
        sfu=Diag::swap_usage();
      }
    }
    if (NoNet::is_network_mode()&&KbdStat::next_view())
    {
      int curr=world->get_current_displayed();
      for (int i=1;i<=NoNet::max_users+1;i++)
      {
        int selected=(curr+i)%(NoNet::max_users+1);
        if (NoNet::check_id(selected))
        {
          world->set_current_displayed(selected);
          break;
        }
      }
    }
    if (gamma_changed>0)
    {
      gamma_changed-=KbdStat::time();
      if (gamma_changed<=0)
      {
        gamma_changed=0;
        MSGTXT(GAMETXT("GAMMA_CHANGED"),NoNet::get_this_id());
      }
    }
    View::flush();
    Radar::clean();
    Kill::run();
    Radar::clean();
    LowColis::clean();
    FOR_ALL_USERS(ID)
    {
      NoNet::get_ship(ID)->go();
    }
    world->get_objects_from_level();
    Listmanager::run_all();
    Radar::compute();
    MManager::run();
    SManager::run();
    Fade::run();
    Debuginfo::run();
    NetMsg::run();
    GameManager::check_restart();
    Rand::lock();
    make_csum();
  }
  if (!NoNet::is_network_mode())
  {
    if (!Menu::in_use()&&!Fade::in_use())
    {
      if (paused)
      {
        if (KbdStat::pause())
        {
          unpause_CD();
          paused=0;
        }
      }
      else
      {
        if (KbdStat::pause())
        {
          paused=NoNet::get_this_id();
          flicker=0;
          pause_CD();
        }
      }
    }
  }
  else
  {
    if (paused)
    {
      if (!NoNet::is_alive(paused)||KbdStat::pause(paused))
      {
        paused=0;
        unpause_CD();
      }
    }
    else
    {
      paused=KbdStat::pause(-1);
      flicker=0;
      if (paused&&(KbdStat::using_menu(paused)||KbdStat::using_netmsg(paused)||Fade::in_use()))
      {
        paused=0;
      }
      if (paused)
      {
        pause_CD();
      }
    }
  }
  keyboard_service();
  Menu::serve();
  if (++flicker>20)
  {
    flicker=0;
  }
  Game::work_mode=OTHER;
}

void Game::display_service(Screen& screen)
{
  Game::work_mode=DISPLAYING;
  screen.debug_origins=display_org;
  int fade_in_use=Fade::in_use(0)||Fade::in_use(world->get_current_displayed());
  int display_nine=(!fade_in_use&&!Menu::in_use()&&capt!=NODISPINFO);
  int display_pause=(paused&&!Menu::in_use()&&capt!=NODISPINFO);
  int call_builders=(NoNet::is_network_mode())?(!paused):(!paused&&!Menu::in_use());
  world->display(screen,call_builders,display_nine);
  if (display_nine)
  {
    Debuginfo::draw(screen);
    draw_snail(screen);
    MManager::display(screen);
    NetMsg::draw(screen);
    if (display_fps)
    {
      draw_fps(screen);
    }
    if (display_sfu&&Mp::DEBUG)
    {
      draw_sfu(screen);
    }
  }
  Fade::draw(screen);
  if (paused)
  {
    if (display_pause&&flicker<=10)
    {
      draw_paused(screen);
    }
  }
  Menu::display(screen);
  if (capt!=IDLE)
  {
    screen.capture();
    MSGTXT("screenshot made",NoNet::get_this_id());
    capt=IDLE;
  }
  if (KbdStat::is_demo()==KbdStat::SAVE_DEMO)
  {
    draw_demorecord(screen);
  }
  else if (KbdStat::is_demo()==KbdStat::PLAY_DEMO)
  {
    draw_demoplay(screen);
  }
  Game::work_mode=OTHER;
}

void Game::draw_snail(Screen &screen)
{
  static int phs=0;
  if (Video::frame_rate<10)     screen.put(Mp::SX-1,Mp::SY,Mysprites::snail,phs); 
  else if (Video::frame_rate<12) screen.put(Mp::SX-1,Mp::SY,Mysprites::snail,phs,Spr::with_tspsoft,(void*)&tsp);
  else if (Video::frame_rate<14) screen.put(Mp::SX-1,Mp::SY,Mysprites::snail,phs,Spr::with_tsphard,(void*)&tsp);
  if (++phs>=Mysprites::snail.phases) phs=0;
}

                            
void Game::draw_fps(Screen &screen)
{
  int orgx=screen.ox;
  int orgy=screen.oy;
  screen.origin(0,0);
  char fps[1024];
  sprintf(fps,"%3d FPS",Video::frame_rate);
  int fpssx=41;
  int fpssy=Op::DISPINFOINVSY;
  int fpsx=Mp::SX-fpssx-Op::DISPINFOWEAPX+1;
  int fpsy=Op::DISPINFOINVY+Op::DISPINFOINVSY+2;
  int fpsinx=fpssx/2;
  int fpsiny=Op::DISPINFOINVINRECTY;
  Tool& tool=Tools::tool[Tools::DISPINFO];
  screen.rectangle(tool,fpsx,fpsy,fpssx,fpssy);
  Print::print(screen,Print::TFONT,Print::_FLAST,1+fpsx+fpsinx-Print::get_dx(Print::TFONT,fps)/2,fpsy+fpsiny,fps);
  Print::print(screen,Print::TFONT,Print::FINVOVR,fpsx+fpsinx-Print::get_dx(Print::TFONT,fps)/2,fpsy+fpsiny,fps);
  screen.origin(orgx,orgy);
}

void Game::draw_sfu(Screen &screen)
{
  int orgx=screen.ox;
  int orgy=screen.oy;
  screen.origin(0,0);
  char sfu[1024];
  sprintf(sfu,"%3d SFU",Game::sfu);
  int sfusx=41;
  int sfusy=Op::DISPINFOINVSY;
  int sfux=Mp::SX-sfusx-Op::DISPINFOWEAPX+1;
  int sfuy=Op::DISPINFOINVY+Op::DISPINFOINVSY+2+2+Op::DISPINFOINVSY;
  int sfuinx=sfusx/2;
  int sfuiny=Op::DISPINFOINVINRECTY;
  Tool& tool=Tools::tool[Tools::DISPINFO];
  screen.rectangle(tool,sfux,sfuy,sfusx,sfusy);
  Print::print(screen,Print::TFONT,Print::_FLAST,1+sfux+sfuinx-Print::get_dx(Print::TFONT,sfu)/2,sfuy+sfuiny,sfu);
  Print::print(screen,Print::TFONT,Print::FINVOVR,sfux+sfuinx-Print::get_dx(Print::TFONT,sfu)/2,sfuy+sfuiny,sfu);
  screen.origin(orgx,orgy);
}

void Game::draw_paused(Screen &screen)
{
  int orgx=screen.ox;
  int orgy=screen.oy;
  screen.origin(0,0);
  char msg[1024];
  sprintf(msg,(NoNet::is_network_mode())?"game paused by %s":"game paused",NoNet::get_player_name(paused));
  _strupr(msg);
  int posx=Mp::SX/2-Print::get_dx(Print::TFONT,msg)/2;
  int posy=Mp::SY/2-Print::get_dy(Print::TFONT,msg)/2;
  Print::print(screen,Print::TFONT,Print::_FLAST,posx+1,posy,msg);
  Print::print(screen,Print::TFONT,Print::FINVOVR,posx,posy,msg);
  screen.origin(orgx,orgy);
}

void Game::draw_demorecord(Screen &screen)
{
  int orgx=screen.ox;
  int orgy=screen.oy;
  screen.origin(0,0);
  int timesx=121;
  int timesy=Op::DISPINFOINVSY;
  int timex=Mp::SX-timesx-Op::DISPINFOWEAPX;
  int timey=Mp::SY-timesy-Op::DISPINFOWEAPY;
  int timeinx=61;
  int timeiny=Op::DISPINFOINVINRECTY;
  Tool& tool=Tools::tool[Tools::DISPINFO];
  screen.rectangle(tool,timex,timey,timesx,timesy);
  char time[256];
  int timesec=KbdStat::counter()/Mp::METRONQUALITY;
  int timerest=((KbdStat::counter()%Mp::METRONQUALITY)*100)/Mp::METRONQUALITY;
  if (timesec<0)
  {
    timesec=0;
  }
  sprintf(time,(flicker<=10)?"RECORDING DEMO %02d:%02d:%02d.%02d":"               %02d:%02d:%02d.%02d",timesec/3600,(timesec%3600)/60,timesec%60,timerest);
  Print::print(screen,Print::TFONT,Print::_FLAST,1+timex+timeinx-Print::get_dx(Print::TFONT,time)/2,timey+timeiny,time);
  Print::print(screen,Print::TFONT,Print::FINVOVR,timex+timeinx-Print::get_dx(Print::TFONT,time)/2,timey+timeiny,time);
  screen.origin(orgx,orgy);
}

void Game::draw_demoplay(Screen &screen)
{
  if (flicker<=10)
  {
    int orgx=screen.ox;
    int orgy=screen.oy;
    screen.origin(0,0);
    char *msg="DEMO MODE";
    int timesx=Print::get_dx(Print::TFONT,msg);
    int timesy=Print::get_dy(Print::TFONT,msg);
    int timex=(Mp::SX-timesx)/2;
    int timey=(Mp::SY-timesy)/2;
    Print::print(screen,Print::TFONT,Print::_FLAST,1+timex,timey,msg);
    Print::print(screen,Print::TFONT,Print::FINVOVR,timex,timey,msg);
    screen.origin(orgx,orgy);
  }
}

void Game::go(void)
{
  if (Mp::BUILDSPRITESMODE)
  {
    while (1)
    {
      play();
    }
  }
  else if (NoNet::is_network_mode())
  {
    while (1)
    {
      NoNet::refresh();
      header_net();
      NoNet::refresh();
      play();
    }
  }
  else
  {
    while (Header::header())
    {
      play_demo();
    }
    while (1)
    {
      switch (header_single())
      {
          case PLAY: play();
                     break;
     case DEMO_PLAY: play_demo();
                     break;
   case DEMO_RECORD: record_demo();
                     break;
      }
    }
  }
}

void Game::make_csum(void)
{
  sync=0;
  if (NoNet::is_network_mode()||KbdStat::is_demo())
  {
    FOR_ALL_USERS(ID)
    {
      Myship *s=NoNet::get_ship(ID);
      sync+=s->get_x();
      sync+=s->get_y();
      sync+=s->get_angle();
      sync+=s->get_life();
      if (NoNet::is_network_mode())
      {
        sync+=Statistics::get(ID);
      }
    }
    sync+=Rand::curr_indx();
    if (Mp::SYNCFAIL)
    {
      DBG_CHECK(Mp::DEBUG);
      Game::syncfail++;
      if (NoNet::get_this_id()==NoNet::get_server_id()&&Game::syncfail>=Mp::METRONQUALITY)
      {
        sync=!sync;
      }
    }
  }
}

void Game::_clear_flags(void)
{
  Game::capt=IDLE;
  Game::gamma_changed=0;
  Game::sfu=0;
  Game::sfu_timer=0;
  Game::display_sfu=0;
  Game::display_fps=0;
  Game::display_org=0;
  Game::played_track=-1;
  Game::timer=0;
  Game::paused=0;
  Game::progress=0;
  Game::last_progress=0;
  Game::last_volume_progress=0;
  Game::flicker=0;
  Game::failure_detected=0;
  Game::syncfail=0;
  Game::demo_playtype=Game::_UNKNOWN;
  Game::demo_number=0;
  Game::demo_filename=NULL;
}

void Game::init_all(HINSTANCE this_instance, HINSTANCE previous_instance,
                   LPSTR command_line,int window_state)
{
  srand(0);
  progress_start(100,this_instance);
  progress_text("Connecting players");
  Cwe_param cp;
  cp.hinstance=this_instance;
  cp.command_line=command_line;
  cp.icon_handle=LoadIcon(this_instance,MAKEINTRESOURCE(IDI_ICON1));
  cp.class_name="Firefight";
  cp.log_file="Firefght.log";
  Cwe::init(&cp);

  Registry::set_mode(Registry::mode_Machine);
  unsigned tmp_len=sizeof(patch_path_buffer);
  if(Registry::get_string(NULL,"patch",NULL,patch_path_buffer,&tmp_len))
  {
    patch_path=patch_path_buffer;
    char *patch_path_ptr=strrchr(patch_path_buffer,'\\');
    CHECK(patch_path_ptr!=NULL);
    *patch_path_ptr=0;
    MESSAGE("patch path '%s'",patch_path);
  }
  Registry::set_mode(Registry::mode_User);

#ifndef UNPROTECT
  if(GetDriveType(NULL)!=DRIVE_CDROM) FAILURE2(RegData::CopyProtection,"FF must be played from CD-ROM");
#endif
  progress_text("Loading game data");
  Comm::begin_load_section();
  RegData::Init();
  RegData::FromRegistry();
  volume_access_flags=ACCESS_LORES|((RegData::DataToLoad!=RegData::load_Lores)?ACCESS_HIRES:0);
  FastAlloc::init();
  NoNet::init();
  File::access_on("update,global,params",patch_path,volume_access_flags,::progress);
  Mp::init("mainparams");
  Op::init("objectparams");
  demo_text.load("demo");
  KbdStat::debug_init();
  Layout::init();
  no_volumes=!File::volume();
  SysSet::init("syssetdefaults");
  Pilot::init(Mp::BUILDSPRITESMODE);
  Rand::init();
  progress_text("Initializing video engine");
  Spr::load_prologue();
  progress_set(70);
  Statistics::init();
  Video::load_palette("palette");
  Menu::init("title_descript","title_sprite_in","title_sprite_out");
  Print::init("small_letters","small_italic_letters","big_letters","big_italic_letters","print_descript");
  progress_set(80);
  GameManager::start();
  progress_set(90);
  Mysprites::init();
  progress_set(100);
  progress_text("Initializing sound system");
  Mysound::init();
  Tools::init("tools");
  Spr::load_epilogue(0);
  File::access_off();
  init_CD();
  Comm::end_load_section();
  progress_stop();
  Myship::init_debug();
  Handle::init();
  SManager::init();
  gamemanager=NEW(GameManager(NoNet::is_network_mode()),gamemanager_mbn);
  if (Mp::MEMORYDEBUG)
  {
    Heap::check(FILE_LINE);
    MESSAGE("heap usage after init_all():");
    Heap::usage(NULL);
  }
  Video::get_screen()->cls();
  Video::release_screen();
  Video::show_screen();
  if (NoNet::is_network_mode())
  {
    Display::clear_vga(9);
    draw_net_connect();
  }
  NoNet::init_levels();
}

void Game::quit_all(void)
{
  NoNet::quit_levels();
  if (Mp::MEMORYDEBUG)
  {
    Heap::check(FILE_LINE);
    MESSAGE("heap usage before quit_all():");
    Heap::usage(NULL);
  }
  if (Mp::BUILDSPRITESMODE)
  {
    MESSAGE("BUILD_SPRITES Build mode finished.");
  }
  if (gamemanager!=NULL)
  {
    delete gamemanager;
    gamemanager=NULL;
  }
  demo_text.free();
  Statistics::quit();
  Pilot::quit();
  SysSet::quit();
  Tools::quit();
  Mysprites::quit();
  Mysound::quit();
  GameManager::stop();
  Handle::done();
  Menu::done();
  Print::quit();
  NoNet::quit();
  KbdStat::debug_done();
  Myship::quit_debug();
  Rand::quit();
  RegData::ToRegistry();
  RegData::Quit();
#if HI_DEBUG
  if (!Mp::BUILDSPRITESMODE)
  {
    NoQueue_statistics();
    List_statistics();
    View::statistics();
  }
#endif
  FastAlloc::quit();
}

void Game::draw_net_connect(void)
{
  char buf[1024];
  sprintf(buf,"WAITING FOR OTHERS...");
  Screen scr=Display::screen();
  scr.cls(0);
  int posx=10;
  int posy=190-Print::get_dy(Print::TSPRITE,buf);
  Print::print(scr,Print::TSPRITE,Print::FWARNING,posx,posy,buf);
  Display::copy2vga();
}
//PROGRESS
void Game::add_volumes_progress(int percent)
{
  DBG_CHECK(!no_volumes);
  int result=(percent-Game::last_volume_progress)*50/100;
  if (result>0)
  {
    Game::progress+=result;
    DBG_CHECK(Game::progress>=0&&Game::progress<=100);
    if (Game::progress>100)
    {
      Game::progress=100;
    }
    Game::last_volume_progress=percent;
    draw_progress();
  }
}

void Game::add_novolumes_progress(int percent)
{
  if (no_volumes)
  {
    Game::progress+=percent;
    DBG_CHECK(Game::progress>=0&&Game::progress<=100);
    draw_progress();
  }
}

void Game::add_other_progress(int percent)
{
  Game::progress+=percent;
  DBG_CHECK(Game::progress>=0&&Game::progress<=100);
  draw_progress();
}

void Game::add_end_progress(void)
{
  Game::progress=100;
  draw_progress();
}

void Game::draw_progress(void)
{
  if ((Game::last_progress/10)<(Game::progress/10)||Game::progress==0||Game::progress==100)
  {
    Game::last_progress=Game::progress;
    Screen scr=Display::screen();
    Mysprites::put_pattern(scr);
    int levels_numb=GameManager::level_descript.size("Missions_order");
    int curr_level=Pilot::get_level();
    char buf[1024];
    if (Mp::BUILDSPRITESMODE)
    {
      sprintf(buf,"*BUILD SPRITES MODE, PROCESSING %d. of %d LEVELS...",curr_level+1,levels_numb);
    }
    else if (KbdStat::is_demo()==KbdStat::PLAY_DEMO)
    {
      sprintf(buf,"LOADING DEMO...");
    }
    else if (KbdStat::is_demo()==KbdStat::SAVE_DEMO)
    {
      sprintf(buf,"INITIALIZING DEMO RECORDER...");
    }
    else
    {
      sprintf(buf,"LOADING...");
    }
    int totsx=51;
    int totsy=Print::get_dy(Print::TSPRITE,buf);
    int posx=10;
    int posy=190-totsy;
    int sx=4;
    int sy=totsy-2;
    int rect_num=(Game::progress/10);
    DBG_CHECK(rect_num>=0&&rect_num<=10);
    if (rect_num>10)
    {
      rect_num=10;
    }
    scr.rectangle(Print::get_tool(Print::_FLAST),posx,posy,totsx,totsy);
    for (int i=0;i<rect_num;i++)
    {
      scr.rectangle(Print::get_color(Print::FWARNING),posx+1+i*(sx+1),posy+1,sx,sy);
    }
    Print::print(scr,Print::TSPRITE,Print::FWARNING,posx+totsx+4,posy,buf);
    Display::copy2vga();
  }
}

//GAME PROGRESS (API)
void progress_text (char *text)
{
  SendMessage(idc_progress_text,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text);
}

BOOL CALLBACK progress_proc(HWND,UINT msg,WPARAM w,LPARAM l)
{
  if((msg==WM_COMMAND)&&(HIWORD(w)==BN_CLICKED))
  {
    DestroyWindow(idd_progress);
    idd_progress=idc_progress=idc_progress_text=NULL;
    PostMessage(Comm::hwnd,WM_CLOSE,0,0);
  }
  return(0);
}

void progress_start(int range,HINSTANCE hinstance)
{
  InitCommonControls();
  idd_progress=CreateDialog(hinstance,MAKEINTRESOURCE(IDD_PROGRESS),Comm::hwnd,(DLGPROC)progress_proc);
  idc_progress=GetDlgItem(idd_progress,IDC_PROGRESS);
  idc_progress_text=GetDlgItem(idd_progress,IDC_PROGRESS_TEXT);
  SendMessage(idc_progress,PBM_SETRANGE,0,MAKELPARAM(0, range));
  Comm::init_info=progress_text;
}

void progress_stop(void)
{
  if(idd_progress)
  {
    DestroyWindow(idd_progress);
    idd_progress=idc_progress=idc_progress_text=NULL;
  }
}

void progress_set(int position)
{
  PostMessage(idc_progress,PBM_SETPOS,position,0);
  Comm::process_messages();
}

void progress(int percent)
{
  progress_set(percent*60/100);
}

void Game::play_song(void)
{
#ifndef SHAREWARE
  if (NoNet::is_network_mode())
    play_CD(GameManager::level_descript.value("netsongs",Pilot::get_level()));
  else
    play_CD(GameManager::level_descript.value("songs",Pilot::get_level()));
#else
  if (RegData::UseCD)
  {
    if (NoNet::is_network_mode())
      Game::mission_song.play(Comm::get_file_path(GameManager::level_descript.string("sw_netsongs",Pilot::get_level())));
    else
      Game::mission_song.play(Comm::get_file_path(GameManager::level_descript.string("sw_songs",Pilot::get_level())));
  }
#endif
}
//CD AUDIO SUPPORT
void Game::play_CD(int track,int repeat)
{  
#ifndef SHAREWARE
  if (RegData::UseCD)
  {
    DBG_CHECK(CD::tracks>=0);
    if (track!=-1)
    {
      played_track=track;
    }
    else
    {
      switch (CD::tracks)
      {
         case 0: played_track=-1;
                 break;
         case 1: played_track=1;
                 break;
        default: played_track=(timeGetTime()%(CD::tracks-1))+1;
                 break;
      }
    }
    if (played_track>=1&&played_track<=CD::tracks&&CD::tracks>0)
    {
      CD::play(played_track,repeat);
      DBG_MESSAGE("CD audio started, track %d",played_track);
    }
    else
    {
      played_track=-1;
      DBG_MESSAGE("CD playing failed");
    }
  }
#endif
}

void Game::stop_CD(void)
{  
#ifndef SHAREWARE
  if (RegData::UseCD)
  {
    if (played_track!=-1)
    {
      CD::stop();
      played_track=-1;
    }
  }
#else
  if (RegData::UseCD)
    Game::mission_song.stop();
#endif
}

void Game::pause_CD(void)
{
#ifndef SHAREWARE
  if (RegData::UseCD)
  {
    if (played_track!=-1)
    {
      CD::stop();
    }
  }
#else
  if (RegData::UseCD)
    Game::mission_song.stop();
#endif
}

void Game::unpause_CD(void)
{
#ifndef SHAREWARE
  if (RegData::UseCD)
  {
    if (played_track!=-1)
    {
      CD::play(played_track);
    }
  }
#else
  if (RegData::UseCD)
  {
    if (NoNet::is_network_mode())
      Game::mission_song.play(Comm::get_file_path(GameManager::level_descript.string("sw_netsongs",Pilot::get_level())));
    else
      Game::mission_song.play(Comm::get_file_path(GameManager::level_descript.string("sw_songs",Pilot::get_level())));
  }
#endif
}

void Game::init_CD(void)
{
#ifndef SHAREWARE
  if (RegData::UseCD)
  {
    CD::init();
  }
#endif
}

void Game::quit_CD(void)
{
#ifndef SHAREWARE
  if (RegData::UseCD)
  {
    CD::stop();
  }
#endif
}


