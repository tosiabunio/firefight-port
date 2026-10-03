#include "headers.h"

int KbdStat::initialized=0;
int KbdStat::demo_started=0;
KbdStat::Pack KbdStat::pack[NoNet::max_users+1];
int KbdStat::demo_mode=0;
int KbdStat::abort_demo=0;
int KbdStat::cnt=0;
int KbdStat::display=0;
int KbdStat::time_ticks=0;
int KbdStat::frame_numb=0;
int KbdStat::local_frame_numb=0;
int KbdStat::ignore_my=0;
NoQueue<KbdStat::Data> *KbdStat::queue=NULL;
char *KbdStat::letters="abcdefghijklmnopqrstuvwxyz0123456789 \\\'.,+-*/=[];";

void KbdStat::debug_init(void)
{
  frame_numb=0;
  local_frame_numb=0;
  if (Mp::NETDEBUGLEVEL>0)
  {
    queue=NEW(NoQueue<KbdStat::Data>(Mp::NETDEBUGLEVEL,"net debug data",1),queue_mbn);
    DBG_CHECK(queue!=NULL);
    Data d;
    memset(d.pack,0,sizeof(d.pack));
    d.frame=0;
    for (int i=0;i<Mp::NETDEBUGLEVEL;i++)
    {
      queue->add(&d);
    }
  }
}

void KbdStat::debug_done(void)
{
  if (queue!=NULL)
  {
    DBG_CHECK(Mp::NETDEBUGLEVEL);
    delete queue;
    queue=NULL;
  }
}

void KbdStat::dump(void)
{
  if (queue!=NULL)
  {
    DBG_CHECK(Mp::NETDEBUGLEVEL>0);
    MESSAGE("NET DEBUGINFO DUMP:");
    while (!queue->empty())
    {
      Data* pck=queue->head();
      MESSAGE("frame %d:",pck->frame);
      for(int j=1;j<NoNet::max_users+1;j++)
      {
        unsigned int csum=0;
        char str[100];
        str[0]=0;
        REPEAT(sizeof(Pack))
        {
          char tmp[256];
          unsigned char data=((unsigned char*)(&(pck->pack[j])))[RPTCNT];
          sprintf(tmp,"%02x",data);
          strcat(str,tmp);
          csum+=data;
        }
        MESSAGE("player %d: %s (csum %x)",j,str,csum);
      }
    }
    MESSAGE("Total number of frames made: %d",frame_numb);
  }
}

int KbdStat::demo_init(DemoMode mode,char *filename,char *data,unsigned *size)
{
  DBG_CHECK(!demo_started);
  DBG_CHECK(mode==PLAY_DEMO||mode==SAVE_DEMO);
  demo_mode=mode;
  switch (demo_mode)
  {
    case PLAY_DEMO: DBG_CHECK(!demo_started);
                    if (Eem::demo_set_play_mode(filename,data,size))
                    {
                      demo_started=PLAY_DEMO; 
                    }
                    else
                    {
                      demo_started=0; 
                      demo_mode=0;
                    }
                    break;
    case SAVE_DEMO: DBG_CHECK(!demo_started);
                    Eem::demo_set_record_mode(filename,max_demo_size,data,*size);
                    demo_started=SAVE_DEMO; 
                    break;
           default: DBG_CHECK(0);
                    break;
  }
  return demo_started;
}

int KbdStat::is_demo(void)
{
  DBG_CHECK(demo_mode==0||demo_mode==PLAY_DEMO||demo_mode==SAVE_DEMO);
  return (demo_mode);
}

void KbdStat::clear_demo_mode(void)
{
  demo_mode=0;
  demo_started=0;
}

void KbdStat::init(void)
{
  DBG_CHECK(!initialized);
  initialized=1;
  display=0;
  time_ticks=Mp::METRONQUALITY/Eem::get_frequency();
  cnt=0;
  local_frame_numb=0;
  ignore_my=0;
  abort_demo=0;
  for (int i=0;i<NoNet::max_users+1;i++)
  {
    pack[i].CHAT_BUF[0]='\0';
  }
  Eem::start();
  DBG_MESSAGE("KbdStat initialized");
}

void KbdStat::done(void)
{
  if (initialized)
  {
    Eem::stop();
    if (demo_started)
    {
      DBG_CHECK(demo_mode);
      Eem::demo_terminate();
    }
  }
  demo_started=0;
  demo_mode=0;
  local_frame_numb=0;
  initialized=0;
  DBG_MESSAGE("KbdStat deinitialized");
}

void KbdStat::fill_keys(void)
{
  DBG_CHECK(initialized);
  memset(pack,0,sizeof(pack));
  if (demo_mode==0)
  {
    FOR_ALL_USERS(ID)
    {
      fill_keys(ID);
    }
  }
  else 
  {
    DBG_CHECK(!NoNet::is_network_mode());
    fill_keys(NoNet::get_this_id());
    demo_update_keys(2);
  }
  if (Mp::NETDEBUGLEVEL>0)
  {
    DBG_CHECK(queue!=NULL);
    Data d;
    d.frame=frame_numb;
    memcpy((void*)d.pack,(void*)pack,sizeof(d.pack));
    queue->head();
    queue->add(&d);
  }

}

unsigned char KbdStat::make_user_flags(void)
{
  unsigned char value=0;
  DBG_CHECK(SysSet::get(SysSet::INPUT_DEVICE)>=0&&SysSet::get(SysSet::INPUT_DEVICE)<3);
  value|=SysSet::get(SysSet::INPUT_DEVICE);
  if (SysSet::get(SysSet::AUTORUN))
  {
    value|=0x04;
  }
  if (ignore_my)
  {
    value|=0x08;
  }
  if (Menu::in_use())
  {
    value|=0x10;
  }
  if (NetMsg::in_use())
  {
    value|=0x20;
  }
  //if (slowing_down)
  {
    value|=0x40;
  }
  return value;
}

void KbdStat::fill_keys(int id)
{
  DBG_CHECK(initialized);
  check_user(id);

  Eem::select(id);

  unsigned char user_flags=Eem::get_user_flags();
  pack[id].USER_FLAGS=user_flags;
  pack[id].STEERING_MODE=(user_flags&0x03);
  pack[id].TURBO_LOCKED=(user_flags&0x04)?1:0;
  int ignore_ship_keys=(user_flags&0x08)?1:0;
  pack[id].USING_MENU=(user_flags&0x10)?1:0;
  pack[id].USING_NETMSG=(user_flags&0x20)?1:0;
  pack[id].SLOWING_DOWN=(user_flags&0x40)?1:0;

  if (!ignore_ship_keys&&!pack[id].USING_MENU&&!pack[id].USING_NETMSG)
  {
    int MoveLeft    =RegData::Controls.VKeys[RegData::ctl_MoveLeft];
    int MoveRight   =RegData::Controls.VKeys[RegData::ctl_MoveRight];
    int MoveForward =RegData::Controls.VKeys[RegData::ctl_MoveForward];
    int MoveBack    =RegData::Controls.VKeys[RegData::ctl_MoveBack];
    int Fire1       =RegData::Controls.VKeys[RegData::ctl_Fire1];
    int Fire2       =RegData::Controls.VKeys[RegData::ctl_Fire2];
    int StrafeLeft  =RegData::Controls.VKeys[RegData::ctl_StrafeLeft];
    int StrafeRight =RegData::Controls.VKeys[RegData::ctl_StrafeRight];
    int StrafeShift =RegData::Controls.VKeys[RegData::ctl_StrafeShift];
    int PrevInvent  =RegData::Controls.VKeys[RegData::ctl_PrevInvent];
    int NextInvent  =RegData::Controls.VKeys[RegData::ctl_NextInvent];
    int UseInvent   =RegData::Controls.VKeys[RegData::ctl_UseInvent];
    int TurboShift  =RegData::Controls.VKeys[RegData::ctl_TurboShift];
    int TurboLock   =RegData::Controls.VKeys[RegData::ctl_TurboLock];
    int GotoMouse   =RegData::Controls.VKeys[RegData::ctl_GotoMouse];
    int Weapon1     =RegData::Controls.VKeys[RegData::ctl_Weapon1];
    int Weapon2     =RegData::Controls.VKeys[RegData::ctl_Weapon2];
    int Weapon3     =RegData::Controls.VKeys[RegData::ctl_Weapon3];
    int Weapon4     =RegData::Controls.VKeys[RegData::ctl_Weapon4];
    int Weapon5     =RegData::Controls.VKeys[RegData::ctl_Weapon5];
    int Weapon6     =RegData::Controls.VKeys[RegData::ctl_Weapon6];
    int NextWeapon  =RegData::Controls.VKeys[RegData::ctl_NextWeapon];
    int PrevWeapon  =RegData::Controls.VKeys[RegData::ctl_PrevWeapon];
    if (pack[id].STEERING_MODE==1)
    {
      int mx=0,my=0;
      Mouse::get_xy(&mx,&my);
      pack[id].MOUSE_X=mx;
      pack[id].MOUSE_Y=my;
      pack[id].MOUSE_GOTO=Vkey::is(GotoMouse);
      pack[id].MOUSE_READ=1;
    }
    pack[id].LEFT=Vkey::is(MoveLeft)&&!Vkey::is(StrafeShift);
    pack[id].RIGHT=Vkey::is(MoveRight)&&!Vkey::is(StrafeShift);
    pack[id].STRAFE_LEFT=(Vkey::is(MoveLeft)&&Vkey::is(StrafeShift))||(Vkey::is(StrafeLeft)&&!Vkey::is(StrafeShift));
    pack[id].STRAFE_RIGHT=(Vkey::is(MoveRight)&&Vkey::is(StrafeShift))||(Vkey::is(StrafeRight)&&!Vkey::is(StrafeShift));
    pack[id].FORWARD=Vkey::is(MoveForward);
    pack[id].BACKWARD=Vkey::is(MoveBack);
    pack[id].FIRE=Vkey::is(Fire1)||Vkey::is(Fire2);
    pack[id].TURBO=Vkey::is(TurboShift);
    pack[id].AUTO_RUN=Vkey::struck(TurboLock);
    pack[id].WEAP1=Vkey::struck(Weapon1);
    pack[id].WEAP2=Vkey::struck(Weapon2);
    pack[id].WEAP3=Vkey::struck(Weapon3);
    pack[id].WEAP4=Vkey::struck(Weapon4);
    pack[id].WEAP5=Vkey::struck(Weapon5);
    pack[id].WEAP6=Vkey::struck(Weapon6);
    pack[id].NEXT_WEAP=Vkey::struck(NextWeapon);
    pack[id].PREV_WEAP=Vkey::struck(PrevWeapon);
    pack[id].NEXT_ITEM=Vkey::struck(NextInvent);
    pack[id].PREV_ITEM=Vkey::struck(PrevInvent);
    pack[id].USE_ITEM=Vkey::struck(UseInvent);
    pack[id].START=(pack[id].LEFT||
                    pack[id].RIGHT||
                    pack[id].FORWARD||
                    pack[id].BACKWARD||
                    pack[id].STRAFE_LEFT||
                    pack[id].STRAFE_RIGHT||
                    pack[id].FIRE||
                   (pack[id].STEERING_MODE==1&&pack[id].MOUSE_GOTO));
    pack[id].ABORT_BEAMING=(pack[id].FORWARD||
                            pack[id].BACKWARD||
                            pack[id].STRAFE_LEFT||
                            pack[id].STRAFE_RIGHT||
                           (pack[id].STEERING_MODE==1&&pack[id].MOUSE_GOTO));
  }

  pack[id].MENU_ON=Kbd::struck(Kbd::esc);
  pack[id].MENU_SECRET_ON=Kbd::struck(Kbd::f12)&&!Kbd::is(Kbd::alt)&&!Kbd::is(Kbd::ctrl);
  pack[id].MENU_UP=Kbd::struck(Kbd::up)||Joy::struck(Joy::up);
  pack[id].MENU_DOWN=Kbd::struck(Kbd::down)||Joy::struck(Joy::down);
  pack[id].MENU_PAGE_UP=Kbd::struck(Kbd::pageup);
  pack[id].MENU_PAGE_DOWN=Kbd::struck(Kbd::pagedn);
  pack[id].MENU_LEFT=Kbd::is(Kbd::left)||Joy::is(Joy::left);
  pack[id].MENU_RIGHT=Kbd::is(Kbd::right)||Joy::is(Joy::right);
  pack[id].MENU_LEFT_STRUCK=Kbd::struck(Kbd::left)||Joy::struck(Joy::left);
  pack[id].MENU_RIGHT_STRUCK=Kbd::struck(Kbd::right)||Joy::struck(Joy::right);
  pack[id].MENU_QUIT=Kbd::struck(Kbd::esc);
  pack[id].MENU_ACCEPT=Kbd::struck(Kbd::enter)||Kbd::struck(Kbd::spacebar)||Joy::struck(Joy::button1)||Joy::struck(Joy::button2);
  pack[id].MENU_SET_OPTION=Kbd::struck(Kbd::enter)||Kbd::struck(Kbd::spacebar)||Joy::struck(Joy::button1)||Joy::struck(Joy::button2);
  pack[id].MENU_ANYKEY=(Kbd::struck(Kbd::anykey)&&!Kbd::struck(Kbd::alt))||Joy::struck(Joy::button1)||Joy::struck(Joy::button2);

  pack[id].PAUSE=Kbd::struck(Kbd::pause);
  pack[id].HELP=Kbd::struck(Kbd::f1);
  pack[id].LAST_OBJECTIVE=Kbd::struck(Kbd::f2);
  pack[id].HIRES=Kbd::struck(Kbd::f5);
  pack[id].DETAIL_LEVEL=Kbd::struck(Kbd::f6);
  pack[id].GAMMA_MINUS=Kbd::is(Kbd::f7);
  pack[id].GAMMA_PLUS=Kbd::is(Kbd::f8);
  pack[id].RESTART=Kbd::struck(Kbd::f9);
  pack[id].ABORT=Kbd::struck(Kbd::f10);
  pack[id].CONTROLS=Kbd::struck(Kbd::f11);
  pack[id].QUIT=Kbd::struck(Kbd::keyx)&&Kbd::is(Kbd::alt);
  pack[id].DISPLAY_FPS=Kbd::is(Kbd::alt)&&Kbd::is(Kbd::ctrl)&&Kbd::struck(Kbd::keyf);
  pack[id].CHEATING=Kbd::is(Kbd::keyc)&&Kbd::is(Kbd::keyw)&&Kbd::struck(Kbd::plus);
  pack[id].CAPTSCREEN=Kbd::struck(Kbd::f12)&&Kbd::is(Kbd::alt);
  pack[id].CAPTSCREEN_NOINFO=Kbd::struck(Kbd::f12)&&Kbd::is(Kbd::ctrl);
  pack[id].HEADER_ANYKEY=Kbd::struck(Kbd::anykey);
  if (Mp::DEBUGKEYS)
  {
    pack[id].TEST_OBJECTIVE=Kbd::is(Kbd::alt)&&Kbd::is(Kbd::ctrl)&&Kbd::struck(Kbd::keyt);
    pack[id].COLISWORLD=Kbd::is(Kbd::alt)&&Kbd::is(Kbd::ctrl)&&Kbd::struck(Kbd::keyc);
    pack[id].NEXT_VIEW=Kbd::is(Kbd::alt)&&Kbd::is(Kbd::ctrl)&&Kbd::struck(Kbd::keyn);
    pack[id].MAKE_FAILURE=Kbd::is(Kbd::alt)&&Kbd::is(Kbd::ctrl)&&Kbd::struck(Kbd::keye);
    pack[id].POINTS=Kbd::is(Kbd::alt)&&Kbd::is(Kbd::ctrl)&&Kbd::struck(Kbd::keyo);
    pack[id].NO_WALL=Kbd::is(Kbd::alt)&&Kbd::is(Kbd::ctrl)&&Kbd::struck(Kbd::keyw);
//    pack[id].NET_CHEATING=Kbd::is(Kbd::alt)&&Kbd::is(Kbd::ctrl)&&Kbd::struck(Kbd::keyz);
  }

  pack[id].NET_HEAD_ACCEPT=Kbd::struck(Kbd::enter)||Kbd::struck(Kbd::spacebar)||Joy::struck(Joy::button1)||Joy::struck(Joy::button2);
  pack[id].NET_HEAD_REJECT=Kbd::struck(Kbd::esc)||Kbd::struck(Kbd::right);
  pack[id].NET_HEAD_NEXTLEV=Kbd::struck(Kbd::up)||Joy::struck(Joy::up);
  pack[id].NET_HEAD_PREVLEV=Kbd::struck(Kbd::down)||Joy::struck(Joy::down);

  pack[id].NETMSG_REQUEST=Kbd::struck(Kbd::f4);
  pack[id].NETMSG_ABORT=Kbd::struck(Kbd::f4)||Kbd::struck(Kbd::esc);
  pack[id].NETMSG_BACKSPACE=Kbd::struck(Kbd::backspace);
  pack[id].CHAT_REQUEST=Kbd::struck(Kbd::f3);
  pack[id].CHAT_ACCEPT=Kbd::struck(Kbd::enter);
  pack[id].CHAT_ABORT=Kbd::struck(Kbd::esc);
  pack[id].CHAT_PREVWORD=Kbd::is(Kbd::ctrl)&&Kbd::is(Kbd::left);
  pack[id].CHAT_NEXTWORD=Kbd::is(Kbd::ctrl)&&Kbd::is(Kbd::right);
  pack[id].CHAT_BACKSPACE=Kbd::is(Kbd::backspace);
  pack[id].CHAT_DELETE=Kbd::is(Kbd::del);
  pack[id].CHAT_LEFT=Kbd::is(Kbd::left);
  pack[id].CHAT_RIGHT=Kbd::is(Kbd::right);
  pack[id].CHAT_BEG=Kbd::struck(Kbd::home);
  pack[id].CHAT_END=Kbd::struck(Kbd::end);
  pack[id].CHAT_PREV_COMMAND=Kbd::struck(Kbd::up);
  pack[id].CHAT_HOLDED_KEY='\0';
  for (char *ptr=letters;(*ptr);ptr++)
  {
    if (Kbd::is(*ptr))
    {
      pack[id].CHAT_HOLDED_KEY=(*ptr);
      break;
    }
  }
  strncpy(pack[id].CHAT_BUF,Kbd::get_string(),chat_buf_size);
  pack[id].CHAT_BUF[chat_buf_size-1]='\0';

  //plomba dla Bolca
  if (pack[id].PAUSE&&pack[id].CHAT_REQUEST&&pack[id].NETMSG_REQUEST)
  {
    memset((void*)&pack[id],0,sizeof(pack[id]));
  }
}

void KbdStat::demo_update_keys(int id)
{
  DBG_CHECK(demo_mode==PLAY_DEMO||demo_mode==SAVE_DEMO);
  int this_id=NoNet::get_this_id();
  Eem::select(id);

  if (demo_mode==PLAY_DEMO)
  {
    pack[this_id].HIRES=Kbd::struck(Kbd::f5);
    pack[this_id].DISPLAY_FPS=Kbd::is(Kbd::alt)&&Kbd::is(Kbd::ctrl)&&Kbd::struck(Kbd::keyf);
    pack[this_id].DETAIL_LEVEL=Kbd::struck(Kbd::f6);
    pack[this_id].CAPTSCREEN=Kbd::struck(Kbd::f12)&&Kbd::is(Kbd::alt);
    pack[this_id].CAPTSCREEN_NOINFO=Kbd::struck(Kbd::f12)&&Kbd::is(Kbd::ctrl);
    pack[this_id].GAMMA_MINUS=Kbd::is(Kbd::f7);
    pack[this_id].GAMMA_PLUS=Kbd::is(Kbd::f8);
    pack[this_id].CONTROLS=0;
    abort_demo=Kbd::struck(Kbd::esc)||Kbd::struck(Kbd::enter)||Kbd::struck(Kbd::spacebar);
  }
  else
  {
    Eem::select(this_id);
    abort_demo=Kbd::is(Kbd::alt)&&Kbd::is(Kbd::ctrl)&&Kbd::struck(Kbd::enter);
  }
}

void KbdStat::demo_check(void)
{
  if (demo_mode)
  {
    if (!Eem::demo_is_active()||abort_demo)
    {
      abort_demo=0;
      throw TerminateMission();
    }
  }
}

int KbdStat::run(unsigned sync)
{
  DBG_CHECK(initialized);

  for(;;)
  {
    if (Eem::read(sync,make_user_flags()))
    {
      demo_check();
      NoNet::refresh(1);
      cnt+=time_ticks;
      frame_numb++;
      local_frame_numb++;
      fill_keys();
      display=1;
      return 1;
    }
    else
    {
      if (display)
      {
        display=0;
        return 0;
      }
    }
  }
}

