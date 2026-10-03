#include "headers.h"

//KLASA WORLD
World::World(char* _name,int _sub_level,int _dx,int _dy)
{
  name=_name;
  sub_level=_sub_level;
  dx=_dx;
  dy=_dy;
  sx=Mp::SX;
  sy=Mp::SY;
  char num[1024];
  sprintf(num,"%d",sub_level);
  level.load(name,num);
  active_plane=level.active[sub_level];
  cls_color=Video::closest(level.backcolor);
  LevImp::init(this);
  konvert();
  displayed_now=-1;
  Update::freez(this);
}

World::~World(void)
{
  LevImp::quit();
}

void World::register_builders(void)
{
  LevImp::regbuild_global();
  LevImp::regbuild_local();
}

void World::cancel_builders(void)
{
  LevImp::regbuild_cancel();
}

void World::update_my_xy(int id,int x,int y,int offset_x,int offset_y)
{
  DBG_CHECK(NoNet::check_id(id));
  posit_table[id].x=x-(sx>>1)+offset_x;
  posit_table[id].y=y-(sy>>1)+offset_y;
}

int World::get_current_displayed(void)
{
  DBG_CHECK(displayed_now!=-1);
  DBG_CHECK(NoNet::check_id(displayed_now));
  return displayed_now;
}

void World::set_current_displayed(int id)
{
  if (id==-1)
  {
    id=NoNet::get_this_id();
  }
#if HI_DEBUG
  check_id(id);
#endif
  displayed_now=id;
}

void World::display(Screen& screen,int call_builders,int nine_plane)
{
  char planes_str[1024];
  int beg_plane=1;
  if (!SysSet::get(SysSet::DETAIL_LEVEL)) beg_plane=3;
  View::run(screen,0);
  for (int plane=beg_plane;plane<9;plane++)
  {
    sprintf(planes_str,"%d%d",gamemanager->get_sublevel(),plane);
    if (call_builders)
    {

      level.look_at(get_x(),get_y(),sx,sy,planes_str);
    }
    else
    {
      level.look_atnb(get_x(),get_y(),sx,sy,planes_str);
    }
    int a=level.buf[1].orgx;
    int b=level.buf[1].orgy;
    screen.origin(a,b);
    for (unsigned short* i=level.buf[1].object; *i; i++)
    {
      Lobject *o=&level[*i];
      if (Snow::initialized&&plane==2)
      {
        Snow::draw_2plane(screen,o);
        continue;
      }
      switch (LevImp::get_parameters(o->type)->disp_type)
      {
#if !HI_DEBUG
        default:
#endif
        case LevImp::PINVISIBLE: break;
        case LevImp::PNORMAL:    screen.put(o->x,o->y,level.sprite[o->sprite], o->phase,o->mirror);
                                 break;
        case LevImp::P30TSP:     screen.put(o->x,o->y,level.sprite[o->sprite], o->phase,o->mirror|Spr::with_tspsoft, (void*)&tsp);
                                 break;
        case LevImp::P70TSP:     screen.put(o->x,o->y,level.sprite[o->sprite], o->phase,o->mirror|Spr::with_tsphard, (void*)&tsp);
                                 break;
        case LevImp::PSHADOW:    screen.shape(o->x,o->y,level.sprite[o->sprite], o->phase,o->mirror|Spr::with_tool, (void*)&Tools::tool[Tools::SHADOW]);
                                 break;
  #if HI_DEBUG
        default:                 FAILURE("World::put(Screen& screen,int plane) - object number %d from level has illegal display type.",*i);
  #endif
      }
    }
    View::run(screen,plane);
  }
  if (nine_plane)
  {
    View::run(screen,plane);
  }
}

void World::check_id(int id)
{
  DBG_CHECK(NoNet::check_id(id));
  (void)id;
}

void World::get_objects_from_level(void)
{
  char planes_str[1024];
  int create_range=LevImp::max_crange();
  FOR_ALL_USERS(ID)
  {
    check_id(ID);
    int beg_x=get_x(ID)-create_range;
    int beg_y=get_y(ID)-create_range;
    int sx_to_get=sx+create_range*2;
    int sy_to_get=sy+create_range*2;
    sprintf(planes_str,"%d9",gamemanager->get_sublevel());
    level.look_atno(beg_x,beg_y,sx_to_get,sy_to_get,planes_str);
  }
}

int World::count_plane(int type)
{
  for (int i=1;i<9;i++)
  {
    if (type<level.plane[gamemanager->get_sublevel()][i].begin+level.plane[gamemanager->get_sublevel()][i].size) return i;
  }
  return 9;
}

void World::konvert(void)
{
  Text *text=LevImp::types_text;
  if (level.plane[GameManager::sublevel_number+1][9].reserved<500)
    DBG_FAILURE("World::konvert(void) - less than 500 free reserved objects in level plane 9.");

  int statistic=0;
  int num9=1;
  int c=0;
  char name[100];
  for (int i=0;i<level.types_num;i++)
  {
    strcpy(name,level.type[i]);
    if (LevImp::check_area(level.type[i],"type",*text))
    {
      text->area(level.type[i]);
      strcpy(name,(*text).string("type"));
      if (strstr((*text).string("type"),"obj_")!=(*text).string("type"))
      {
        (*text).endarea();
        continue;
      }
      (*text).endarea();
    }
    else
    {
      if (strstr(level.type[i],"obj_")!=level.type[i]) continue;
    }

    int seek=i;
    for(int i=1;i<level.plane[gamemanager->get_sublevel()][8].begin+
                   level.plane[gamemanager->get_sublevel()][8].size;i++)
    {
      if(level[i].type==seek)
      {
        int count=level.plane[gamemanager->get_sublevel()][9].begin+num9;
        level[count]=level[i];
        //tu obliczam plan
        level[count].aux1=count_plane(i);
        level[count].active=level[i].active;
        level[i].active=0;
        level[i].status=(short)CONVERTED;
        //level[count].special=(level.oplane(i)>=get_active_plane());
        if (count==1881) DBG_MESSAGE("to ten ponoc obiekt %s faza %d",level.type[level[count].type],level[count].phase);
        int act=level[i].link;
        while (act)
        {
          if (level[act].link==i)
          {
            level[act].link=count;
            break;
          }
          act=level[act].link;
        }
        level[i].link=0;
        level.update(count);
        level.update(i);
        num9++;
#if HI_DEBUG
        if (count>=level.plane[gamemanager->get_sublevel()][9].begin
                 +level.plane[gamemanager->get_sublevel()][9].size)
        {
          FAILURE("World::konvert(void) - too many objects to convert.");
        }
        statistic++;
#endif
      }
    }
  }
  DBG_MESSAGE("%d objects have been converted",statistic);
}

Lobject *World::new_lobject(int plane,int *num)
{
  DBG_CHECK(level.plane[gamemanager->get_sublevel()][plane].reserved>0);
  (*num)=level.plane[gamemanager->get_sublevel()][plane].begin;
  level.plane[gamemanager->get_sublevel()][plane].begin++;
  return &level[(*num)];
}

//UPDATE

Lobject*   Update::lobject;
Parameters* Update::params;

void Update::freez(World* world)
{
  lobject=(Lobject*)Heap::alloc(world->get_level().size*sizeof(Lobject),"Update::lobjects");
  params=(Parameters*)Heap::alloc(LevImp::parameters_indx*sizeof(Parameters),"Update::params");
  memcpy(lobject,&world->get_level()[1],world->get_level().size*sizeof(Lobject));
  memcpy(params,LevImp::parameters,LevImp::parameters_indx*sizeof(Parameters));
}

void Update::realase(World* world,GameManager* gm)
{
  SManager::flush();
  MManager::flush();
  Game::stop_CD();
  Sounds::stop_all();
  if (KbdStat::is_demo())
  {
    throw TerminateMission();
  }
  Display::clear_vga(gamemanager->get_sublevel());
  Rand::unlock();
  KbdStat::done();
  ShipFucker::quit();
  Kill::run();
  Menu::flush();
  Listmanager::quit();
  Brick::quit();
  NoNet::quit_resources();
  gm->clean_up();
  memcpy(&world->get_level()[1],lobject,world->get_level().size*sizeof(Lobject));
  memcpy(LevImp::parameters,params,LevImp::parameters_indx*sizeof(Parameters));
  world->get_level().update(0);
  Counter::run(&world->get_level());
  DBG_CHECK(gm->restart==GameManager::KILL||gm->restart==GameManager::RESTART);
  Game::_clear_flags();
  Listmanager::init();
  NoNet::init_resources();
  world->register_builders();
  gm->load_events();
  gamemanager->weather(&world->get_level());
  NEW(Background,gobjects_mbn);
  NEW(Wall,gobjects_mbn);
  MManager::init(10);
  Brick::init();
  GameManager::init_stuff();
  ShipFucker::init();
  Fade::init();
  Fade::start(Fade::ONLYOUT);
  Game::play_song();
  KbdStat::init();
  SManager::unlock();
  Rand::lock();
  Display::clear_vga(gamemanager->get_sublevel());
}

void Update::quit(void)
{
  Heap::free(lobject);
  Heap::free(params);
}

//STATISTICS
int  Statistics::sum[NoNet::max_users+1];
int  Statistics::last[NoNet::max_users+1];
int  Statistics::posit[NoNet::max_users+1];
int  Statistics::selected[NoNet::max_users+1];
int  Statistics::initialized(0);
int  Statistics::level(0);
int  Statistics::X;
int  Statistics::Y;
int  Statistics::snd;
int  Statistics::t_kill;
int  Statistics::t_secret;
int  Statistics::t_time;
int  Statistics::kill;
int  Statistics::secret;
int  Statistics::time;
int  Statistics::MAX_SINGLELEVEL;
int  Statistics::active_time;
int  Statistics::active_phs;
Tool Statistics::tools[7];
Screen*  Statistics::backscreen=NULL;
int  Statistics::rent=0;
int  Statistics::first_time=1;
void*  Statistics::ptr;
int  Statistics::say_who=1;

void Statistics::reinit(void)
{
  if (backscreen)
  {
    backscreen->reinit();
    rent=1;
  }
}


void Statistics::init(void)
{
  DBG_CHECK(!initialized);
  initialized=1;
  memset(sum,0,sizeof(sum));
  memset(last,0,sizeof(last));
  memset(selected,0,sizeof(selected));
  ptr=NULL;
  backscreen=NULL;
}

void Statistics::quit(void)
{
  if (!initialized) return;
  initialized=0;
  if (backscreen)
  {
    delete backscreen;
    backscreen=NULL;
  }

  if (ptr)
  {
    Heap::free(ptr);
    ptr=NULL;
  }
}


void Statistics::reset_last(void)
{
  memset(last,0,sizeof(last));
}

void Statistics::add(int player)
{
  DBG_CHECK(NoNet::check_id(player));
  last[player]++;
}

void Statistics::recount(void)
{
  for (int p=1;p<NoNet::max_users+1;p++) sum[p]+=last[p];
}


int  Statistics::get_land(int player)
{
  DBG_CHECK(NoNet::check_id(player));
  DBG_CHECK(NoNet::is_network_mode(NoNet::NETWORKFLAG));
  if (last[player]<Op::MAXSHIPBRICK) return 0;
  int max=-1;
  int max_id=-1;
  int sec=-1;
  FOR_ALL_USERS(ID)
  {
    if (last[ID]>max)
    {
      max=last[ID];
      max_id=ID;
    }
  }
  if (last[player]!=max) return 0;

  FOR_ALL_USERS(ID1)
  {
    if ((max_id!=ID1)&&(last[ID1]==max)) return 0;
  }

  return 1;
}

int Statistics::get(int player)
{
  DBG_CHECK(NoNet::check_id(player));
  return last[player];
}

void Statistics::update(int player,int count)
{
  DBG_CHECK(NoNet::check_id(player));
  last[player]+=count;
  if(NoNet::is_network_mode(NoNet::NETWORKFLAG))
    DBG_CHECK(last[player]>=0);
}


void Statistics::sub(int player)
{
  DBG_CHECK(NoNet::check_id(player));
  DBG_CHECK(NoNet::is_network_mode(NoNet::NETWORKMATCH));
  last[player]--;
  if (last[player]<0) last[player]=0;
}

void Statistics::set(int player,int count)
{
  DBG_CHECK(NoNet::check_id(player));
  last[player]=count;
}

int Statistics::compare(int id1,int id2)
{
  DBG_CHECK(NoNet::check_id(id1));
  DBG_CHECK(NoNet::check_id(id2));
  int comp_last=(last[id1]>last[id2])?1:((last[id1]==last[id2])?0:-1);
  int comp_sum=(sum[id1]>sum[id2])?1:((sum[id1]==sum[id2])?0:-1);
  int comp_name=strcmpi(NoNet::get_player_name(id1),NoNet::get_player_name(id2));
  if (comp_sum>0)
  {
    return 1;
  }
  if (comp_sum==0)
  {
    if (comp_last>0)
    {
      return 1;
    }
    if (comp_last==0)
    {
      if (comp_name>0)
      {
        return 1;
      }
      if (comp_name==0)
      {
        return 0;
      }
      return -1;
    }
    return -1;
  }
  return -1;
}

void Statistics::put_netback(void)
{
  backscreen->reinit();
  Mysprites::put_pattern(*backscreen,1);

  //panel z tytulem
  backscreen->rectangle(tools[0],Layout::netpanel[0].x,Layout::netpanel[0].y,Layout::netpanel[0].w,Layout::netpanel[0].h);
  backscreen->rectangle(Color::black,Layout::netpanel[0].x+Layout::SINGLE_STRIPEX,Layout::netpanel[0].y+Layout::netpanel[0].h,Layout::netpanel[0].w,Layout::SINGLE_STRIPEY);
  backscreen->rectangle(Color::black,Layout::netpanel[0].x+Layout::netpanel[0].w,Layout::netpanel[0].y+Layout::SINGLE_STRIPEY,Layout::SINGLE_STRIPEX,Layout::netpanel[0].h);


  //panel z statystyka
  backscreen->rectangle(tools[0],Layout::netpanel[1].x,Layout::netpanel[1].y,Layout::netpanel[1].w,Layout::SINGLE_TITLE);
  backscreen->rectangle(tools[1],Layout::netpanel[1].x,Layout::netpanel[1].y+Layout::SINGLE_TITLE,Layout::netpanel[1].w,Layout::netpanel[1].h-Layout::SINGLE_TITLE);
  backscreen->rectangle(tools[2],Layout::netpanel[1].x+Layout::netpanel[1].mr1,Layout::netpanel[1].y,Layout::netpanel[1].mrw,Layout::netpanel[1].h);
  backscreen->rectangle(tools[2],Layout::netpanel[1].x+Layout::netpanel[1].mr2,Layout::netpanel[1].y,Layout::netpanel[1].mrw,Layout::netpanel[1].h);
  backscreen->rectangle(Color::black,Layout::netpanel[1].x+Layout::SINGLE_STRIPEX,Layout::netpanel[1].y+Layout::netpanel[1].h,Layout::netpanel[1].w,Layout::SINGLE_STRIPEY);
  backscreen->rectangle(Color::black,Layout::netpanel[1].x+Layout::netpanel[1].w,Layout::netpanel[1].y+Layout::SINGLE_STRIPEY,Layout::SINGLE_STRIPEX,Layout::netpanel[1].h);

  //panel z obrazkami
  backscreen->rectangle(tools[3],Layout::netpanel[2].x,Layout::netpanel[2].y,Layout::netpanel[2].w,Layout::netpanel[2].h);
  backscreen->rectangle(Color::black,Layout::netpanel[2].x+Layout::SINGLE_STRIPEX,Layout::netpanel[2].y+Layout::netpanel[2].h,Layout::netpanel[2].w,Layout::SINGLE_STRIPEY);
  backscreen->rectangle(Color::black,Layout::netpanel[2].x+Layout::netpanel[2].w,Layout::netpanel[2].y+Layout::SINGLE_STRIPEY,Layout::SINGLE_STRIPEX,Layout::netpanel[2].h);

  //panel z wersja gry
  backscreen->rectangle(tools[1],Layout::netpanel[3].x,Layout::netpanel[3].y,Layout::netpanel[3].w,Layout::netpanel[3].h);
  backscreen->rectangle(Color::black,Layout::netpanel[3].x+Layout::SINGLE_STRIPEX,Layout::netpanel[3].y+Layout::netpanel[3].h,Layout::netpanel[3].w,Layout::SINGLE_STRIPEY);
  backscreen->rectangle(Color::black,Layout::netpanel[3].x+Layout::netpanel[3].w,Layout::netpanel[3].y+Layout::SINGLE_STRIPEY,Layout::SINGLE_STRIPEX,Layout::netpanel[3].h);
  rent=0;
}


void Statistics::Net(void)
{
  DBG_CHECK(!Mp::BUILDSPRITESMODE);
  DBG_CHECK(NoNet::is_network_mode());
  memset(posit,0,sizeof(posit));
  memset(selected,0,sizeof(selected));
  ptr=(void*)Heap::alloc(Screen::mem_required(Mp::SX,Mp::SY,Spr::hires),"statistics screen");
  backscreen=NEW(Screen((unsigned char*)ptr,Mp::SX,Mp::SY,Mp::SX,Spr::hires),"screen");
  for(int t=0;t<7;t++) tools[t].brightness(Layout::NET_TOOLS[t]);
  put_netback();

  recount();
  Tool tool;
  tool.brightness(50);
  int stop=1;
  int capt=0;
  int this_id=NoNet::get_this_id();
  int levels_numb=NoNet::get_levels_number();
  SManager::unlock();
  if (say_who)
  {
    SManager::add_sample(&Mysound::speech_you);
    SManager::add_sample(&Mysound::speech_are);
    SManager::add_sample(&Mysound::speech_colours[NoNet::get_this_id()]);
    say_who=0;
  }
  KbdStat::init();
  do
  {
    while (KbdStat::run(KbdStat::local_frame()))
    {
      NoNet::refresh();
      stop=1;
      runNet();
      SManager::run();
      FOR_ALL_USERS(ID)
      {
        if ((!selected[ID])||(posit[ID]!=posit[this_id]))
        {
          stop=0;
        }
      }
      if (!stop)
      {
        if (KbdStat::chat_request(-1)&&NoNet::is_network_mode())
        {
          DBG_CHECK(NoNet::is_network_mode());
          Chat::talk(0,KbdStat::chat_request(-1));
        }
        else if (KbdStat::menu_on()&&!Menu::in_use())
        {
          Menu::options();
        }
        else if (KbdStat::quit())
        {
          Menu::quit();
        }
        else if (KbdStat::hires()&&Mp::HIRESAVAIL)
        {
          SysSet::set(SysSet::RESOLUTION,!SysSet::get(SysSet::RESOLUTION));
        }
        else if (KbdStat::gamma_minus())
        {
          int curr=SysSet::get(SysSet::GAMMA);
          if (curr>Video::min_gamma)
          {
            SysSet::set(SysSet::GAMMA,curr-1);
          }
        }
        else if (KbdStat::gamma_plus())
        {
          int curr=SysSet::get(SysSet::GAMMA);
          if (curr<Video::max_gamma)
          {
            SysSet::set(SysSet::GAMMA,curr+1);
          }
        }
        else if (KbdStat::captscreen_noinfo()||(KbdStat::captscreen()))
        {
          capt=1;
        }
        Menu::serve();
      }
      else
      {
        break;
      }
    }
    Screen scr=Display::screen();
    Statistics::drawNet(scr,tool);
    if (capt)
    {
      scr.capture();
      capt=0;
    }
    Display::copy2vga();
  }
  while (!stop);
  KbdStat::done();
  Statistics::reset_last();
  Display::clear_vga(9);

  if (backscreen)
  {
    delete backscreen;
    backscreen=NULL;
  }
  if (ptr)
  {
    Heap::free(ptr);
    ptr=NULL;
  }

  if (stop&&(posit[NoNet::get_server_id()]<levels_numb))
  {
    int local=NoNet::get_local_level_number(posit[NoNet::get_server_id()]);
    Pilot::set_level(local);
  }
  else
  {
    throw TerminateGame();
  }
  delete backscreen;
}

void Statistics::runNet(void)
{
  int this_id=NoNet::get_this_id();
  int levels_numb=NoNet::get_levels_number();
  int possible_states=levels_numb+1;
  FOR_ALL_USERS(ID)
  {
    if (!KbdStat::using_menu(ID))
    {
      if (!selected[ID])
      {
        if (KbdStat::net_head_nextlev(ID))
        {
          if (ID==this_id) Sound::st_play(&Mysound::options_updown);
          if ((--posit[ID])<0)
          {
            posit[ID]=possible_states-1;
          }
        }
        if (KbdStat::net_head_prevlev(ID))
        {
          if (ID==this_id)Sound::st_play(&Mysound::options_updown);
          if ((++posit[ID])>=possible_states)
          {
            posit[ID]=0;
          }
        }
        if (KbdStat::net_head_accept(ID))
        {
          if (ID==this_id) Sound::st_play(&Mysound::options_accept);
          selected[ID]=1;
        }
      }
      else
      {
        if (KbdStat::net_head_nextlev(ID))
        {
          if (ID==this_id)Sound::st_play(&Mysound::options_updown);
          if ((--posit[ID])<0)
          {
            posit[ID]=possible_states-1;
          }
          selected[ID]=0;
        }
        if (KbdStat::net_head_prevlev(ID))
        {
          if (ID==this_id)Sound::st_play(&Mysound::options_updown);
          if ((++posit[ID])>=possible_states)
          {
            posit[ID]=0;
          }
          selected[ID]=0;
        }
        if (KbdStat::net_head_reject(ID))
        {
          selected[ID]=0;
        }
      }
    }
  }
}

void Statistics::drawNet(Screen& screen,Tool& tool)
{
  DBG_CHECK(initialized);
  if (!Menu::in_use())
  {
    if (rent) put_netback();
    backscreen->copy(screen);
    int size=Mysprites::level_pictures[0].r-Mysprites::level_pictures[0].l+Layout::STATSPRITEW;
    int ysize=Mysprites::level_pictures[0].d-Mysprites::level_pictures[0].u;
    int levels_numb=NoNet::get_levels_number();
    int our_num=posit[NoNet::get_this_id()];


    int dd=Print::get_dy(Print::TSPRITE,"*LEVEL")+5;
    int cnsty=4;
    for(int j=0;j<levels_numb;j++)
    {
      screen.rectangle(tools[5-(j%2)],Layout::netpanel[2].x+Layout::netpanel[2].mrw,Layout::netpanel[2].mr1+3+j*dd,Layout::netpanel[2].w-(Layout::netpanel[2].mrw*2),dd);

      char *levname=GAMETXT(NoNet::get_level_name(j));
      Print::print(screen,Print::TSPRITE,Print::FMENULEVELTEXT,Layout::netpanel[2].x+Layout::SINGLE_MARGIN+Layout::netpanel[2].mrw,Layout::netpanel[2].mr1+j*dd+cnsty,"*%d.%s",j+1,levname);
      if (j==our_num)
        screen.rectangle(tools[6],Layout::netpanel[2].x+Layout::netpanel[2].mrw,Layout::netpanel[2].mr1+3+j*dd,Layout::netpanel[2].w-(Layout::netpanel[2].mrw*2),dd);
    }
    j++;
    screen.rectangle(tools[5-((j-1)%2)],Layout::netpanel[2].x+Layout::netpanel[2].mrw,Layout::netpanel[2].mr1+3+j*dd,Layout::netpanel[2].w-(Layout::netpanel[2].mrw*2),dd);
    Print::print(screen,Print::TSPRITE,Print::FMENULEVELTEXT,Layout::netpanel[2].x+Layout::SINGLE_MARGIN+Layout::netpanel[2].mrw,Layout::netpanel[2].mr1+j*dd+cnsty,"*EXIT");
    //wersja gry
    if (NoNet::is_network_mode(NoNet::NETWORKMATCH))
    {
      Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::netpanel[3].x+Layout::SINGLE_MARGIN,Layout::netpanel[3].y+4,"*%s",GAMETXT("firefight"));
      char tmp[1024];
      if (NoNet::end_condition()==NoNet::FRAGS)
      {
        sprintf(tmp,"*%s %d",GAMETXT("netfrags"),NoNet::max_frags());
        int _x=Layout::netpanel[3].x+Layout::netpanel[3].w-Print::get_dx(Print::TSPRITE,tmp)-Layout::SINGLE_MARGIN;
        Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,_x,Layout::netpanel[3].y+4,tmp);
      }
      if (NoNet::end_condition()==NoNet::TIMEUP)
      {
        sprintf(tmp,"*%s %d min",GAMETXT("nettimeup"),NoNet::game_time());
        int _x=Layout::netpanel[3].x+Layout::netpanel[3].w-Print::get_dx(Print::TSPRITE,tmp)-Layout::SINGLE_MARGIN;
        Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,_x,Layout::netpanel[3].y+4,tmp);
      }
    }

    if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
    {
      char tmp[1024];
      sprintf(tmp,"*%s %d",GAMETXT("netbrickcount"),Op::MAXSHIPBRICK);
      int _x=Layout::netpanel[3].x+Layout::netpanel[3].w-Print::get_dx(Print::TSPRITE,tmp)+3;
      Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::netpanel[3].x+Layout::SINGLE_MARGIN,Layout::netpanel[3].y+4,"*%s",GAMETXT("basebuild"));
      Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::netpanel[3].x+Layout::netpanel[3].mr1,Layout::netpanel[3].y+4,tmp);
    }

    if (our_num>=levels_numb)
    {
      screen.rectangle(tools[6],Layout::netpanel[2].x+Layout::netpanel[2].mrw,Layout::netpanel[2].mr1+3+j*dd,Layout::netpanel[2].w-(Layout::netpanel[2].mrw*2),dd);
      screen.put(Layout::netpanel[2].x+Layout::netpanel[2].mrw,Layout::netpanel[2].y+5,Mysprites::level_pictures,Mysprites::level_pictures.phases-1);
    }

    if (our_num<levels_numb&&our_num<Mysprites::level_pictures.phases)
    {
      int local=NoNet::get_local_level_number(our_num);
      screen.put(Layout::netpanel[2].x+Layout::netpanel[2].mrw,Layout::netpanel[2].y+5,Mysprites::level_pictures,local);
    }

    if (our_num<levels_numb)
    {
      char tmp[1024];
      sprintf(tmp,"%s",NoNet::get_level_name(our_num));
      int dd=Print::get_dx(Print::TSPRITE,"^%d. %s",our_num+1,GAMETXT(tmp,0));
      Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::netpanel[0].x+Layout::netpanel[0].w-Layout::SINGLE_MARGIN-dd,Layout::netpanel[0].y+2,"^%d. %s",our_num+1,GAMETXT(tmp,0));
    }
    else
    {
      int dd=Print::get_dx(Print::TSPRITE,"^EXIT");
      Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::netpanel[0].x+Layout::netpanel[0].w-Layout::SINGLE_MARGIN-dd,Layout::netpanel[0].y+2,"^EXIT");
    }


    int users_num=0;
    int users_table[NoNet::max_users+1];
    FOR_ALL_USERS(ENUMID)
    {
      users_table[users_num++]=ENUMID;
    }
    users_table[users_num]=0;
    int beg=0;
    int end=(users_num-1);
    while (beg<end)
    {
      for (int k=0;k<end;k++)
      {
        if (compare(users_table[k],users_table[k+1])<0)
        {
          users_table[k]^=users_table[k+1];
          users_table[k+1]^=users_table[k];
          users_table[k]^=users_table[k+1];
        }
      }
      end--;
    }

    int pn1=(Layout::netpanel[1].mrw-Print::get_dx(Print::TSPRITE,"*TOTAL"))/2;
    int pn2=(Layout::netpanel[1].mrw-Print::get_dx(Print::TSPRITE,"*LAST"))/2;
    Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::netpanel[1].x+Layout::netpanel[1].mr2+pn2,Layout::netpanel[1].y+3,"*LAST");
    Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::netpanel[1].x+Layout::netpanel[1].mr1+pn1,Layout::netpanel[1].y+3,"*TOTAL");

    int xmid=Layout::netpanel[1].x+Layout::netpanel[1].mr1+Layout::netpanel[1].mrw+(Layout::netpanel[1].mr2-(Layout::netpanel[1].mr1+Layout::netpanel[1].mrw))/2;
    int cnstline=18;
    for (int i=0;i<users_num;i++)
    {
      int ID=users_table[i];
      char name[1024];

      sprintf(name,"*%s",NoNet::get_player_name(ID));
      for(int j=strlen(name)-1;j>=0;j--)
      {
        if (Print::get_dx(Print::TSPRITE,name)<=Layout::netpanel[1].mr1-Layout::SINGLE_MARGIN-4)
          break;
        name[j]='\0';
      }



      Print::print(screen,Print::TSPRITE,Print::FMENULEVELTEXT,Layout::netpanel[1].x+Layout::SINGLE_MARGIN,Layout::netpanel[1].y+6+(1+i)*cnstline,name);
      int p1=(Layout::netpanel[1].mrw-Print::get_dx(Print::TSPRITE,"*%d",sum[ID]))/2;
      int p2=(Layout::netpanel[1].mrw-Print::get_dx(Print::TSPRITE,"*%d",last[ID]))/2;

      Print::print(screen,Print::TSPRITE,Print::FMENULEVELTEXT,Layout::netpanel[1].x+Layout::netpanel[1].mr1+p1,Layout::netpanel[1].y+6+(1+i)*cnstline,"*%d",sum[ID]);
      Print::print(screen,Print::TSPRITE,Print::FMENULEVELTEXT,Layout::netpanel[1].x+Layout::netpanel[1].mr2+p2,Layout::netpanel[1].y+6+(1+i)*cnstline,"*%d",last[ID]);
      screen.put(xmid,Layout::netpanel[1].y+11+(1+i)*cnstline,Mysprites::pointer,ID);

      if (selected[ID])
      {

        if (posit[ID]>=levels_numb)
          screen.put(Layout::netpanel[2].x+Layout::netpanel[2].w-Layout::netpanel[2].mrw-4*ID,Layout::netpanel[2].mr1+(1+posit[ID])*dd+11,Mysprites::pointer,ID);
        else
          screen.put(Layout::netpanel[2].x+Layout::netpanel[2].w-Layout::netpanel[2].mrw-4*ID,Layout::netpanel[2].mr1+posit[ID]*dd+11,Mysprites::pointer,ID);
      }
    }
  }
  else Mysprites::put_pattern(screen);
  Menu::display(screen);
}

//single
const __MINUSONE__=10000000;

int Statistics::Single(void)
{
  DBG_CHECK(!NoNet::is_network_mode());
  Display::clear_vga(9);
  ptr=(void*)Heap::alloc(Screen::mem_required(Mp::SX,Mp::SY,Spr::hires),"statistics screen");
  backscreen=NEW(Screen((unsigned char*)ptr,Mp::SX,Mp::SY,Mp::SX,Spr::hires),"screen");

  int levels_numb=0;
#ifdef SHAREWARE
    levels_numb=Mysprites::missions_pics_dark.phases;
#else
    levels_numb=GameManager::level_descript.size("Missions_order");
#endif
  int myposit=0;
  int available=0;
  int demo=0;
  active_time=active_phs=0;
  MAX_SINGLELEVEL=levels_numb;

  int demo_status=0;
  int demo_number=0;
  int avail_demos=Game::demo_text.size("demo_list");
  int demo_time=0;

  memset(posit,0,sizeof(posit));

#ifdef SHAREWARE
  Song song;
  if (RegData::UseCD) song.play(Comm::get_file_path(GameManager::level_descript.string("sw_headersong")),0);
#else
  Game::play_CD(GameManager::level_descript.value("headersong"),0);
#endif

  for (int i=0;i<levels_numb;i++)
  {
    if (Pilot::level_status(i))  posit[myposit]++;
    else
    {
      available=i;
      break;
    }
  }
  if (i>=levels_numb)
  {
    available=levels_numb-1;
    posit[myposit]=0;
  }

  t_time=t_secret=t_kill=0;
  int global_sec=0;
  int done_sec=0;
  int global_kills=0;
  int done_kills=0;
  for (i=0;i<levels_numb;i++)
  {
    if (Pilot::level_status(i))
    {
      t_time+=Pilot::time(i);
      global_sec+=Pilot::secrets_num(i);
      done_sec+=Pilot::secrets(i);

      global_kills+=Pilot::kills_num(i);
      done_kills+=Pilot::kills(i);
    }
  }
  if (global_sec)
    t_secret=done_sec*100/global_sec;
  else t_secret=100;

  if (global_kills)
    t_kill=done_kills*100/global_kills;
  else t_kill=100;

  memset(selected,0,sizeof(selected));
  for (int p=1;p<NoNet::max_users+1;p++) sum[p]+=last[p];
  Tool light1;
  Tool light2;
  Tool dark;

  light1.brightness(Layout::SINGLELIGHT1);
  light2.brightness(Layout::SINGLELIGHT2);
  dark.brightness(Layout::SINGLEDARK);

  tools[0].brightness(Layout::SINGLE_TOOLS[0]);
  tools[1].brightness(Layout::SINGLE_TOOLS[1]);
  tools[2].brightness(Layout::SINGLE_TOOLS[2]);
  tools[3].brightness(Layout::SINGLE_TOOLS[3]);

  put_singleback();
  SManager::unlock();
  KbdStat::init();

  int size=(Mysprites::missions_pics_nor[0].d-Mysprites::missions_pics_nor[0].u+3)*MAX_SINGLELEVEL;
  int sizew=Mysprites::missions_pics_nor[0].r-Mysprites::missions_pics_nor[0].l;
  int onesize=Mysprites::missions_pics_nor[0].d-Mysprites::missions_pics_nor[0].u+3;

  Y= __MINUSONE__;
  int stop=0;
  int capt=0;
  snd=0;
  first_time=1;

  while (!stop)
  {
    while (KbdStat::run(KbdStat::local_frame()))
    {
      if (!demo_status||!Mp::DEBUG)
      {
        if (!Menu::in_use()&&!first_time)
        {
          runSingle();

          demo_time+=KbdStat::time();
          if (demo_time>60*Mp::METRONQUALITY)
          {
            demo_time=0;
            Game::set_demo_properties(Game::PLAY_RANDOM,NULL,0);
            demo=Game::DEMO_PLAY;
            stop=1;
            break;
          }

          active_time+=KbdStat::time();
          if (active_time>=Layout::SINGLE_ACTIVETIME)
          {
             if (++active_phs>=Mysprites::active_mission.phases) active_phs=0;
             active_time=0;
          }

          if (KbdStat::menu_secret_on()&&SysSet::get(SysSet::CHEATING)&&posit[myposit]<GameManager::level_descript.size("Missions_order"))
          {
            Sound::st_play(&Mysound::options_accept);
            GameManager::set_testing();
            stop=1;
            break;
          }
          else if (KbdStat::menu_up())
          {
            if ((--posit[myposit])<0)  posit[myposit]=MAX_SINGLELEVEL-1;
            Sound::st_play(&Mysound::options_updown);
            demo_time=0;
          }
          else if (KbdStat::menu_down())
          {
            Sound::st_play(&Mysound::options_updown);
            if ((++posit[myposit])>=MAX_SINGLELEVEL) posit[myposit]=0;
            demo_time=0;
          }
          else if (KbdStat::menu_accept())
          {
            if (posit[myposit]<GameManager::level_descript.size("Missions_order"))
            {
              if (posit[myposit]<levels_numb&&available>=posit[myposit])
              {
                Sound::st_play(&Mysound::options_accept);
                stop=1;
              }
              else Sound::st_play(&Mysound::options_na);
            }
            else
            {
              Sound::st_play(&Mysound::options_na);
            }
            demo_time=0;
          }
          else if ((KbdStat::menu_page_up()||KbdStat::menu_page_down())&&Mp::DEBUG)
          {
            demo_number=0;
            demo_status=1;
            demo_time=0;
          }
        }
        if (KbdStat::menu_on()&&!Menu::in_use())
        {
          Menu::options();
          demo_time=0;
        }
        else if (KbdStat::quit())
        {
          Menu::quit();
          demo_time=0;
        }
        else if (KbdStat::hires()&&Mp::HIRESAVAIL)
        {
          SysSet::set(SysSet::RESOLUTION,!SysSet::get(SysSet::RESOLUTION));
          demo_time=0;
        }
        else if (KbdStat::gamma_minus())
        {
          int curr=SysSet::get(SysSet::GAMMA);
          if (curr>Video::min_gamma)
          {
            SysSet::set(SysSet::GAMMA,curr-1);
          }
        }
        else if (KbdStat::gamma_plus())
        {
          int curr=SysSet::get(SysSet::GAMMA);
          if (curr<Video::max_gamma)
          {
            SysSet::set(SysSet::GAMMA,curr+1);
          }
        }
        else if (KbdStat::captscreen_noinfo()||(KbdStat::captscreen()))
        {
          capt=1;
          demo_time=0;
        }
        else if (KbdStat::cheating()&&!NoNet::is_network_mode())
        {
          demo_time=0;
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

        int y=-(posit[0]*onesize);
        if (y>=size) y-=size;
        if (y<0) y+=size;

        if (Y== __MINUSONE__)  Y=y;

        if (Y!=y)
        {
          snd=0;

          int dir=(y<Y)?-1:1;
          if (abs(y-Y)>size/2) dir=-dir;

          Y+=dir*Layout::SINGLEANIMSPEED;

          if (Y<0) Y+=size;
          if (Y>=size) Y-=size;

          int s=(abs(Y-y)>size/2)?size-abs(Y-y):abs(Y-y);
          if (s<Layout::SINGLEANIMSPEED) Y=y;
        }
        else  if (!snd)
        {
          snd=1;
        }
        first_time=0;
        Menu::serve();
      }
      else
      {
        if (KbdStat::menu_page_up())
        {
          demo_time=0;
          demo_number++;
          if (demo_number>=avail_demos+2)
          {
            demo_number=0;
          }
        }
        else if (KbdStat::menu_page_down())
        {
          demo_time=0;
          demo_number--;
          if (demo_number<0)
          {
            demo_number=avail_demos+1;
          }
        }
        else if (KbdStat::menu_quit())
        {
          demo_time=0;
          demo_number=0;
          demo_status=0;
        }
        else if (KbdStat::menu_accept())
        {
          switch (demo_number)
          {
             case 0: Game::set_demo_properties(Game::RECORD_LEVELNUMB,NULL,posit[myposit]);
                     demo=Game::DEMO_RECORD;
                     stop=1;
                     break;
             case 1: Game::set_demo_properties(Game::PLAY_LASTSAVED,NULL,0);
                     demo=Game::DEMO_PLAY;
                     stop=1;
                     break;
            default: Game::set_demo_properties(Game::PLAY_FILENAME,Game::demo_text.string("demo_list",demo_number-2),0);
                     demo=Game::DEMO_PLAY;
                     stop=1;
                     break;
          }
        }
      }
    }

    if (Pilot::level_status(posit[0]))
    {
      time=Pilot::time(posit[0]);
      secret=Pilot::secrets(posit[0]);

      if (Pilot::secrets_num(posit[0]))
        secret=Pilot::secrets(posit[0])*100/Pilot::secrets_num(posit[0]);
      else secret=100;

      if (Pilot::kills_num(posit[0]))
        kill=Pilot::kills(posit[0])*100/Pilot::kills_num(posit[0]);
      else kill=100;
    }
    else time=secret=kill=-1;

    Screen scr=Display::screen();
    Statistics::drawSingle(scr,light1,light2,dark);
    if (demo_status)
    {
      int orgx=scr.ox;
      int orgy=scr.oy;
      scr.origin(0,0);
      int timesx=121;
      int timesy=Op::DISPINFOINVSY;
      int timex=Mp::SX-timesx-Op::DISPINFOWEAPX;
      int timey=Mp::SY-timesy-Op::DISPINFOWEAPY;
      int timeinx=61;
      int timeiny=Op::DISPINFOINVINRECTY;
      Tool& tool=Tools::tool[Tools::DISPINFO];
      scr.rectangle(tool,timex,timey,timesx,timesy);
      char time[1024];
      switch (demo_number)
      {
         case 0: sprintf(time,"RECORD DEMO");
                 break;
         case 1: sprintf(time,"PLAY LAST SAVED DEMO");
                 break;
        default: sprintf(time,"PLAY '%s' DEMO",Game::demo_text.string("demo_list",demo_number-2));
                 break;
      }
      Print::print(scr,Print::TFONT,Print::_FLAST,1+timex+timeinx-Print::get_dx(Print::TFONT,time)/2,timey+timeiny,time);
      Print::print(scr,Print::TFONT,Print::FINVOVR,timex+timeinx-Print::get_dx(Print::TFONT,time)/2,timey+timeiny,time);
      scr.origin(orgx,orgy);
    }
    if (capt)
    {
      scr.capture();
      capt=0;
    }
    Display::copy2vga();
  }
  KbdStat::done();
  Statistics::reset_last();
  Display::clear_vga(9);


  if (backscreen)
  {
    delete backscreen;
    backscreen=NULL;
  }
  if (ptr)
  {
    Heap::free(ptr);
    ptr=NULL;
  }

#ifdef SHAREWARE
  if (RegData::UseCD) song.stop();
#else
  Game::stop_CD();
#endif

  if (demo==Game::DEMO_PLAY)
  {
    return Game::DEMO_PLAY;
  }
  if (demo==Game::DEMO_RECORD)
  {
    return Game::DEMO_RECORD;
  }
  DBG_CHECK(posit[myposit]<levels_numb);
  Pilot::set_level(posit[myposit]);
  return Game::PLAY;
}

void Statistics::runSingle(void)
{
}

void Statistics::put_singleback(void)
{
  backscreen->reinit();
  Mysprites::put_pattern(*backscreen,1);
  //najwiekszy kwadrat
  backscreen->rectangle(tools[0],Layout::panel[3].x,Layout::panel[3].y,Layout::panel[3].w,Layout::panel[3].h);
  //panel z tytulem
  backscreen->rectangle(tools[1],Layout::panel[0].x,Layout::panel[0].y,Layout::panel[0].w,Layout::panel[0].h);
  backscreen->rectangle(Color::black,Layout::panel[0].x+Layout::SINGLE_STRIPEX,Layout::panel[0].y+Layout::panel[0].h,Layout::panel[0].w,Layout::SINGLE_STRIPEY);
  backscreen->rectangle(Color::black,Layout::panel[0].x+Layout::panel[0].w,Layout::panel[0].y+Layout::SINGLE_STRIPEY,Layout::SINGLE_STRIPEX,Layout::panel[0].h);

  //panel z opisem
  //backscreen->rectangle(tools[1],Layout::panel[1].x,Layout::panel[1].y,Layout::panel[1].w,Layout::SINGLE_TITLE);
  backscreen->rectangle(tools[2],Layout::panel[1].x,Layout::panel[1].y,Layout::panel[1].w,Layout::panel[1].h);
  backscreen->rectangle(Color::black,Layout::panel[1].x+Layout::SINGLE_STRIPEX,Layout::panel[1].y+Layout::panel[1].h,Layout::panel[1].w,Layout::SINGLE_STRIPEY);
  backscreen->rectangle(Color::black,Layout::panel[1].x+Layout::panel[1].w,Layout::panel[1].y+Layout::SINGLE_STRIPEY,Layout::SINGLE_STRIPEX,Layout::panel[1].h);


  //panel z statystyka
  backscreen->rectangle(tools[1],Layout::panel[2].x,Layout::panel[2].y,Layout::panel[2].w,Layout::SINGLE_TITLE);
  backscreen->rectangle(tools[2],Layout::panel[2].x,Layout::panel[2].y+Layout::SINGLE_TITLE,Layout::panel[2].w,Layout::panel[2].h-Layout::SINGLE_TITLE);
  backscreen->rectangle(tools[3],Layout::panel[2].x+Layout::panel[2].mr1,Layout::panel[2].y,Layout::panel[2].mrw,Layout::panel[2].h);
  backscreen->rectangle(tools[3],Layout::panel[2].x+Layout::panel[2].mr2,Layout::panel[2].y,Layout::panel[2].mrw,Layout::panel[2].h);
  backscreen->rectangle(Color::black,Layout::panel[2].x+Layout::SINGLE_STRIPEX,Layout::panel[2].y+Layout::panel[2].h,Layout::panel[2].w,Layout::SINGLE_STRIPEY);
  backscreen->rectangle(Color::black,Layout::panel[2].x+Layout::panel[2].w,Layout::panel[2].y+Layout::SINGLE_STRIPEY,Layout::SINGLE_STRIPEX,Layout::panel[2].h);
  rent=0;
}


void Statistics::drawSingle(Screen &screen,Tool& light1,Tool& light2,Tool& dark)
{
  if (!Menu::in_use())
  {
    if (rent) put_singleback();
    //screen.copy(*backscreen);
    backscreen->copy(screen);
    int levels_numb=GameManager::level_descript.size("Missions_order");
    int size=(Mysprites::missions_pics_nor[0].d-Mysprites::missions_pics_nor[0].u+3)*MAX_SINGLELEVEL;
    int sizew=Mysprites::missions_pics_nor[0].r-Mysprites::missions_pics_nor[0].l;
    int onesize=Mysprites::missions_pics_nor[0].d-Mysprites::missions_pics_nor[0].u+3;
    int x=Layout::SINGLEBRICKX;

    if (posit[0]<levels_numb)
    {
      char tmp[1024];
      sprintf(tmp,"%s",GameManager::level_descript.string("Missions_order",posit[0]));
      int dd=Print::get_dx(Print::TSPRITE,"^%d. %s",posit[0]+1,GAMETXT(tmp,0));
      Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[0].x+Layout::panel[0].w-Layout::SINGLE_MARGIN-dd,Layout::panel[0].y+2,"^%d. %s",posit[0]+1,GAMETXT(tmp,0));
      for (int i=1;i<GameManager::game_text.size(tmp);i++)
      {
        int dy=(i-1)*13+3;
        //int dx=(Layout::panel[1].w-Print::get_dx(Print::TSPRITE,"%s",GAMETXT(tmp,i)))/2;
        Print::print(screen,Print::TSPRITE,Print::FMENULEVELTEXT,Layout::panel[1].x+Layout::SINGLE_MARGIN,Layout::panel[1].y+dy,GAMETXT(tmp,i));
      }
    }
    else
    {
      char tmp[1024];
      sprintf(tmp,"%s","not_share_descript");
      int dd=Print::get_dx(Print::TSPRITE,"^%d. %s",posit[0]+1,GAMETXT("not_shareware_mission"));
      Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[0].x+Layout::panel[0].w-Layout::SINGLE_MARGIN-dd,Layout::panel[0].y+2,"^%d. %s",posit[0]+1,GAMETXT("not_shareware_mission"));
      for (int i=0;i<GameManager::game_text.size(tmp);i++)
      {
        int dy=i*13+3;
        Print::print(screen,Print::TSPRITE,Print::FMENULEVELTEXT,Layout::panel[1].x+Layout::SINGLE_MARGIN,Layout::panel[1].y+dy,GAMETXT(tmp,i));
      }
    }

    int lst=0;
    if (!Pilot::level_status(posit[0]))
    {
      if (posit[0]-1>=0)
      {
        if (Pilot::level_status(posit[0]-1)) lst=1;
      }
      else lst=1;
    }

    int ms=(Layout::panel[2].mrw-Print::get_dx(Print::TSPRITE,"*MISSION"))/2;
    int mst=(Layout::panel[2].mrw-Print::get_dx(Print::TSPRITE,"*TOTAL"))/2;
    if (lst)
      Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr1+ms,Layout::panel[2].y+3,"*PREVIOUS");
    else
      Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr1+ms,Layout::panel[2].y+3,"*MISSION");
    Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr2+mst,Layout::panel[2].y+3,"*TOTAL");

    Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::SINGLE_MARGIN,Layout::panel[2].y+Layout::SINGLE_TITLE+3,"*KILLS");
    Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::SINGLE_MARGIN,Layout::panel[2].y+12+Layout::SINGLE_TITLE+3,"*SECRETS");
    Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::SINGLE_MARGIN,Layout::panel[2].y+24+Layout::SINGLE_TITLE+3,"*TIME");

    if (time>0)
    {
      Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr1+ms,Layout::panel[2].y+Layout::SINGLE_TITLE+3,"*%d%%",kill);
      Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr1+ms,Layout::panel[2].y+Layout::SINGLE_TITLE+3+12,"*%d%%",secret);
      int h=time/3600;
      int m=time/60-(h*60);
      int s=time-(m*60)-(h*3600);
      Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr1+ms,Layout::panel[2].y+Layout::SINGLE_TITLE+3+24,"*%02d:%02d:%02d",h,m,s);
    }
    else
    {
      if (!lst)
      {
        Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr1+ms,Layout::panel[2].y+Layout::SINGLE_TITLE+3,"------");
        Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr1+ms,Layout::panel[2].y+Layout::SINGLE_TITLE+3+12,"------");
        Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr1+ms,Layout::panel[2].y+Layout::SINGLE_TITLE+3+24,"------");
      }
      else
      {
        if (posit[0]-1>=0)
        {
          time=Pilot::time(posit[0]-1);
          secret=Pilot::secrets(posit[0]-1);

          if (Pilot::secrets_num(posit[0]-1))
            secret=Pilot::secrets(posit[0]-1)*100/Pilot::secrets_num(posit[0]-1);
          else secret=100;

          if (Pilot::kills_num(posit[0]-1))
            kill=Pilot::kills(posit[0]-1)*100/Pilot::kills_num(posit[0]-1);
          else kill=100;
          Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr1+ms,Layout::panel[2].y+Layout::SINGLE_TITLE+3,"*%d%%",kill);
          Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr1+ms,Layout::panel[2].y+Layout::SINGLE_TITLE+3+12,"*%d%%",secret);
          int h=time/3600;
          int m=time/60-(h*60);
          int s=time-(m*60)-(h*3600);
          Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr1+ms,Layout::panel[2].y+Layout::SINGLE_TITLE+3+24,"*%02d:%02d:%02d",h,m,s);
        }
        else
        {
          Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr1+ms,Layout::panel[2].y+Layout::SINGLE_TITLE+3,"------");
          Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr1+ms,Layout::panel[2].y+Layout::SINGLE_TITLE+3+12,"------");
          Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr1+ms,Layout::panel[2].y+Layout::SINGLE_TITLE+3+24,"------");
        }
      }
    }

    int h=t_time/3600;
    int m=t_time/60-(h*60);
    int s=t_time-(m*60)-(h*3600);

    Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr2+mst,Layout::panel[2].y+Layout::SINGLE_TITLE+3,"*%d%%",t_kill);
    Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr2+mst,Layout::panel[2].y+Layout::SINGLE_TITLE+3+12,"*%d%%",t_secret);
    Print::print(screen,Print::TSPRITE,Print::FMENULEVELPANEL,Layout::panel[2].x+Layout::panel[2].mr2+mst,Layout::panel[2].y+Layout::SINGLE_TITLE+3+24,"*%02d:%02d:%02d",h,m,s);

    int p1=1;
    int p2=5;
    int p3=4;

    screen.rectangle(light2,x-p2,0,sizew+2*p2,Mp::SY);
    screen.rectangle(light1,x-p2-p1,0,p1,Mp::SY);
    screen.rectangle(dark,x+sizew+p2,0,p3,Mp::SY);

    int __y=Y;
    int prev=1;
    int ax=0;
    int ay=0;
    int pic_count=0;


    for (int i=0;i<MAX_SINGLELEVEL;i++)
    {
      if (((!Pilot::level_status(i)&&prev)||Pilot::level_status(i))&&i<GameManager::level_descript.size("Missions_order"))
      {
        screen.put(x,__y+Layout::SINGLEBRICKY,Mysprites::missions_pics_nor,i);
        if (!Pilot::level_status(i)&&prev)
          screen.put(x+ax,ay+__y+Layout::SINGLEBRICKY,Mysprites::active_mission,active_phs);
      }
      else
        screen.put(x,__y+Layout::SINGLEBRICKY,Mysprites::missions_pics_dark,i);
      __y+=onesize;
      prev=Pilot::level_status(i);
    }

    __y=Y;
    prev=1;
    for (i=0;i<MAX_SINGLELEVEL;i++)
    {
      if (((!Pilot::level_status(i)&&prev)||Pilot::level_status(i))&&i<GameManager::level_descript.size("Missions_order"))
      {
        screen.put(x,__y-size+Layout::SINGLEBRICKY,Mysprites::missions_pics_nor,i);
        if (!Pilot::level_status(i)&&prev)
          screen.put(x+ax,ay+__y-size+Layout::SINGLEBRICKY,Mysprites::active_mission,active_phs);
      }
      else
        screen.put(x,__y-size+Layout::SINGLEBRICKY,Mysprites::missions_pics_dark,i);
      __y+=onesize;
      prev=Pilot::level_status(i);
    }
    screen.put(x,Layout::SINGLEBRICKY,Mysprites::frame,0);
  }
  else  Mysprites::put_pattern(screen);
  Menu::display(screen);
}
//KLASA CHAT
class AbortChat {};

char Chat::screenbuf[Chat::lines][Chat::screenline_len];
Chat::LineInfo Chat::screenbufinfo[lines];
char Chat::lastlinebuf[NoNet::max_users+1][Chat::bufline_len];
char Chat::linebuf[NoNet::max_users+1][Chat::bufline_len];
int  Chat::terminating=0;
int  Chat::capt=0;
char *Chat::screenlines[Chat::lines];
int  Chat::free_line=0;
int  Chat::first_time=1;
int  Chat::display_cursor[NoNet::max_users+1];
int  Chat::display_pos[NoNet::max_users+1];
int  Chat::cursor_pos[NoNet::max_users+1];
int  Chat::leftcount[NoNet::max_users+1];
int  Chat::rightcount[NoNet::max_users+1];
int  Chat::backcount[NoNet::max_users+1];
int  Chat::delcount[NoNet::max_users+1];
int  Chat::leftwordcount[NoNet::max_users+1];
int  Chat::rightwordcount[NoNet::max_users+1];
int  Chat::oper[NoNet::max_users+1];
char Chat::lastchar[NoNet::max_users+1];
int  Chat::lastcharcount[NoNet::max_users+1];
char Chat::namebuf[NoNet::max_users+1][name_len];

void Chat::talk(int in_game,int caller_id)
{
  DBG_CHECK(NoNet::is_network_mode());
  Display::clear_vga(9);
  first_time=1;
  free_line=0;
  terminating=0;
  capt=0;
  int serv_id=NoNet::get_server_id();
  for (int i=0;i<NoNet::max_users+1;i++)
  {
    cursor_pos[i]=0;
    display_pos[i]=0;
    display_cursor[i]=0;
    leftcount[i]=0;
    rightcount[i]=0;
    backcount[i]=0;
    delcount[i]=0;
    leftwordcount[i]=0;
    rightwordcount[i]=0;
    lastchar[i]='\0';
    lastcharcount[i]=0;
    oper[i]=0;
    lastlinebuf[i][0]='\0';
    linebuf[i][0]='\0';
  }
  for (i=0;i<lines;i++)
  {
    screenbuf[i][0]='\0';
    screenlines[i]=screenbuf[i];
    screenbufinfo[i].owners_id=0;
  }
  oper[serv_id]=1;

  FOR_ALL_USERS(NAMEID)
  {
    strncpy(namebuf[NAMEID],NoNet::get_player_name(NAMEID),name_len);
    namebuf[NAMEID][name_len-1]='\0';
  }

  FOR_ALL_USERS(ID1)
  {
    int duplicated=0;
    FOR_ALL_USERS(ID2)
    {
      if (ID2>ID1)
      {
        if (!strcmpi(namebuf[ID1],namebuf[ID2]))
        {
          duplicated++;
        }
      }
    }
    if (duplicated>0)
    {
      DBG_CHECK(duplicated<10);
      int pos=strlen(namebuf[ID1]);
      if (pos>=name_len-1)
      {
        pos=name_len-2;
      }
      namebuf[ID1][pos]=(char)(duplicated-'0');
    }
  }

  FOR_ALL_USERS(ID2)
  {
    char *name=namebuf[ID2];
    int indx=0;
    while (name[indx])
    {
      if (!isalnum(name[indx]))
      {
        name[indx]='_';
      }
      indx++;
    }
    DBG_CHECK(indx<name_len);
  }

  SManager::add_sample(&Mysound::speech_chatreqby);
  SManager::add_sample(&Mysound::speech_colours[caller_id]);

  log_string(0,"Chat daemon v1.01.");
  log_string(0,"Chat requested by %s.",player_name(caller_id));
  log_users_status();
  log_string(0,"");
  log_string(0,"Type '.help' to view summary of special commands.");
  log_string(0,"Type '.quit' to end the conversation.");
  log_string(0,"");

  try
  {
    do
    {
      while (KbdStat::run(KbdStat::local_frame()))
      {
        run();
        SManager::run();
        FOR_ALL_USERS(ID)
        {
          if ((KbdStat::menu_on(ID)||KbdStat::quit(ID))&&!KbdStat::using_menu(ID))
          {
            log_string(0,"Player %s is using menu now.",player_name(ID));
          }
        }
        if (KbdStat::menu_on()&&!Menu::in_use())
        {
          Menu::options();
        }
        else if (KbdStat::quit())
        {
          Menu::quit();
        }
        else if (KbdStat::hires()&&Mp::HIRESAVAIL)
        {
          SysSet::set(SysSet::RESOLUTION,!SysSet::get(SysSet::RESOLUTION));
        }
        else if (KbdStat::gamma_minus())
        {
          int curr=SysSet::get(SysSet::GAMMA);
          if (curr>Video::min_gamma)
          {
            SysSet::set(SysSet::GAMMA,curr-1);
          }
        }
        else if (KbdStat::gamma_plus())
        {
          int curr=SysSet::get(SysSet::GAMMA);
          if (curr<Video::max_gamma)
          {
            SysSet::set(SysSet::GAMMA,curr+1);
          }
        }
        else if (KbdStat::captscreen_noinfo()||(KbdStat::captscreen()))
        {
          capt=1;
        }
        Menu::serve();
        first_time=0;
      }
      Screen& scr=Display::screen();
      draw(scr);
      if (capt)
      {
        scr.capture();
        capt=0;
      }
      Display::copy2vga();
    } while (1);
  }
  catch (AbortChat) {}
  Display::clear_vga(in_game?gamemanager->get_sublevel():9);
}

void Chat::run(void)
{
  update_actions();
  if (terminating>0)
  {
    terminating--;
    if (terminating==25)
    {
      log_string(0,"Connection closed by foreign host...");
    }
    else if (terminating==20)
    {
      log_string(0,"Disconnecting players:");
      log_users_status();
    }
    else if (terminating<=0)
    {
      throw AbortChat();
    }
  }
  else
  {
    FOR_ALL_USERS(ID)
    {
      int dont_clear_linebuf=0;
      if (!KbdStat::using_menu(ID))
      {
        if (KbdStat::chat_prev_command(ID)&&strlen(lastlinebuf[ID])>0)
        {
          strncpy(linebuf[ID],lastlinebuf[ID],bufline_len);
          linebuf[ID][bufline_len-1]='\0';
          cursor_pos[ID]=strlen(linebuf[ID]);
          display_pos[ID]=0;
        }

        input_string(ID);

        if (KbdStat::chat_accept(ID)&&strlen(linebuf[ID])>0)
        {
          strncpy(lastlinebuf[ID],linebuf[ID],bufline_len);
          lastlinebuf[ID][bufline_len-1]='\0';

          if ((*linebuf[ID]=='.')||(*linebuf[ID]=='/')||(*linebuf[ID]=='#'))
          {
            char *command=strtok(linebuf[ID]+1," ");
            char *param=strtok(NULL,"");
            if (command==NULL)
            {
              serve_nocommand(ID,param);
            }
            else if (!strcmpi(command,"leave")||
                     !strcmpi(command,"quit")||
                     !strcmpi(command,"exit")||
                     !strcmpi(command,"terminate")||
                     !strcmpi(command,"bye"))
            {
              serve_quit(ID,param);
            }
            else if (!strcmpi(command,"cls"))
            {
              serve_cls(ID,param);
            }
            else if (!strcmpi(command,"help"))
            {
              serve_help(ID,param);
            }
            else if (!strcmpi(command,"query"))
            {
              serve_query(ID,param);
            }
            else if (!strcmpi(command,"users"))
            {
              serve_users(ID,param);
            }
            else if (!strcmpi(command,"whoami"))
            {
              serve_whoami(ID,param);
            }
            else
            {
              serve_invalidcommand(ID,command);
            }
          }
          else
          {
            log_string(ID,linebuf[ID]);
          }
          if (!dont_clear_linebuf)
          {
            linebuf[ID][0]='\0';
            display_pos[ID]=0;
            cursor_pos[ID]=0;
          }
        }
        update_display_string(ID);
      }
    }
  }
}

void Chat::draw(Screen& screen)
{
#ifndef SHAREWARE
  Mysprites::put_pattern(screen,0,1);
#else
  Mysprites::put_pattern(screen,0);
#endif

  if (!Menu::in_use())
  {
    int line_begx=5;
    int line_begy=175;
    int line_sy=Spr::font_sy;
    int separ_begx=5;
    int separ_begy=180;
    int cmd_linex=5;
    int cmd_liney=185;
    char *separ="--------";

    int x=line_begx;
    int y=line_begy;
    for (int i=lines-1;i>=0;i--)
    {
      display_string(screen,screenbufinfo[i].owners_id,x,y,screenlines[i]);
      y-=line_sy;
    }

    display_string(screen,0,separ_begx,separ_begy,separ);

    if (terminating<=0)
    {
      char *text=linebuf[NoNet::get_this_id()]+display_pos[NoNet::get_this_id()];
      display_string(screen,NoNet::get_this_id(),cmd_linex,cmd_liney,text);

      if (disp_cursor(NoNet::get_this_id()))
      {
        int x=cmd_linex+(cursor_pos[NoNet::get_this_id()]-display_pos[NoNet::get_this_id()])*Spr::font_sx;
        int y=cmd_liney;
        display_string(screen,0,x,y,"_");
      }
    }
  }

  Menu::display(screen);
}

void Chat::insert_new_line(int id,char *line)
{
  DBG_CHECK(id==0||NoNet::check_id(id));
  for (int i=0;i<lines-1;i++)
  {
    screenlines[i]=screenlines[i+1];
    screenbufinfo[i]=screenbufinfo[i+1];
  }
  strncpy(screenbuf[free_line],line,screenline_len);
  screenbuf[free_line][screenline_len-1]='\0';
  screenlines[lines-1]=screenbuf[free_line];
  screenbufinfo[lines-1].owners_id=id;
  free_line=(free_line+1)%lines;
}

void Chat::log_string(int id,char *format,...)
{
  DBG_CHECK(id==0||NoNet::check_id(id));
  char inbuffer[bufline_len+bufline_len];
  va_list argptr;
  va_start(argptr,format);
  vsprintf(inbuffer,format,argptr);
  va_end(argptr);
  char outbuffer[2*bufline_len];
  char namebuffer[name_len];
  char prefixbuffer[name_len+2];
  sprintf(namebuffer,"%s",(id==0)?"daemon":player_name(id));
  _strupr(namebuffer);
  sprintf(prefixbuffer,(id==NoNet::get_this_id())?"<%s>":"(%s)",namebuffer);

  int stop=0;
  char *ptr=inbuffer;
  do
  {
    DBG_CHECK(strlen(prefixbuffer)+strlen(ptr)+1<2*bufline_len);
    sprintf(outbuffer,"%s %s",prefixbuffer,ptr);
    if (strlen(outbuffer)>=screenline_len)
    {
      ptr+=screenline_len-1-(strlen(prefixbuffer)+1);
      int to_left=0;
      while (*(ptr-to_left)!=' '&&to_left<10)
      {
        to_left++;
      }
      if (*(ptr-to_left)==' ')
      {
        ptr-=(to_left-1);
        outbuffer[screenline_len-1-to_left]='\0';
      }
      while ((*ptr)==' ')
      {
        ptr++;
      }
      if ((*ptr)=='\0')
      {
        stop=1;
      }
    }
    else
    {
      stop=1;
    }
    insert_new_line(id,outbuffer);
  }
  while (!stop);
}

void Chat::update_actions(void)
{
  FOR_ALL_USERS(ID)
  {
    display_cursor[ID]=(display_cursor[ID]+1)%20;
    leftcount[ID]=(KbdStat::chat_left(ID))?leftcount[ID]+1:0;
    rightcount[ID]=(KbdStat::chat_right(ID))?rightcount[ID]+1:0;
    backcount[ID]=(KbdStat::chat_backspace(ID))?backcount[ID]+1:0;
    delcount[ID]=(KbdStat::chat_delete(ID))?delcount[ID]+1:0;
    leftwordcount[ID]=(KbdStat::chat_prevword(ID))?leftwordcount[ID]+1:0;
    rightwordcount[ID]=(KbdStat::chat_nextword(ID))?rightwordcount[ID]+1:0;
    if (KbdStat::chat_holded_key(ID))
    {
      if (KbdStat::chat_holded_key(ID)==lastchar[ID]&&!(*KbdStat::chat_buf(ID)))
      {
        lastcharcount[ID]++;
      }
      else
      {
        lastchar[ID]=KbdStat::chat_holded_key(ID);
        lastcharcount[ID]=1;
      }
    }
    else
    {
      lastcharcount[ID]=0;
    }
  }
}

int Chat::del_char(int id)
{
  DBG_CHECK(NoNet::check_id(id));
  return _is_action(delcount[id]);
}

int Chat::back_char(int id)
{
  DBG_CHECK(NoNet::check_id(id));
  return _is_action(backcount[id]);
}

int Chat::left_char(int id)
{
  DBG_CHECK(NoNet::check_id(id));
  return _is_action(leftcount[id]);
}

int Chat::right_char(int id)
{
  DBG_CHECK(NoNet::check_id(id));
  return _is_action(rightcount[id]);
}

int Chat::left_word(int id)
{
  DBG_CHECK(NoNet::check_id(id));
  return _is_action(leftwordcount[id]);
}

int Chat::right_word(int id)
{
  DBG_CHECK(NoNet::check_id(id));
  return _is_action(rightwordcount[id]);
}

char Chat::auto_char(int id)
{
  DBG_CHECK(NoNet::check_id(id));
  int value=lastcharcount[id];
  if ((value>20&&!(value%2)))
  {
    return lastchar[id];
  }
  else
  {
    return '\0';
  }
}

int Chat::_is_action(int value)
{
  return (value==1||(value>10&&!(value%2)));
}

int Chat::disp_cursor(int id)
{
  DBG_CHECK(NoNet::check_id(id));
  return (display_cursor[id]<10);
}

void Chat::serve_help(int id,char *param)
{
  DBG_CHECK(NoNet::check_id(id));
  if (id==NoNet::get_this_id())
  {
    log_string(0,"--------------------------------------------------------------------");
    if (param==NULL)
    {
      log_string(0,"Chat daemon v1.01 available commands:");
      log_string(0,"HELP QUIT QUERY USERS WHOAMI CLS");
      log_string(0,"All commands must be preceded with '.' character.");
      log_string(0,"For more information type .HELP <COMMAND>.");
    }
    else
    {
      if (!strcmpi(param,"HELP"))
      {
        log_string(0,"HELP command:");
        log_string(0,"Prints information about available commands.");
        log_string(0,"Usage: type .HELP");
      }
      else if (!strcmpi(param,"QUIT"))
      {
        log_string(0,"QUIT command:");
        log_string(0,"Terminates the current session.");
        log_string(0,"Usage: type .QUIT");
      }
      else if (!strcmpi(param,"CLS"))
      {
        log_string(0,"CLS command:");
        log_string(0,"Clears the console.");
        log_string(0,"Usage: type .CLS");
      }
      else if (!strcmpi(param,"QUERY"))
      {
        log_string(0,"QUERY command:");
        log_string(0,"Sends a message to the player.");
        log_string(0,"Usage: .QUERY <USER> <MESSAGE>");
        log_string(0,"where USER is the username or color of one of the players,");
        log_string(0,"MESSAGE is a string you want to send to him. This string is");
        log_string(0,"sent only to the specified player, so nobody else sees it.");
        log_string(0,"At recipient's console it will appear with '--->' string");
        log_string(0,"at the beginning of the message. You can obtain a list of active");
        log_string(0,"players using USERS command.");
        log_string(0,"See also: USERS.");
      }
      else if (!strcmpi(param,"USERS"))
      {
        log_string(0,"USERS command:");
        log_string(0,"Lists all players connected to the network game.");
        log_string(0,"Usage: type .USERS");
      }
      else if (!strcmpi(param,"WHOAMI"))
      {
        log_string(0,"WHOAMI command:");
        log_string(0,"Displays your name.");
        log_string(0,"Usage: type .WHOAMI");
      }
      else if (!strcmpi(param,"<COMMAND>")||!strcmpi(param,"(COMMAND)"))
      {
        log_string(0,"HELP error.");
        log_string(0,"You should have called help with name of command as a parameter.");
        log_string(0,"If you still have problems, try to copy following examples.");
        log_string(0,"help quit; help for QUIT command");
        log_string(0,"help whoami; help for WHOAMI command");
        log_string(0,"etc.");
      }
      else if (!strcmpi(param,"ME"))
      {
        log_string(0,"I'm sorry, but it seems that there is nothing I can help _YOU_");
        log_string(0,"with. Try to use HELP command with the command name as a parameter.");
      }
      else
      {
        log_string(0,"HELP error (invalid help topic).");
        log_string(0,"Available help topics:");
        log_string(0,"HELP QUIT QUERY USERS WHOAMI CLS");
      }
    }
  }
}

void Chat::serve_quit(int id,char *param)
{
  DBG_CHECK(NoNet::check_id(id));
  log_string(0,"%s requests to leave chat.",player_name(id));
  SManager::add_sample(&Mysound::speech_colours[id]);
  SManager::add_sample(&Mysound::speech_chatterm);
  terminating=60;
}

void Chat::serve_users(int id,char *param)
{
  DBG_CHECK(NoNet::check_id(id));
  if (id==NoNet::get_this_id())
  {
    log_string(0,"Current players:");
    FOR_ALL_USERS(ID)
    {
      log_string(0,"%s",player_name(ID));
    }
  }
}

void Chat::serve_cls(int id,char *param)
{
  DBG_CHECK(NoNet::check_id(id));
  if (id==NoNet::get_this_id())
  {
    free_line=0;
    for (int i=0;i<lines;i++)
    {
      screenbuf[i][0]='\0';
      screenlines[i]=screenbuf[i];
      screenbufinfo[i].owners_id=0;
    }
  }
}

void Chat::serve_query(int id,char *param)
{
  DBG_CHECK(NoNet::check_id(id));
  if (param!=NULL)
  {
    char *name=strtok(param," ");
    char *text=strtok(NULL,"");
    int user=0;
    user=(param!=NULL)?get_id(param):0;
    if (!user)
    {
      if (id==NoNet::get_this_id())
      {
        log_string(0,"QUERY error ('%s' is not a valid username).",name);
      }
    }
    else
    {
      if (text!=NULL)
      {
        if (id==NoNet::get_this_id())
        {
          log_string(0,"Secret message to %s sent.",player_name(user));
        }
        if (user==NoNet::get_this_id())
        {
          log_string(id,"---> %s",text);
          SManager::add_sample(&Mysound::speech_incomingmsg);
          SManager::add_sample(&Mysound::speech_colours[id]);
        }
      }
      else
      {
        if (id==NoNet::get_this_id())
        {
          log_string(0,"QUERY error (no message specified).",name);
        }
      }
    }
  }
  else
  {
    if (id==NoNet::get_this_id())
    {
      log_string(0,"QUERY error (no parameters).");
    }
  }
}

void Chat::serve_invalidcommand(int id,char *param)
{
  DBG_CHECK(NoNet::check_id(id));
  if (id==NoNet::get_this_id())
  {
    if (param!=NULL)
    {
      log_string(0,"Invalid command: '%s'.",param);
    }
    else
    {
      log_string(0,"No command specified.");
    }
  }
}

void Chat::serve_nocommand(int id,char *param)
{
  DBG_CHECK(NoNet::check_id(id));
  if (id==NoNet::get_this_id())
  {
    log_string(0,"No command specified.");
  }
}

void Chat::serve_whoami(int id,char *param)
{
  DBG_CHECK(NoNet::check_id(id));
  if (id==NoNet::get_this_id())
  {
    log_string(0,"You are %s.",player_name(id));
  }
}

void Chat::display_string(Screen& screen,int id,int x,int y,char *text)
{
  DBG_CHECK(strlen(text)<bufline_len);
  DBG_CHECK(id==0||NoNet::check_id(id));
  char buf[bufline_len];
  strncpy(buf,text,bufline_len);
  buf[bufline_len-1]='\0';
  if (Video::mode_flag==Spr::lores)
  {
    _strupr(buf);
  }
  screen.ink=Color::black;
  screen.print(x+1,y,buf);
  screen.ink=(id!=0)?(Tools::colors[Tools::NET_FIRST-1+id]):(Color::white);
  screen.print(x,y,buf);
}

void Chat::input_string(int id)
{
  DBG_CHECK(NoNet::check_id(id));
  if (!first_time)
  {
    if (left_char(id)&&cursor_pos[id]>0)
    {
      cursor_pos[id]--;
    }
    if (right_char(id)&&cursor_pos[id]<(int)strlen(linebuf[id]))
    {
      cursor_pos[id]++;
    }
    char before_cursor[bufline_len+256]={'\0'};
    char afterbuf[bufline_len+256]={'\0'};
    char *after_cursor=afterbuf;
    DBG_CHECK(cursor_pos[id]<=bufline_len);
    strncpy(before_cursor,linebuf[id],cursor_pos[id]);
    before_cursor[cursor_pos[id]]='\0';
    strncpy(after_cursor,linebuf[id]+cursor_pos[id],bufline_len);
    after_cursor[bufline_len-1]='\0';
    if (back_char(id)&&strlen(before_cursor)>0)
    {
      before_cursor[strlen(before_cursor)-1]='\0';
      cursor_pos[id]--;
    }
    if (del_char(id)&&strlen(after_cursor)>0)
    {
      after_cursor++;
    }

    int actual_length=strlen(before_cursor)+strlen(after_cursor);
    if (*KbdStat::chat_buf(id))
    {
      char buf[KbdStat::chat_buf_size];
      strncpy(buf,KbdStat::chat_buf(id),KbdStat::chat_buf_size);
      buf[KbdStat::chat_buf_size-1]='\0';
      if (actual_length+strlen(buf)>=bufline_len-1)
      {
        DBG_CHECK(bufline_len-1-actual_length>=0&&bufline_len-1-actual_length<KbdStat::chat_buf_size);
        if (bufline_len-1-actual_length<(int)strlen(buf))
        {
          if (id==NoNet::get_this_id())
          {
            Sound::st_play(&Mysound::buffer_overflow);
          }
        }
        buf[bufline_len-1-actual_length]='\0';
      }
      sprintf(linebuf[id],"%s%s%s",before_cursor,buf,after_cursor);
      cursor_pos[id]+=strlen(buf);
    }
    else
    {
      char new_char=auto_char(id);
      if (new_char!='\0')
      {
        if (actual_length<bufline_len-1)
        {
          sprintf(linebuf[id],"%s%c%s",before_cursor,new_char,after_cursor);
          cursor_pos[id]++;
        }
        else
        {
          if (id==NoNet::get_this_id())
          {
            Sound::st_play(&Mysound::buffer_overflow);
          }
        }
      }
      else
      {
        sprintf(linebuf[id],"%s%s",before_cursor,after_cursor);
      }
    }
    if (KbdStat::chat_beg(id))
    {
      cursor_pos[id]=0;
    }
    if (KbdStat::chat_end(id))
    {
      cursor_pos[id]=strlen(linebuf[id]);
    }
    if (left_word(id))
    {
      while (cursor_pos[id]>0&&linebuf[id][cursor_pos[id]]==' ')
      {
        cursor_pos[id]--;
      }
      while (cursor_pos[id]>0&&linebuf[id][cursor_pos[id]]!=' ')
      {
        cursor_pos[id]--;
      }
      if (cursor_pos[id]>0)
      {
        cursor_pos[id]++;
      }
    }
    if (right_word(id))
    {
      while (cursor_pos[id]<(int)strlen(linebuf[id])&&linebuf[id][cursor_pos[id]]!=' ')
      {
        cursor_pos[id]++;
      }
      while (cursor_pos[id]<(int)strlen(linebuf[id])&&linebuf[id][cursor_pos[id]]==' ')
      {
        cursor_pos[id]++;
      }
    }
  }
  DBG_CHECK(cursor_pos[id]>=0&&cursor_pos[id]<=(int)strlen(linebuf[id]));
}

void Chat::update_display_string(int id)
{
  DBG_CHECK(NoNet::check_id(id));
  DBG_CHECK(cursor_pos[id]<=(int)strlen(linebuf[id]));
  if (cursor_pos[id]-display_pos[id]>=screenline_len)
  {
    display_pos[id]=cursor_pos[id]-(screenline_len-1);
  }
  else if (cursor_pos[id]-display_pos[id]<0)
  {
    display_pos[id]=cursor_pos[id];
  }
  DBG_CHECK(display_pos[id]<=cursor_pos[id]);
}

void Chat::log_users_status(void)
{
  log_string(0,"Current players:");
  FOR_ALL_USERS(ID)
  {
    log_string(0,"%s",player_name(ID));
  }
}

void Chat::log_text_strings(char *label)
{
  int size=GameManager::game_text.size(label);
  for (int i=0;i<size;i++)
  {
    log_string(0,GAMETXT(label,i));
  }
}

char *Chat::player_name(int id)
{
  DBG_CHECK(NoNet::check_id(id));
  return namebuf[id];
}

int Chat::get_id(char *player_name)
{
  FOR_ALL_USERS(ID)
  {
    if (!strcmpi(Chat::player_name(ID),player_name))
    {
      return ID;
    }
  }
  int ret_id=0;
  if (!strcmpi("RED",player_name))
  {
    ret_id=1;
  }
  else if (!strcmpi("YELLOW",player_name))
  {
    ret_id=2;
  }
  else if (!strcmpi("GREEN",player_name))
  {
    ret_id=3;
  }
  else if (!strcmpi("BLUE",player_name))
  {
    ret_id=4;
  }
  return ((NoNet::check_id(ret_id))?ret_id:0);
}

//KLASA NETMSG
char NetMsg::buffer[NoNet::max_users+1][NetMsg::line_len];
int NetMsg::editing[NoNet::max_users+1];
int NetMsg::initialized=0;
int NetMsg::disp_cursor=0;

void NetMsg::init(void)
{
  if (NoNet::is_network_mode())
  {
    initialized=1;
    disp_cursor=0;
    memset((void*)buffer,0,sizeof(buffer));
    memset((void*)editing,0,sizeof(editing));
  }
}

void NetMsg::quit(void)
{
  if (NoNet::is_network_mode())
  {
    initialized=0;
  }
}

void NetMsg::run(void)
{
  if (NoNet::is_network_mode())
  {
    DBG_CHECK(initialized);
    if (++disp_cursor>20)
    {
      disp_cursor=0;
    }
    FOR_ALL_USERS(ID)
    {
      if (!KbdStat::using_menu(ID))
      {
        if (editing[ID])
        {
          if (KbdStat::netmsg_abort(ID))
          {
            editing[ID]=0;
            buffer[ID][0]='\0';
          }
          else if (KbdStat::chat_accept(ID))
          {
            if ((int)strlen(buffer[ID])>0)
            {
              char buf[1024];
              sprintf(buf,"%s: %s",NoNet::get_player_name(ID),buffer[ID]);
              Myship *s=NoNet::get_ship(world->get_current_displayed());
              s->play_ship_sample(Mysound::speech_incomingmsg,Myship::SPEECH);
              s->play_ship_sample(Mysound::speech_colours[ID],Myship::SPEECH);
              MSGTXT(buf,0,ID);
              editing[ID]=0;
              buffer[ID][0]='\0';
            }
            else
            {
              editing[ID]=0;
              buffer[ID][0]='\0';
            }
          }
          else
          {
            int space=line_len-1-strlen(buffer[ID]);
            if ((int)strlen(KbdStat::netmsg_buf(ID))>space)
            {
              if (ID==world->get_current_displayed())
              {
                Sound::st_play(&Mysound::buffer_overflow);
              }
            }
            strncat(buffer[ID],KbdStat::netmsg_buf(ID),space);
            if (KbdStat::netmsg_backspace(ID)&&(int)strlen(buffer[ID])>0)
            {
              buffer[ID][strlen(buffer[ID])-1]='\0';
            }
          }
        }
        else
        {
          if (KbdStat::netmsg_request(ID))
          {
            editing[ID]=1;
            buffer[ID][0]='\0';
          }
        }
      }
    }
  }
}

void NetMsg::draw(Screen& screen)
{
  if (NoNet::is_network_mode())
  {
    DBG_CHECK(initialized);
    int orgx=screen.ox;
    int orgy=screen.oy;
    if (editing[world->get_current_displayed()])
    {
      char buf[1024];
      strncpy(buf,buffer[NoNet::get_this_id()],256);
      if (!SysSet::get(SysSet::RESOLUTION))
      {
        _strupr(buf);
      }
      int posx=80;
      int posy=Mp::SY-5-Spr::font_sy;
      int sx=164;
      int sy=Op::DISPINFOINVSY;
      Tool& tool=Tools::tool[Tools::DISPINFO];
      screen.rectangle(tool,posx-2,posy-2,sx,sy);
      Print::print(screen,Print::TFONT,Print::_FLAST,posx+1,posy,(disp_cursor<=10)?"%s_":"%s",buf);
      Print::print(screen,Print::TFONT,Print::FINVOVR,posx,posy,(disp_cursor<=10)?"%s_":"%s",buf);
    }
    screen.origin(orgx,orgy);
  }
}

int NetMsg::in_use(int id)
{
  if (NoNet::is_network_mode()&&initialized)
  {
    if (id==0)
    {
      id=NoNet::get_this_id();
    }
    DBG_CHECK(NoNet::check_id(id));
    return editing[id];
  }
  else
  {
    return 0;
  }
}

//KLASA HEADER
int Header::timer=0;
int Header::phase=0;
int Header::totalsy=0;
int Header::linespaceing=0;
int Header::offset=0;

float Header::bx;
float Header::by;
float Header::vx;
float Header::vy;
int   Header::level;
float Header::mx;
float Header::my;
int   Header::time;
int   Header::snd;
int   Header::press;
int   Header::disp;
int   Header::first(1);
unsigned int   Header::gtime;

int Header::header_run(void)
{
  if (!disp)  return 1;
  if(time==0&&level==0)   Sound::st_play(&Mysound::logo[0]);
  time+=KbdStat::time();
  if (time>1000&&level<3)
  {
    if (!snd)
    {
      Sound::st_play(&Mysound::titlemove);
      snd=1;
    }
    mx+=((float)0.1*(float)mx);
    my+=((float)0.1*(float)my);
    bx+=mx;
    by+=my;
  }


  switch(level)
  {
   case 0:  if (bx+Mysprites::backscreen[0].r<=Mp::SX)
             {
               time=0;
               mx=(float)0;
               my=(float)-0.5;
               level++;
               snd=0;
               bx=(float)(Mp::SX-Mysprites::backscreen[0].r);
               Sound::st_play(&Mysound::logo[1]);
             }
             break;
    case 1:  if (by+Mysprites::backscreen[0].d<=Mp::SY)
             {
               time=0;
               mx=(float)0.5;
               my=(float)0;
               level++;
               Sound::st_play(&Mysound::logo[2]);
               by=(float)(Mp::SY-Mysprites::backscreen[0].d);
               snd=0;
             }
             break;
    case 2:
    case 3:  if (bx>=0)
             {
               mx=(float)0;
               my=(float)0;
               bx=(float)0;
               by=(float)(Mp::SY-Mysprites::backscreen[0].d);
               if (level==2)
               {
                 Sound::st_play(&Mysound::logo[3]);
                 level++;
                 time=0;
               }
               press=1;
       //        if (timeGetTime()-gtime>54000)
               if (timeGetTime()-gtime>20000)
               {
                 first=0;
                 return 0;
               }
             }
             break;

  }
  if (KbdStat::header_anykey()&&((level>=3||SysSet::get(SysSet::CHEATING))||!first))
  {
    first=0;
    return 0;
  }
  return 1;
}

void Header::header_draw(Screen& screen)
{
  int orgx=screen.ox;
  int orgy=screen.oy;
  screen.put((int)bx,(int)by,Mysprites::backscreen);
  screen.put((int)(bx*vx),(int)(by*vy),Mysprites::titlescreen);
  /*
  if (press)
  {
    if ((time%Mp::METRONQUALITY)>Mp::METRONQUALITY/2)
    {
      int dx=(Mp::SX-Mysprites::press[0].r)/2;
      //screen.put(dx,195,Mysprites::press);
    }
  }
  else
  {
    int dx=(Mp::SX-Mysprites::copy[0].r)/2;
    screen.put(dx,195,Mysprites::copy);
  }
  */
  int dx=(Mp::SX-Mysprites::copy[0].r)/2;
  screen.put(dx,195,Mysprites::copy);
  screen.origin(orgx,orgy);
}

int Header::header(void)
{
  DBG_CHECK(!NoNet::is_network_mode());
  Mysprites::load_title();
  Display::clear_vga(2);
  timer=Op::HEADERFRAMETIME;
  gtime=timeGetTime();

  int stop=0;
  snd=0;
  press=0;
  disp=0;
  mx=(float)-1;
  my=(float)0;
  time=level=0;
  by=bx=(float)0;
  vx=(float)(Mysprites::titlescreen[0].r-Mp::SX)/(float)(Mysprites::backscreen[0].r-Mp::SX);
  vy=(float)(Mysprites::titlescreen[0].d-Mp::SY)/(float)(Mysprites::backscreen[0].d-Mp::SY);

  //Game::play_CD(GameManager::level_descript.value("headersong"),0);
  gtime=timeGetTime();
  SManager::unlock();
  KbdStat::init();
  while (!stop)
  {
    while (KbdStat::run())
    {
      if (!header_run())
      {
        stop=1;
        break;
      }
    }
    if (!stop)
    {
      if (!disp) time=0;
      disp=1;
      header_draw(Display::screen());
      Display::copy2vga();
    }
  }

  Display::clear_vga(9);
  Mysprites::free_title();

  if (KbdStat::header_anykey())
  {
    //Game::stop_CD();
    KbdStat::done();
    return 0;
  }
  else
  {
    //Game::stop_CD();
    KbdStat::done();
    Game::set_demo_properties(Game::PLAY_RANDOM,NULL,0);
    return 1;
  }


}

int Header::footer_run(void)
{
  timer+=KbdStat::time();
  offset=(int)((timer*Op::HEADERTEXTSPEED)/Mp::METRONQUALITY);
  if (offset>totalsy+Mp::SY)
  {
    timer=0;
  }
  return (!KbdStat::header_anykey());
}

void Header::footer_draw(Screen& screen)
{
  int orgx=screen.ox;
  int orgy=screen.oy;
  Mysprites::put_pattern(screen);
  Text &text=GameManager::game_text;
  int labels=text.size("finishtext");
  int posy=Mp::SY-offset;
  for (int i=0;i<labels;i++)
  {
    char *str=text.string("finishtext",i);
    int xsize=Print::get_dx(Print::TSPRITE,str);
    int ysize=Print::get_dy(Print::TSPRITE,str);
    int posx=Mp::SX/2-xsize/2;
    if (posy+ysize>0&&posy<Mp::SY)
    {
      Print::print(screen,Print::TSPRITE,Print::FNORMAL,posx,posy,str);
    }
    posy+=ysize+linespaceing;
  }
  screen.origin(orgx,orgy);
}

void Header::footer(void)
{
  DBG_CHECK(!NoNet::is_network_mode());
  Display::clear_vga(9);
#ifndef SHAREWARE
  Game::play_CD(GameManager::level_descript.value("footersong"));
#else
  Song song;
  if (RegData::UseCD) song.play(Comm::get_file_path(GameManager::level_descript.string("sw_footersong")));
#endif
  timer=0;
  phase=0;
  totalsy=0;
  linespaceing=2;
  offset=0;
  Text &text=GameManager::game_text;
  int labels=text.size("finishtext");
  for (int i=0;i<labels;i++)
  {
    char *str=text.string("finishtext",i);
    int ysize=Print::get_dy(Print::TSPRITE,str);
    totalsy+=ysize+linespaceing;
  }

  int stop=0;
  while (!stop)
  {
    while (KbdStat::run())
    {
      if (!footer_run())
      {
        stop=1;
        break;
      }
    }
    if (!stop)
    {
      footer_draw(Display::screen());
      Display::copy2vga();
    }
  }
  Display::clear_vga(9);
#ifdef SHAREWARE
  if (RegData::UseCD) song.stop();
#endif
}

void Header::ordering(void)
{
  DBG_CHECK(!NoNet::is_network_mode());
  Menu::register_menu();
  while (Menu::in_use())
  {
    while (KbdStat::run())
    {
      Menu::serve();
    }
    if (Menu::in_use())
    {
      Menu::display(Display::screen());
      Display::copy2vga();
    }
  }
  Display::clear_vga(9);
}




