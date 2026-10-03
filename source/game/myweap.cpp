#include "headers.h"

//SWARMER
Swarmer::Swarmer(int _x,int _y,int _angle,int _id,int _number,int _straight_angle):
Posit(_x,_y,Posit::UP),
Move(_angle,_SWARMERS.speed,_SWARMERS),
Colis(C_MYSHOT(_id),_CSWARMERS,"swarmer"),
Listmanager(OMWEAP),
Shadow(Mysprites::myswarmer,View::MYWEAP)
{
  DBG_CHECK(_number>=0&&_number<swarm_numb);
  id=_id;
  number=_number;
  mode=INIT;
  target=NULL;
  life_time=Op::MYSWARMERLIFETIME;
  time=Op::MYSWARMERINITTIME;
  straight_angle=_straight_angle;
  wave=NEW(Wave(x,y,angle,Mysprites::farmerdym),gobjects_mbn);
}

void Swarmer::run(void)
{
  if ((life_time-=KbdStat::time())<0)
  {
    E_ROCKET;
    wave->set(x,y,1);
    KILLME
  }
  if (!Visible::is_in_overscan(id))
  {
    wave->set(x,y,1);
    KILLME
  }
  switch (mode)
  {
      case INIT: time-=KbdStat::time();
                 if (time<=0)
                 {
                   time=0;
                   mode=SEEK;
                 }
                 break;
      case SEEK: {
                   const int obj_numb=100;
                   LowColis *ptrs[obj_numb];
                   int found=0;
                   for (LowColis *temp=LowColis::first_logged();temp!=NULL;temp=LowColis::next_logged())
                   {
                     Cmsg *msg=temp->respond();

                     int matched=0;
                     if (C_SWARM_LOCK_ALIEN(msg,id))
                     {
                       matched=1;
                     }
                     else if (C_SWARM_LOCK_MYSHIP(msg,id))
                     {
                       DBG_CHECK(NoNet::check_id(msg->MYSHIP));
                       Myship *s=NoNet::get_ship(msg->MYSHIP);
                       if (!s->is_invisible())
                       {
                         matched=1;
                       }
                     }

                     if (matched)
                     {
                       if (Posit::is_in_overscan(0,msg->x,msg->y,id))
                       {
                         if (found<obj_numb)
                         {
                           ptrs[found]=temp;
                           found++;
                         }
                         else
                         {
                           break;
                         }
                       }
                     }
                   }
                   if (found>0)
                   {
                     target=ptrs[RAND%found];
                     Cmsg *msg;
                     msg=target->respond();
                     int obj_x=msg->x;
                     int obj_y=msg->y;
                     int obj_angle=Turn::get_angle(obj_x,obj_y);
                     int obj_dangle=sub_angle(angle,obj_angle);
                     if (abs(obj_dangle)<=Op::MYSWARMERTRANSANGLE)
                     {
                       angle=obj_angle;
                     }
                     else
                     {
                       int tangle=Op::MYSWARMERTRANSANGLE;
                       if (obj_dangle<0)
                       {
                         angle=get_angle(-(tangle-RAND%tangle/2));
                       }
                       else
                       {
                         angle=get_angle((tangle-RAND%tangle/2));
                       }
                     }
                     mode=LOCKED;
                   }
                   else
                   {
                     target=NULL;
                     angle=straight_angle;
                     mode=LOST;
                   }
                   transform_speed();
                 }
                 break;
    case LOCKED: {
                   int found=0;
                   for (LowColis *temp=LowColis::first_logged();temp!=NULL;temp=LowColis::next_logged())
                   {
                     if (temp==target)
                     {
                       found=1;
                     }
                   }
                   if (!found)
                   {
                     mode=LOST;
                     break;
                   }
                   Cmsg *msg;
                   msg=target->respond();
                   int obj_x=msg->x;
                   int obj_y=msg->y;
                   int obj_angle=Turn::get_angle(obj_x,obj_y);
                   turn_angle(obj_angle);
                   transform_speed();
                   faster();
                 }
                 break;
      case LOST: faster();
                 break;
  }

  if ((mode!=INIT||time<Op::MYSWARMERINITTIME/2))
  {
    wave->set(x,y,0);
  }

  Colis::check(id,vx,vy,get_sprite(),get_phase());
  for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
  {
    if (C_CHECK_MY(temp,id))
    {
      E_ROCKET;
      wave->set(x,y,1);
      KILLME
    }
  }

  update_phase(get_dir());
  go();

  log_to_view();
}

void Swarmer::add_cmsg(Cmsg* message,int _id)
{
  DBG_CHECK(_id==id);
  LowColis::add_cmsg(message,_id);
}
//MYROCKET
MyRocket::MyRocket(int _x,int _y,int _angle,int _id,int _mode):
Posit(_x,_y,Posit::UP),
Move(_angle,_ROCKET.speed,_ROCKET),
Colis(C_MYSHOT(_id),_CROCKET,"myrocket"),
Listmanager(OMWEAP),
Visible(Mysprites::myrocket,View::MYWEAP)
{
  id=_id;
  mode=_mode;
  life_time=Op::MYROCKETLIFETIME;
  idle_time=Op::MYROCKETIDLETIME;
  wave=NEW(Wave(x,y,angle,Mysprites::dymki),gobjects_mbn);
}

void MyRocket::run(void)
{
  if ((life_time-=KbdStat::time())<0)
  {
    E_ROCKET;
    wave->set(x,y,1);
    KILLME
  }

  if (!Visible::is_in_overscan((mode)?-1:id))
  {
    wave->set(x,y,1);
    KILLME
  }

  if (idle_time>0)
  {
    idle_time-=KbdStat::time();
    if (idle_time<=0)
    {
      idle_time=0;
    }
  }
  if (idle_time<=0)
  {
    int min_alienship_angle=Op::MYROCKETALIENANGLE;
    int min_alienship_range=Op::MYROCKETALIENRANGE;
    int alienship_angle=-1;
    int min_destroy_angle=Op::MYROCKETDESTROYANGLE;
    int min_destroy_range=Op::MYROCKETDESTROYRANGE;
    int destroy_angle=-1;
    for (LowColis *temp=LowColis::first_logged();temp!=NULL;temp=LowColis::next_logged())
    {
      Cmsg *msg=temp->respond();
      if (C_TARGET_MY(msg,id))
      {
        int obj_x=msg->x;
        int obj_y=msg->y;
        int obj_angle=Turn::get_angle(obj_x,obj_y);
        int obj_dangle=abs(Turn::sub_angle(angle,obj_angle));
        int obj_distance=distance(obj_x,obj_y);
        if (C_TARGET_MY_SHIP(msg))
        {
          if (obj_dangle<=min_alienship_angle)
          {
            if (obj_distance<min_alienship_range)
            {
              min_alienship_range=obj_distance;
              alienship_angle=obj_angle;
            }
          }
        }
        else
        {
          if (obj_dangle<=min_destroy_angle)
          {
            if (obj_distance<min_destroy_range)
            {
              min_destroy_range=obj_distance;
              destroy_angle=obj_angle;
            }
          }
        }
      }
    }
    if (alienship_angle!=-1)
    {
      turn_angle(alienship_angle);
      transform_speed();
    }
    else if (destroy_angle!=-1)
    {
      turn_angle(destroy_angle);
      transform_speed();
    }
  }

  plus_phase();
  faster();
  go();
  wave->set(x,y,0);

  Colis::check(((mode)?-1:id),vx,vy,get_sprite(),get_phase());

  for (Cmsg* t=get_cmsg();t!=NULL;t=get_cmsg())
  {
    if (C_CHECK_MY(t,id))
    {
      E_ROCKET;
      wave->set(x,y,1);
      KILLME
    }
  }

  log_to_view();
}

void MyRocket::add_cmsg(Cmsg* message,int _id)
{
  if (!mode)
  {
    DBG_CHECK(_id==id);
  }
  LowColis::add_cmsg(message,_id);
}

void MyRocket::draw_object(Screen& screen,int plane)
{
  Light::Ray(screen,&Mysprites::lihgtexpl,x,y,3);
  Visible::draw_object(screen,plane);
}

//MYBOMB
MyBomb::MyBomb(int _x,int _y,int _id):
Posit(_x,_y,Posit::DOWN),
LowColis(C_GOLDENEYE(_id),Op::MYBOMBPOWER,"my bomb"),
Visible(Mysprites::mybomb,View::MYWEAP),
Listmanager(OMWEAP)
{
  id=_id;
  done=0;
  bigflash=16;
  smlflash=8;
  time=Op::MYBOMBLIFETIME;
  size=Op::MYBOMBSIZE;
  DBG_CHECK(size<70);
  int i;
  for (i=0;i<NoNet::max_users+1;i++)
  {
    flashing[i]=0;
  }
  for (i=0;i<size;i++)
  {
    int a=RAND%Mp::TURNQUALITY;
    int d=RAND%Op::MYBOMBRADIUS;
    buffer[i].x=x+x_offset(a,d);
    buffer[i].y=y+y_offset(a,d);
    buffer[i].phase=RAND%get_phases_num();
  }
}

void MyBomb::run(void)
{
  if (!NoNet::check_id(id))
  {
    KILLME
  }
  if (time>0)
  {
    time-=KbdStat::time();
    if (time<=0)
    {
      time=0;
      Myship *s=NoNet::get_ship(id);
      if (s!=NULL)
      {
        x=s->get_x();
        y=s->get_y();
      }
      for (LowColis *temp=LowColis::first_logged();temp!=NULL;temp=LowColis::next_logged())
      {
        Cmsg *msg=temp->respond();
        if (Posit::distance(msg->x,msg->y)<=Op::MYBOMBRADIUS)
        {
          temp->add_cmsg(respond(),id);
        }
      }

      play(Mysound::explode[RAND%MAXEXPLODE]);

      FOR_ALL_USERS(ID)
      {
        s=NoNet::get_ship(ID);
        int dist=Posit::distance(s->get_x(),s->get_y());
        if (ID==id||dist<=Op::MYBOMBRADIUS)
        {
          s->shake_screen(Op::MYBOMBSHAKETIME);
          flashing[ID]=bigflash;
        }
        else if (dist<2*Op::MYBOMBRADIUS)
        {
          s->shake_screen(Op::MYBOMBSHAKETIME/2);
          flashing[ID]=smlflash;
        }
      }
      done=1;
    }
  }
  if (!done)
  {
    Myship *s=NoNet::get_ship(id);
    if (s!=NULL)
    {
      x=s->get_x();
      y=s->get_y();
    }
    for (int i=0;i<size;i++)
    {
      if ((--buffer[i].phase)<0)
      {
        int a=RAND%Mp::TURNQUALITY;
        int d=RAND%Op::MYBOMBRADIUS;
        buffer[i].x=x+x_offset(a,d);
        buffer[i].y=y+y_offset(a,d);
        buffer[i].phase=RAND%get_phases_num();
      }
    }
  }
  else
  {
    FOR_ALL_USERS(UID)
    {
      if (UID==world->get_current_displayed())
      {
        int lights=flashing[UID]*4/bigflash;
        for (int i=0;i<lights;i++)
        {
          Night::add_light(x,y);
        }
      }
      if (flashing[UID]>0)
      {
        flashing[UID]--;
      }
    }
    int stop=1;
    FOR_ALL_USERS(ID)
    {
      Myship *s=NoNet::get_ship(ID);
      if (distance(s->get_x(),s->get_y())>=2*Op::MYBOMBRADIUS)
      {
        flashing[ID]=0;
      }
      if (flashing[ID]>0)
      {
        stop=0;
      }
    }
    if (stop)
    {
      KILLME
    }
  }
  log_to_view(get_plane()+3);
  log_to_view(9);
}

void MyBomb::draw_object(Screen& screen,int plane)
{
  (void)plane;
  if (!done)
  {
    if (plane==get_plane()+3)
    {
      for (int i=0;i<size;i++)
      {
        if (buffer[i].phase<get_phases_num())
        {
          screen.put(buffer[i].x,buffer[i].y,get_sprite(),buffer[i].phase);
        }
      }
    }
  }
  else
  {
    if (plane==9)
    {
      int disp=world->get_current_displayed();
      if (flashing[disp]>0)
      {
        int phs=Tools::LIGHT16-bigflash+flashing[disp];
        DBG_CHECK(phs>=Tools::LIGHT1&&phs<=Tools::LIGHT16);
        int orgx=screen.ox;
        int orgy=screen.oy;
        screen.origin(0,0);
        screen.rectangle(Tools::tool[phs],0,0,Mp::SX,Mp::SY);
        screen.origin(orgx,orgy);
      }
    }
  }
}
//LASER
int Laser::move_table[MAX_LASEROFFSET]={1,2,3,2,1,0,-1,-2,-3,-2,-1,0};

Laser::Laser(int _x,int _y,int _vx,int _vy,int _angle,int _id):
Posit(_x,_y,Posit::UP),
Move(_angle,_LASER.speed,_LASER),
Colis(C_MYSHOT(_id),_CLASER,"my laser"),
Listmanager(OMWEAP),
Visible(Mysprites::fireball,View::MYWEAP)
{
  id=_id;
  vx+=_vx;
  vy+=_vy;
  update_speed();
  x1=x2=x;
  y1=y2=y;
  index=RAND%MAX_LASEROFFSET;
  sstop=time=0;
  life_time=Op::MYLASERLIFETIME;
}

void Laser::run(void)
{

  if ((life_time-=KbdStat::time())<0) KILLME
  if (!Visible::is_in_overscan(id))   KILLME

  if (time>30)
  {
    if((++index)>=MAX_LASEROFFSET) index=0;
    time=0;
  }
  else time+=KbdStat::time();

  faster();
  x2=x1;
  x1=x;
  y2=y1;
  y1=y;
  go();
  if (move_table[index]<0)
  {
    x+=Posit::x_offset(get_angle(Mp::QUARTANGLE),-move_table[index]);
    y+=Posit::y_offset(get_angle(Mp::QUARTANGLE),-move_table[index]);
  }
  if (move_table[index]>0)
  {
    x+=Posit::x_offset(get_angle(-Mp::QUARTANGLE),move_table[index]);
    y+=Posit::y_offset(get_angle(-Mp::QUARTANGLE),move_table[index]);
  }

  Colis::check(id,vx,vy,get_sprite(),0);
  for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
  {
    if (C_CHECK_MY(temp,id))
    {
      NEW(Explode(x,y,Posit::UP,0,NULL,Mysprites::expl_laser,Explode::DISABLE,0),gobjects_mbn);
      KILLME
    }
  }
  reset_cmsg();
  log_to_view();
}

void Laser::draw_object(Screen& screen,int plane)
{
  (void)plane;
  screen.put(x,y,get_sprite(),0);
  screen.put(x,y,get_sprite(),1,Spr::with_tspsoft,(void*)&tsp);
  screen.put(x,y,get_sprite(),2,Spr::with_tsphard,(void*)&tsp);

  screen.put(x+(x1-x)/3,y+(y1-y)/3,get_sprite(),0,Spr::with_tspsoft,(void*)&tsp);
  screen.put(x+(x1-x)/3,y+(y1-y)/3,get_sprite(),1,Spr::with_tsphard,(void*)&tsp);

  screen.put(x+((x1-x)/3)*2,y+((y1-y)/3)*2,get_sprite(),0,Spr::with_tsphard,(void*)&tsp);
}

void Laser::draw_shadow(Screen& screen,int plane)
{
  (void)plane;
}

void Laser::add_cmsg(Cmsg* message,int _id)
{
  DBG_CHECK(_id==id);
  LowColis::add_cmsg(message,_id);
}
//Wave
Wave::Wave(int _x,int _y,int _angle,Sprite& spr,int _type,int _sttime):
Posit(_x,_y,Posit::UP),
Move(_angle,_ROCKET.speed,_ROCKET),
Listmanager(OMWEAP),
Visible(spr,View::MYWEAP)
{
  memset(table,get_phases_num(),sizeof(Wkt)*MAX_WAVE);
  addtime=del=time=last=0;
  Lx=x;
  Ly=y;
  sttime=_sttime;
  type=_type;
}


void Wave::add(void)
{
  if (distance(Lx,Ly)>4)
  {
    Lx=table[last].x=x+RAND%4-2;
    Ly=table[last].y=y+RAND%4-2;
    table[last].phase=0;
    last++;
    if (last>=MAX_WAVE) last=0;
  }
}

void Wave::add_time(void)
{
  addtime+=KbdStat::time();
  if (addtime>30)
  {
    table[last].x=x+RAND%4-2;
    table[last].y=y+RAND%4-2;
    table[last].phase=0;
    last++;
    if (last>=MAX_WAVE) last=0;
    addtime=0;
  }
}

void Wave::run(void)
{
  time+=KbdStat::time();
  if (!type) add();
  else add_time();
  int t=0,change=0;
  if (time>sttime)
  {
    int i;
    for(i=0;i<MAX_WAVE;i++)
    {
      if (table[i].phase<get_phases_num())
      {
        table[i].phase++;
        change=1;
      }
      t=1;
    }
    time=0;
  }
  log_to_view();
  if (del&&t&&!change) KILLME
}

void Wave::draw_object(Screen& screen,int plane)
{
  (void)plane;
  for(int i=0;i<MAX_WAVE;i++)
  {
    if (table[i].phase<get_phases_num())
    screen.put(table[i].x,table[i].y,get_sprite(),table[i].phase,Spr::with_tsphard,(void*)&tsp);
  }
}

//CHGUN
int ChGun::vdy[MAX_VDY]={256,250,240,200,180,120,80,50,20,10,0,-20,-60,-100,-150,200,
                            256,300,420,560};


ChGun::ChGun(int _x,int _y,int _angle,int _destroy,int _id):
Posit(_x,_y,Posit::UP),
Listmanager(OAWEAP),
LowColis(C_MYSHOT(_id),_destroy),
Visible(View::NEXPL)
{
  stx=x;
  sty=y;
  id=_id;
  angle=_angle;
  done=0;
}

void ChGun::init_bullet(void)
{
  return;
  /*
  body[0].x=(stx+RAND%6-3)<<8;
  body[0].y=(sty+RAND%6-3)<<8;
  int tangle=(angle+Mp::QUARTANGLE+RAND%6);
  if (tangle>=Mp::TURNQUALITY)
     tangle-=Mp::TURNQUALITY;
  body[0].vx=x_offset(tangle,256);
  body[0].vy=y_offset(tangle,256);
  body[0].time=Mp::METRONQUALITY+RAND%Mp::METRONQUALITY;
  body[0].dt=0;
  */
}


void ChGun::init(int _x,int _y)
{
  for (int i=0;i<MAX_CHGUNDOTT;i++)
  {
    body[i].x=_x<<8;
    body[i].y=_y<<8;
    body[i].vx=RAND%400-200;
    body[i].vy=RAND%400-200;
    body[i].time=RAND%Mp::METRONQUALITY;
    body[i].dt=0;
  }
  done=1;
}

void ChGun::run(void)
{

  if (!done)
  {
    Cmsg *temp=NULL;
    for (;;)
    {
      move_me(angle,5);
      check(id,x,y,0,0);
      temp=get_cmsg();
      if (temp&&C_CHECK_MY(temp,id))
      {
        if (temp->KILLABLE)
        {
          int t=RAND%5;
          switch (t)
          {
            case 0: Sound::play(Mysound::spl_ship1); break;
            case 1: Sound::play(Mysound::spl_ship2); break;
            case 2: Sound::play(Mysound::spl_ship3); break;
            default: break;
          }

        }
        else
        {
          int t=RAND%MAXSPLINTER+2;
          if (t<MAXSPLINTER) Sound::play(Mysound::splinter[t]);
        }
        init_bullet();
        init(x,y);
        break;
      }
      if (!Posit::is_in_overscan())
      {
        memset(body,0,sizeof(body));
        init_bullet();
        done=1;
        break;
        //KILLME
      }
    }
  }

  int t=0;
  for (int i=0;i<MAX_CHGUNDOTT;i++)
  {
    body[i].x+=body[i].vx;
    body[i].y+=body[i].vy;
    body[i].time-=KbdStat::time();
    /*
    body[i].y+=vdy[body[i].dt];
    if ((body[i].dt++)>=MAX_VDY) body[i].dt=MAX_VDY-1;
    */
    if (body[i].time>t) t=body[i].time;
  }
  if (t<=0) KILLME
  log_to_view(get_plane()+2);
}

void ChGun::draw_object(Screen& screen,int plane)
{
  (void)plane;
  for (int i=0;i<MAX_CHGUNDOTT;i++)
  {
    if (body[i].time>0)
    {
      screen.plot(Tools::tool[Tools::LIGHT16][screen.check(body[i].x>>8,body[i].y>>8)],body[i].x>>8,body[i].y>>8);
    }
  }
}
//KLASA SHOOTGUN
ShootGun::ShootGun(int _x,int _y,int _angle,int _id):
Posit(_x,_y,Posit::UP),
Listmanager(OAWEAP),
LowColis(C_MYSHOT(_id),_CSHOTGUN)
{
  angle=_angle;
  id=_id;
}

void ShootGun::run (void)
{
  Cmsg *temp=NULL;
  int wsp=(_CSHOTGUN<<8)/((Mp::SX/2+Mp::OVERSCANX)/5);
  int _vx=x_offset(angle,Mp::MAXOFFSET);
  int _vy=y_offset(angle,Mp::MAXOFFSET);
  for (int i=0;;i++)
  {
    move_me(angle,5);
    check(id,x,y,_vx,_vy);
    temp=get_cmsg();
    if (temp&&C_CHECK_MY(temp,id))
    {
      if (temp->MYSHIP)
      {
        Myship* ship=NoNet::get_ship(temp->MYSHIP);
        if (ship) ship->kick_ship(angle,Move::MEDIUM);
      }
      NEW(Explode(x,y,get_chl(),0,Explode::NOLIGHT,Mysprites::expl_rocket,Explode::DISABLE,0),gobjects_mbn);
      if (temp->KILLABLE)
      {
        int t=RAND%5;
        switch (t)
        {
          case 0: Sound::play(Mysound::spl_ship1); break;
          case 1: Sound::play(Mysound::spl_ship2); break;
          case 2: Sound::play(Mysound::spl_ship3); break;
          default: break;
        }

      }
      else
      {
        int t=RAND%MAXSPLINTER+2;
        if (t<MAXSPLINTER) Sound::play(Mysound::splinter[t]);
      }
      KILLME
    }

    int pow=_CSHOTGUN-((i*wsp)>>8);
    if (pow<_CSHOTGUN/5) set_power(_CSHOTGUN/5);
    else set_power(pow);

    if (!Posit::is_in_overscan(id))  KILLME
  }
}

//SPLASH
Splash::Splash(int _x,int _y,int _angle,int _id,Sprite& _sprite,int _mode):
Posit(_x,_y,Posit::UP),
Move(_angle,_SPLASH.speed,_SPLASH),
Colis(C_MYSHOT(_id),_CSPLASH,"splash"),
Listmanager(OMWEAP),
Visible(_sprite,View::MYWEAP)
{
  id=_id;
  mode=_mode;
  update_phase(get_dir());
  life_time=Op::MYSPLASHLIFETIME;
}

void Splash::run(void)
{
  if ((life_time-=KbdStat::time())<0)
  {
    KILLME
  }
  if (mode)
  {
    if (!Visible::is_in_overscan()) KILLME
  }
  else
  {
    if (!Visible::is_in_overscan(id)) KILLME
  }
  go();
  if (mode) Colis::check(-1,vx,vy,get_sprite(),get_phase());
  else Colis::check(id,vx,vy,get_sprite(),get_phase());
  for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
  {
    if (C_CHECK_MY(temp,id))
    {
      NEW(Boom(x,y,get_chl()),gobjects_mbn);
      KILLME
    }
  }
  log_to_view();
  Sound::run();
}

void Splash::fire(int stage)
{
  (void) stage;
}

void Splash::add_cmsg(Cmsg* message,int _id)
{
  if (!mode)
  {
    DBG_CHECK(_id==id);
  }
  LowColis::add_cmsg(message,_id);
}

void Splash::draw_object(Screen& screen,int plane)
{
  (void)plane;
  screen.put(x,y,get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp);
}
//MYMINE
Mymine::Mymine(int _x,int _y,int _vx,int _vy,int _angle,int _id):
Posit(_x,_y,Posit::UP),
Move(_angle,_MYMINE.speed,_MYMINE),
Colis(C_MYMINES(_id),0,"bomb"),
Listmanager(OMWEAP),
Shadow(Mysprites::mine,View::MYWEAP)
{
  id=_id;
  time=0;
  vx+=_vx;
  vy+=_vy;
  update_speed();
}

void Mymine::wall(void)
{
  Cmsg* temp=NULL;
  int t=0;
  int _x=0;
  int _y=0;
  int dx=0;
  int dy=0;

  for (int i=0;i<Mp::TURNQUALITY;i+=16)
  {
    _x=x+x_offset(i,5);
    _y=y+y_offset(i,5);
    if (Posit::is_in_overscan(0,_x,_y,id))
    {
      temp=LowColis::see(id,_x,_y,0);
      if (temp&&temp->WALL)
      {
        dx-=x_offset(i,5);
        dy-=y_offset(i,5);
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
    t=Turn::get_dangle(dx,dy);
  }

  if (t!=-1)
  {
    int speed_angle=get_dangle(vx,vy);
    int wangle;
    if (abs(sub_angle(speed_angle,t))>=Mp::QUARTANGLE)
    {
      wangle=t+sub_angle(normalize_angle(speed_angle+Mp::HALFANGLE),t);
      wangle=normalize_angle(wangle);
      transform_speed(wangle);
      update_speed();
    }
  }

}

void Mymine::run(void)
{
  go();
  brake();
  time+=KbdStat::time();
  plus_phase();
  wall();
  if (time>2*Mp::METRONQUALITY)
  {
    set_power(_CMYMINE);
    Myship* shp=NoNet::get_ship(id);
    if (shp&&distance(shp->get_x(),shp->get_y())<Op::MYMINERANGE)
    {
      set_cinfo(C_MYMINEM);
      shp->add_cmsg(respond(),id);
    }
    set_cinfo(C_MYMINES(id));
    for (LowColis *temp=LowColis::first_logged();temp!=NULL;temp=LowColis::next_logged())
    {
      Cmsg *msg=temp->respond();
      if (distance(msg->x,msg->y)<Op::MYMINERANGE)
        temp->add_cmsg(respond(),id);
    }
    E_SMOKE;
    KILLME
  }
  log_to_view();
}

void Mymine::add_cmsg(Cmsg* message,int _id)
{
  DBG_CHECK(_id==id);
  LowColis::add_cmsg(message,_id);
}
//NETMINE
NetMine::NetMine(int _x,int _y,int _id):
Posit(_x,_y,Posit::UP),
Move(0,0,Op::MINE),
Shadow(Mysprites::staticmine[_id-1],View::AMOV),
Listmanager(ONETMINE),
Colis(C_NETMINEBEGIN(0),0,"mine"),
Life(Op::MINE)
{
  nonsmp=smpl=t=tiku=range=0;
  global_time=0;
  bomb_time=0;
  id=_id;
  smpl=flash=counter=0;
}


void NetMine::kill(void)
{
  for (LowColis *temp=LowColis::first_logged();temp!=NULL;temp=LowColis::next_logged())
  {
    Cmsg *msg=temp->respond();
    if (distance(msg->x,msg->y)<90) temp->add_cmsg(respond(),0);
  }
  myinfo.MY=0;
  Myship* ship=NoNet::get_ship(id);
  if (ship)
  {
    Cmsg *msg=ship->respond();
    if (distance(msg->x,msg->y)<90) ship->add_cmsg(respond(),0);
  }

 /*
  FOR_ALL_USERS(ID)
  {
    ship=NoNet::get_ship(ID);
    Cmsg *msg=ship->respond();
    if (distance(msg->x,msg->y)<90) ship->add_cmsg(respond(),0);
  }
  */
}

int NetMine::check_obj(void)
{
  Myship* ship=NULL;
  FOR_ALL_USERS(ID)
  {
    ship=NoNet::get_ship(ID);
    Cmsg *msg=ship->respond();
    if (C_MINE_CHECKOBJ(msg))
    {
      if (distance(msg->x,msg->y)<80)  return 1;
    }
  }
  return 0;
}

void NetMine::serv_kill(void)
{
  Colis::check(-1,0,0,get_sprite(),get_phase());
  for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
  {
    if (C_MINE_CHECK(temp))  decrease(temp->destroy);

    if (C_MINE_BLOW(temp))
    {
      flash=1;
      counter=5*Mp::METRONQUALITY;
    }
  }
  Colis::write(-1,0,0,get_sprite(),get_phase());
  if (get_life()<=0) flash=1;
}

void NetMine::run(void)
{
  bomb_time+=KbdStat::time();

  plus_phase();
  if (bomb_time>=Op::MINENONACTIVETIME)
  {
    if (!nonsmp)
    {
      nonsmp=1;
      play(Mysound::mineactive);
    }
    t=check_obj();
  }
  else t=0;

  if (bomb_time>(120*Mp::METRONQUALITY+Op::MINENONACTIVETIME)) t=1;

  if (!flash) tiku=flash=t;

  if (flash)
  {
    if (counter>1200)
    {
      myinfo.MY=id;
      myinfo.destroy=Op::MINE.destroy;
//      kill();
//      myinfo.MY=0;
//      set_cinfo(C_NETMINEKILLMY);
      kill();
      E_AIR;
      KILLME
    }
    else  counter+=KbdStat::time();
    if (!smpl)
    {
      play(Mysound::mine);
      smpl=1;
    }
  }

  if (!(bomb_time<Op::MINENONACTIVETIME))serv_kill();
  log_to_view();
  Sound::run();
}

void NetMine::draw_object(Screen& screen,int plane)
{
  (void)plane;
  if (flash&&(counter/100%2))
    screen.put(x,y,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::LIGHT]);
  else
    screen.put(x,y,get_sprite(),get_phase());
}

//KLASA WEAPON
Weapon::Weapon(int _x,int _y,User* _user,int _id):
Posit(_x,_y,Posit::UP),
Turn(0,Op::myship),
Visible(Mysprites::ship_tower,View::MYSHIP)
{
  myuser=_user;
  id=_id;
  int i;
  for (i=0;i<6;i++)
  {
    weap_freq[i]=Op::weaponary_frq[i];
  }
  tower_height=1;
  ship_radius=16;
  for (i=0;i<3;i++)
  {
    morph_ranges[i]=(i+1)*Op::WEAPONMORPHPHASES/3;
  }
  reset();
}

void Weapon::reset(void)
{
  weap_in_use=myuser->get_weap_in_use();
  reset_time_service();
  update_sprite_range();
  mode=GOINGOFF;
  angle=0;
  dir=0;
  morphing_phase=0;
  ship_flipper=0;
  weap1_flipper=0;
  cannon_fire=0;
  last_angle=0;
  display_message=0;
  out_of_ammo_played=0;
}

void Weapon::update_sprite_range(void)
{
  set_range(weap_in_use*32,(weap_in_use+1)*32-1);
}

int Weapon::preffered_angle(void)
{
  int _angle=0;
  if (KbdStat::steering_mode(id)==1)
  {
    if (KbdStat::mouse_read(id))
    {
      int mx=KbdStat::mouse_x(id);
      int my=KbdStat::mouse_y(id);
      _angle=get_dangle(world->get_x(id)+mx-Posit::x,world->get_y(id)+my-Posit::y);
    }
    else
    {
      _angle=last_angle;
    }
  }
  else
  {
    _angle=ship_angle;
    int alienship_angle=-1;
    int destroy_angle=-1;
    int min_alienship_angle=Op::WEAPONALIENANGLE;
    int min_alienship_range=Op::WEAPONALIENRANGE;
    int min_destroy_angle=Op::WEAPONDESTROYANGLE;
    int min_destroy_range=Op::WEAPONDESTROYRANGE;

    if (NoNet::is_network_mode())
    {
      FOR_ALL_USERS(ID)
      {
        Myship *s=NoNet::get_ship(ID);
        int obj_x=s->get_x();
        int obj_y=s->get_y();
        if ((ID!=id)&&(Posit::is_in_overscan(0,obj_x,obj_y,id))&&(!s->is_invisible()))
        {
          int obj_angle=Turn::get_angle(obj_x,obj_y);
          int obj_dangle=abs(Turn::sub_angle(_angle,obj_angle));
          int obj_distance=distance(obj_x,obj_y);
          if (obj_dangle<=min_alienship_angle)
          {
            alienship_angle=obj_angle;
          }
        }
      }
    }

    if (alienship_angle==-1)
    {
      for (LowColis *temp=LowColis::first_logged();temp!=NULL;temp=LowColis::next_logged())
      {
        Cmsg *msg=temp->respond();
        if (C_TARGET_MY(msg,id))
        {
          int obj_x=msg->x;
          int obj_y=msg->y;
          int obj_angle=Turn::get_angle(obj_x,obj_y);
          int obj_dangle=abs(Turn::sub_angle(_angle,obj_angle));
          int obj_distance=distance(obj_x,obj_y);
          if (C_TARGET_MY_SHIP(msg))
          {
            if (obj_dangle<=min_alienship_angle)
            {
              if (obj_distance<min_alienship_range)
              {
                min_alienship_range=obj_distance;
                alienship_angle=obj_angle;
              }
            }
          }
          else
          {
            if (obj_dangle<=min_destroy_angle)
            {
              if (obj_distance<min_destroy_range)
              {
                min_destroy_range=obj_distance;
                destroy_angle=obj_angle;
              }
            }
          }
        }
      }
    }

    if (alienship_angle!=-1)
    {
      _angle=alienship_angle;
    }
    else if (destroy_angle!=-1)
    {
      _angle=destroy_angle;
    }
  }

  last_angle=_angle;
  return _angle;
}

void Weapon::reset_time_service(void)
{
  weap_stat=0;
}

int Weapon::auto_change_weapon(void)
{
  if (!myuser->get_weap_ammo(weap_in_use))
  {
    int *table=NULL;
    switch (weap_in_use)
    {
      case 0: table=Op::WEAPONAUTO1;
              break;
      case 1: table=Op::WEAPONAUTO2;
              break;
      case 2: table=Op::WEAPONAUTO3;
              break;
      case 3: table=Op::WEAPONAUTO4;
              break;
      case 4: table=Op::WEAPONAUTO5;
              break;
      case 5: table=Op::WEAPONAUTO6;
              break;
    }
    DBG_CHECK(table!=NULL);
    int indx=0;
    while (table[indx]>0&&indx<9)
    {
      if (myuser->get_weap_ammo(table[indx]-1)>0)
      {
        myuser->set_weap_in_use(table[indx]-1);
        mode=MORPHING;
        if (Mp::SHIPDEBUGLEVEL>0)
        {
          NoNet::get_ship(id)->send_debug_message("weapon automaticly switched to %d",weap_in_use+1);
        }
        return 1;
      }
      indx++;
    }
  }
  return 0;
}

void Weapon::fire_service(void)
{
  if (auto_change_weapon())
  {
    return;
  }
  if (Mp::SHIPDEBUGLEVEL>0)
  {
    NoNet::get_ship(id)->send_debug_message("firing with weapon %d",weap_in_use+1);
  }
  switch (weap_in_use)
  {
    case 0: {
              if (myuser->get_weap_ammo(0)>0)
              {
                weap1_flipper++;
                if (weap1_flipper>2)
                {
                  weap1_flipper=1;
                }
                if ((--weap_stat)<=0)
                {
                  Sound::play(Mysound::ship_chgun);
                  NEW(ChGun(x,y,angle,_CCHGUN ,id),gobjects_mbn);
                  myuser->decrease_weap_ammo(0,1);
                  weap_stat=30/weap_freq[0];
                }
              }
              else
              {
                weap1_flipper=0;
                weap_stat=Mp::METRONQUALITY;
                mode=IDLE;
              }
            }
            break;
    case 1: if (myuser->get_weap_ammo(1)>0)
            {
              Sound::play(Mysound::ship_swarm);
              int wangle=Op::MYSWARMERSTARTDANGLE;
              for (int i=0;i<Swarmer::swarm_numb;i++)
              {
                int a=normalize_angle(angle-wangle/2+(wangle*i)/(Swarmer::swarm_numb-1));
                NEW(Swarmer(x,y+tower_height,a,id,i,angle),gobjects_mbn);
              }
              myuser->decrease_weap_ammo(1,1);
              weap_stat=Mp::METRONQUALITY;
              mode=IDLE;
            }
            break;
    case 2: if (myuser->get_weap_ammo(2)>0)
            {
              Sound::play(Mysound::ship_plasma);
              NEW(Laser(x,y+tower_height,vx,vy,angle,id),gobjects_mbn);
              myuser->decrease_weap_ammo(2,1);
              weap_stat=Mp::METRONQUALITY;
              mode=IDLE;
            }
            break;
    case 3: if (myuser->get_weap_ammo(3)>0)
            {
              Sound::play(Mysound::ship_homing);
              int temp_angle=get_angle();
              angle=ship_angle;
              int ang=get_angle((ship_flipper=!ship_flipper)?Mp::QUARTANGLE:-Mp::QUARTANGLE);
              int posx=x-x_offset(angle,5)+x_offset(ang,ship_radius);
              int posy=y-y_offset(angle,5)+y_offset(ang,ship_radius)+4;
              NEW(MyRocket(posx,posy,angle,id),gobjects_mbn);
              angle=temp_angle;
              myuser->decrease_weap_ammo(3,1);
              weap_stat=Mp::METRONQUALITY;
              mode=IDLE;
            }
            break;
    case 4: if (myuser->get_weap_ammo(4)>0)
            {
              Sound::play(Mysound::ship_shotgun);
              NoNet::get_ship(id)->kick_ship(Turn::get_angle(Mp::HALFANGLE),Move::MEDIUM);
              NEW(ShootGun(x,y,angle,id),gobjects_mbn);
              cannon_fire=KbdStat::local_frame();
              myuser->decrease_weap_ammo(4,1);
              weap_stat=Mp::METRONQUALITY;
              mode=IDLE;
            }
            break;
    case 5: if (myuser->get_weap_ammo(5)>0)
            {
              Sound::play(Mysound::ship_cobain);
              int ang=get_angle();
              int ox=x_offset(ang,5);
              int oy=y_offset(ang,5)+tower_height;
              NEW(Mymine(x+ox,y+oy,vx,vy,angle,id),gobjects_mbn);
              myuser->decrease_weap_ammo(5,1);
              weap_stat=Mp::METRONQUALITY;
              mode=IDLE;
            }
            break;
   default: DBG_CHECK(0);
            break;
  }
  auto_change_weapon();
  Sound::run();
}

void Weapon::time_service(void)
{
  if (weap_stat>0)
  {
    weap_stat-=weap_freq[weap_in_use]*KbdStat::time();
  }
  if (cannon_fire>0&&KbdStat::local_frame()-cannon_fire>10)
  {
    cannon_fire=0;
  }
}

void Weapon::run(int _x,int _y,int _vx,int _vy,int _dir,int _angle,int _fire,Myship::ServMode _ship_mode)
{
  x=_x+Mysprites::ship[_dir].r-Mysprites::ship_tower_point[_dir].r;
  y=_y+Mysprites::ship[_dir].d-Mysprites::ship_tower_point[_dir].d;
  vx=_vx;
  vy=_vy;
  dir=_dir;
  ship_angle=_angle;

  if (display_message>0)
  {
    display_message-=KbdStat::time();
    if (display_message<=0)
    {
      display_message=0;
      if (id==world->get_current_displayed())
      {
        char *weap=NULL;
        Sample *smp=NULL;
        switch (weap_in_use)
        {
          case 0: weap="weap1";
                  smp=&Mysound::speech_vulcan;
                  break;
          case 1: weap="weap2";
                  smp=&Mysound::speech_swarmers;
                  break;
          case 2: weap="weap3";
                  smp=&Mysound::speech_plasma;
                  break;
          case 3: weap="weap4";
                  smp=&Mysound::speech_missiles;
                  break;
          case 4: weap="weap5";
                  smp=&Mysound::speech_cannon;
                  break;
          case 5: weap="weap6";
                  smp=&Mysound::speech_grenade;
                  break;
         default: DBG_CHECK(0);
                  break;
        }
        char *status=NULL;
        Sample *statussmp=NULL;
        if (myuser->get_weap_ammo(weap_in_use)>0)
        {
          status="activated";
          statussmp=&Mysound::speech_activated;
        }
        else
        {
          status="outofammo";
          statussmp=&Mysound::speech_outofammo;
        }
        char temp[1024];
        sprintf(temp,"%s_%s",weap,status);
        MSGTXT(GAMETXT(temp),id);
        Myship *s=NoNet::get_ship(id);
        s->play_ship_sample(*smp,Myship::SPEECH);
        s->play_ship_sample(*statussmp,Myship::SPEECH);
      }
    }
  }
  
  switch (mode)
  {
     case IDLE: if (_ship_mode==Myship::LANDING)
                {
                  reset_time_service();
                  mode=GOINGOFF;
                  break;
                }
                angle=preffered_angle();
                update_phase(get_dir());
                time_service();
                if (weap_in_use!=myuser->get_weap_in_use())
                {
                  reset_time_service();
                  mode=MORPHING;
                  break;
                }
                if (weap_stat<=0)
                {
                  if (_fire)
                  {
                    reset_time_service();
                    mode=FIRING;
                  }
                  else
                  {
                    reset_time_service();
                    mode=READY;
                  }
                  break;
                }
                break;
    case READY: if (_ship_mode==Myship::LANDING)
                {
                  reset_time_service();
                  mode=GOINGOFF;
                  break;
                }
                angle=preffered_angle();
                update_phase(get_dir());
                if (weap_in_use!=myuser->get_weap_in_use())
                {
                  mode=MORPHING;
                  break;
                }
                if (_fire)
                {
                  reset_time_service();
                  mode=FIRING;
                  break;
                }
                break;
 case MORPHING: angle=preffered_angle();
                update_phase(get_dir());
                if (morph())
                {
                  mode=READY;
                }
                break;
   case FIRING: if (_ship_mode==Myship::LANDING)
                {
                  reset_time_service();
                  mode=GOINGOFF;
                  break;
                }
                if (weap_in_use!=myuser->get_weap_in_use())
                {
                  reset_time_service();
                  mode=MORPHING;
                  break;
                }
                if (weap_in_use==0&&!_fire)
                {
                  reset_time_service();
                  mode=READY;
                  break;
                }
                angle=preffered_angle();
                update_phase(get_dir());
                fire_service();
                break;
 case GOINGOFF: turn_angle(get_dir_angle(dir));
                update_phase(get_dir());
                if (weap_in_use!=myuser->get_weap_in_use())
                {
                  morph();
                }
                if (_ship_mode==Myship::STARTING)
                {
                  reset_time_service();
                  mode=GOINGON;
                  break;
                }
                break;
  case GOINGON: turn_angle(preffered_angle());
                update_phase(get_dir());
                if (weap_in_use!=myuser->get_weap_in_use())
                {
                  morph();
                }
                if (_ship_mode==Myship::USER||_ship_mode==Myship::TURBO)
                {
                  reset_time_service();
                  mode=READY;
                  break;
                }
                break;
       default: DBG_CHECK(0);
                break;
  }
}

int Weapon::morph(void)
{
  morphing_phase++;
  if (morphing_phase>Op::WEAPONMORPHPHASES)
  {
    weap_in_use=myuser->get_weap_in_use();
    update_sprite_range();
    update_phase(get_dir());
    reset_time_service();
    morphing_phase=0;
    display_message=Mp::METRONQUALITY/2;
    return 1;
  }
  return 0;
}

void Weapon::draw_object(Screen& screen,int plane,int flash,int cloak,Tool *net_tool)
{
  DBG_CHECK(!(!NoNet::is_network_mode()&&net_tool!=NULL));
  (void)plane;

  int weap=myuser->get_weap_in_use();
  Sprite &spr=get_sprite();
  int phase=get_phase();
  int morph=get_dir()+32*weap;
  int fire_phase=get_dir();

  if (cloak)
  {
    screen.put(x,y,spr,phase,Spr::with_tsphard,(void*)&tsp);
    if (morphing_phase>0)
    {
      screen.put(x,y,spr,morph,Spr::with_tsphard,(void*)&tsp);
    }
    if (net_tool!=NULL)
    {
      screen.shape(x,y,spr,phase,Spr::with_tool,(void*)net_tool);
    }
  }
  else
  {
    if (morphing_phase>0)
    {
      if (morphing_phase<=morph_ranges[0])
      {
        screen.put(x,y,spr,phase);
        screen.put(x,y,spr,morph,Spr::with_tsphard,(void*)&tsp);
        if (net_tool!=NULL)
        {
          screen.shape(x,y,spr,phase,Spr::with_tool,(void*)net_tool);
        }
      }
      else if (morphing_phase<=morph_ranges[1])
      {
        screen.put(x,y,spr,phase,Spr::with_tspsoft,(void*)&tsp);
        screen.put(x,y,spr,morph,Spr::with_tspsoft,(void*)&tsp);
        if (net_tool!=NULL)
        {
          screen.shape(x,y,spr,morph,Spr::with_tool,(void*)net_tool);
        }
      }
      else if (morphing_phase<=morph_ranges[2])
      {
        screen.put(x,y,spr,phase,Spr::with_tsphard,(void*)&tsp);
        screen.put(x,y,spr,morph);
        if (net_tool!=NULL)
        {
          screen.shape(x,y,spr,morph,Spr::with_tool,(void*)net_tool);
        }
      }
    }
    else
    {
      if (flash)
      {
        screen.put(x,y,spr,phase,Spr::with_tool,(void*)&Tools::tool[Tools::LIGHT]);
      }
      else
      {
        screen.put(x,y,spr,phase);
      }
      if (net_tool!=NULL)
      {
        screen.shape(x,y,spr,phase,Spr::with_tool,(void*)net_tool);
      }
      if (mode==FIRING&&weap_in_use==0&&weap1_flipper>0)
      {
        screen.put(x,y,Mysprites::weap1fire,2*fire_phase+weap1_flipper-1);
      }
      else if (cannon_fire>0)
      {
        if (KbdStat::local_frame()-cannon_fire<10)
        {
          screen.put(x,y,Mysprites::shotgunfire,fire_phase);
        }
        cannon_fire=0;
      }
    }
  }
}
