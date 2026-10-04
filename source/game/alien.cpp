#include "headers.h"

//KLASA TARAN
Taran::Taran(int _x,int _y,int lev,int _chl,int _type,int _visible):
Posit(_x,_y,_chl,Op::obj_enemy[Op::TARAN]),
Move(0,0,Op::obj_enemy[Op::TARAN]),
Shadow(Mysprites::taran,View::AMOV),
Colis(C_ALIENHERO,Op::obj_enemy[Op::TARAN],"taran"),
Life(Op::obj_enemy[Op::TARAN]),
Listmanager(OAOBJ),
Handle(DESTROY_OBJ),
Radar(0),
Impexp((_type)?-1:lev)
{
  type=_type;
  ship=NoNet::get_ship(x,y);
  invisible=_visible;
  order=START;
  if (type)
  {
    order=SEEK;
    angle=get_angle(ship->get_x(),ship->get_y());
  }
  if (!type&&lobject->status) height=Mp::PIXELSBETWEENPLANES;
  kick_angle=kick=counter=global_time=unblock=time=0;
  angle_rand=RAND%Mp::TURNQUALITY;
  smoke=NEW(AlienSmoke(x,y,angle,Mysprites::farmerdym,100),gobjects_mbn);
}

Taran::~Taran(void)
{
  Mysound::taran.stop();
  if (!type&&!to_return)
  {
    lobject->x=lobject->aux1;
    lobject->y=lobject->aux2;
    lobject->status=-1;
    world->get_level().update(level_num);
  }
}

void Taran::update_posit(void)
{
  lobject->y=y;
  lobject->x=x;
  world->get_level().update(level_num);
}

void Taran::wall(void)
{
  int t=wall_angle2();
  if (t==-2)
  {
    stop();
    return;
  }
  if (t!=-1)
  {
    DBG_CHECK(t>=0&&t<Mp::TURNQUALITY);
    int speed_angle=0;

    if (vx==0&&vy==0)   speed_angle=angle;
    else  speed_angle=get_dangle(vx,vy);

    int wangle;
    if (abs(sub_angle(speed_angle,t))<Mp::QUARTANGLE) wangle=t;
    else wangle=t+sub_angle(normalize_angle(speed_angle+Mp::HALFANGLE),t);
    wangle=normalize_angle(wangle);
    if (get_speed()<50)
    {
      set_speed(50);
    }
    transform_speed(wangle);
    update_speed();
    order=WESCAPE;
  }
  else
  {
    if (kick>0)
    {
      kick--;
      kick_me(kick_angle,Move::WEAK);
      update_speed();
    }
  }
}

void Taran::HandleEvent(int message)
{
  if (!type&&message==lobject->type&&lobject->aux2==-200)
  {
    decrease(get_life()+100);
  }
}


void Taran::run(void)
{
  if (!invisible||height==Mp::PIXELSBETWEENPLANES) radar_log();
  ship=NoNet::get_ship(Posit::x,Posit::y);
  if (type) global_time+=KbdStat::time();
  if (type&&global_time>2*Op::MBOSS4TARANLIFETIME)  order=DISP;
  if (!type&&!Posit::is_in_overscan(0,x,y))
  {
    if (!LevImp::in_crange(lobject->type,x,y))
    {
      if (!type)
	    {
		    update_posit();
	    }
      smoke->kill();
      KILLME
    }
    return ;
  }
  smoke->set(x,y);
  switch (order)
  {
    case START:
              {
                if (get_life()<=0)
                {
                  E_AIR;
                  NEW(Capsule(x,y,Posit::UP,Capsule::LIFE,0),gobjects_mbn);
                  if (!type) Handle::post(GMANAGER,GameManager::code(lobject->type,DESTROY));
                  remove_from_level();
                  smoke->kill();
                  KILLME
                }
                if (!type&&!lobject->status)
                {
                  if (!invisible) log_to_view();
                  return;
                }
                turn_point(ship->get_x(),ship->get_y());
                update_phase(get_dir());
                go_up();
                if (invisible==1&&height<Mp::PIXELSBETWEENPLANES/2) invisible=2;
                if (!type&&is_up())
                {
	        			  Link::send_impulse(level_num);
                  invisible=3;
                  order=SEEK;
                }
                else
                {
                  log_to_view();
                  return ;
                }
                break;
              }
    case SEEK:
              {
                time+=KbdStat::time();
                turn_point(ship->get_x(),ship->get_y());
                if (time>Op::TARANFLYTIME+RAND%Op::TARANFLYTIME)
                {
                  time=0;
                  if (global_time>Op::MBOSS4TARANLIFETIME)
                  {
                    order=DISP;
                  }
                  else
                  {
                    play(Mysound::taran);
                    order=ATTACK;
                  }
                }
                update_phase(get_dir());
                go();
                break;
              }
    case ATTACK:
              {
                set_power(Op::obj_enemy[Op::TARAN].weapon);
                time+=KbdStat::time();
                turn_point(ship->get_x(),ship->get_y());
                if (time>Op::TARANATTACKTIME)
                {
                  brake();
                  if (get_speed()<10)
                  {
                    set_power(Op::obj_enemy[Op::TARAN].destroy);
                    time=0;
                    order=SEEK;
                  }
                }
                else
                {
                  faster();
                }
                update_phase(get_dir());
                go();
                break;
              }
    case WESCAPE:
              {
                int speed_angle=get_angle(vx,vy);
                turn_angle(speed_angle);
                update_phase(get_dir());
                brake();
                go();
                if (global_time>2*Op::MBOSS4TARANLIFETIME)  order=DISP;

                if (angle==speed_angle)
                {
                  order=SEEK;
                }
                break;
              }
    case DISABLE:
              {
                turn_angle(angle_rand);
                if (!(RAND%50))
                {
                  angle_rand=RAND%Mp::TURNQUALITY;
                }
                update_phase(get_dir());
                brake();
                go();
                if (!ship->is_invisible())
                {
                  order=SEEK;
                }
                break;
              }
    case DISP:
              {
                time+=KbdStat::time();
                if (time>100)
                {
                  counter++;
                  time=0;
                }
                if (counter>=4)
                {
                  remove_from_level();
                  smoke->kill();
                  KILLME
                }
                brake();
                break;
              }

  }
  if (ship->is_invisible())
  {
    order=DISABLE;
  }
  wall();
  if (get_chl()==Posit::UP)
  {
    for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
    {
      if (C_CHECK_ALIEN(temp))
      {
        if (temp->destroy>50&&temp->destroy<200)
        {
          kick=2;
          kick_angle=get_dangle(temp->vx,temp->vy);
        }
        decrease(temp->destroy);
      }

    }
    Colis::write(-1,vx,vy,get_sprite(),get_phase());
  }
  if (get_life()<=0)
  {
    E_AIR;
    if (!type)
    {
      NEW(Capsule(x,y,Posit::UP,Capsule::LIFE,0),gobjects_mbn);
      Handle::post(GMANAGER,GameManager::code(lobject->type,DESTROY));
      remove_from_level();
      Counter::present_alien(level_num);
    }
    smoke->kill();
    KILLME
  }
  else
    smoke->set_power(get_life()*100/Op::obj_enemy[Op::TARAN].life);

  log_to_view();
}

void Taran::draw_object(Screen& screen,int plane)
{
  (void)plane;

  switch (order)
  {
    case DISP:
      {
        switch (counter)
        {
          case 0:  screen.put(x,y,get_sprite(),get_phase());
			             break;
          case 1:  screen.put(x,y,get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp);
			             break;
          case 2:  screen.put(x,y,get_sprite(),get_phase(),Spr::with_tsphard,(void*)&tsp);
			             break;
        }
        break;
      }
    case START:
      {
        switch (invisible)
        {
          case 0:  screen.put(x,y,get_sprite(),get_phase()); break;
          case 2:  screen.put(x,y-(Mp::PIXELSBETWEENPLANES-height),get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp);
			             break;
          case 1:  screen.put(x,y-(Mp::PIXELSBETWEENPLANES-height),get_sprite(),get_phase(),Spr::with_tsphard,(void*)&tsp);
			             break;
        }
        break;
      }
    default : screen.put(x,y,get_sprite(),get_phase()); break;
  }
}

void Taran::draw_shadow(Screen& screen,int plane)
{
  (void)plane;
  if (order==DISP)
  {
    switch (counter)
    {
      case 0: screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
              break;
      case 1: screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::LSHADOW]);
              break;
      case 2: screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::VLSHADOW]);
              break;
    }
  }
  else
  {
    switch (invisible)
    {
      case 0: screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
              break;
      case 1: screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::VLSHADOW]);
              break;
      case 2: screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::LSHADOW]);
              break;
      default:screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
    }
  }
}

//SIMPLE1
Simple1::Simple1(int _x,int _y,int lev,int _type,int _visible):
Posit(_x,_y,Posit::DOWN,(_type==NORMAL)?Op::obj_enemy[Op::SIMPLE1]:Op::obj_enemy[Op::SIMPLE2]),
Move(0,0,(_type==NORMAL)?Op::obj_enemy[Op::SIMPLE1]:Op::obj_enemy[Op::SIMPLE2]),
Shadow((_type==NORMAL)?Mysprites::simple1:Mysprites::simple2,View::AMOV),
Colis(C_ALIENHERO,(_type==NORMAL)?Op::obj_enemy[Op::SIMPLE1]:Op::obj_enemy[Op::SIMPLE2],"simple1"),
Life((_type==NORMAL)?Op::obj_enemy[Op::SIMPLE1]:Op::obj_enemy[Op::SIMPLE2]),
Listmanager(OAOBJ),
Handle(DESTROY_OBJ),
Radar(0),
Impexp(lev)
{
  if (lobject->status) height=Mp::PIXELSBETWEENPLANES;
  invisible=_visible;
  ship=NoNet::get_ship(Posit::x,Posit::y);
  shcount=pangle=unblock=time=0;
  type=_type;
  tx=ship->get_x()+RAND%Op::SIMPLE1APPROX-Op::SIMPLE1APPROX/2;
  ty=ship->get_y()+RAND%Op::SIMPLE1APPROX-Op::SIMPLE1APPROX/2;
  order=START;
  constshcount=(Op::def_enemy[Op::SIMPLE1].active*Mp::METRONQUALITY)/KbdStat::time();
  smoke=NEW(AlienSmoke(x,y,angle,Mysprites::farmerdym,100),gobjects_mbn);
  if (type==SPECIAL)
    shtime=RAND%Op::SIMPLE2SHOOTTIME;
  else
    shtime=RAND%Op::SIMPLE1SHOOTTIME;
   phs1=phs2=0;
}

Simple1::~Simple1(void)
{
  if (type==NORMAL) Mysound::karabin.stop();
  if (!to_return)
  {
    lobject->x=lobject->aux1;
    lobject->y=lobject->aux2;
    lobject->status=-1;
    world->get_level().update(level_num);
  }
}

void Simple1::update_posit(void)
{
  lobject->y=y;
  lobject->x=x;
  world->get_level().update(level_num);
}

void Simple1::HandleEvent(int message)
{
  if (message==lobject->type&&lobject->aux2==-200)
  {
    decrease(get_life()+100);
  }
}


void Simple1::wall(void)
{
  int t=wall_angle2();
  if (t==-2)
  {
    stop();
    return;
  }

  if (t!=-1)
  {
    int speed_angle=get_dangle(vx,vy);
    int wangle;
    if (abs(sub_angle(speed_angle,t))<Mp::QUARTANGLE)
    {
      wangle=t;
    }
    else
    {
      wangle=t+sub_angle(normalize_angle(speed_angle+Mp::HALFANGLE),t);
    }
    wangle=normalize_angle(wangle);
    if (get_speed()<50)
    {
      set_speed(50);
    }
    transform_speed(wangle);
    update_speed();
    order=WESCAPE;
  }
}

void Simple1::bang(void)
{
  int t;
  if (type==SPECIAL) t=Op::SIMPLE2SHOOTTIME;
  else t=Op::SIMPLE1SHOOTTIME;
  if (shtime>t+RAND%t)
  {
    shcount=constshcount;
    shtime=0;
    if (type==SPECIAL)
    {
      int ang=normalize_angle(pangle+Mp::QUARTANGLE);
      int ox=x_offset(ang,10);
      int oy=y_offset(ang,10);
      int speed=get_speed();
      play(Mysound::asplash);
      NEW(ASplash(x,y,x,y,pangle,Op::simple2_splash,Op::obj_enemy[Op::SIMPLE2].weapon,Mysprites::asplash),gobjects_mbn);
      shcount=0;
    }
  }
  if (type==NORMAL)
  {
    if (shcount>0&&!(shcount%2))
    {
      int ang=normalize_angle(pangle+Mp::QUARTANGLE);
      int ox=x_offset(ang,10);
      int oy=y_offset(ang,10);
      if (shcount==constshcount) play(Mysound::karabin);
      NEW(Plask(x,y,pangle,Op::obj_enemy[Op::SIMPLE1].weapon),gobjects_mbn);
    }
    if (shcount>0)
    {
      if (shcount==constshcount) play(Mysound::karabin);
      shcount--;

      phs1=get_phase()+RAND%2-1;
      phs1=(phs1>=Mysprites::dym.phases)? phs1-=Mysprites::dym.phases:phs1;
      phs1=(phs1<0)?                      phs1+=Mysprites::dym.phases:phs1;

      phs2=get_phase()+RAND%2-1;
      phs2=(phs2>=Mysprites::dym.phases)? phs2-=Mysprites::dym.phases:phs2;
      phs2=(phs2<0)?                      phs2+=Mysprites::dym.phases:phs2;
    }
  }
}

void Simple1::turn(void)
{
  if (Posit::distance(ship->get_x(),ship->get_y())<90)
  {
    int sh=get_dangle(x-ship->get_x(),y-ship->get_y());
    turn_angle(sh);
    angle=sh;
  }
  else
  {
    turn_point(tx,ty);
  }
}

void Simple1::run(void)
{
  if (!invisible||height==Mp::PIXELSBETWEENPLANES) radar_log();
  ship=NoNet::get_ship(Posit::x,Posit::y);
  if (!Posit::is_in_overscan(0,x,y))
  {

    if (!LevImp::in_crange(lobject->type,x,y))
    {
	    update_posit();
      smoke->kill();
      KILLME
    }
    return ;
  }
  smoke->set(x,y);
  switch (order)
  {
    case START:
              {
                if (get_life()<=0)
                {
                  E_AIR;
                  if (type==SPECIAL) NEW(Capsule(x,y,Posit::UP,Capsule::WEAP0,0),gobjects_mbn);
                  Handle::post(GMANAGER,GameManager::code(lobject->type,DESTROY));
                  remove_from_level();
                  KILLME
                }

                if (!lobject->status)
                {
                  if (!invisible) log_to_view();
                  return;
                }
                turn_point(ship->get_x(),ship->get_y());
                update_phase(get_dir());
                go_up();
                if (invisible==1&&height>Mp::PIXELSBETWEENPLANES/2) invisible=2;
                if (is_up())
                {
        				  if (lobject->status!=2)Link::send_impulse(level_num);
                  pangle=angle;
                  invisible=3;
                  order=GO;
                }
                else
                {
                  log_to_view();
                  return ;
                }
                break;
              }
    case GO:
              {
                time+=KbdStat::time();
                if (!shcount) shtime+=KbdStat::time();
                turn();

                int temp=angle;
                angle=pangle;
                turn_point(ship->get_x(),ship->get_y());
                update_phase(get_dir());
                pangle=angle;
                angle=temp;

                if (time>Op::SIMPLE1TARGETTIME)
                {
                  tx=ship->get_x()+RAND%Op::SIMPLE1APPROX-Op::SIMPLE1APPROX/2;
                  ty=ship->get_y()+RAND%Op::SIMPLE1APPROX-Op::SIMPLE1APPROX/2;
                  time=0;
                }
                bang();
                faster();
                go();
                break;
              }
    case WESCAPE:
              {
                int speed_angle=get_angle(vx,vy);
                turn_angle(speed_angle);

                int temp=angle;
                angle=pangle;
                turn_point(ship->get_x(),ship->get_y());
                update_phase(get_dir());
                pangle=angle;
                angle=temp;

                brake();
                go();
                if (angle==speed_angle)
                {
                  order=GO;
                }
                break;
              }
    case DISABLE:
              {
                time+=KbdStat::time();
                shcount=0;
                //shtime+=KbdStat::time();

                int temp=angle;
                angle=pangle;
                int t=turn_angle(temp);
                update_phase(get_dir());
                pangle=angle;
                angle=temp;
                if (t)
                {
                  angle=RAND%Mp::TURNQUALITY;
                }

                bang();

                if (abs(sub_angle(pangle,angle))<Mp::ANGLE16)
                {
                  brake();
                }
                else
                {
                  faster();
                }
                if (!ship->is_invisible())
                {
                  order=GO;
                }
                go();
                break;

              }
  }
  if (ship->is_invisible())
  {
    order=DISABLE;
  }
  wall();
  if (get_chl()==Posit::UP)
  {
    for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
    {
      if (C_CHECK_ALIEN(temp))
      {
        decrease(temp->destroy);
      }
    }
    Colis::write(-1,vx,vy,get_sprite(),get_phase());
  }
  if (get_life()<=0)
  {
    E_AIR;
    if (type==SPECIAL) NEW(Capsule(x,y,Posit::UP,Capsule::WEAP2,0),gobjects_mbn);
    else               NEW(Capsule(x,y,Posit::UP,Capsule::WEAP0,0),gobjects_mbn);

    Handle::post(GMANAGER,GameManager::code(lobject->type,DESTROY));
    remove_from_level();
    smoke->kill();
    Counter::present_alien(level_num);
    KILLME
  }
  else
  {
    int t=(type==NORMAL)?Op::obj_enemy[Op::SIMPLE1].life:Op::obj_enemy[Op::SIMPLE2].life;
    smoke->set_power(get_life()*100/t);
  }
  log_to_view();
}

void Simple1::draw_object(Screen& screen,int plane)
{
  (void)plane;
  if (shcount>0&&order!=START&&order!=WESCAPE&&type==NORMAL)
  {
    int ang1=normalize_angle(pangle+Mp::QUARTANGLE);
    int o1x=x_offset(ang1,12);
    int o1y=y_offset(ang1,12);
    screen.put(x+o1x,y+o1y,Mysprites::dym,phs1,Spr::with_tspsoft,(void*)&tsp);
    screen.put(x-o1x,y-o1y,Mysprites::dym,phs2,Spr::with_tspsoft,(void*)&tsp);
  }
  switch (order)
  {
      case START:
      {
        switch (invisible)
        {
          case 0:  screen.put(x,y,get_sprite(),get_phase());break;
          case 2:  screen.put(x,y-(Mp::PIXELSBETWEENPLANES-height),get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp);
			             break;
          case 1:  screen.put(x,y-(Mp::PIXELSBETWEENPLANES-height),get_sprite(),get_phase(),Spr::with_tsphard,(void*)&tsp);
			             break;
        }
        break;
      }
      default : screen.put(x,y,get_sprite(),get_phase()); break;
  }
}

void Simple1::draw_shadow(Screen& screen,int plane)
{
  switch (invisible)
  {
    case 0: screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
            break;
    case 1: screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::VLSHADOW]);
            break;
    case 2: screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::LSHADOW]);
            break;
    default:screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
  }
}

//SIMPLE3
Simple3::Simple3(int _x,int _y,int lev,int _visible):
Posit(_x,_y,Posit::DOWN,Op::obj_enemy[Op::SIMPLE3]),
Move(0,0,Op::obj_enemy[Op::SIMPLE3]),
Shadow(Mysprites::simple3,View::AMOV),
Colis(C_ALIENHERO,Op::obj_enemy[Op::SIMPLE3],"simple3"),
Life(Op::obj_enemy[Op::SIMPLE3]),
Listmanager(OAOBJ),
Radar(0),
Handle(DESTROY_OBJ),
Impexp(lev)
{
  phs1=phs2=0;
  if (lobject->status) height=Mp::PIXELSBETWEENPLANES;
  invisible=_visible;
  ship=NoNet::get_ship(Posit::x,Posit::y);
  shcount=shtime=pangle=unblock=time=0;
  tx=ship->get_x()+RAND%Op::SIMPLE1APPROX-Op::SIMPLE1APPROX/2;
  ty=ship->get_y()+RAND%Op::SIMPLE1APPROX-Op::SIMPLE1APPROX/2;
  order=START;
  constshcount=(Op::def_enemy[Op::SIMPLE1].active*Mp::METRONQUALITY)/KbdStat::time();
  smoke=NEW(AlienSmoke(x,y,angle,Mysprites::farmerdym,100),gobjects_mbn);

  if (type==SPECIAL)
    shtime=RAND%Op::SIMPLE2SHOOTTIME;
  else
    shtime=RAND%Op::SIMPLE1SHOOTTIME;
}

Simple3::~Simple3(void)
{
  if (!to_return)
  {
    lobject->x=lobject->aux1;
    lobject->y=lobject->aux2;
    lobject->status=-1;
    world->get_level().update(level_num);
  }
}

void Simple3::update_posit(void)
{
  lobject->y=y;
  lobject->x=x;
  world->get_level().update(level_num);
}

void Simple3::HandleEvent(int message)
{
  if (message==lobject->type&&lobject->aux2==-200)
  {
    decrease(get_life()+100);
  }
}


void Simple3::wall(void)
{
  int t=wall_angle2();
  if (t==-2)
  {
    stop();
    return;
  }

  if (t!=-1)
  {

    int speed_angle=get_dangle(vx,vy);
    int wangle;
    if (abs(sub_angle(speed_angle,t))<Mp::QUARTANGLE)
    {
      wangle=t;
    }
    else
    {
      wangle=t+sub_angle(normalize_angle(speed_angle+Mp::HALFANGLE),t);
    }
    wangle=normalize_angle(wangle);
    if (get_speed()<50)
    {
      set_speed(50);
    }
    transform_speed(wangle);
    update_speed();
    order=WESCAPE;
  }
}

void Simple3::bang(void)
{
  int t;
  if (type==SPECIAL) t=Op::SIMPLE2SHOOTTIME;
  else t=Op::SIMPLE1SHOOTTIME;
  if (shtime>t+RAND%t)
  {
    shcount=constshcount;
    shtime=0;
    play(Mysound::arocket);
    NEW(Rocket(x,y,x,y,pangle,Op::CANNONROCKET,Op::obj_enemy[Op::SIMPLE3].weapon),gobjects_mbn);
    shcount=0;
  }
}

void Simple3::turn(void)
{
  if (Posit::distance(ship->get_x(),ship->get_y())<90)
  {
    int sh=get_dangle(x-ship->get_x(),y-ship->get_y());
    turn_angle(sh);
    angle=sh;
  }
  else
  {
    turn_point(tx,ty);
  }
}

void Simple3::run(void)
{
  if (!invisible||height==Mp::PIXELSBETWEENPLANES) radar_log();
  ship=NoNet::get_ship(Posit::x,Posit::y);
  if (!Posit::is_in_overscan(0,x,y))
  {
    if (!LevImp::in_crange(lobject->type,x,y))
    {
	    update_posit();
      smoke->kill();
      KILLME
    }
    return ;
  }
  smoke->set(x,y);
  switch (order)
  {
    case START:
              {
                if (get_life()<=0)
                {
                  E_AIR;
                  NEW(Capsule(x,y,Posit::UP,Capsule::WEAP1,0),gobjects_mbn);
                  Handle::post(GMANAGER,GameManager::code(lobject->type,DESTROY));
                  remove_from_level();
                  smoke->kill();
                  KILLME
                }

                if (!lobject->status)
                {
                  if (!invisible) log_to_view();
                  return;
                }
                turn_point(ship->get_x(),ship->get_y());
                update_phase(get_dir());
                go_up();
                if (invisible==1&&height>Mp::PIXELSBETWEENPLANES/2) invisible=2;
                if (is_up())
                {
        				  Link::send_impulse(level_num);
                  pangle=angle;
                  invisible=3;
                  order=GO;
                }
                else
                {
                  log_to_view();
                  return ;
                }
                break;
              }
    case GO:
              {
                time+=KbdStat::time();
                if (!shcount) shtime+=KbdStat::time();
                turn();

                int temp=angle;
                angle=pangle;
                turn_point(ship->get_x(),ship->get_y());
                update_phase(get_dir());
                pangle=angle;
                angle=temp;

                if (time>Op::SIMPLE1TARGETTIME)
                {
                  tx=ship->get_x()+RAND%Op::SIMPLE1APPROX-Op::SIMPLE1APPROX/2;
                  ty=ship->get_y()+RAND%Op::SIMPLE1APPROX-Op::SIMPLE1APPROX/2;
                  time=0;
                  if ((RAND%2)) strafe_left();
                  else strafe_right();
                }
                bang();
                faster();
                go();
                break;
              }
    case WESCAPE:
              {
                int speed_angle=get_angle(vx,vy);
                turn_angle(speed_angle);

                int temp=angle;
                angle=pangle;
                turn_point(ship->get_x(),ship->get_y());
                update_phase(get_dir());
                pangle=angle;
                angle=temp;

                brake();
                go();
                if (angle==speed_angle)
                {
                  order=GO;
                }
                break;
              }
    case DISABLE:
              {
                time+=KbdStat::time();
                shcount=0;
                //shtime+=KbdStat::time();

                int temp=angle;
                angle=pangle;
                int t=turn_angle(temp);
                update_phase(get_dir());
                pangle=angle;
                angle=temp;
                if (t)
                {
                  angle=RAND%Mp::TURNQUALITY;
                }

                bang();

                if (abs(sub_angle(pangle,angle))<Mp::ANGLE16)
                {
                  brake();
                }
                else
                {
                  faster();
                }
                if (!ship->is_invisible())
                {
                  order=GO;
                }
                go();
                break;

              }
  }
  if (ship->is_invisible())
  {
    order=DISABLE;
  }
  wall();
  if (get_chl()==Posit::UP)
  {
    for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
    {
      if (C_CHECK_ALIEN(temp))
      {
        decrease(temp->destroy);
      }
    }
    Colis::write(-1,vx,vy,get_sprite(),get_phase());
  }
  if (get_life()<=0)
  {
    E_AIR;
    NEW(Capsule(x,y,Posit::UP,Capsule::WEAP3,0),gobjects_mbn);
    Handle::post(GMANAGER,GameManager::code(lobject->type,DESTROY));
    remove_from_level();
    smoke->kill();
    Counter::present_alien(level_num);
    KILLME
  }
  else
    smoke->set_power(get_life()*100/Op::obj_enemy[Op::SIMPLE3].life);


  log_to_view();
}

void Simple3::draw_object(Screen& screen,int plane)
{
  switch (order)
  {
    case START:
      {
        switch (invisible)
        {
          case 0:  screen.put(x,y,get_sprite(),get_phase()); break;
          case 2:  screen.put(x,y-(Mp::PIXELSBETWEENPLANES-height),get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp);
			             break;
          case 1:  screen.put(x,y-(Mp::PIXELSBETWEENPLANES-height),get_sprite(),get_phase(),Spr::with_tsphard,(void*)&tsp);
			             break;
        }
        break;
      }
    default : screen.put(x,y,get_sprite(),get_phase()); break;
  }
}


void Simple3::draw_shadow(Screen& screen,int plane)
{
  switch (invisible)
  {
    case 0: screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
            break;
    case 1: screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::VLSHADOW]);
            break;
    case 2: screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::LSHADOW]);
            break;
    default:screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
  }
}
//MBOSS1
MBoss1::MBoss1(int _x,int _y,int lev,Sprite& _sprite,int _type,int _visible,int obj):
Posit(_x,_y,Posit::DOWN),
Move(0,0,Op::obj_mboss[_type]),
Shadow(_sprite,View::AMOV),
Colis(C_ALIENHERO,Op::obj_mboss[_type],"mboss"),
Life(Op::obj_mboss[_type]),
Listmanager(OAOBJ),
Radar(world->get_level()[lev].type),
Handle(DESTROY_OBJ),
Impexp(lev)
{
  obj_mission=obj;
  if (lobject->status) height=Mp::PIXELSBETWEENPLANES;
  if (lobject->user.short2>0) set_life(lobject->user.short2);
  ship=NoNet::get_ship(Posit::x,Posit::y);
  frqtime=chgun=shcounter=unblock=counter=time=0;
  lorder=order=START;
  type=_type;
  invisible=_visible;
  tx=ship->get_x()+RAND%Op::MB1APROXPOINT-Op::MB1APROXPOINT/2;
  ty=ship->get_y()+RAND%Op::MB1APROXPOINT-Op::MB1APROXPOINT/2;
  power=get_life();
  pangle=angle;
  rocketside=strafe_side=1;
  appear_snd=0;
  smoke1=NEW(AlienSmoke(x+x_offset(pangle,10),y+y_offset(pangle,10),angle,Mysprites::farmerdym,life_percent()),gobjects_mbn);
  smoke2=NEW(AlienSmoke(x-x_offset(pangle,10),y-y_offset(pangle,10),angle,Mysprites::farmerdym,life_percent()),gobjects_mbn);
  shtime=RAND%(Op::def_mboss[type].passive*Mp::METRONQUALITY);
}

void MBoss1::update_posit(void)
{
  lobject->y=y;
  lobject->x=x;
  world->get_level().update(level_num);
}


void MBoss1::bang(void)
{
  switch(type)
  {
    case Op::MBOSS1:  bang1(); break;
    case Op::MBOSS2:  bang2(); break;
    case Op::MBOSS3:  bang3(); break;
    case Op::MBOSS4:  bang4(); break;
  }
}

void MBoss1::bang1(void)
{
  if (counter>=Op::def_mboss[type].frq)
  {
    if (ship->is_invisible()) return;
    int ang=normalize_angle(pangle+Mp::QUARTANGLE);
    int ox=x_offset(ang,12);
    int oy=y_offset(ang,12);
    int speed=get_speed();
    Sound::play(Mysound::mboss1_fire);
    NEW(ASplash(x,y,x,y,pangle,Op::mb1splash,Op::def_mboss[type].weapon,Mysprites::asplash),gobjects_mbn);
    counter=0;
  }
  else
    counter+=KbdStat::time();
}

void MBoss1::bang2(void)
{
  if (counter>=Op::def_mboss[type].frq)
  {
    if (ship->is_invisible()) return;
    Sound::play(Mysound::mboss2_fire);
    int _x=0;
    int _y=0;

    if (rocketside>0)
    {
      _x=x_offset(get_angle(Mp::QUARTANGLE),30);
      _y=y_offset(get_angle(Mp::QUARTANGLE),30);
    }
    else
    {
      _x=x_offset(get_angle(-Mp::QUARTANGLE),30);
      _y=y_offset(get_angle(-Mp::QUARTANGLE),30);
    }
    NEW(Rocket(x,y,x+_x,y+_y,pangle,Op::mboss2rocket,Op::def_mboss[type].weapon),gobjects_mbn);
    rocketside=-rocketside;
    counter=0;
  }
  else
    counter+=KbdStat::time();
}

void MBoss1::bang3(void)
{
  if (counter>=Op::def_mboss[type].frq)
  {
    if (ship->is_invisible())
    {
      chgun=0;
      return;
    }
    Sound::play(Mysound::mboss3_fire);
    NEW(AlienChgun(x,y,pangle,Op::def_mboss[type].weapon),gobjects_mbn);
    counter=0;
    chgun=6;
  }
  else
  {
    if (chgun>0) chgun--;
    counter+=KbdStat::time();
  }
}

void MBoss1::bang4(void)
{
  if (counter>=Op::def_mboss[type].frq)
  {
    if (ship->is_invisible()) return;
    Sound::play(Mysound::mboss4_fire);
    NEW(Morph(x,y,pangle,level_num),gobjects_mbn);
    counter=0;
  }
  else
    counter+=KbdStat::time();
}


void MBoss1::HandleEvent(int message)
{
  if (message==lobject->type&&lobject->aux2==-200)
  {
    decrease(get_life()+100);
  }
}


void MBoss1::turn(void)
{
  if (Posit::distance(ship->get_x(),ship->get_y())<120&&!ship->is_invisible())
  {
    int sh=get_dangle(x-ship->get_x(),y-ship->get_y());
    turn_angle(sh);
    angle=sh;
    return;
  }
  turn_point(tx,ty);
}


void MBoss1::app_point(void)
{
  tx=ship->get_x()+RAND%Op::MB1APROXPOINT-Op::MB1APROXPOINT/2;
  ty=ship->get_y()+RAND%Op::MB1APROXPOINT-Op::MB1APROXPOINT/2;
  /*
  if (Posit::distance(lobject->user.short3,lobject->user.short4)<350)
  {
    tx=ship->get_x()+RAND%Op::MB1APROXPOINT-Op::MB1APROXPOINT/2;
    ty=ship->get_y()+RAND%Op::MB1APROXPOINT-Op::MB1APROXPOINT/2;
  }
  else
  {
    tx=lobject->user.short3+RAND%Op::MB1APROXPOINT-Op::MB1APROXPOINT/2;
    ty=lobject->user.short4+RAND%Op::MB1APROXPOINT-Op::MB1APROXPOINT/2;
  }
  */
}

void MBoss1::wall(void)
{
  int t=wall_angle2(13);
  if (t==-2)
  {
    stop();
    return;
  }
  if (t!=-1)
  {
    int speed_angle=get_dangle(vx,vy);
    int wangle;
    if (abs(sub_angle(speed_angle,t))<Mp::QUARTANGLE)
    {
      wangle=t;
    }
    else
    {
      wangle=t+sub_angle(normalize_angle(speed_angle+Mp::HALFANGLE),t);
      strafe_side=-strafe_side;
    }
    wangle=normalize_angle(wangle);
    if (get_speed()<50)
    {
      set_speed(50);
    }
    transform_speed(wangle);
    update_speed();
    if (order!=WESCAPE) lorder=order;
    order=WESCAPE;
  }
}

void MBoss1::phs_turn(void)
{
   int temp=angle;
   angle=pangle;

   if (!ship->is_invisible()) turn_point(ship->get_x(),ship->get_y());
   else turn_point(tx,ty);
   update_phase(get_dir());
   pangle=angle;
   angle=temp;
}

void MBoss1::appear(void)
{
  if (!appear_snd)
  {
    ship->play_ship_sample(Mysound::speech_largeshiprange,Myship::SPEECH);
    MSGTXT(GAMETXT("LARGE_SHIP"),0);
    appear_snd=1;
  }
}


void MBoss1::run(void)
{
  //if (!obj_mission) radar_log();
  radar_log();
  ship=NoNet::get_ship(Posit::x,Posit::y);
  if (!Visible::is_in_overscan())
  {
    if (!obj_mission&&!LevImp::in_crange(lobject->type,x,y))
    {
	    update_posit();
      smoke1->kill();
      smoke2->kill();
      lobject->user.short2=get_life();
      KILLME
    }
    return ;
  }
  smoke1->set(x+x_offset(pangle,10),y+y_offset(pangle,10));
  smoke2->set(x-x_offset(pangle,10),y-y_offset(pangle,10));
  switch (order)
  {
    case START:
             {
                if (get_life()<=0)
                {
                  E_AIR;
                  NEW(Capsule(x,y,Posit::UP,Capsule::LIFE,1),gobjects_mbn);
                  Handle::post(GMANAGER,GameManager::code(lobject->type,DESTROY));
                  smoke1->kill();
                  smoke2->kill();
                  remove_from_level();
                  KILLME
                }

               if (lobject->status&&!unblock)
               {
                 unblock=1;
               }
               if (!unblock)
               {
                 log_to_view();
                 return;
               }
               turn_left();
               update_phase(get_dir());
               go_up();
           //    if (invisible==1&&height>Mp::PIXELSBETWEENPLANES/2) invisible=2;
               if (is_up())
               {
				         Link::send_impulse(level_num);
                 pangle=angle;
                 order=GO;
               }
               else
               {
                 log_to_view();
                 return ;
               }
               break;
             }
    case ATTACK:
             {
               appear();
               int toangle=get_angle(ship->get_x(),ship->get_y());
               if (strafe_side>0) strafe(normalize_angle(toangle+Mp::QUARTANGLE));
               else strafe(normalize_angle(toangle-Mp::QUARTANGLE));

               faster();
               time+=KbdStat::time();
               shtime+=KbdStat::time();
               turn();
               phs_turn();
               bang();
               if (shtime>Op::def_mboss[type].active*Mp::METRONQUALITY)
               {
                 order=GO;
                 chgun=shcounter=counter=time=shtime=0;
               }
               if (time>Op::MB1TARGETTIME)
               {
                 app_point();
                 time=0;
               }
               break;
             }
    case GO:
             {
               appear();
               time+=KbdStat::time();
               shtime+=KbdStat::time();
               turn();

               phs_turn();

               if (time>Op::MB1TARGETTIME)
               {
                 app_point();
                 time=0;
               }

               if (shtime>Op::def_mboss[type].passive*Mp::METRONQUALITY)
               {
                 shcounter=counter=time=shtime=0;
                 order=ATTACK;
               }
               faster();
               break;
             }
    case WESCAPE:
             {
               time+=KbdStat::time();
               shtime+=KbdStat::time();

               int speed_angle=get_angle(vx,vy);
               turn_angle(speed_angle);

               phs_turn();

               if (lorder==ATTACK)
               {
                 if (shtime>Op::def_mboss[type].active*Mp::METRONQUALITY)
                 {
                   lorder=GO;
                   chgun=shcounter=counter=time=shtime=0;
                 }
                 bang();
               }

               if (lorder==GO)
               {
                 if (shtime>Op::def_mboss[type].passive*Mp::METRONQUALITY)
                 {
                   lorder=ATTACK;
                   chgun=shcounter=counter=time=shtime=0;
                 }
               }

               brake();
               if (angle==speed_angle)
               {
                 app_point();
                 order=lorder;
               }
               break;
             }
  }
  if (get_chl()==Posit::UP)
  {
    for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
    {
      if (C_CHECK_ALIEN(temp))
      {
        decrease(temp->destroy);
      }
    }
    Colis::write(-1,vx,vy,get_sprite(),get_phase());
  }
  if (get_life()<=0)
  {
    kill();
    Handle::post(GMANAGER,GameManager::code(lobject->type,DESTROY));
    remove_from_level();
    smoke1->kill();
    smoke2->kill();

    switch(type)
    {
      case Op::MBOSS1:   NEW(Capsule(x,y,Posit::UP,Capsule::WEAP2,1),gobjects_mbn); break;
      case Op::MBOSS2:   NEW(Capsule(x,y,Posit::UP,Capsule::WEAP3,1),gobjects_mbn); break;
      case Op::MBOSS3:   NEW(Capsule(x,y,Posit::UP,Capsule::WEAP0,1),gobjects_mbn); break;
      case Op::MBOSS4:   NEW(Capsule(x,y,Posit::UP,Capsule::LIFE,1),gobjects_mbn);  break;
    }
    KILLME
  }
  else
  {
    smoke1->set_power(life_percent());
    smoke2->set_power(life_percent());
  }
  wall();
  go();
  log_to_view();
}

void MBoss1::kill(void)
{
  Counter::present_alien(level_num);
  NEW(Explode(x+RAND%10+10,y+RAND%10+10,Posit::UP,0,Explode::AIR,Mysprites::expl_air,Explode::ENABLE,1),gobjects_mbn);
  NEW(Explode(x-RAND%10-10,y+RAND%10+10,Posit::UP,8,Explode::AIR,Mysprites::expl_air,Explode::ENABLE,1),gobjects_mbn);
  NEW(Explode(x,y-RAND%20+20,Posit::UP,0,Explode::AIR,Mysprites::expl_air,Explode::ENABLE,1),gobjects_mbn);
  if (type==Op::MBOSS4)
    REPEAT(3)  NEW(Morph(x,y,RAND%Mp::TURNQUALITY,level_num),gobjects_mbn);
  Myship* ship=NoNet::get_ship();
  //parametry
  ship->shake_screen(Mp::METRONQUALITY);
  for (int i=0;i<5;i++)
  {
    int sp=RAND%100+150;
    NEW(Bullet(x,y,RAND%Mp::TURNQUALITY,sp,sp/10),gobjects_mbn);
  }
}


void MBoss1::draw_object(Screen& screen,int plane)
{
  (void)plane;
  switch (order)
  {
  case START:
      {
        switch (invisible)
        {
          case 0:  screen.put(x,y,get_sprite(),get_phase()); break;
          case 2:  screen.put(x,y-(Mp::PIXELSBETWEENPLANES-height),get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp);
			             break;
          case 1:  screen.put(x,y-(Mp::PIXELSBETWEENPLANES-height),get_sprite(),get_phase(),Spr::with_tsphard,(void*)&tsp);
			             break;
        }
        break;
      }
    default : screen.put(x,y,get_sprite(),get_phase()); break;
  }

  if (chgun>0)
  {
    if (chgun>3) screen.put(x,y,Mysprites::mboss3_fire_left,get_phase());
    else         screen.put(x,y,Mysprites::mboss3_fire_right,get_phase());
  }
}

void MBoss1::draw_shadow(Screen& screen,int plane)
{
  (void)plane;
  switch (invisible)
  {
    case 0: screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
            break;
    case 1: screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::VLSHADOW]);
            break;
    case 2: screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::LSHADOW]);
            break;
    default:screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
  }
}

//MINE
Mine::Mine(int _x,int _y,int _chl,Sprite& _sprite,int _level_num,int _type):
Posit(_x,_y,_chl),
Move(0,0,Op::MINE),
Shadow(_sprite,View::AMOV),
Listmanager(OMINE),
Colis(C_MINEBEGIN,Op::MINE,"mine"),
Life(Op::MINE),
Impexp(_level_num)
{
  nonsmp=smpl=t=tiku=range=0;
  global_time=Mp::BEYONDTIME;
  bomb_time=0;
  if (lobject->status==2) height=0;
  type=_type;
  smpl=flash=counter=0;
}

Mine::~Mine(void)
{
  if (!to_return&&type!=MYMINE)
  {
    lobject->x=lobject->aux1;
    lobject->y=lobject->aux2;
    lobject->status=-1;
    world->get_level().update(level_num);
  }
}

void Mine::kill(void)
{
  for (LowColis *temp=LowColis::first_logged();temp!=NULL;temp=LowColis::next_logged())
  {
    Cmsg *msg=temp->respond();
    if (distance(msg->x,msg->y)<90) temp->add_cmsg(respond(),0);
  }
  Myship* ship=NULL;
  FOR_ALL_USERS(ID)
  {
    ship=NoNet::get_ship(ID);
    Cmsg *msg=ship->respond();
    if (distance(msg->x,msg->y)<90) ship->add_cmsg(respond(),0);
  }
}

int Mine::check_obj(void)
{
  for (LowColis *temp=LowColis::first_logged();temp!=NULL;temp=LowColis::next_logged())
  {
    Cmsg *msg=temp->respond();

    if (C_MINE_CHECKOBJ(msg))
    {
      if (distance(msg->x,msg->y)<80)  return 1;
    }
  }

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

void Mine::update_posit(void)
{
  lobject->y=y;
  lobject->x=x;
  world->get_level().update(level_num);
}


void Mine::serv_kill(void)
{
  if (get_chl()==Posit::UP)
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
  }
  if (get_life()<=0) flash=1;
}

void Mine::run(void)
{
  if (!Posit::is_in_overscan())
  {
    if ((global_time-=KbdStat::time())<0)
    {
      if(!flash)
      {
        update_posit();
        KILLME
      }
    }
  }
  else global_time=Mp::BEYONDTIME;

  if (height<Mp::PIXELSBETWEENPLANES)
  {
    plus_phase();
    log_to_view();
    go_up();
    return;
  }

  if (type==TRACER)
  {
    FOR_ALL_USERS(ID)
    {
      Myship* ship=NoNet::get_ship(ID);
      if (ship->is_invisible()||distance(ship->get_x(),ship->get_y())>350) continue;
      int ship_angle=Turn::get_dangle(ship->get_x()-x,ship->get_y()-y);
      faster(ship_angle);
      update_speed();
    }
    auto_brake();
    go();
  }

  plus_phase();
  t=check_obj();

  if (get_life()<-50)
  {
    flash=1;
    counter=5*Mp::METRONQUALITY;
  }

  if (!flash)
    tiku=flash=t;

  if (flash)
  {
    if (counter>1200)
    {
      if (!NoNet::is_network_mode())
      {
        set_cinfo(C_MINEKILLALIEN);
        kill();
      }
      set_cinfo(C_MINEKILLMY);
      kill();
      E_AIR;
      if (type!=MYMINE)
      {
        Handle::post(GMANAGER,GameManager::code(lobject->type,DESTROY));
        remove_from_level();
      }
      /*
      if (NoNet::is_network_mode())
      {
        bomb_time=1;
        return;
      }
      */
      KILLME
    }
    else
    {
      counter+=KbdStat::time();
    }
    if (!smpl)
    {
      play(Mysound::mine);
      smpl=1;
    }
  }

  serv_kill();
  log_to_view();
  Sound::run();
}

void Mine::draw_object(Screen& screen,int plane)
{
  (void)plane;
  if (flash&&(counter/100%2))
  {
    screen.put(x,y,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::LIGHT]);
  }
  else
  {
    screen.put(x,y,get_sprite(),get_phase());
  }
}
//ACANNON
const int CANNONHEIGHT=8;

ACannon::ACannon(int _x,int _y,Sprite& _sprite,int _level_num,int _plane,Op::CannonPack *_param):
Posit(_x,_y,Posit::UP),
Visible(_sprite,View::ASTAT,world->get_level()[_level_num].phase),
Colis(C_ALIENSTATIC,0,"ACannon"),
Listmanager(OSTAT),
Life(_param->life),
Handle(DESTROY_OBJ),
Turn(0,_param->turn_speed),
Impexp(_level_num)
{
  spx=_x;
  spy=_y;
  y-=15;
  ship=NoNet::get_ship(Posit::x,Posit::y);
  if (lobject->user.short2>0) set_life(lobject->user.short2);
  angle=startangle=get_dir_angle(get_phase());
  angle=normalize_angle(angle+lobject->user.short1);
  plane=_plane;
  param=_param;
  fr_shoot=0;
  global_time=Stale_memory::word(dynamic_cast<void*>(this));  // was global_time==Mp::BEYONDTIME (stale.h)
  smoketime=shtime=shoot=time=0;
  time=Game::get_timer()%(param->active+param->passive);
  if (time>=param->passive)
  {
    time-=param->passive;
    order=GO;
  }
  else order=FIRE;
  smoke=NEW(AlienSmoke(x,y,angle,Mysprites::farmerdym,life_percent(),_plane),gobjects_mbn);
  y-=CANNONHEIGHT;
}

void ACannon::HandleEvent(int message)
{
  if (message==lobject->type&&lobject->aux2==-200)
    decrease(get_life()+100);
}

void ACannon::service(void)
{
  if (ship->is_invisible()) return;
  if (param->angle==Mp::HALFANGLE)
  {
    turn_point(ship->get_x(),ship->get_y());
    update_phase(get_dir());
    return;
  }
  int opp=normalize_angle(startangle+Mp::HALFANGLE);
  int shipang=get_dangle(ship->get_x()-x,ship->get_y()-y);

  if (sign(sub_angle(opp,angle))==sign(sub_angle(opp,shipang))||angle==startangle)
  {
    turn_angle(shipang);
  }
  else
  {
    turn_angle(startangle);
  }
  int sgn=sub_angle(startangle,angle);
  if (abs(sgn)>param->angle)
  {
    if (sgn<0) angle=normalize_angle(startangle-param->angle);
    else angle=normalize_angle(startangle+param->angle);
  }
  update_phase(get_dir());
}

void ACannon::bang(void)
{

  if (!Posit::is_in_overscan()) return;
  int t=!(abs(sub_angle(get_angle(ship->get_x(),ship->get_y()),startangle))<=param->angle);
  if (ship->is_invisible()||t) return;
  if (param->gun==1)
  {
    if (sub_angle(get_dangle(ship->get_x()-x,ship->get_y()-y),angle)<Mp::EIGHTANGLE)
    {
      int tx=lobject->x+(get_sprite()[get_phase()].r-Mysprites::cannonpoint1[get_phase()].r);
      int ty=lobject->y+(get_sprite()[get_phase()].d-Mysprites::cannonpoint1[get_phase()].d);
      shtime+=KbdStat::time();
      if (shtime>Op::CANNONSPLASHFRQ)
      {
        play(Mysound::bullet);
        NEW(ASplash(lobject->x,lobject->y,tx,ty,angle,Op::CANNONSPLASH,param->weaponpower,Mysprites::cannonsplash,plane),gobjects_mbn);
        shtime=0;
      }
    }
  }
  else if (param->gun==2)
  {
    if (sub_angle(get_dangle(ship->get_x()-x,ship->get_y()-y),angle)<Mp::EIGHTANGLE)
    {
      int tx=lobject->x+(get_sprite()[get_phase()].r-Mysprites::cannonpoint2[get_phase()].r);
      int ty=lobject->y+(get_sprite()[get_phase()].d-Mysprites::cannonpoint2[get_phase()].d);
      if (param->active/2<time&&!shoot)
      {
        if (!fr_shoot)
        {
          ship->play_ship_sample(Mysound::speech_incomemissile,Myship::SPEECH);
          MSGTXT(GAMETXT("INCOMING_MISSILE"),0);
          fr_shoot=1;
        }
        smoke->start();
        play(Mysound::cannonrocket);
        NEW(Rocket(spx,spy,tx,ty,angle,Op::CANNONROCKET,param->weaponpower,plane),gobjects_mbn);
        smoketime=300;
        shoot=1;
      }
    }
  }
}

void ACannon::serv_kill(void)
{
  for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
  {
    if (C_CHECK_ALIEN(temp))
    {
      decrease(temp->destroy);
    }
  }
  LowColis::write(-1,spx,spy,0,0,get_sprite(),get_phase());

  if (get_life()<=0)
  {
    E_BIG;
    Handle::post(GMANAGER,GameManager::code(lobject->type,DESTROY));
    smoke->kill();
    Counter::present_alien(level_num);
    if (param->gun==1)  NEW(Cinders(spx,spy,Mysprites::cinder_pulse,level_num,Cinders::PULSE,plane),gobjects_mbn);
    if (param->gun==2)  NEW(Cinders(spx,spy,Mysprites::cinder_cannon,level_num,Cinders::CANNON,plane),gobjects_mbn);
    remove_from_level();
    KILLME
  }

}


void ACannon::run(void)
{
  if (!Posit::is_in_overscan(0,spx,spy))
  {
    if ((global_time-=KbdStat::time())<0)
    {
      smoke->kill();
      lobject->user.short2=get_life();
      lobject->user.short1=sub_angle(startangle,angle);
      KILLME
    }
  }
  else  global_time=Mp::BEYONDTIME;

  ship=NoNet::get_ship(Posit::x,Posit::y);
  time=Game::get_timer()%(param->active+param->passive);
  smoketime-=KbdStat::time();
  if (time>=param->passive)
  {
    time-=param->passive;
    order=FIRE;
  }
  else
  {
    order=GO;
    shoot=0;
  }

  smoke->set_power(life_percent());

  switch(order)
  {
    case FIRE: bang();
    case   GO: service(); break;
  }
  serv_kill();
  log_to_view(plane);
}


void ACannon::draw_object(Screen& screen,int plane)
{
  screen.put(spx,spy,get_sprite(),get_phase());
}

//RAFT

Raft::Raft(int _x,int _y,Sprite& _sprite,int _level_num,int _id):
Posit(_x,_y,Posit::UP),
Move(0,0,Op::RAFT),
Visible(_sprite,View::AMOV),
Colis( C_MYRAFE(_id),0,"raft"),
Life(Op::RAFT),
Listmanager(OAOBJ),
Radar(world->get_level()[_level_num].link),
Impexp(_level_num)
{
  if (!lobject->link) FAILURE ("obj_raft nie dolinkowany");
  angle=Mp::HALFANGLE;
  set_speed(Op::RAFT.speed);
  time=0;
  id=_id;
}

void Raft::run(void)
{
  go();
  Lobject* o=&world->get_level()[lobject->link];
  if (o->y<y)
  {
    Handle::post(GMANAGER,GameManager::code(o->type,DESTROY));
    remove_from_level();
    KILLME
  }
  time+=KbdStat::time();
  if (time>150)
  {
    plus_phase();
    time=0;
  }
  for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
  {
    if (C_CHECK_RAFT(temp,id))
      decrease(temp->destroy);
  }
  Colis::write(-1,vx,vy,get_sprite(),get_phase());

  if (get_life()<=0)
  {
    E_BIG;
    Handle::post(GMANAGER,GameManager::code(lobject->type,DESTROY));
    remove_from_level();
    KILLME
  }
  log_to_view();
}


//BUOY
Buoy::Buoy(int _x,int _y,Sprite& _sprite,int _level_num,World* _world):
Posit(_x,_y,Posit::UP),
Visible(_sprite,View::AMOV),
Listmanager(OMINE),
Colis(C_MINEBEGIN,Op::RAFTMINEPOWER,"mine"),
Life(Op::RAFTMINELIFE),
Impexp(_level_num,_world)
{
}

void Buoy::kill(void)
{
  for (LowColis *temp=LowColis::first_logged();temp!=NULL;temp=LowColis::next_logged())
  {
    Cmsg *msg=temp->respond();
    if (distance(msg->x,msg->y)<90) temp->add_cmsg(respond(),0);
  }
  Myship* ship=NULL;
  FOR_ALL_USERS(ID)
  {
    ship=NoNet::get_ship(ID);
    Cmsg *msg=ship->respond();
    if (distance(msg->x,msg->y)<90) ship->add_cmsg(respond(),0);
  }
}

int Buoy::check_obj(void)
{
  for (LowColis *temp=LowColis::first_logged();temp!=NULL;temp=LowColis::next_logged())
  {
    Cmsg *msg=temp->respond();

    if (C_MINE_CHECKOBJ(msg))
    {
      if (distance(msg->x,msg->y)<80)  return 1;
    }
  }

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

void Buoy::serv_kill(void)
{
  if (get_chl()==Posit::UP)
  {
    Colis::check(-1,0,0,get_sprite(),get_phase());
    for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
    {
      if (C_MINE_CHECK(temp))  decrease(temp->destroy);
    }
    Colis::write(-1,0,0,get_sprite(),get_phase());
  }
}

void Buoy::run(void)
{
  plus_phase();
  if (check_obj()||get_life()<=0)
  {
    set_cinfo(C_MINEKILLALIEN);
    kill();
    set_cinfo(C_MINEKILLMY);
    kill();
    E_AIR;
    remove_from_level();
    KILLME
  }

  serv_kill();
  log_to_view();
}

//THIEF
Thief::Thief(int _x,int _y,Sprite& _sprite,int _level_num):
Posit(_x,_y,Posit::UP),
Move(0,Op::THIEF.speed,Op::THIEF),
Shadow(_sprite,View::AMOV),
Colis(C_ALIENHERO,Op::THIEF,"Thief"),
Listmanager(OAOBJ),
Life(Op::THIEF),
Radar(world->get_level()[_level_num].type),
Impexp(_level_num)
{
  time=0;
  link=lobject->link;
  smoke=NEW(AlienSmoke(x,y,angle,Mysprites::farmerdym,100),gobjects_mbn);
}


void Thief::run(void)
{
  Lobject *o=&world->get_level()[link];
  angle=get_angle(o->x,o->y);
  time+=KbdStat::time();
  if (time>150)
  {
    plus_phase();
    time=0;
  }
  faster();
  if (Posit::distance(o->x,o->y)<15)
  {
    Handle::post(GMANAGER,GameManager::code(o->type,DESTROY));
    if (o->link==level_num)
    {
      remove_from_level();
      smoke->kill();
      KILLME
    }
    link=o->link;
  }
  go();
  smoke->set(x,y);
  for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
  {
    if (C_CHECK_ALIEN(temp))
    {
      decrease(temp->destroy);
    }
  }
  Colis::write(-1,vx,vy,get_sprite(),get_phase());

  smoke->set_power(life_percent());

  if (get_life()<=0)
  {
    E_AIR;
    Handle::post(GMANAGER,GameManager::code(lobject->type,DESTROY));
    remove_from_level();
    smoke->kill();
    KILLME
  }
  log_to_view();
}
//KLASA SHIPFUCKER
int ShipFucker::initialized=0;

void ShipFucker::init(void)
{
  DBG_CHECK(!initialized);
  initialized=1;
}

void ShipFucker::quit(void)
{
  initialized=0;
}

void ShipFucker::start(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(!NoNet::is_network_mode());
  NoNet::get_ship()->start_shipfuck();
}

void ShipFucker::stop(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(!NoNet::is_network_mode());
  NoNet::get_ship()->stop_shipfuck();
}

















































