#include "headers.h"

//KLASA DISPINFO
DispInfo::DispInfo(User *_myuser,int _id):
Visible(View::INFO)
{
  id=_id;
  myuser=_myuser;
  reset();
}

void DispInfo::reset(void)
{
  disp_flicker=0;
}

void DispInfo::run(void)
{
  if (disp_flicker++>=15)
  {
    disp_flicker=0;
  }
}

int DispInfo::color_indx(int max,int curr)
{
  DBG_CHECK(max>=curr);
  return (curr*Op::DISPINFOCOLORS/(max+1));
}

void DispInfo::print_number(Screen& screen,Sprite& spr,int x,int y,int number,int _id)
{
  if(number<0)
  {
    number=0;
  }
  char buff[256];
  char *temp=buff;
  sprintf(temp,"%d",number);
  while (*temp!='\0')
  {
    if (_id==-1)
    {
      screen.shape(x-spr[*temp-'0'].l+1,y,spr,*temp-'0',Spr::nomirror,(void*)&Color::black);
      screen.put(x-spr[*temp-'0'].l,y,spr,*temp-'0');
    }
    else
    {
      screen.shape(x-spr[*temp-'0'].l+1,y,spr,*temp-'0',Spr::nomirror,(void*)&Color::black);
      screen.shape(x-spr[*temp-'0'].l,y,spr,*temp-'0',Spr::nomirror,(void*)&Tools::colors[Tools::NET_FIRST-1+_id]);
    }
    x-=spr[*temp-'0'].l+1;
    temp++;
  }
}

int DispInfo::number_sx(Sprite& spr,int number)
{
  if(number<0)
  {
    number=0;
  }
  int xsize=0;
  char buff[256];
  char *temp=buff;
  sprintf(temp,"%d",number);
  while (*temp!='\0')
  {
    xsize-=spr[*temp-'0'].l+1;
    temp++;
  }
  return xsize;
}

void DispInfo::draw_object(Screen& screen,int plane)
{
  (void)plane;
  int scr_orgx=screen.ox;
  int scr_orgy=screen.oy;
  screen.origin(-Op::DISPINFOORGX,-Op::DISPINFOORGY);

  {
    int totalx=Op::DISPINFOLIFEX;
    int totaly=Op::DISPINFOLIFEY;
    int totalmarginx=Op::DISPINFOMARGINX;
    int totalmarginy=Op::DISPINFOMARGINY;
    int rectsx=Op::DISPINFOLIFERECTSX;
    int rectsy=Op::DISPINFOLIFERECTSY;
    int rectspacex=Op::DISPINFOLIFERECTSPACEX;
    int totalsize=2*Op::DISPINFOCOLORS;
    int totalsx=2*totalmarginx+totalsize*rectsx+(totalsize-1)*rectspacex;
    int totalsy=2*totalmarginy+rectsy;
    int size=(myuser->get_life()*totalsize)/(myuser->get_life_max()+1);
    int posx=totalx+totalmarginx;
    int posy=totaly+totalmarginy;
    Tool& tool=Tools::tool[Tools::DISPINFO];
    screen.rectangle(tool,totalx,totaly,totalsx,totalsy);
    if (myuser->get_life()>0)
    {
      for (int i=0;i<=size;i++,posx+=rectsx+rectspacex)
      {
        int color=(i<Op::DISPINFOCOLORS)?(Op::DISPINFOBEGCOLOR+i):(Op::DISPINFOBEGCOLOR+Op::DISPINFOCOLORS-1);
        screen.rectangle(color,posx,posy,rectsx,rectsy);
      }
    }
  }

  {
    int weapx=Op::DISPINFOWEAPX;
    int weapy=Op::DISPINFOWEAPY;
    int weapsx=Op::DISPINFOWEAPSX;
    int weapsy=Op::DISPINFOWEAPSY;
    int weapinx=Op::DISPINFOWEAPINRECTX;
    int weapiny=Op::DISPINFOWEAPINRECTY;
    int weapspacex=Op::DISPINFOWEAPSPACEX;
    for (int i=0;i<6;i++,weapx+=weapsx+weapspacex)
    {
      int weapamount=myuser->get_weap_ammo(i);
      int weapmax=myuser->get_weap_ammo_max(i);
      if (myuser->get_weap_in_use()==i)
      {
        int color=Op::DISPINFOBEGCOLOR+color_indx(weapmax,weapamount);
        screen.rectangle(color,weapx-1,weapy-1,1,weapsy+2);
        screen.rectangle(color,weapx,weapy-1,weapsx,1);
        screen.rectangle(color,weapx+weapsx,weapy-1,1,weapsy+2);
        screen.rectangle(color,weapx,weapy+weapsy,weapsx,1);
      }
      int numbsize=number_sx(Mysprites::disp_figuressm,weapamount);
      Tool& tool=Tools::tool[Tools::DISPINFO];
      screen.rectangle(tool,weapx,weapy,weapsx,weapsy);
      print_number(screen,Mysprites::disp_figuressm,weapx+weapinx-numbsize/2,weapy+weapiny,weapamount);
    }
  }

  if (NoNet::is_network_mode())
  {
    int statx=Op::DISPINFOWEAPX;
    int staty=Mp::SY-Op::DISPINFOWEAPSY-Op::DISPINFOWEAPY;
    int statsx=Op::DISPINFOWEAPSX;
    int statsy=Op::DISPINFOWEAPSY;
    int statinx=Op::DISPINFOWEAPINRECTX;
    int statiny=Op::DISPINFOWEAPINRECTY;
    int statspacex=Op::DISPINFOWEAPSPACEX;
    Tool& tool=Tools::tool[Tools::DISPINFO];
    FOR_ALL_USERS(ID)
    {
      int value=Statistics::get(ID);
      int color=Tools::colors[Tools::NET_FIRST-1+ID];
      screen.rectangle(tool,statx,staty,statsx,statsy);
      if (ID==world->get_current_displayed())
      {
        screen.rectangle(color,statx-1,staty-1,1,statsy+2);
        screen.rectangle(color,statx,staty-1,statsx,1);
        screen.rectangle(color,statx+statsx,staty-1,1,statsy+2);
        screen.rectangle(color,statx,staty+statsy,statsx,1);
      }
      Myship *ship=NoNet::get_ship(ID);
      int disp_now=!NoNet::is_network_mode(NoNet::NETWORKFLAG)||(ship!=NULL)&&(disp_flicker<=7||!Statistics::get_land(ID));
      if (disp_now)
      {
        int numbsize=number_sx(Mysprites::disp_figuressm,value);
        print_number(screen,Mysprites::disp_figuressm,statx+statinx-numbsize/2,staty+statiny,value,ID);
      }
      statx+=statsx+statspacex;
    }

    if (NoNet::is_network_mode(NoNet::NETWORKMATCH)&&NoNet::end_condition()==NoNet::TIMEUP)
    {
       int timesx=39;
       int timesy=Op::DISPINFOINVSY;
       int timex=Mp::SX-timesx-Op::DISPINFOWEAPX;
       int timey=Mp::SY-timesy-Op::DISPINFOWEAPY;
       int timeinx=20;
       int timeiny=Op::DISPINFOINVINRECTY;
       Tool& tool=Tools::tool[Tools::DISPINFO];
       screen.rectangle(tool,timex,timey,timesx,timesy);
       char time[256];
       int timesec=NoNet::game_time()*60-Game::get_timer()/Mp::METRONQUALITY;
       if (timesec<0)
       {
         timesec=0;
       }
       sprintf(time,"%02d:%02d:%02d",timesec/3600,(timesec%3600)/60,timesec%60);
       Print::print(screen,Print::TFONT,Print::_FLAST,1+timex+timeinx-Print::get_dx(Print::TFONT,time)/2,timey+timeiny,time);
       Print::print(screen,Print::TFONT,Print::FINVOVR,timex+timeinx-Print::get_dx(Print::TFONT,time)/2,timey+timeiny,time);
    }
  }

  {
    int invx=Op::DISPINFOINVX;
    int invy=Op::DISPINFOINVY;
    int invsx=Op::DISPINFOINVSX;
    int invsy=Op::DISPINFOINVSY;
    int invinx=Op::DISPINFOINVINRECTX;
    int inviny=Op::DISPINFOINVINRECTY;
    Tool& tool=Tools::tool[Tools::DISPINFO];
    screen.rectangle(tool,invx,invy,invsx,invsy);
    char *invdescr=myuser->get_item_descript();
    Print::print(screen,Print::TFONT,Print::_FLAST,1+invx+invinx-Print::get_dx(Print::TFONT,invdescr)/2,invy+inviny,invdescr);
    Print::print(screen,Print::TFONT,Print::FINVOVR,invx+invinx-Print::get_dx(Print::TFONT,invdescr)/2,invy+inviny,invdescr);
  }

  screen.origin(scr_orgx,scr_orgy);
}
//KLASA USER
User::User(int _id)
{
  id=_id;
  max_values.life=Op::myship.life;
  for (int i=0;i<6;i++)
  {
    max_values.weap_ammo[i]=Op::max_weapon[i];
  }
  life_upgrade_amount=Op::USERLIFEUPGRAMOUNT;
  life_time=Op::USERLIFEUPGRTIME;
  reset();
}

void User::reset(void)
{
  last_life=params.life;
  life_counter=life_time;
  params.life=Op::myship.life;
  params.weap_in_use=0;
  for (int i=0;i<_LAST;i++)
  {
    params.inventory[i]=0;
  }
  params.inv_in_use=_EMPTY;
  if (GameManager::get_testing())
  {
    DBG_CHECK(!NoNet::is_network_mode());
    for (int i=0;i<6;i++)
    {
      params.weap_ammo[i]=(int)Op::weaponary_begin[i];
    }
  }
  else
  {
    if (!NoNet::is_network_mode())
    {
      for (int i=0;i<6;i++)
      {
        params.weap_ammo[i]=(int)Pilot::data.user_data[Pilot::WEAP0+i];
      }
      params.inventory[MEDIKIT]=Pilot::data.user_data[Pilot::MEDIKIT];
      params.inventory[CLOAK]=Pilot::data.user_data[Pilot::CLOAK];
      params.inventory[IMMORT]=Pilot::data.user_data[Pilot::IMMORT];
      params.inventory[GOLDENEYE]=Pilot::data.user_data[Pilot::GOLDENEYE];
      if (params.weap_ammo[0]<Op::weaponary_begin[0])
      {
        params.weap_ammo[0]=Op::weaponary_begin[0];
      }
    }
    else
    {
      for (int i=0;i<6;i++)
      {
        params.weap_ammo[i]=(int)Op::weaponary_net_begin[i];
      }
    }
  }
  update_inv_state();
}

int User::insert_item(Invent item,int count)
{
  DBG_CHECK(item>(int)_EMPTY&&item<(int)_LAST);
  int &ref=params.inventory[item];
  DBG_CHECK(ref>=0);
  int accepted=0;
  switch(item)
  {
   case MEDIKIT:
     case CLOAK:
    case IMMORT: DBG_CHECK(count==1);
                 if (ref<=0)
                 {
                   ref=1;
                   accepted=1;
                 }
                 break;
   case NETMINE: if (ref<Op::INVENTMAXMINE)
                 {
                   ref=(ref+count>Op::INVENTMAXMINE)?Op::INVENTMAXMINE:(ref+count);
                   accepted=1;
                 }
                 break;
 case GOLDENEYE: if (ref<Op::INVENTMAXGOLDENEYE)
                 {
                   ref=(ref+count>Op::INVENTMAXGOLDENEYE)?Op::INVENTMAXGOLDENEYE:(ref+count);
                   accepted=1;
                 }
                 break;
        default: DBG_CHECK(0);
                 break;
  }
  if (accepted)
  {
    if (Mp::SHIPDEBUGLEVEL>0)
    {
      NoNet::get_ship(id)->send_debug_message("%d inserted to inventory (count==%d)",(int)item,count);
    }
    update_inv_state();
    return 1;
  }
  else
  {
    return 0;
  }
}

User::Invent User::get_item(void)
{
  DBG_CHECK(params.inv_in_use>=(int)_EMPTY&&params.inv_in_use<(int)_LAST);
  DBG_CHECK(params.inv_in_use==(int)_EMPTY||(params.inventory[params.inv_in_use]>0));
  if (params.inv_in_use>0&&params.inventory[params.inv_in_use]>0)
  {
    int item_to_return=params.inv_in_use;
    if (Mp::SHIPDEBUGLEVEL>0)
    {
      NoNet::get_ship(id)->send_debug_message("%d taken from inventory",(int)item_to_return);
    }
    params.inventory[params.inv_in_use]--;
    update_inv_state();
    return ((Invent)item_to_return);
  }
  return _EMPTY;
}

User::Invent User::curr_item(void)
{
  DBG_CHECK(params.inv_in_use>=(int)_EMPTY&&params.inv_in_use<(int)_LAST);
  DBG_CHECK(params.inv_in_use==(int)_EMPTY||(params.inventory[params.inv_in_use]>0));
  if (params.inv_in_use>0&&params.inventory[params.inv_in_use]>0)
  {
    return ((Invent)params.inv_in_use);
  }
  else
  {
    return _EMPTY;
  }
}

char *User::get_item_descript(void)
{
  DBG_CHECK(params.inv_in_use>=(int)_EMPTY&&params.inv_in_use<(int)_LAST);
  DBG_CHECK(params.inv_in_use==(int)_EMPTY||(params.inventory[params.inv_in_use]>0));
  int count=params.inventory[params.inv_in_use];
  sprintf(buffer,"dummy");
  switch(params.inv_in_use)
  {
    case _EMPTY: sprintf(buffer,Op::INVENTDESCRIPT[0]);
                 break;
   case MEDIKIT: sprintf(buffer,Op::INVENTDESCRIPT[1]);
                 break;
     case CLOAK: sprintf(buffer,Op::INVENTDESCRIPT[2]);
                 break;
    case IMMORT: sprintf(buffer,Op::INVENTDESCRIPT[3]);
                 break;
   case NETMINE: sprintf(buffer,Op::INVENTDESCRIPT[4],count);
                 break;
 case GOLDENEYE: sprintf(buffer,Op::INVENTDESCRIPT[5],count);
                 break;
        default: DBG_CHECK(0);
                 break;
  }
  DBG_CHECK(strcmp(buffer,"dummy"));
  return buffer;
}

void User::next_item(void)
{
  DBG_CHECK(params.inv_in_use>=(int)_EMPTY&&params.inv_in_use<(int)_LAST);
  DBG_CHECK(params.inv_in_use==(int)_EMPTY||(params.inventory[params.inv_in_use]>=0));
  for (int i=1;i<=(int)_LAST;i++)
  {
    int indx=(i+params.inv_in_use)%((int)_LAST);
    if (indx>0&&params.inventory[indx]>0)
    {
      params.inv_in_use=indx;
      return;
    }
  }
  params.inv_in_use=_EMPTY;
}

void User::prev_item(void)
{
  DBG_CHECK(params.inv_in_use>=(int)_EMPTY&&params.inv_in_use<(int)_LAST);
  DBG_CHECK(params.inv_in_use==(int)_EMPTY||(params.inventory[params.inv_in_use]>=0));
  for (int i=(int)_LAST-1;i>=0;i--)
  {
    int indx=(i+params.inv_in_use)%((int)_LAST);
    if (indx>0&&params.inventory[indx]>0)
    {
      params.inv_in_use=indx;
      return;
    }
  }
  params.inv_in_use=_EMPTY;
}

void User::update_inv_state(void)
{
  DBG_CHECK(params.inv_in_use>=(int)_EMPTY&&params.inv_in_use<(int)_LAST);
  if (params.inv_in_use==0||params.inventory[params.inv_in_use]<=0)
  {
    next_item();
  }
  DBG_CHECK(params.inv_in_use==(int)_EMPTY||(params.inventory[params.inv_in_use]>0));
}

int User::item_numb(User::Invent item)
{
  DBG_CHECK(item>=(int)_EMPTY&&item<(int)_LAST);
  return (params.inventory[item]);
}

void User::run(void)
{
  if (params.life!=last_life)
  {
    last_life=params.life;
    life_counter=life_time;
  }
  life_counter-=KbdStat::time();
  if (life_counter<0)
  {
    reset_lifeupgrade_timer();
    if (!Pilot::get_skill()||NoNet::is_network_mode())
    {
      increase_life(life_upgrade_amount);
    }
  }
}

void User::reset_lifeupgrade_timer(void)
{
  life_counter=life_time;
}

void User::check_weap_range(int weap)
{
  DBG_CHECK(weap>=0&&weap<=6);
}

int User::get_weap_ammo(int weap)
{
  check_weap_range(weap);
  return params.weap_ammo[weap];
}

int User::get_weap_ammo_max(int weap)
{
  check_weap_range(weap);
  return max_values.weap_ammo[weap];
}

void User::increase_life(int dlife)
{
  int old_life=params.life;
  params.life=(params.life+dlife>max_values.life)?max_values.life:params.life+dlife;
  if (Mp::SHIPDEBUGLEVEL>0)
  {
    NoNet::get_ship(id)->send_debug_message("life increased by %d",params.life-old_life);
  }
}

void User::decrease_life(int dlife)
{
  int old_life=params.life;
  params.life=(params.life-dlife<0)?0:params.life-dlife;
  if (Mp::SHIPDEBUGLEVEL>0)
  {
    NoNet::get_ship(id)->send_debug_message("life decreased by %d",old_life-params.life);
  }
}

int User::get_life(void)
{
  return params.life;
}

int User::get_life_percent(void)
{
  return (params.life*100)/max_values.life;
}

int User::get_life_max(void)
{
  return max_values.life;
}

int User::is_alive(void)
{
  return params.life>0;
}

int User::is_full_weap(int weap)
{
  check_weap_range(weap);
  return params.weap_ammo[weap]>=max_values.weap_ammo[weap];
}

int User::is_full_life(void)
{
  return params.life>=max_values.life;
}

void User::increase_weap_ammo(int weap,int dammo)
{
  int old_ammo=params.weap_ammo[weap];
  check_weap_range(weap);
  params.weap_ammo[weap]=(params.weap_ammo[weap]+dammo>max_values.weap_ammo[weap])?max_values.weap_ammo[weap]:params.weap_ammo[weap]+dammo;
  if (Mp::SHIPDEBUGLEVEL>0)
  {
    NoNet::get_ship(id)->send_debug_message("%d weap ammo increased by %d",weap+1,params.weap_ammo[weap]-old_ammo);
  }
}

void User::decrease_weap_ammo(int weap,int dammo)
{
  int old_ammo=params.weap_ammo[weap];
  check_weap_range(weap);
  params.weap_ammo[weap]=(params.weap_ammo[weap]-dammo<0)?0:params.weap_ammo[weap]-dammo;
  if (Mp::SHIPDEBUGLEVEL>0)
  {
    NoNet::get_ship(id)->send_debug_message("%d weap ammo decreased by %d",weap+1,old_ammo-params.weap_ammo[weap]);
  }
}

int User::get_weap_in_use(void)
{
  return params.weap_in_use;
}

void User::set_weap_in_use(int weap)
{
  check_weap_range(weap);
  params.weap_in_use=weap;
  if (Mp::SHIPDEBUGLEVEL>0)
  {
    NoNet::get_ship(id)->send_debug_message("weapon changed to %d",weap+1);
  }
}

void User::set_full_life(void)
{
  params.life=max_values.life;
}

void User::set_full_weap(void)
{
  for (int i=0;i<6;i++)
  {
    params.weap_ammo[i]=max_values.weap_ammo[i];
  }
}

void User::set_full_invent(void)
{
  if (NoNet::is_network_mode())
  {
    params.inventory[CLOAK]=1;
    params.inventory[IMMORT]=1;
    params.inventory[MEDIKIT]=1;
    params.inventory[GOLDENEYE]=Op::INVENTMAXGOLDENEYE;
    params.inventory[NETMINE]=Op::INVENTMAXMINE;
  }
  else
  {
    params.inventory[CLOAK]=1;
    params.inventory[IMMORT]=1;
    params.inventory[MEDIKIT]=1;
    params.inventory[GOLDENEYE]=Op::INVENTMAXGOLDENEYE;
  }
  update_inv_state();
}

void User::store_data(void)
{
  for (int i=0;i<6;i++)
  {
    Pilot::data.user_data[Pilot::WEAP0+i]=params.weap_ammo[i];
  }
  Pilot::data.user_data[Pilot::MEDIKIT]=params.inventory[MEDIKIT];
  Pilot::data.user_data[Pilot::CLOAK]=params.inventory[CLOAK];
  Pilot::data.user_data[Pilot::IMMORT]=params.inventory[IMMORT];
  Pilot::data.user_data[Pilot::GOLDENEYE]=params.inventory[GOLDENEYE];
}
//SCRCATCH
ScrCatch::ScrCatch(void):
Posit(0,0,Posit::UP,Op::scrcatch),
Move(0,0,Op::scrcatch)
{
  dx=0;
  dy=0;
  destination=(ScrCatch::DestPoint*)Heap::alloc(sizeof(ScrCatch::DestPoint)*Mp::TURNQUALITY,ship_mbn);
  for (int i=0;i<Mp::TURNQUALITY;i++)
  {
    destination[i].x=x_offset(i,Op::SCRCATCHRADIUS);
    destination[i].y=y_offset(i,Op::SCRCATCHRADIUS);
  }
  reset();
}

ScrCatch::~ScrCatch(void)
{
  if (destination!=NULL)
  {
    Heap::free((void*)destination);
    destination=NULL;
  }
}

void ScrCatch::reset(void)
{
  dx=((destination[0].x)<<16);
  dy=((destination[0].y)<<16);
  x=destination[0].x;
  y=destination[0].y;
  vx=0;
  vy=0;
}

void ScrCatch::run(int _angle)
{
  DBG_CHECK((_angle>=0&&_angle<Mp::TURNQUALITY)||_angle==-1);
  int tox=(_angle!=-1)?destination[_angle].x:0;
  int toy=(_angle!=-1)?destination[_angle].y:0;
  set_speed(distance(tox,toy)*2);
  angle=get_angle(tox,toy);
  transform_speed();
  go();
  dx+=vx*KbdStat::time();
  dy+=vy*KbdStat::time();
}

void ScrCatch::run(void)
{
#if HI_DEBUG
  FAILURE("ScrCatch::run(void) - use ScrCatch::run(int _angle).");
#endif
}

//KLASA MYSHIP
NoQueue<Myship::DebugData> *Myship::queue[NoNet::max_users+1];
int Myship::debug_initialized=0;

void Myship::init_debug(void)
{
  debug_initialized=1;
  for (int i=0;i<NoNet::max_users+1;i++)
  {
    queue[i]=NULL;
  }
  if (Mp::SHIPDEBUGLEVEL>0)
  {
    for (int i=1;i<NoNet::max_users+1;i++)
    {
      if (NoNet::check_id(i))
      {
        queue[i]=NEW(NoQueue<Myship::DebugData>(Mp::SHIPDEBUGLEVEL,"ship debug data",1),queue_mbn);
        DBG_CHECK(queue[i]!=NULL);
        DebugData d;
        d.frame=0;
        d.x=0;
        d.y=0;
        d.angle=0;
        d.life=0;
        strcpy(d.msg,"*** not used yet ***");
        for (int j=0;j<Mp::SHIPDEBUGLEVEL;j++)
        {
          queue[i]->add(&d);
        }
      }
    }
  }
}

void Myship::quit_debug(void)
{
  if (debug_initialized)
  {
    for (int i=1;i<NoNet::max_users+1;i++)
    {
      if (queue[i]!=NULL)
      {
        delete queue[i];
        queue[i]=NULL;
      }
    }
  }
  debug_initialized=0;
}

void Myship::dump_debug(void)
{
  if (debug_initialized)
  {
    for (int i=1;i<NoNet::max_users+1;i++)
    {
      if (queue[i]!=NULL)
      {
        MESSAGE((i==NoNet::get_this_id())?"SHIP %d (LOCAL ONE) DEBUGINFO DUMP:":"SHIP %d DEBUGINFO DUMP:",i);
        MESSAGE("f:frame,x:posx,y:posy,a:angle,l:life,s:frags/base,msg:message");
        while (!queue[i]->empty())
        {
          char buff[1024];
          DebugData *d=queue[i]->head();
          sprintf(buff,"SHIP %d (f:%d,x:%4d,y:%4d,a:%3d,l:%3d,s:%d,msg: %s)",i,d->frame,d->x,d->y,d->angle,d->life,Statistics::get(i),d->msg);
          MESSAGE(buff);
        }
      }
    }
  }
}

void Myship::debug_service(Myship *myship,char *msg)
{
  DBG_CHECK(debug_initialized);
  if (Mp::SHIPDEBUGLEVEL)
  {
    int id=myship->id;
    if (queue[id]!=NULL)
    {
      queue[id]->head();
      DebugData d;
      d.frame=KbdStat::local_frame();
      d.x=myship->x;
      d.y=myship->y;
      d.angle=myship->angle;
      d.life=myship->myuser->get_life();
      strncpy(d.msg,msg,DebugData::msg_len);
      d.msg[DebugData::msg_len-1]='\0';
      queue[id]->add(&d);
    }
  }
}

Myship::Myship(int x,int y,Sprite& sprite,int _id):
Posit(x,y,Posit::DOWN,Op::myship),
Move(0,0,Op::myship),
Shadow(sprite,View::MYSHIP),
Listmanager(OMYSHIP),
Colis(C_MYHERO(_id),Op::myship,"myship"),
Radar(0)
{
  reset_all_variables();
  id=_id;
  myuser=NEW(User(id),ship_mbn);
  LowColis::create_cworld(id);
  immortal=0;
  wall_through=0;
  Phase &p=get_spr_phase();
  int size_x=abs(p.r-p.l);
  int size_y=abs(p.d-p.u);
  ship_radius=(((size_x>size_y)?size_x:size_y)*6)/10;
  DBG_CHECK(ship_radius<=Mp::MAXOFFSET);
  scrcatch=NEW(ScrCatch,ship_mbn);
  dispinfo=NEW(DispInfo(myuser,id),ship_mbn);
  weapon=NEW(Weapon(x,y,myuser,id),ship_mbn);
  smoke=NEW(AlienSmoke(x,y,angle,Mysprites::farmerdym,100),gobjects_mbn);
  start_x=x;
  start_y=y;
  world->update_my_xy(id,x,y,get_offset_x(),get_offset_y());
  cloak=0;
  shield=0;
  cloak_warn_played=0;
  shield_warn_played=0;
  shld_low_played=0;
  shld_crit_played=0;
  kill_value=0;
  kill_countdown=0;
  last_killer_id=0;
  last_kill_value=0;
  morph_strong=0;
  morph_strong_phase=0;
  first_start=1;
  flash=0;
  accelerating=0;
  engine_flipper=0;
  bricks=0;
  last_x=x;
  last_y=y;
  nocolis_x=x;
  nocolis_y=y;
  reset_trailers();
  land_angle=normalize_angle(Turn::get_angle(Radar::TargetX,Radar::TargetY)+Mp::HALFANGLE);
  mode=WAITFORSTARTSIG;
  if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
  {
    life_brick_update();
  }
  shake_time=0;
  teleport_phase=0;
  teleport_x=0;
  teleport_y=0;
  teleport_tox=0;
  teleport_toy=0;
  teleport_speed=0;
  teleport_angle=0;
  teleport_vx=0;
  teleport_vy=0;
  shipfuck_time=0;
  shipfuck_status=0;
  leaving_notified=0;
  kill_alreadykilled=0;
  fadeing=0;
  old_life=myuser->get_life();
  SEND_DEBUG_MESSAGE("ship created");
}

Myship::~Myship(void)
{
  SEND_DEBUG_MESSAGE("ship destructed");
  if (smoke!=NULL)
  {
    delete smoke;
    smoke=NULL;
  }
  if (scrcatch!=NULL)
  {
    delete scrcatch;
    scrcatch=NULL;
  }
  if (dispinfo!=NULL)
  {
    delete dispinfo;
    dispinfo=NULL;
  }
  if (weapon!=NULL)
  {
    delete weapon;
    weapon=NULL;
  }
  if (myuser!=NULL)
  {
    delete myuser;
    myuser=NULL;
  }
  Colis::delete_cworld(id);
}

void Myship::reset_all_variables(void)
{
  cloak=0;
  shield=0;
  cloak_warn_played=0;
  shield_warn_played=0;
  shld_low_played=0;
  shld_crit_played=0;
  old_life=0;
  immortal=0;
  wall_through=0;
  engine_flipper=0;
  flash=0;
  morph_strong=0;
  morph_strong_phase=0;
  accelerating=0;
  kill_countdown=0;
  kill_value=0;
  last_kill_value=0;
  last_killer_id=0;
  last_x=0;
  last_y=0;
  nocolis_x=0;
  nocolis_y=0;
  land_x=0;
  land_y=0;
  land_angle=0;
  first_start=0;
  bricks=0;
  life_brick_ratio=0;
  life_brick_cnt=0;
  shake_time=0;
  shipfuck_time=0;
  shipfuck_status=0;
  teleport_phase=0;
  teleport_x=0;
  teleport_y=0;
  teleport_tox=0;
  teleport_toy=0;
  teleport_speed=0;
  teleport_angle=0;
  teleport_vx=0;
  teleport_vy=0;
  kill_alreadykilled=0;
  fadeing=0;
}

void Myship::kick_ship(int _x,int _y)
{
  if (is_up()&&(mode==USER||mode==TURBO))
  {
    SEND_DEBUG_MESSAGE("kicked from %d,%d",_x,_y);
    int _angle=Turn::get_dangle(x-_x,y-_y);
    int  dis=Posit::distance(_x,_y)/15;
    int accel=0;
    switch (dis)
    {
       case 3: accel=3000;
               break;
       case 2: accel=6000;
               break;
       case 1: accel=9000;
               break;
       case 0: accel=12000;
               break;
      default: return;
    }
    _kick_me(_angle,accel);
  }
}

void Myship::kick_ship(int _angle,Move::KickPower _power)
{
  if (is_up()&&(mode==USER||mode==TURBO))
  {
    SEND_DEBUG_MESSAGE("kicked to angle %d with power %d",_angle,(int)_power);
    kick_me(_angle,_power);
  }
}

void Myship::beaming_attract(int _x,int _y)
{
  if (is_up()&&(mode==USER||mode==TURBO))
  {
    SEND_DEBUG_MESSAGE("beming attraction from %d,%d",_x,_y);
    if (!KbdStat::abort_beaming(id))
    {
      int dist=Posit::distance(_x,_y);
      int angl=Turn::get_dangle(_x-x,_y-y);
      set_speed(2*dist,angl);
    }
  }
}

int Myship::bricks_numb(void)
{
  DBG_CHECK(bricks>=0);
  return bricks;
}

int Myship::bricks_full(void)
{
  DBG_CHECK(bricks>=0);
  return (bricks>=Op::MAXSHIPBRICK);
}

void Myship::life_brick_update(void)
{
  DBG_CHECK(bricks>=0);
  life_brick_ratio=(myuser->get_life()*2)/(bricks+1);
  life_brick_cnt=0;
}

void Myship::throw_brick(void)
{
  DBG_CHECK(bricks>=1);
  NEW(Brick(x,y,get_chl(),RAND%Mp::TURNQUALITY,150+RAND%200,-1,1),gobjects_mbn);
  Base::impulse(id,-1);
  bricks--;
}

void Myship::play_ship_sample(Sample &sample,SampleMode mode)
{
  switch (mode)
  {
      case SPEECH: if (id==world->get_current_displayed())
                   {
                     SManager::add_sample(&sample);
                   }
                   break;
    case INTERNAL: if (id==world->get_current_displayed())
                   {
                     Sound::st_play(&sample);
                   }
                   break;
    case EXTERNAL: play(sample);
                   break;
          default: DBG_CHECK(0);
                   break;
  }
}

void Myship::netcheating_service(void)
{
  if (KbdStat::net_cheating(id)&&NoNet::is_network_mode())
  {
    myuser->set_full_life();
    myuser->set_full_weap();
    myuser->set_full_invent();
    char buf[1024];
    sprintf(buf,"%s is cheating",NoNet::get_player_name(id));
    MSGTXT("*** WARNING ***",0);
    MSGTXT(buf,0);
    MSGTXT("*** WARNING ***",0);
  }
}

void Myship::steering_service(void)
{
  int turbo=0;
  int auto_run=KbdStat::turbo_locked(id);
  if ((auto_run&&!KbdStat::turbo(id))||
      (!auto_run&&KbdStat::turbo(id)))
  {
    turbo=1;
  }
  if (turbo)
  {
    if (mode==USER)
    {
      set_turbo_mode_params();
      mode=TURBO;
      if (accelerating)
      {
        play(Mysound::ship_turboon);
      }
      SEND_DEBUG_MESSAGE("turbo mode engaged");
    }
  }
  else
  {
    if (mode==TURBO)
    {
      set_user_mode_params();
      mode=USER;
      if (accelerating)
      {
        play(Mysound::ship_turbooff);
      }
      SEND_DEBUG_MESSAGE("turbo mode disabled");
    }
  }
  if (!KbdStat::left(id)||!KbdStat::right(id))
  {
    if (KbdStat::left(id))
    {
      Turn::turn_left();
    }
    else if (KbdStat::right(id))
    {
      Turn::turn_right();
    }
  }
  if (KbdStat::strafe_left(id))
  {
    strafe_left();
  }
  if (KbdStat::strafe_right(id))
  {
    strafe_right();
  }
  if (KbdStat::forward(id)||KbdStat::backward(id)||
     (KbdStat::steering_mode(id)==1&&KbdStat::mouse_goto(id)))
  {
    if (!accelerating)
    {
      if (mode==TURBO&&!auto_run)
      {
        play(Mysound::ship_turboon);
      }
    }
    if (KbdStat::forward(id))
    {
      accelerating=1;
      Move::faster();
      if (mode==TURBO)
      {
        Move::faster();
      }
    }
    else if (KbdStat::backward(id))
    {
      accelerating=2;
      Move::rear_faster();
      if (mode==TURBO)
      {
        Move::rear_faster();
      }
    }
    if (KbdStat::steering_mode(id)==1&&KbdStat::mouse_goto(id))
    {
      int a=Turn::get_angle(KbdStat::mouse_x(id)+world->get_x(id),KbdStat::mouse_y(id)+world->get_y(id));
      turn_point(KbdStat::mouse_x(id)+world->get_x(id),KbdStat::mouse_y(id)+world->get_y(id));
      accelerating=1;
      faster(a);
      if (mode==TURBO)
      {
        faster(a);
      }
    }
  }
  else
  {
    if (accelerating)
    {
      accelerating=0;
      if (mode==TURBO&&!auto_run)
      {
        play(Mysound::ship_turbooff);
      }
    }
    update_speed();
    Move::auto_brake();
  }
}

void Myship::go(void)
{
  DBG_CHECK(NoNet::check_id(id));
  switch (mode)
  {
           case DYING: break;
     case TELEPORTING: Move::go();
                       break;
 case WAITFORSTARTSIG: break;
         case LANDING: Posit::go_down();
                       Move::go();
                       break;
        case STARTING: Posit::go_up();
                       Move::go();
                       break;
           case TURBO:
            case USER: Move::go();
                       break;
              default: DBG_CHECK(0);
  }
  update_position();
}

void Myship::update_position(void)
{
  int wx=x;
  int wy=y;
  if (shake_time>0)
  {
    int t=((shake_time/KbdStat::time())>2)?(2):(shake_time/KbdStat::time());
    if (t>0)
    {
      wx+=RAND%(2*t)-t;
      wy+=RAND%(2*t)-t;
    }
  }
  world->update_my_xy(id,wx,wy,get_offset_x(),get_offset_y());
}

void Myship::landing_service(void)
{
  turn_angle(land_angle);
  int dist=Posit::vect_len(land_x-Posit::get_down_x(),land_y-Posit::get_down_y());
  int ang=get_dangle(land_x-Posit::get_down_x(),land_y-Posit::get_down_y());
  set_speed(dist*2,ang);
}

void Myship::teleporting_service(void)
{
  DBG_CHECK(mode==TELEPORTING);
  if (teleport_phase>0)
  {
    if (Fade::is_faded(id))
    {
      stop();
      x=teleport_tox;
      y=teleport_toy;
      teleport_phase=0;
      FOR_ALL_USERS(ID)
      {
        if (ID!=id)
        {
          Myship *ship=NoNet::get_ship(ID);
          if (Posit::distance(ship->get_x(),ship->get_y())<=2*Op::TELEPORTSTARTDISTANCE)
          {
            ship->kill(id);
            SEND_DEBUG_MESSAGE("killing ship %d in teleport",id);
          }
        }
      }
    }
    else
    {
      int dist=Posit::distance(teleport_x,teleport_y);
      int angl=Turn::get_dangle(teleport_x-x,teleport_y-y);
      set_speed(2*dist,angl);
    }
  }
  else
  {
    if (!Fade::in_use(id))
    {
      vx=teleport_vx;
      vy=teleport_vy;
      set_user_mode_params();
      mode=USER;
      SEND_DEBUG_MESSAGE("teleporting finished at %d,%d",x,y);
    }
  }
}

void Myship::starting_service(void)
{
  int turned=!turn_angle(start_angle);
  if (is_up()&&!turned)
  {
    set_user_mode_params();
    mode=USER;
    SEND_DEBUG_MESSAGE("starting finished at %d,%d",x,y);
  }
}

void Myship::inventory_service(void)
{
  if (KbdStat::use_item(id))
  {
    switch (myuser->curr_item())
    {
      case User::_EMPTY: break;
     case User::MEDIKIT: myuser->get_item();
                         myuser->set_full_life();
                         if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
                         {
                           life_brick_update();
                         }
                         MSGTXT(GAMETXT("SHIELDRECHARGED"),id);
                         play_ship_sample(Mysound::speech_shldrecharged,SPEECH);
                         SEND_DEBUG_MESSAGE("medikit used");
                         break;
       case User::CLOAK: myuser->get_item();
                         cloak=Op::MYSHIPCLOAKTIME;
                         MSGTXT(GAMETXT("CLOAKACTIVE"),id);
                         play_ship_sample(Mysound::speech_cloakactive,SPEECH);
                         SEND_DEBUG_MESSAGE("cloaking device engaged");
                         break;
      case User::IMMORT: myuser->get_item();
                         shield=Op::MYSHIPSHIELDTIME;
                         morph_strong=1;
                         morph_strong_phase=0;
                         MSGTXT(GAMETXT("SHIELDACTIVE"),id);
                         play_ship_sample(Mysound::ship_strongmorph_on,INTERNAL);
                         play_ship_sample(Mysound::speech_dshieldactive,SPEECH);
                         SEND_DEBUG_MESSAGE("shield engaged");
                         break;
     case User::NETMINE: myuser->get_item();
                         Sound::play(Mysound::ship_mine);
                         NEW(NetMine(x,y,id),gobjects_mbn);
                         SEND_DEBUG_MESSAGE("mine thrown");
                         break;
   case User::GOLDENEYE: myuser->get_item();
                         Sound::play(Mysound::ship_emshock);
                         NEW(MyBomb(x,y,id),gobjects_mbn);
                         SEND_DEBUG_MESSAGE("magnetic shock used");
                         break;
                default: DBG_CHECK(0);
                         break;
    }
  }
}

void Myship::shake_service(void)
{
  if (shake_time>0)
  {
    shake_time-=KbdStat::time();
    if (shake_time<=0)
    {
      shake_time=0;
      SEND_DEBUG_MESSAGE("screen shaking disabled");
    }
  }
}

void Myship::timer_service(void)
{
  if (shipfuck_status)
  {
    DBG_CHECK(!NoNet::is_network_mode());
    if ((shipfuck_time-=KbdStat::time())<0)
    {
      SEND_DEBUG_MESSAGE("rebeliants in action");
      kill_value+=Op::SHIPFUCKPOWER;
      last_killer_id=id;
      SEND_DEBUG_MESSAGE("last killer id set on %d by rebeliants",last_killer_id);
      shipfuck_time=Op::SHIPFUCKTIME;

      int ang=RAND%Mp::TURNQUALITY;
      int pow=RAND%2;
      int sample=RAND%MAXEXPLODE;
      kick_ship(ang,(pow)?Move::MEDIUM:Move::STRONG);
      play_ship_sample(Mysound::explode[sample],EXTERNAL);
      for (int i=0;i<4;i++)
      {
        int spl=RAND%Mysprites::splinters_count();
        NEW(Splinter(x,y,get_chl(),Mysprites::splinter[spl]),gobjects_mbn);
      }
      switch (RAND%5)
      {
        case 0: MSGTXT(GAMETXT("FUELLEAK"),0);
                play_ship_sample(Mysound::speech_fuelleak,SPEECH);
                break;
        case 1: MSGTXT(GAMETXT("HULLDAMAGE"),0);
                play_ship_sample(Mysound::speech_hulldamage,SPEECH);
                break;
        case 2: MSGTXT(GAMETXT("CARGOUNSTABLE"),0);
                play_ship_sample(Mysound::speech_cargounstable,SPEECH);
                break;
        case 3: MSGTXT(GAMETXT("EXPLOSION"),0);
                play_ship_sample(Mysound::speech_explosion,SPEECH);
                break;
        case 4: MSGTXT(GAMETXT("COOLINGFAIL"),0);
                play_ship_sample(Mysound::speech_coolingfail,SPEECH);
                break;
      }
    }
  }
}

void Myship::kill(int killer_id)
{
  SEND_DEBUG_MESSAGE("%d attempts to kill me",killer_id);
  DBG_CHECK(NoNet::check_id(killer_id));
  if (is_up()&&(mode==USER||mode==TURBO||mode==TELEPORTING))
  {
    kill_value+=myuser->get_life();
    last_killer_id=killer_id;
    SEND_DEBUG_MESSAGE("last killer id set on %d by kill function",last_killer_id);
  }
}

void Myship::weapon_service(void)
{
  if (KbdStat::weap1(id))
  {
    myuser->set_weap_in_use(0);
  }
  else if (KbdStat::weap2(id))
  {
    myuser->set_weap_in_use(1);
  }
  else if (KbdStat::weap3(id))
  {
    myuser->set_weap_in_use(2);
  }
  else if (KbdStat::weap4(id))
  {
    myuser->set_weap_in_use(3);
  }
  else if (KbdStat::weap5(id))
  {
    myuser->set_weap_in_use(4);
  }
  else if (KbdStat::weap6(id))
  {
    myuser->set_weap_in_use(5);
  }
  else if (KbdStat::prev_weap(id))
  {
    int i=myuser->get_weap_in_use();
    if (++i>5)
    {
      i=0;
    }
    myuser->set_weap_in_use(i);
  }
  else if (KbdStat::next_weap(id))
  {
    int i=myuser->get_weap_in_use();
    if (--i<0)
    {
      i=5;
    }
    myuser->set_weap_in_use(i);
  }
  int firing=0;
  if (KbdStat::fire(id)&&(mode==USER||mode==TURBO))
  {
    firing=1;
  }

  weapon->run(x,y,vx,vy,get_dir(),angle,firing,mode);

  if (KbdStat::next_item(id))
  {
    myuser->next_item();
  }
  if (KbdStat::prev_item(id))
  {
    myuser->prev_item();
  }
}

void Myship::dispinfo_service(void)
{
  dispinfo->run();
}

void Myship::radar_service(void)
{
  if (id==NoNet::get_this_id())
  {
    Radar::set(x,y);
  }
}

void Myship::check_walls_col(void)
{
  reset_cmsg();
  int wall_kill_value=0;
  int collided=0;
  int in_wall=0;
  int counter=0;

  do
  {
    int result_vectorx=0;
    int result_vectory=0;
    Cmsg *temp=NULL;
    for (int i=0;i<Mp::TURNQUALITY;i+=8)
    {
      int offsetx=x_offset(i,ship_radius);
      int offsety=y_offset(i,ship_radius);
      temp=LowColis::see(id,x+offsetx,y+offsety,0);
      if (temp&&temp->WALL)
      {
        result_vectorx-=offsetx;
        result_vectory-=offsety;
        if (temp->KILLER&&temp->destroy>wall_kill_value)
        {
          wall_kill_value=temp->destroy;
          SEND_DEBUG_MESSAGE("killing wall detected");
        }
      }
    }

    if ((result_vectorx!=0||result_vectory!=0)&&!wall_through)
    {
      int wangle=get_dangle(result_vectorx,result_vectory);
      collided=1;
      in_wall=1;
      counter++;

      if (counter==1)
      {
        SEND_DEBUG_MESSAGE("collision with wall detected at %d,%d",x,y);
      }
      else if (counter>30)
      {
        SEND_DEBUG_MESSAGE("wall collision resolved abnormally");
        x=nocolis_x;
        y=nocolis_y;
        reset_trailers();
        break;
      }

      move_me(wangle,1);

      if (counter==1)
      {

        if (wall_kill_value>0)
        {
          kill_value+=wall_kill_value;
          last_killer_id=id;
          SEND_DEBUG_MESSAGE("last killer id set on %d by wall",last_killer_id);
        }
        if (get_speed()>Op::MYSHIPDESTROYCOLSPEED&&(shield<=0))
        {
          kill_value+=Op::MYSHIPDESTROYCOLAMOUNT;
          last_killer_id=id;
          SEND_DEBUG_MESSAGE("last killer id set on %d by driving too fast",last_killer_id);
          SEND_DEBUG_MESSAGE("driving too fast (wall collision damage)");
        }
        if (get_speed()>Op::MYSHIPCOLSNDSPEED&&!Mysound::ship_wallcollis.playing())
        {
          play(Mysound::ship_wallcollis);
        }

        int speed_angle=get_dangle(vx,vy);
        int normal_angle=normalize_angle(wangle+Mp::QUARTANGLE);
        int normspeed_dangle=0;
        int newspeed_angle=0;
        int dupax=x_offset(normal_angle,Mp::MAXOFFSET);
        int dupay=y_offset(normal_angle,Mp::MAXOFFSET);

        int tangle=get_dangle((int)result_vectorx,(int)result_vectory);

        if (dupax*vy-dupay*vx>0)
        {
          in_wall=0;

          normspeed_dangle=speed_angle-normal_angle;
          if (normspeed_dangle<0)
          {
            normspeed_dangle+=Mp::TURNQUALITY;
          }
          newspeed_angle=normal_angle-normspeed_dangle;
          if (newspeed_angle<0)
          {
            newspeed_angle+=Mp::TURNQUALITY;
          }

          transform_speed(newspeed_angle);
          int skladowa_prostopadla=(Posit::_shiftsin(normspeed_dangle)*get_speed())/Mp::METRONQUALITY;
          int s=(skladowa_prostopadla<0)?-1:1;
          int norm=abs(skladowa_prostopadla);
          vx-=s*x_offset(tangle,norm);
          vy-=s*y_offset(tangle,norm);
          update_speed();
        }

      }
    }
    else
    {
      nocolis_x=x;
      nocolis_y=y;
      in_wall=0;
    }
  }
  while (in_wall);

  last_x=x;
  last_y=y;
  if (collided)
  {
    SEND_DEBUG_MESSAGE("new position after wall collision set: %d,%d",x,y);
  }

}

int Myship::collect_capsule(Capsule::Type type,int power,int additional_info,int force_message)
{
  DBG_CHECK(type>Capsule::_FIRST&&type<Capsule::_LAST);
  if (mode!=DYING)
  {
    switch (type)
    {
       case Capsule::WEAP0:
       case Capsule::WEAP1:
       case Capsule::WEAP2:
       case Capsule::WEAP3:
       case Capsule::WEAP4:
       case Capsule::WEAP5: {
                              int weap_numb=(int)type-(int)Capsule::WEAP0;
                              if (!myuser->is_full_weap(weap_numb))
                              {
                                if (!myuser->get_weap_ammo(weap_numb)||force_message)
                                {
                                  char buff[1024];
                                  char *pow_str=NULL;
                                  switch (additional_info)
                                  {
                                    case 0: pow_str="MIN";
                                            break;
                                    case 1: pow_str="MED";
                                            break;
                                    case 2: pow_str="MAX";
                                            break;
                                   default: DBG_CHECK(0);
                                            break;
                                  }
                                  sprintf(buff,"WEAP%d%s_COLLECTED",weap_numb+1,pow_str);
                                  MSGTXT(GAMETXT(buff),id);
                                  switch (type)
                                  {
                                    case Capsule::WEAP0: play_ship_sample(Mysound::speech_vulcan,SPEECH);
                                                         break;
                                    case Capsule::WEAP1: play_ship_sample(Mysound::speech_swarmers,SPEECH);
                                                         break;
                                    case Capsule::WEAP2: play_ship_sample(Mysound::speech_plasma,SPEECH);
                                                         break;
                                    case Capsule::WEAP3: play_ship_sample(Mysound::speech_missiles,SPEECH);
                                                         break;
                                    case Capsule::WEAP4: play_ship_sample(Mysound::speech_cannon,SPEECH);
                                                         break;
                                    case Capsule::WEAP5: play_ship_sample(Mysound::speech_grenade,SPEECH);
                                                         break;
                                                default: DBG_CHECK(0);
                                                         break;
                                  }
                                }
                                myuser->increase_weap_ammo(weap_numb,power);
                                play_ship_sample(Mysound::ship_weap_collected,INTERNAL);
                                SEND_DEBUG_MESSAGE("weapon %d, power %d upgrade collected",weap_numb+1,additional_info);
                                return 1;
                              }
                            }
                            break;
       case Capsule::LIFE:  if (!myuser->is_full_life())
                            {
                              if (!myuser->get_life()||force_message)
                              {
                                char buff[1024];
                                char *pow_str=NULL;
                                switch (additional_info)
                                {
                                  case 0: pow_str="MIN";
                                          break;
                                  case 1: pow_str="MED";
                                          break;
                                  case 2: pow_str="MAX";
                                          break;
                                 default: DBG_CHECK(0);
                                          break;
                                }
                                sprintf(buff,"LIFE%s_COLLECTED",pow_str);
                                MSGTXT(GAMETXT(buff),id);
                                play_ship_sample(Mysound::speech_shield,SPEECH);
                              }
                              myuser->increase_life(power);
                              if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
                              {
                                life_brick_update();
                              }
                              play_ship_sample(Mysound::ship_life_collected,INTERNAL);
                              SEND_DEBUG_MESSAGE("life, power %d upgrade collected",additional_info);
                              return 1;
                            }
                            break;
    case Capsule::MEDIKIT:  if (myuser->insert_item(User::MEDIKIT,power))
                            {
                              if (myuser->item_numb(User::MEDIKIT)==1||force_message)
                              {
                                MSGTXT(GAMETXT("MEDIKIT_COLLECTED"),id);
                                play_ship_sample(Mysound::speech_shield,SPEECH);
                                play_ship_sample(Mysound::speech_collected,SPEECH);
                              }
                              play_ship_sample(Mysound::ship_invent_collected,INTERNAL);
                              SEND_DEBUG_MESSAGE("medikit collected");
                              return 1;
                            }
                            break;
       case Capsule::CLOAK: if (myuser->insert_item(User::CLOAK,power))
                            {
                              if (myuser->item_numb(User::CLOAK)==1||force_message)
                              {
                                MSGTXT(GAMETXT("CLOAK_COLLECTED"),id);
                                play_ship_sample(Mysound::speech_cloak,SPEECH);
                                play_ship_sample(Mysound::speech_collected,SPEECH);
                              }
                              play_ship_sample(Mysound::ship_invent_collected,INTERNAL);
                              SEND_DEBUG_MESSAGE("cloak device collected");
                              return 1;
                            }
                            break;
      case Capsule::IMMORT: if (myuser->insert_item(User::IMMORT,power))
                            {
                              if (myuser->item_numb(User::IMMORT)==1||force_message)
                              {
                                MSGTXT(GAMETXT("IMMORTALITY_COLLECTED"),id);
                                play_ship_sample(Mysound::speech_dshield,SPEECH);
                                play_ship_sample(Mysound::speech_collected,SPEECH);
                              }
                              play_ship_sample(Mysound::ship_invent_collected,INTERNAL);
                              SEND_DEBUG_MESSAGE("shields collected");
                              return 1;
                            }
                            break;
       case Capsule::BRICK: DBG_CHECK(NoNet::is_network_mode(NoNet::NETWORKFLAG));
                            DBG_CHECK(bricks>=0);
                            {
                              int msg=0;
                              bricks++;
                              if (!Statistics::get_land(id))
                              {
                                msg=1;
                              }
                              Base::impulse(id,1);
                              if (msg&&Statistics::get_land(id))
                              {
                                MSGTXT(GAMETXT("YOU_CLEAR_TO_LAND"),id);
                                if (id==world->get_current_displayed())
                                {
                                  play_ship_sample(Mysound::speech_youclearland,SPEECH);
                                }
                                else
                                {
                                  FOR_ALL_USERS(ID)
                                  {
                                    if (ID!=id)
                                    {
                                      char buf[1024];
                                      sprintf(buf,"%s %s",NoNet::get_player_name(id),GAMETXT("CLEAR_TO_LAND"));
                                      MSGTXT(buf,ID);
                                      Myship *s=NoNet::get_ship(ID);
                                      s->play_ship_sample(Mysound::speech_colours[id],SPEECH);
                                      s->play_ship_sample(Mysound::speech_clearland,SPEECH);
                                    }
                                  }
                                }
                              }
                              life_brick_update();
                              if (bricks==1||force_message)
                              {
                                MSGTXT(GAMETXT("BRICK_COLLECTED"),id);
                              }
                              play_ship_sample(Mysound::ship_brick_collected,INTERNAL);
                              SEND_DEBUG_MESSAGE("brick collected");
                              return 1;
                            }
                            break;
     case Capsule::NETMINE: {  
                              int play_sample=(myuser->item_numb(User::NETMINE)==0);
                              if (myuser->insert_item(User::NETMINE,power))
                              {
                                if (play_sample||force_message)
                                {
                                  MSGTXT(GAMETXT("MINE_COLLECTED"),id);
                                  play_ship_sample(Mysound::speech_mine,SPEECH);
                                  play_ship_sample(Mysound::speech_collected,SPEECH);
                                }
                                play_ship_sample(Mysound::ship_invent_collected,INTERNAL);
                                SEND_DEBUG_MESSAGE("netmine collected");
                                return 1;
                              }
                            }
                            break;
   case Capsule::GOLDENEYE: if (myuser->insert_item(User::GOLDENEYE,power))
                            {
                              if (myuser->item_numb(User::GOLDENEYE)==1||force_message)
                              {
                                MSGTXT(GAMETXT("MAGNETIC_COLLECTED"),id);
                                play_ship_sample(Mysound::speech_emshock,SPEECH);
                                play_ship_sample(Mysound::speech_collected,SPEECH);
                              }
                              play_ship_sample(Mysound::ship_invent_collected,INTERNAL);
                              SEND_DEBUG_MESSAGE("emshock collected");
                              return 1;
                            }
                            break;
       case Capsule::CHAOS: myuser->set_full_life();
                            myuser->set_full_weap();
                            MSGTXT(GAMETXT("CHAOS_COLLECTED"),id);
                            play_ship_sample(Mysound::ship_weap_collected,INTERNAL);
                            play_ship_sample(Mysound::speech_chaosupgrade,SPEECH);
                            SEND_DEBUG_MESSAGE("mega upgrade collected");
                            return 1;
                            break;
                   default: DBG_CHECK(0);
                            break;
    }
  }
  return 0;
}

void Myship::take_skin(Skin *skin)
{
  int weap_played=0;
  for (int i=0;i<6;i++)
  {
    if (!myuser->get_weap_ammo(i)&&skin->ship_weap[i])
    {
      switch (i)
      {
        case 0: play_ship_sample(Mysound::speech_vulcan,SPEECH);
                break;
        case 1: play_ship_sample(Mysound::speech_swarmers,SPEECH);
                break;
        case 2: play_ship_sample(Mysound::speech_plasma,SPEECH);
                break;
        case 3: play_ship_sample(Mysound::speech_missiles,SPEECH);
                break;
        case 4: play_ship_sample(Mysound::speech_cannon,SPEECH);
                break;
        case 5: play_ship_sample(Mysound::speech_grenade,SPEECH);
                break;
       default: DBG_CHECK(0);
                break;
      }
      weap_played=1;
    }
    myuser->increase_weap_ammo(i,skin->ship_weap[i]);
  }
  if (weap_played)
  {
  }
  MSGTXT(GAMETXT("SKIN_COLLECTED"),id);
  play_ship_sample(Mysound::ship_weap_collected,INTERNAL);
  SEND_DEBUG_MESSAGE("skin from %d upgrade collected",skin->id);
}

void Myship::check_object_col(void)
{
  int ship_collided=0;
  for (Cmsg *temp=get_cmsg();temp;temp=get_cmsg())
  {
    if (temp->KILLER&&!temp->WALL&&(temp->MY!=id||temp->ALIEN))
    {
      SEND_DEBUG_MESSAGE("collision with object detected (MY:%d,ALIEN:%d,KILLER:%d,KILLABLE:%d,WALL:%d,destroy power:%d)",((temp->MY)!=0),((temp->ALIEN)!=0),((temp->KILLER)!=0),((temp->KILLABLE)!=0),((temp->WALL)!=0),(int)temp->destroy);
      if ((((temp->MY>0)&&(temp->MY!=id))||temp->ALIEN)&&temp->KILLABLE)
      {
        ship_collided=1;
        SEND_DEBUG_MESSAGE("collision object classified as ship or mine");
      }
      kill_value+=temp->destroy;
      last_killer_id=temp->MY;
      SEND_DEBUG_MESSAGE("last killer id set on %d by object collision",last_killer_id);
    }
  }
  if (ship_collided&&!Mysound::ship_shipcollis.playing())
  {
    play(Mysound::ship_shipcollis);
  }
}

void Myship::check_col(void)
{
  check(-1,vx,vy,get_sprite(),get_phase());
  check_object_col();
  check_walls_col();
  if (shield>0)
  {
    if (kill_value>0)
    {
      kill_value/=3;
      SEND_DEBUG_MESSAGE("kill value decreased (due to shields)");
    }
  }
  if (kill_value&&!immortal&&(flash<=0||kill_value>last_kill_value))
  {
    last_kill_value=kill_value;
    myuser->decrease_life(kill_value);

    if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
    {
      life_brick_cnt+=kill_value;
      while (life_brick_cnt>=life_brick_ratio&&bricks>0)
      {
        throw_brick();
        life_brick_cnt-=life_brick_ratio;
      }
    }

    flash=Op::MYSHIPFLASHTIME;
    set_power(0);
    SEND_DEBUG_MESSAGE("flash immortality engaged");
  }
  kill_value=0;
  write(-1,vx,vy,get_sprite(),get_phase());
}

void Myship::shake_screen(int _time)
{
  shake_time=_time;
  SEND_DEBUG_MESSAGE("screen shaked, time %d",shake_time);
}

void Myship::display_service(void)
{
  engine_flipper=!engine_flipper;
  if (cloak>0)
  {
    cloak-=KbdStat::time();
    if (cloak<=0)
    {
      cloak=0;
      cloak_warn_played=0;
      MSGTXT(GAMETXT("CLOAKDEACTIVE"),id);
      play_ship_sample(Mysound::speech_cloakdeactive,SPEECH);
      SEND_DEBUG_MESSAGE("cloaking device disabled");
    }
    else if (cloak<=11*Mp::METRONQUALITY)
    {
      if (!cloak_warn_played)
      {
        MSGTXT(GAMETXT("CLOAK10DEACTIVE"),id);
        play_ship_sample(Mysound::speech_cloak10deactive,SPEECH);
        cloak_warn_played=1;
      }
    }
    else
    {
      cloak_warn_played=0;
    }
  }
  if (shield>0)
  {
    shield-=KbdStat::time();
    if (shield<=0)
    {
      shield=0;
      shield_warn_played=0;
      morph_strong=1;
      morph_strong_phase=0;
      MSGTXT(GAMETXT("SHIELDDEACTIVE"),id);
      play_ship_sample(Mysound::ship_strongmorph_off,INTERNAL);
      play_ship_sample(Mysound::speech_dshielddeactive,SPEECH);
      SEND_DEBUG_MESSAGE("shields disabled");
    }
    else if (shield<=11*Mp::METRONQUALITY)
    {
      if (!shield_warn_played)
      {
        MSGTXT(GAMETXT("SHIELD10DEACTIVE"),id);
        play_ship_sample(Mysound::speech_dshield10deactive,SPEECH);
        shield_warn_played=1;
      }
    }
    else
    {
      shield_warn_played=0;
    }
  }
  if (old_life!=myuser->get_life())
  {
    if (id==world->get_current_displayed())
    {
      if (myuser->get_life()>old_life)
      {
        if (!Mysound::ship_lifeincrease.playing())
        {
          play_ship_sample(Mysound::ship_lifeincrease,INTERNAL);
        }
      }
      else
      {
        if (!Mysound::ship_lifedecrease.playing())
        {
          play_ship_sample(Mysound::ship_lifedecrease,INTERNAL);
        }
      }
    }
    old_life=myuser->get_life();
  }
  if (id==world->get_current_displayed())
  {
    if (myuser->get_life_percent()<=10&&myuser->is_alive())
    {
      if (!shld_crit_played)
      {
        play_ship_sample(Mysound::speech_shldcritical,SPEECH);
        MSGTXT(GAMETXT("SHIELDCRITICAL"),0);
        shld_crit_played=1;
      }
    }
    else
    {
      shld_crit_played=0;
      if (myuser->get_life_percent()<=30&&myuser->is_alive())
      {
        if (!shld_low_played)
        {
          play_ship_sample(Mysound::speech_shldlow,SPEECH);
          MSGTXT(GAMETXT("SHIELDLOW"),0);
          shld_low_played=1;
        }
      }
      else
      {
        shld_low_played=0;
      }
    }
  }

  
  
  if (flash>0)
  {
    flash-=KbdStat::time();
    if (flash<=0)
    {
      flash=0;
      set_power(Op::myship.destroy);
      SEND_DEBUG_MESSAGE("flash immortality disabled");
    }
  }
  if (morph_strong)
  {
    morph_strong_phase++;
    if (morph_strong_phase>=13)
    {
      morph_strong=0;
      morph_strong_phase=0;
    }
  }
  scrcatch->run(angle);
  update_phase(get_dir());
  if (get_chl()==Posit::UP)
  {
    Shadow::log_to_view();
  }
  else
  {
    Visible::log_to_view();
  }
  Visible::log_to_view(9);
  if (NoNet::is_network_mode()&&id!=NoNet::get_this_id()&&!is_invisible())
  {
    radar_log();
  }
}

void Myship::smoke_service(void)
{
  if (smoke)
  {
    smoke->set(x,y);
    smoke->set_power(myuser->get_life_percent());
  }
}

void Myship::check_alive(void)
{
  if (!myuser->is_alive())
  {
    if (kill_countdown==0)
    {
      SEND_DEBUG_MESSAGE("killed at position %d,%d by %d",x,y,last_killer_id);
      if (NoNet::is_network_mode())
      {
        if (Op::MYSHIPENABLEFUTERKO)
        {
          NEW(Skin(x,y,Posit::UP,id,myuser->get_weap_ammo(0),myuser->get_weap_ammo(1),myuser->get_weap_ammo(2),myuser->get_weap_ammo(3),myuser->get_weap_ammo(4),myuser->get_weap_ammo(5)),gobjects_mbn);
          SEND_DEBUG_MESSAGE("skin at position %d,%d created",x,y);
        }
        if (NoNet::check_id(last_killer_id)&&(last_killer_id!=id))
        {
          DBG_MESSAGE("(player %d, frame %d) - ship destroyed by player %d",id,KbdStat::local_frame(),last_killer_id);
          char buf[1024];
          if (id==world->get_current_displayed())
          {
            sprintf(buf,"you have been killed by %s",NoNet::get_player_name(last_killer_id));
            play_ship_sample(Mysound::speech_youbeenkilledby,SPEECH);
            play_ship_sample(Mysound::speech_colours[last_killer_id],SPEECH);
          }
          else if (last_killer_id==world->get_current_displayed())
          {
            sprintf(buf,"you have killed %s",NoNet::get_player_name(id));
            Myship *s=NoNet::get_ship(last_killer_id);
            s->play_ship_sample(Mysound::speech_youkilled,SPEECH);
            s->play_ship_sample(Mysound::speech_colours[id],SPEECH);
          }
          else        
          {
            sprintf(buf,"%s has been killed by %s",NoNet::get_player_name(id),NoNet::get_player_name(last_killer_id));
            Myship *s=NoNet::get_ship(world->get_current_displayed());
            s->play_ship_sample(Mysound::speech_colours[last_killer_id],SPEECH);
            s->play_ship_sample(Mysound::speech_haskilled,SPEECH);
            s->play_ship_sample(Mysound::speech_colours[id],SPEECH);
          }
          MSGTXT(buf,0);
          if (NoNet::is_network_mode(NoNet::NETWORKMATCH))
          {
            Statistics::add(last_killer_id);
            SEND_DEBUG_MESSAGE("frag is given to %d",last_killer_id);
          }
        }
        else
        {
          DBG_MESSAGE("(player %d, frame %d) - ship destroyed (suicide)",id,KbdStat::local_frame());
          char buf[1024];
          if (id==world->get_current_displayed())
          {
            sprintf(buf,"you have killed yourself");
            play_ship_sample(Mysound::speech_youkilledyourself,SPEECH);
          }
          else
          {
            sprintf(buf,"%s has killed himself",NoNet::get_player_name(id));
            Myship *s=NoNet::get_ship(world->get_current_displayed());
            s->play_ship_sample(Mysound::speech_colours[id],SPEECH);
            s->play_ship_sample(Mysound::speech_haskilledhimself,SPEECH);
          }
          MSGTXT(buf,0);
          if (NoNet::is_network_mode(NoNet::NETWORKMATCH))
          {
            Statistics::sub(id);
            SEND_DEBUG_MESSAGE("decreasing my frags counter (due to suicide)");
          }
        }
        if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
        {
          while (bricks>0)
          {
            throw_brick();
          }
          DBG_CHECK(bricks==0);
        }
        last_killer_id=0;
        SEND_DEBUG_MESSAGE("last killer id cleared");
      }
      stop();
      kill_countdown=4*Mp::METRONQUALITY;
      shake_screen(2*Mp::METRONQUALITY);
      mode=DYING;
      if (smoke)
      {
        smoke->set_power(100);
        smoke->stop();
      }
    }
    else
    {
      if (kill_countdown>2*Mp::METRONQUALITY)
      {
        if (kill_countdown==4*Mp::METRONQUALITY)
        {
          int posx=x;
          int posy=y;
          int splinters=6;
          int sound=1;
          NEW(Explode(posx,posy,get_chl(),splinters,Explode::AIR,Mysprites::expl_air,Explode::ENABLE,sound),gobjects_mbn);
        }
        else if (!(RAND%8))
        {
          int posx=x+RAND%50-25;
          int posy=y+RAND%50-25;
          int splinters=0;
          int sound=RAND%2;
          switch (RAND%3)
          {
            case 0: NEW(Explode(posx,posy,get_chl(),splinters,Explode::AIR,Mysprites::expl_air,Explode::ENABLE,sound),gobjects_mbn);
                    break;
            case 1: NEW(Explode(posx,posy,get_chl(),splinters,Explode::AIR,Mysprites::expl_air,Explode::ENABLE,sound),gobjects_mbn);
                    break;
            case 2: NEW(Explode(posx,posy,get_chl(),splinters,Explode::AIR,Mysprites::expl_air,Explode::ENABLE,sound),gobjects_mbn);
                    break;
           default: DBG_CHECK(0);
                    break;
          }
        }
      }
      kill_countdown-=KbdStat::time();
      if (kill_countdown<=0)
      {
        if (NoNet::is_network_mode())
        {
          if (!fadeing)
          {
            Fade::start(Fade::BOTH,GAMETXT("SHIP_RESPAWN"),id);
            fadeing=1;
          }
          else
          {
            if (Fade::is_faded(id))
            {
              if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
              {
                DBG_CHECK(bricks==0);                                          
              }
              reset_all_variables();
              kill_alreadykilled=0;
              fadeing=0;
              kill_countdown=0;
              kill_value=0;
              last_kill_value=0;
              last_killer_id=0;
              reset_cmsg();
              update_phase(get_dir());
              stop();
              find_startplace();
              height=0;
              angle=0;
              land_angle=0;
              start_angle=Mp::HALFANGLE;
              cloak=0;
              shield=0;
              cloak_warn_played=0;
              shield_warn_played=0;
              shld_low_played=0;
              shld_crit_played=0;
              immortal=0;
              flash=0;
              accelerating=0;
              morph_strong=0;
              morph_strong_phase=0;
              mode=WAITFORSTARTSIG;
              stop();
              scrcatch->reset();
              myuser->reset();
              weapon->reset();
              dispinfo->reset();
              smoke->set(x,y);
              smoke->set_power(myuser->get_life_percent());
              smoke->start();
              set_user_mode_params();
              reset_trailers();
              old_life=myuser->get_life();
              if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
              {
                DBG_CHECK(bricks==0);                                          
                life_brick_update();
              }
              DBG_MESSAGE("(player %d, frame %d) - ship respawned",id,KbdStat::local_frame());
              SEND_DEBUG_MESSAGE("respawned at position %d,%d",x,y);
            }
          }
        }
        else
        {
          if (!kill_alreadykilled)
          {
            gamemanager->set_restart(GameManager::KILL);
            kill_alreadykilled=1;
          }
        }
      }
    }
  }
  else
  {
    myuser->run();
    if (kill_countdown>0)
    {
      kill_countdown=0;
      set_user_mode_params();
      mode=USER;
    }
  }
}

void Myship::find_startplace(void)
{
  if (!NoNet::is_network_mode()||NoNet::is_network_mode(NoNet::NETWORKFLAG))
  {
    x=start_x;
    y=start_y;
  }
  else
  {
    if (NoNet::get_players_number()<=1)
    {
      DBG_CHECK(LevImp::land.size>0);
      int num=RAND%(LevImp::land.size-1)+1;
      x=world->get_level()[LevImp::land.place[num]].x;
      y=world->get_level()[LevImp::land.place[num]].y;
    }
    else
    {
      int start_id=-1;
      int max_dist=0;
      for (int i=1;i<LevImp::land.size;i++)
      {
        int start_x=world->get_level()[LevImp::land.place[i]].x;
        int start_y=world->get_level()[LevImp::land.place[i]].y;
        Myship *s=NoNet::get_ship(start_x,start_y,id);
        DBG_CHECK(s!=NULL);
        int dist=vect_len(start_x-s->get_x(),start_y-s->get_y());
        if (dist>max_dist)
        {
          max_dist=dist;
          start_id=i;
        }
      }
      DBG_CHECK(start_id>=0);
      x=world->get_level()[LevImp::land.place[start_id]].x;
      y=world->get_level()[LevImp::land.place[start_id]].y;
    }
  }
}

void Myship::set_user_mode_params(void)
{
  Move::set_max_speed(Op::myship.max_speed);
  Move::set_strafe_speed(Op::myship.strafe_speed);
  Turn::set_turn_speed(Op::myship.turn_speed);
}

void Myship::set_turbo_mode_params(void)
{
  Move::set_max_speed(Op::MYSHIPTURBOMAXSPEED);
  Move::set_strafe_speed(Op::MYSHIPTURBOSTRAFESPEED);
  Turn::set_turn_speed(Op::MYSHIPTURBOTURNSPEED);
}

void Myship::landing_on(int _x,int _y,int _angle)
{
  if (mode!=DYING)
  {
    SEND_DEBUG_MESSAGE("landing started at %d,%d",x,y);
    play(Mysound::ship_land);
    set_user_mode_params();
    accelerating=0;
    if (_angle==-1)
    {
      land_angle=Turn::get_angle(Mp::HALFANGLE);
    }
    else
    {
      land_angle=_angle;
    }
    land_x=_x;
    land_y=_y;
    mode=LANDING;
  }
}

void Myship::teleporting_on(int this_x,int this_y,int that_x,int that_y)
{
  if (mode!=DYING)
  {
    SEND_DEBUG_MESSAGE("teleporting started at %d,%d",x,y);
    Fade::start(Fade::BOTH,GAMETXT("TRANSMISSION"),id);
    teleport_phase=1;
    teleport_x=this_x;
    teleport_y=this_y;
    teleport_tox=that_x;
    teleport_toy=that_y;
    teleport_vx=vx;
    teleport_vy=vy;
    teleport_speed=get_speed();
    teleport_angle=get_angle();
    stop();
    accelerating=0;
    mode=TELEPORTING;
    FOR_ALL_USERS(ID)
    {
      if (ID!=id)
      {
        Myship *ship=NoNet::get_ship(ID);
        if (Posit::distance(ship->get_x(),ship->get_y())<=Op::TELEPORTSTARTDISTANCE&&ship->is_teleporting())
        {
          ship->kill(id);
        }
      }
    }
  }
}

void Myship::landing_off(int _angle)
{
  if (mode!=DYING)
  {
    if (_angle==-1)
    {
      start_angle=Turn::get_angle(Mp::HALFANGLE);
    }
    else
    {
      start_angle=_angle;
    }
    mode=WAITFORSTARTSIG;
    SEND_DEBUG_MESSAGE("landing phase finished");
  }
}

void Myship::run(void)
{
  DBG_CHECK(NoNet::check_id(id));
  if (!leaving_notified&&!NoNet::is_alive(id))
  {
    char buf[1024];
    if (id!=NoNet::get_server_id())
    {
      sprintf(buf,"%s has left the game",NoNet::get_player_name(id));
      MSGTXT(buf,0);
    }
    else
    {
      sprintf(buf,"server (%s) has left the game",NoNet::get_player_name(id));
      MSGTXT(buf,0);
      sprintf(buf,"now you are the only player");
      MSGTXT(buf,0);
    }
    FOR_ALL_USERS(ID)
    {
      if (ID!=id)
      {
        Myship *s=NoNet::get_ship(ID);
        s->play_ship_sample(Mysound::speech_colours[id],SPEECH);
        s->play_ship_sample(Mysound::speech_leftgame,SPEECH);
      }
    }
    SEND_DEBUG_MESSAGE("left the game");
    leaving_notified=1;
  }
  SEND_DEBUG_MESSAGE("frame started, mode %d",(int)mode);
  shake_service();
  switch (mode)
  {
           case DYING: reset_cmsg();
                       scrcatch->run(-1);
                       dispinfo_service();
                       check_alive();
                       Visible::log_to_view(9);
                       break;
     case TELEPORTING: reset_cmsg();
                       dispinfo_service();
                       radar_service();
                       weapon_service();
                       teleporting_service();
                       display_service();
                       break;
         case LANDING: reset_cmsg();
                       smoke_service();
                       landing_service();
                       radar_service();
                       dispinfo_service();
                       weapon_service();
                       timer_service();
                       check_alive();
                       display_service();
                       break;
 case WAITFORSTARTSIG: reset_cmsg();
                       timer_service();
                       check_alive();
                       if (KbdStat::start(id)&&!Fade::in_use()&&!Fade::in_use(id))
                       {
                         if (!NoNet::is_network_mode())
                         {
                           if (first_start)
                           {
                             if (Radar::enable())
                             {
                               start_angle=Turn::get_angle(Radar::TargetX,Radar::TargetY);
                             }
                             else
                             {
                               start_angle=Turn::get_angle(Mp::HALFANGLE);
                             }
                             first_start=0;
                           }
                         }
                         else
                         {
                           if (first_start)
                           {
                             start_angle=Turn::get_angle(Mp::HALFANGLE);
                             first_start=0;
                           }
                         }
                         SEND_DEBUG_MESSAGE("starting started at %d,%d",x,y);
                         mode=STARTING;
                         play(Mysound::ship_start);
                       }
                       smoke_service();
                       radar_service();
                       dispinfo_service();
                       weapon_service();
                       display_service();
                       break;
        case STARTING: reset_cmsg();
                       timer_service();
                       check_alive();
                       smoke_service();
                       starting_service();
                       weapon_service();
                       dispinfo_service();
                       display_service();
                       radar_service();
                       break;
           case TURBO:
            case USER: steering_service();
                       timer_service();
                       check_col();
                       check_alive();
                       smoke_service();
                       inventory_service();
                       radar_service();
                       weapon_service();
                       dispinfo_service();
                       display_service();
                       netcheating_service();
                       break;
              default: DBG_CHECK(0);
                       break;
  }
  update_trailers();
  SEND_DEBUG_MESSAGE("frame made, mode %d",(int)mode);
}

void Myship::update_trailers(void)
{
  for (int i=0;i<2;i++)
  {
    trailers[i].x=trailers[i+1].x;
    trailers[i].y=trailers[i+1].y;
    trailers[i].phase=trailers[i+1].phase;
  }
  trailers[2].x=x;
  trailers[2].y=y;
  trailers[2].phase=get_phase();
}

void Myship::reset_trailers(void)
{
  for (int i=0;i<3;i++)
  {
    trailers[i].x=x;
    trailers[i].y=y;
    trailers[i].phase=get_phase();
  }
}

void Myship::send_debug_message(char *format,...)
{
  if (Mp::SHIPDEBUGLEVEL>0)
  {
    char buffer[4096];
    va_list argptr;
    va_start(argptr,format);
    vsprintf(buffer,format,argptr);
    va_end(argptr);
    DBG_CHECK(strlen(buffer)<DebugData::msg_len);
    buffer[DebugData::msg_len-1]='\0';
    debug_service(this,buffer);
  }
}

void Myship::draw_normal(Screen &screen)
{
  Sprite &spr=(shield>0)?Mysprites::ship_shield:get_sprite();
  if (mode==TURBO)
  {
    draw_trailers(screen);
  }
  if (accelerating==1)
  {
    draw_engine(screen);
  }
  if (NoNet::is_network_mode())
  {
    screen.shape(x,y,(shield>0)?Mysprites::ship_border_strong:Mysprites::ship_border,get_phase(),Spr::nomirror,(void*)&Tools::colors[Tools::NET_FIRST-1+id]);
  }
  if (morph_strong)
  {
    DBG_CHECK(morph_strong_phase<=12);
    Sprite &from=(shield<=0)?Mysprites::ship_shield:get_sprite();
    Sprite &to=(shield>0)?Mysprites::ship_shield:get_sprite();
    if (morph_strong_phase<=4)
    {
      screen.put(x,y,from,get_phase());
      if (flash>0)
      {
        screen.shape(x,y,from,get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::LIGHT]);
      }
      screen.put(x,y,to,get_phase(),Spr::with_tsphard,(void*)&tsp);
    }
    else if (morph_strong_phase<=8)
    {
      screen.put(x,y,from,get_phase(),Spr::with_tspsoft,(void*)&tsp);
      if (flash>0)
      {
        screen.shape(x,y,from,get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::LIGHT]);
      }
      screen.put(x,y,to,get_phase(),Spr::with_tspsoft,(void*)&tsp);
    }
    else if (morph_strong_phase<=12)
    {
      screen.put(x,y,from,get_phase(),Spr::with_tsphard,(void*)&tsp);
      screen.put(x,y,to,get_phase());
      if (flash>0)
      {
        screen.shape(x,y,to,get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::LIGHT]);
      }
    }
  }
  else
  {
    if (flash>0)
    {
      screen.put(x,y,spr,get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::LIGHT]);
    }
    else
    {
      screen.put(x,y,spr,get_phase());
    }
  }
  weapon->draw_object(screen,get_plane(),(flash>0),0,NULL);
}

void Myship::draw_cloak(Screen &screen)
{
  DBG_CHECK(cloak>0);
  Sprite &spr=(shield>0)?Mysprites::ship_shield:get_sprite();
  Sprite &spr1=(shield<=0)?Mysprites::ship_shield:get_sprite();
  if (accelerating==1)
  {
    draw_engine(screen);
  }
  if (NoNet::is_network_mode())
  {
    screen.shape(x,y,(shield>0)?Mysprites::ship_border_strong:Mysprites::ship_border,get_phase(),Spr::nomirror,(void*)&Tools::colors[Tools::NET_FIRST-1+id]);
  }
  if (morph_strong&&morph_strong_phase<=6)
  {
    screen.put(x,y,spr1,get_phase(),Spr::with_tsphard,(void*)&tsp);
  }
  screen.put(x,y,spr,get_phase(),Spr::with_tsphard,(void*)&tsp);
  if (flash>0)
  {
    screen.shape(x,y,spr,get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::LIGHT]);
  }
  weapon->draw_object(screen,get_plane(),(flash>0),1,NULL);
}

void Myship::draw_trailers(Screen &screen)
{
  Sprite &spr=(shield>0)?Mysprites::ship_shield:get_sprite();
  screen.put(trailers[0].x,trailers[0].y,spr,trailers[0].phase,Spr::with_tsphard,(void*)&tsp);
  screen.put(trailers[1].x,trailers[1].y,spr,trailers[1].phase,Spr::with_tspsoft,(void*)&tsp);
}

void Myship::draw_engine(Screen &screen)
{
  DBG_CHECK(accelerating==1);
  if (engine_flipper)
  {
    screen.put(x,y,Mysprites::ship_exhaust1,get_phase(),Spr::with_tsphard,(void*)&tsp);
  }
  else
  {
    screen.put(x,y,Mysprites::ship_exhaust2,get_phase(),Spr::with_tsphard,(void*)&tsp);
  }
}

void Myship::draw_object(Screen& screen,int plane)
{
  int display_cloak=(cloak>=Mp::METRONQUALITY*5||(2*cloak/Mp::METRONQUALITY)%2);
  int display_shield=(shield>=Mp::METRONQUALITY*5||(2*shield/Mp::METRONQUALITY)%2);
  if (plane!=9)
  {
    if (get_chl()==Posit::DOWN)
    {
      draw_shadow(screen,plane);
    }

    if (cloak>0)
    {
      if (id==NoNet::get_this_id())
      {
        if (display_cloak)
        {
          draw_cloak(screen);
        }
        else
        {
          draw_normal(screen);
        }
      }
    }
    else
    {
      draw_normal(screen);
    }
  }
  else if (id==world->get_current_displayed())
  {
    if (mode!=DYING)
    {
      if (id==NoNet::get_this_id())
      {
        Radar::draw(screen);
      }
      if (KbdStat::steering_mode(id)==1)
      {
        int orgx=screen.ox;
        int orgy=screen.oy;
        screen.origin(0,0);
        screen.put(KbdStat::mouse_x(id),KbdStat::mouse_y(id),Mysprites::spot);
        screen.origin(orgx,orgy);
      }
    }
    dispinfo->draw_object(screen,plane);
  }
}

void Myship::draw_shadow(Screen& screen,int plane)
{
  Sprite &spr=(shield>0)?Mysprites::ship_shield:get_sprite();
  int display_cloak=(cloak>=Mp::METRONQUALITY*5||(2*cloak/Mp::METRONQUALITY)%2);
  if (cloak>0)
  {
    if (id==NoNet::get_this_id())
    {
      if (display_cloak)
      {
        screen.shape(get_down_x(),get_down_y()+Mp::SHADOWDIST,spr,get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::LSHADOW]);
      }
      else
      {
        screen.shape(get_down_x(),get_down_y()+Mp::SHADOWDIST,spr,get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
      }
    }
  }
  else
  {
    screen.shape(get_down_x(),get_down_y()+Mp::SHADOWDIST,spr,get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
  }
}

void Myship::add_cmsg(Cmsg* message,int _id)
{
  if ((message->WALL&&_id!=id))
  {
    return;
  }
  LowColis::add_cmsg(message,_id);
}

//KLASA SKIN
Skin::Skin(int _x,int _y,int _chl,int _id,int weap1,int weap2,int weap3,int weap4,int weap5,int weap6):
Posit(_x,_y,_chl),
Shadow(Mysprites::ship_skin,View::MYCAP,0),
Listmanager(OMOBJ),
Colis(C_COLLECT,0,"skin")
{
  DBG_CHECK(NoNet::is_network_mode());
  id=_id;
  not_active=Mp::METRONQUALITY/4;
  ship_weap[0]=weap1;
  ship_weap[1]=weap2;
  ship_weap[2]=weap3;
  ship_weap[3]=weap4;
  ship_weap[4]=weap5;
  ship_weap[5]=weap6;
}

void Skin::run(void)
{
  if (!is_up())
  {
    go_up();
  }
  if (not_active<=0)
  {
    for (Cmsg *temp=get_cmsg();temp;temp=get_cmsg())
    {
      if (temp->MYSHIP&&temp->MY)
      {
        NoNet::get_ship(temp->MYSHIP)->take_skin(this);
        KILLME
      }
    }
    write(-1,0,0,get_sprite(),get_phase());
  }
  else
  {
    not_active-=KbdStat::time();
    if (not_active<=0)
    {
      not_active=0;
    }
  }
  plus_phase();
  log_to_view();
}
//KLASA CAPSULE
Capsule::Capsule(int _x,int _y,int _chl,Capsule::Type _type,int _power):
Posit(_x,_y,_chl),
Shadow(Mysprites::capsule,View::MYCAP,0),
Listmanager(OMOBJ),
Colis(C_COLLECT,0,"capsule")
{
  type=_type;
  DBG_CHECK(type!=BRICK);
  DBG_CHECK(type>_FIRST&&type<_LAST);
  if (type==ALTERING)
  {
    type=WEAP0;
    altering=1;
  }
  else
  {
    altering=0;
  }
  power=_power;
  to_respawn=0;
  phs_changed=0;
  amount=get_amount(type,power);
  adjust_range();
  adjust_respawntime();
}

void Capsule::adjust_respawntime(void)
{
  DBG_CHECK(type>_FIRST&&type<_LAST);
  if (!NoNet::is_network_mode())
  {
    respawn_time=0;
  }
  else
  {
    RegData::RespawnInfo &info=(NoNet::is_network_mode(NoNet::NETWORKMATCH))?RegData::RespawnDM:RegData::RespawnBB;
    if (altering)
    {
      respawn_time=(info.Flags[RegData::resp_Random])?info.PowerUpTime*Mp::METRONQUALITY:0;
    }
    else
    {
      switch (type)
      {
        case WEAP0: respawn_time=(info.Flags[RegData::resp_UpVulcan])?info.PowerUpTime*Mp::METRONQUALITY:0;
                    break;
        case WEAP1: respawn_time=(info.Flags[RegData::resp_UpSwarmers])?info.PowerUpTime*Mp::METRONQUALITY:0;
                    break;
        case WEAP2: respawn_time=(info.Flags[RegData::resp_UpPlasma])?info.PowerUpTime*Mp::METRONQUALITY:0;
                    break;
        case WEAP3: respawn_time=(info.Flags[RegData::resp_UpMissile])?info.PowerUpTime*Mp::METRONQUALITY:0;
                    break;
        case WEAP4: respawn_time=(info.Flags[RegData::resp_UpCanon])?info.PowerUpTime*Mp::METRONQUALITY:0;
                    break;
        case WEAP5: respawn_time=(info.Flags[RegData::resp_UpBomb])?info.PowerUpTime*Mp::METRONQUALITY:0;
                    break;
         case LIFE: respawn_time=(info.Flags[RegData::resp_UpShield])?info.PowerUpTime*Mp::METRONQUALITY:0;
                    break;
      case MEDIKIT: respawn_time=(info.Flags[RegData::resp_ShieldRecharge])?info.ItemTime*Mp::METRONQUALITY:0;
                    break;
        case CLOAK: respawn_time=(info.Flags[RegData::resp_CloakingDevice])?info.ItemTime*Mp::METRONQUALITY:0;
                    break;
       case IMMORT: respawn_time=(info.Flags[RegData::resp_DoubledShield])?info.ItemTime*Mp::METRONQUALITY:0;
                    break;
      case NETMINE: respawn_time=(info.Flags[RegData::resp_Mine])?info.ItemTime*Mp::METRONQUALITY:0;
                    break;
    case GOLDENEYE: respawn_time=(info.Flags[RegData::resp_MagneticShock])?info.ItemTime*Mp::METRONQUALITY:0;
                    break;
        case CHAOS: DBG_CHECK(!NoNet::is_network_mode());
                    respawn_time=0;
                    break;
           default: DBG_CHECK(0);
                    break;
      }
    }
  }
}

void Capsule::adjust_range(void)
{
  DBG_CHECK(power>=0&&power<=2);
  DBG_CHECK(type>_FIRST&&type<_LAST);
  DBG_CHECK(type!=BRICK);
  int beg=0;
  int cnt=0;
  switch (type)
  {
    case WEAP0: beg=Op::W1BEG[power];
                cnt=Op::W1CNT[power];
                break;
    case WEAP1: beg=Op::W2BEG[power];
                cnt=Op::W2CNT[power];
                break;
    case WEAP2: beg=Op::W3BEG[power];
                cnt=Op::W3CNT[power];
                break;
    case WEAP3: beg=Op::W4BEG[power];
                cnt=Op::W4CNT[power];
                break;
    case WEAP4: beg=Op::W5BEG[power];
                cnt=Op::W5CNT[power];
                break;
    case WEAP5: beg=Op::W6BEG[power];
                cnt=Op::W6CNT[power];
                break;
     case LIFE: beg=Op::LIBEG[power];
                cnt=Op::LICNT[power];
                break;
  case MEDIKIT: beg=Op::MKBEG;
                cnt=Op::MKCNT;
                break;
    case CLOAK: beg=Op::CLBEG;
                cnt=Op::CLCNT;
                break;
   case IMMORT: beg=Op::IMBEG;
                cnt=Op::IMCNT;
                break;
  case NETMINE: beg=Op::NMBEG;
                cnt=Op::NMCNT;
                break;
case GOLDENEYE: beg=Op::GEBEG;
                cnt=Op::GECNT;
                break;
    case CHAOS: beg=Op::CHBEG;
                cnt=Op::CHCNT;
                break;
       default: DBG_CHECK(0);
                break;
  }
  DBG_CHECK(beg>=0);
  DBG_CHECK(cnt>0);
  set_range(beg,beg+cnt-1);
  update_phase(0);
}

void Capsule::collected(void)
{
  DBG_CHECK(NoNet::is_network_mode());
  to_respawn=respawn_time;
}

int Capsule::get_amount(Capsule::Type _type,int power)
{
  DBG_CHECK(power>=0&&power<=2);
  DBG_CHECK(_type>Capsule::_FIRST&&_type<Capsule::_LAST);
  DBG_CHECK(_type!=BRICK);
  int amount=0;
  switch (_type)
  {
      case WEAP0:
      case WEAP1:
      case WEAP2:
      case WEAP3:
      case WEAP4:
      case WEAP5: switch (power)
                  {
                    case 0: amount=Op::weapon_ugr_min[(int)_type-(int)Capsule::WEAP0];
                            break;
                    case 1: amount=Op::weapon_ugr_med[(int)_type-(int)Capsule::WEAP0];
                            break;
                    case 2: amount=Op::weapon_ugr_max[(int)_type-(int)Capsule::WEAP0];
                            break;
                  }
                  break;
       case LIFE: switch (power)
                  {
                    case 0: amount=Op::shields[0];
                            break;
                    case 1: amount=Op::shields[1];
                            break;
                    case 2: amount=Op::shields[2];
                            break;
                  }
                  break;
    case MEDIKIT: amount=1;
                  break;
      case CLOAK: amount=1;
                  break;
     case IMMORT: amount=1;
                  break;
    case NETMINE: amount=5;
                  break;
  case GOLDENEYE: amount=1;
                  break;
      case CHAOS: amount=1;
                  break;
         default: DBG_CHECK(0);
                  break;
  }
  return amount;
}

int Capsule::_run(void)
{
  if (to_respawn>0)
  {
    to_respawn-=KbdStat::time();
    if (to_respawn<=0)
    {
      play(Mysound::respawning);
    }
  }
  else
  {
    if (!is_up())
    {
      go_up();
    }
    for (Cmsg *temp=get_cmsg();temp;temp=get_cmsg())
    {
      if (temp->MYSHIP&&temp->MY)
      {
        int to_collect=0;
        if (NoNet::get_ship(temp->MYSHIP)->collect_capsule(type,amount,power,altering))
        {
          return 1;
        }
      }
    }
    write(-1,0,0,get_sprite(),get_phase());
    if (phs_changed>=2)
    {
      int old_phase=get_phase();
      plus_phase();
      phs_changed=0;
      if (altering&&(get_phase()-old_phase!=1))
      {
        switch (type)
        {
          case WEAP0: type=WEAP1;
                      break;
          case WEAP1: type=WEAP2;
                      break;
          case WEAP2: type=WEAP3;
                      break;
          case WEAP3: type=WEAP4;
                      break;
          case WEAP4: type=WEAP5;
                      break;
          case WEAP5: type=LIFE;
                      break;
           case LIFE: type=WEAP0;
                      break;
             default: type=WEAP0;
                      break;
        }
        amount=get_amount(type,power);
        adjust_range();
        adjust_respawntime();
      }
    }
    else
    {
      phs_changed++;
    }
    log_to_view();
  }
  return 0;
}

void Capsule::run(void)
{
  if (_run())
  {
    if (NoNet::is_network_mode()&&respawn_time>0)
    {
      collected();
    }
    else
    {
      KILLME
    }
  }
}
//KLASA LEVELCAPSULE
LevelCapsule::LevelCapsule(int _x,int _y,int _chl,int _lev_num,Capsule::Type _type,int _power):
Capsule(_x,_y,_chl,_type,_power),
Impexp(_lev_num),
Posit(_x,_y,_chl)
{
}

void LevelCapsule::run(void)
{
  if (!LevImp::in_crange(lobject->type,x,y)&&to_respawn<=0)
  {
    update_posit(Posit::x,Posit::y);
    KILLME
  }
  if (_run())
  {
    if (NoNet::is_network_mode()&&respawn_time>0)
    {
      collected();
    }
    else
    {
	    Link::send_impulse(level_num);
      remove_from_level();
      KILLME
    }
  }
}
//KLASA BRICK
int Brick::initialized=0;
NoQueue<int> *Brick::queue=NULL;

void Brick::init(void)
{
  DBG_CHECK(!initialized);
  initialized=1;
  if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
  {
    queue=NEW(NoQueue<int>(Mp::MAXBRICKS,"bricks",1),queue_mbn);
    DBG_CHECK(queue!=NULL);
  }
}

void Brick::quit(void)
{
  if (initialized)
  {
    if (queue!=NULL)
    {
      delete queue;
      queue=NULL;
    }
  }
  initialized=0;
}

int Brick::get_num(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(NoNet::is_network_mode(NoNet::NETWORKFLAG));
  DBG_CHECK(queue!=NULL);
  return (*Brick::queue->head());
}

Brick::Brick(int _x,int _y,int _chl,int _angle,int _speed,int _level_num,int _not_active):
Posit(_x,_y,_chl),
Move(_angle,_speed,Op::brick),
Impexp((_level_num!=-1)?_level_num:Brick::get_num()),
Shadow(Mysprites::brick,View::MYCAP,0),
Listmanager(OMOBJ),
Colis(C_COLLECT,0,"brick")
{
  DBG_CHECK(initialized);
  DBG_CHECK(NoNet::is_network_mode(NoNet::NETWORKFLAG));
  phs_changed=0;
  set_phase(RAND%get_sprite().phases);
  if (_not_active)
  {
    not_active=Mp::METRONQUALITY;
  }
  else
  {
    not_active=0;
  }
}

void Brick::wall(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(NoNet::is_network_mode(NoNet::NETWORKFLAG));
  Cmsg* temp=NULL;
  int t=0;
  int _x=0;
  int _y=0;
  int dx=0;
  int dy=0;
  int csize=20;
  int id=0;

  FOR_ALL_USERS(ID)
  {
    if (Posit::is_in_overscan(-25,x,y,ID))
    {
      id=ID;
      break;
    }
  }

  if (id)
  {
    for (int i=0;i<Mp::TURNQUALITY;i+=16)
    {
      _x=x+x_offset(i,csize);
      _y=y+y_offset(i,csize);
      if (Posit::is_in_overscan(0,_x,_y,id))
      {
        temp=LowColis::see(id,_x,_y,0);
        if (temp&&temp->WALL)
        {
          dx-=x_offset(i,csize);
          dy-=y_offset(i,csize);
        }
      }
      else
      {
        stop();
        return;
      }

    }
    if (!dx&&!dy)
    {
      t=-1;
    }
    else
    {
      t=Turn::get_dangle((int)dx,(int)dy);
    }

    if (t!=-1)
    {
      int speed_angle=get_dangle(vx,vy);
      int wangle;
      if (abs(sub_angle(speed_angle,t))<Mp::QUARTANGLE) return ;
      else  wangle=t+sub_angle(normalize_angle(speed_angle+Mp::HALFANGLE),t);
      wangle=normalize_angle(wangle);
      transform_speed(wangle);
      update_speed();
    }
  }
  else
  {
    stop();
  }

}

int Brick::_run(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(NoNet::is_network_mode(NoNet::NETWORKFLAG));
  if (not_active>=0)
  {
    not_active-=KbdStat::time();
  }
  if (!is_up())
  {
    go_up();
  }

  if (!stopped())
  {
    wall();
    go();
    brake();
  }

  if ((not_active<=0)&&is_up())
  {
    for (Cmsg *temp=get_cmsg();temp;temp=get_cmsg())
    {
      if (temp->MYSHIP&&temp->MY)
      {
        int to_collect=0;
        if (NoNet::get_ship(temp->MYSHIP)->collect_capsule(Capsule::BRICK,1))
        {
          return 1;
        }
      }
    }
    write(-1,0,0,get_sprite(),get_phase());
  }


  if (phs_changed>=2)
  {
    plus_phase();
    phs_changed=0;
  }
  else
  {
    phs_changed++;
  }      
  log_to_view();

  return 0;
}

void Brick::run(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(NoNet::is_network_mode(NoNet::NETWORKFLAG));
  DBG_CHECK(queue!=NULL);
  if (!LevImp::in_crange(lobject->type,x,y))
  {
    update_posit(Posit::x,Posit::y);
    KILLME
  }
  if (_run())
  {
    remove_from_level();
    queue->add(&level_num);
    KILLME
  }
}
//KLASA TELEPORT
Teleport::Teleport(int _x,int _y,int _lev_num,int _invisible):
Posit(_x,_y,Posit::UP),
Shadow(Mysprites::telep_d,View::MYFIRST),
Listmanager(OTELEPORT),
Link(_lev_num),
Impexp(_lev_num)
{
  next_link();
  memset((void*)served,0,sizeof(served));
  invisible=_invisible;
  phs_changed=0;
}

void Teleport::run()
{
  FOR_ALL_USERS(ID)
  {
    Myship *ship=NoNet::get_ship(ID);
    if (!served[ID])
    {
      if (cool_dist(ship))
      {
        if (ship->is_teleporting())
        {
          served[ID]=1;
        }
        else
        {
          ship->teleporting_on(x,y,link_x(),link_y());
        }
      }
    }
    else
    {
      if (!locked_dist(ship))
      {
        served[ID]=0;
      }
    }
  }
  if (!invisible)
  {
    if (++phs_changed>1)
    {
      phs_changed=0;
      plus_phase();
    }
    Visible::log_to_view(Visible::get_plane()+2);
    Shadow::log_to_view(Visible::get_plane());
  }
}

int Teleport::cool_dist(Myship* ship)
{
  return (Posit::distance(ship->get_x(),ship->get_y())<=Op::TELEPORTSTARTDISTANCE);
}

int Teleport::locked_dist(Myship* ship)
{
  return (Posit::distance(ship->get_x(),ship->get_y())<=Op::TELEPORTINUSEDISTANCE);
}

void Teleport::draw_object(Screen& screen,int plane)
{
  if (plane==Visible::get_plane())
  {
    Visible::draw_object(screen,plane);
  }
  else
  {
    screen.put(x,y,Mysprites::telep_u,get_phase());
  }
}

void Teleport::draw_shadow(Screen& screen,int plane)
{
  screen.shape(x,y+20+height+Mp::SHADOWDIST,Mysprites::telep_u,get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
}
//KLASA LANDPLACE
Landplace::Landplace(int _x,int _y,Sprite& _sprite,int _phase,int _lev_num):
Posit(_x,_y,Posit::DOWN),
Radar(world->get_level()[_lev_num].type,world->get_level()[_lev_num].user.short1),
Visible(_sprite,View::MYFIRST,_phase),
Listmanager(OMOBJ),
Impexp(_lev_num)
{
  ship=NULL;
  order=CLEARWAIT;
  if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
  {
    int t=lobject->type;
    Radar::set_cur_type(&t,1);
  }

}

void Landplace::run()
{
  switch (order)
  {
        case READY: {
                      if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
                      {
                        int found=0;
                        int owner_id=lobject->user.short1;
                        FOR_ALL_USERS(ID)
                        {
                          if (owner_id==ID)
                          {
                            found=1;
                          }
                        }
                        if (found)
                        {
                          ship=NoNet::get_ship(owner_id);
                        }
                        else
                        {
                          break;
                        }
                      }
                      else
                      {
                        ship=NoNet::get_ship(x,y);
                      }
                      if (ship!=NULL&&is_ship_up())
                      {
                        ship->landing_on(x,y);
                        order=COME;
                      }
                    }
                    break;
         case COME: {
                      if (ship->get_landed())
                      {
                        if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
                        {
                          if (Mp::SHIPDEBUGLEVEL>0)
                          {
                            ship->send_debug_message("ship landed at base, bricks count %d",ship->bricks_numb());
                          }
                        }
                        else
                        {
                          Handle::post(GMANAGER,GameManager::code(lobject->type,LAND));
                        }
                        time=Op::LANDTIME;
                        order=BUSY;
                      }
                    }
                    break;
         case BUSY: {
                      if (time>0)
                      {
                        time-=KbdStat::time();
                      }
                      else
                      {
                        order=GO;
                      }
                    }
                    break;
           case GO: {
                      ship->landing_off();
                      order=CLEARWAIT;
                    }
                    break;
    case CLEARWAIT: {
                      ship=NoNet::get_ship(x,y);
                      if (is_ship_gone())
                      {
                        order=READY;
                      }
                    }
                    break;
  }
  Visible::log_to_view();
}

int Landplace::is_ship_up(void)
{
  if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
  {
    return (Posit::distance(ship->get_down_x(),ship->get_down_y())<=Op::LANDDISTANCE&&
      ship->get_speed()<=Op::LANDSPEED&&Statistics::get_land(ship->get_id()));
  }
  else if (NoNet::is_network_mode(NoNet::NETWORKMATCH))
  {
    return 0;
  }
  else
  {
    return (gamemanager->check_cur_type(lobject->type)&&Posit::distance(ship->get_down_x(),ship->get_down_y())<=Op::LANDDISTANCE&&
            ship->get_speed()<=Op::LANDSPEED);
  }
}

int Landplace::is_ship_gone(void)
{
  return (Posit::distance(ship->get_down_x(),ship->get_down_y())>Op::LANDDISTANCE);
}

int Landplace::is_ship_to_prompt(void)
{
  return (Posit::distance(ship->get_down_x(),ship->get_down_y())<=2*Op::LANDDISTANCE);
}

//MYCANNON
#define MyCannonAngle      LevImp::get_parameters(lobject->type)->blocked

MyCannon::MyCannon(int _x,int _y,Sprite& _sprite,int _level_num,int _plane,int _id,int _type,int _level):
Posit(_x,_y,Posit::UP),
Visible(_sprite,View::ASTAT,world->get_level()[_level_num].phase),
Colis(C_MYSTATIC(_id),0,"MyCannon"),
Listmanager(OSTAT),
Life(200),
Turn(0,(type==1)?Op::NETCANNON1:Op::NETCANNON2),
Impexp(_level_num)
{
  if (type==1)
  {
    active=Op::NETCANNON1ACTIVE;
    passive=Op::NETCANNON1PASSIVE;
  }
  else
  {
    active=Op::NETCANNON2ACTIVE;
    passive=Op::NETCANNON2PASSIVE;
  }
  lobject->aux2=-1;
  type=_type;
  level=_level;
  shtime=time=mtime=0;
  angle=startangle=get_dir_angle(get_phase());
  plane=_plane;
  id=_id;
  morph_level=sign=1;
  order=0;
  shoot=0;
}

MyCannon::~MyCannon(void)
{
  if (get_life()>0) lobject->aux2=0;
}


void MyCannon::service(void)
{
  if (!ship) return;
  if (ship->is_invisible()) return;
  if (MyCannonAngle==Mp::HALFANGLE)
  {
    turn_point(ship->get_x(),ship->get_y());
    update_phase(get_dir());
    return;
  }
  int opp=normalize_angle(startangle+Mp::HALFANGLE);
  int shipang=get_dangle(ship->get_x()-x,ship->get_y()-y);

  if (::sign(sub_angle(opp,angle))==::sign(sub_angle(opp,shipang))||angle==startangle)
  {
    turn_angle(shipang);
  }
  else
  {
    turn_angle(startangle);
  }
  int sgn=sub_angle(startangle,angle);
  if (abs(sgn)>MyCannonAngle)
  {
    if (sgn<0) angle=normalize_angle(startangle-MyCannonAngle);
    else angle=normalize_angle(startangle+MyCannonAngle);
  }
  update_phase(get_dir());
}

void MyCannon::bang(void)
{
  if (!ship) return;
  if (!(distance(ship->get_x(),ship->get_y())<350)) return;
  int t=!(abs(sub_angle(get_angle(ship->get_x(),ship->get_y()),startangle))<=MyCannonAngle);
  if (ship->is_invisible()||t) return;
  if (type==2)
  {
    shtime+=KbdStat::time();
    if (shtime>Op::CANNONSPLASHFRQ)
    {
      play(Mysound::bullet);
      int tx=lobject->x+(get_sprite()[get_phase()].r-Mysprites::cannonpoint1[get_phase()].r);
      int ty=lobject->y+(get_sprite()[get_phase()].d-Mysprites::cannonpoint1[get_phase()].d);
      NEW(Splash(tx,ty,angle,id,Mysprites::cannonsplash,1),gobjects_mbn);
      shtime=0;
    }
  }
  else
  {
    if (active/2<(time-passive)&&!shoot)
    {
      play(Mysound::cannonrocket);
      int tx=lobject->x+(get_sprite()[get_phase()].r-Mysprites::cannonpoint2[get_phase()].r);
      int ty=lobject->y+(get_sprite()[get_phase()].d-Mysprites::cannonpoint2[get_phase()].d);
      NEW(MyRocket(tx,ty,angle,id,1),gobjects_mbn);
      shoot=1;
    }
  }
}

void MyCannon::serv_kill(void)
{
  //reset_cmsg();
  //Colis::write(-1,0,0,get_sprite(),get_phase());

  if (get_life()<=0)
  {
    E_BIG;
    remove_from_level();
    KILLME
  }
}

void MyCannon::morph(void)
{
  mtime+=KbdStat::time();
  if (lobject->status<level&&!morph_level)
  {
    sign=-1;
    mtime=0;
    morph_level=1;
  }

  if (morph_level&&mtime>300)
  {
    morph_level++;
    if (morph_level>3) morph_level=0;
  }
}

void MyCannon::run(void)
{
  ship=NoNet::get_ship(Posit::x,Posit::y,id);
  time+=KbdStat::time();
  morph();
  service();
  if (time>active)
  {
    if (time>active+passive)
    {
      shoot=time=0;
    }
    bang();
  }
  if (lobject->status<level&&!morph_level)
  {
    remove_from_level();
    KILLME
  }
  serv_kill();
  log_to_view();
}


void MyCannon::draw_object(Screen& screen,int plane)
{
  switch(morph_level)
  {
    case 1: if (sign<=0) screen.put(x,y,get_sprite(),get_phase());
            if (sign>=0) screen.put(x,y,get_sprite(),get_phase(),Spr::with_tsphard,(void*)&tsp);
            break;
    case 2: if (sign<=0) screen.put(x,y,get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp);
            if (sign>=0) screen.put(x,y,get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp);
            break;
    case 3: if (sign<=0) screen.put(x,y,get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp);
            if (sign>=0) screen.put(x,y,get_sprite(),get_phase());
            break;
    case 0: screen.put(x,y,get_sprite(),get_phase());
            break;
  }
  //screen.print(x,y,"level=%d,status=%d",level,lobject->status);
}

//BASE
Base::Base(int _x,int _y,Sprite& _sprite,int _lev_num,int _id):
Posit(_x,_y,Posit::DOWN),
Visible(_sprite,View::ASTAT),
Listmanager(OSTAT),
Impexp(_lev_num),
Radar(world->get_level()[_lev_num].type,_id)
{
  sgn=brick_count=morph_level=time=0;
  lobject->status=0;
  id=_id;
  set_phase(0);
  bcount_p_phs=Op::MAXSHIPBRICK/get_phases_num();
}


void Base::impulse(int id,int bcount)
{
//  DBG_CHECK(LevImp::land.size>id-1);
  Statistics::update(id,bcount);
}

#define BaseChLevel   LevImp::get_parameters(world->get_level()[i].type)->smoke

void Base::build_cannon(int st)
{
  Level* lev=&world->get_level();
  int prv=Link::prev_lnk(level_num);
  for (int i=world->get_level()[level_num].link;i&&i!=prv;i=world->get_level()[i].link)
  {
    if (!i)  break;
    world->get_level()[i].status=(short)st;
    int h=BaseChLevel;
    if (h<=st&&world->get_level()[i].aux2!=-1)
    {
      world->get_level()[i].active=1;
      world->get_level().update(i);
    }
  }
}

void Base::run(void)
{
  time+=KbdStat::time();
  brick_count=Statistics::get(id);


  if ((brick_count/bcount_p_phs)>get_phase()&&!morph_level&&get_phase()<get_phases_num()-1)
  {
    sgn=1;
    morph_level=1;
    time=0;
  }

  if ((brick_count/bcount_p_phs)<get_phase()&&!morph_level&&get_phase()>0)
  {
    sgn=-1;
    morph_level=1;
    time=0;
  }

  if (morph_level&&time>300)
  {
    morph_level++;
    time=0;
    if (morph_level>=4)
    {
      morph_level=0;
      if (sgn>0&&get_phase()<get_phases_num()-1) plus_phase();
      if (sgn<0) minus_phase();
      build_cannon(get_phase());
    }
  }
  log_to_view();
}

void Base::draw_object(Screen& screen,int plane)
{
  switch(morph_level)
  {
    case 1: screen.put(x,y,get_sprite(),get_phase());
            screen.put(x,y,get_sprite(),get_phase()+sgn,Spr::with_tsphard,(void*)&tsp);
            break;
    case 2: screen.put(x,y,get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp);
            screen.put(x,y,get_sprite(),get_phase()+sgn,Spr::with_tspsoft,(void*)&tsp);
            break;
    case 3: screen.put(x,y,get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp);
            screen.put(x,y,get_sprite(),get_phase()+sgn);
            break;
    case 0: screen.put(x,y,get_sprite(),get_phase());
            break;
  }
}





