#include "headers.h"

//ROCKET
Rocket::Rocket(int sx,int sy,int _x,int _y,int _angle,Op::ObjPack params,int force,int _plane):
Posit(_x,_y,Posit::UP),
Move(_angle,params.speed,params),
Listmanager(OAWEAP),
Colis(C_ALIENSHOT,force,"alien rocket"),
Visible(Mysprites::arocket,View::ACSHT)
{
  stime=time=0;
  X=sx;
  Sx=_x;
  Y=sy;
  Sy=_y;
  plane=(_plane==-1)?get_plane():_plane;
  wave=NEW(Wave(x,y,angle,Mysprites::dymki),gobjects_mbn);
  ship=NoNet::get_ship(Posit::x,Posit::y);
}

void Rocket::run(void)
{
  ship=NoNet::get_ship(Posit::x,Posit::y);
  time+=KbdStat::time();
  stime+=KbdStat::time();

  if ((time>=KbdStat::time()*5)&&!ship->is_invisible()) 
  {
    int ship_angle=get_dangle(ship->get_x()-x,ship->get_y()-y); 
    if (abs(Turn::sub_angle(angle,ship_angle))<(Mp::TURNQUALITY/16))
    {
      turn_point(ship->get_x(),ship->get_y());
    }
  }
  wave->set(x,y);
  plus_phase();
  faster();
  go();

	if (distance(Sx,Sy)>8)
	{
    NEW(Splinter(x+RAND%4-2,y+RAND%4-2,Posit::UP,Mysprites::dymki,2),gobjects_mbn);
    NEW(Splinter(x+RAND%4-2-x_offset(angle,8),y+RAND%4-2-y_offset(angle,8),Posit::UP,Mysprites::dymki,2),gobjects_mbn);
    Sx=x;
    Sy=y;
	}

  Colis::check(-1,vx,vy,get_sprite(),get_phase());
  for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
  {
    if (C_CHECK_ALIENSH(temp))
    {
      if (!(temp->x==X&&temp->y==Y))
      {
        wave->kill();
        E_SMALL;
        KILLME
      }
    }
  }
  LowColis::write_to_logged(x,y,vx,vy);

  log_to_view(plane);
  if (!Visible::is_in_overscan()||time>5*Mp::METRONQUALITY)
  {
    wave->kill();
    E_SMALL;
    KILLME
  }
}

void Rocket::add_cmsg(Cmsg* temp,int _id)
{
  (void)_id;
  if (C_CHECK_ALIENSH(temp))
  {
    LowColis::add_cmsg(temp,_id);
  }
}

void Rocket::draw_object(Screen& screen,int plane)
{
  Light::Ray(screen,&Mysprites::lihgtexpl,x,y,3);
  Visible::draw_object(screen,plane);
}
//BULLET
Bullet::Bullet(int _x,int _y,int _angle,int _speed,int _force):
Posit(_x,_y,Posit::UP),
Listmanager(OAWEAP),
Visible(Mysprites::mbosssplinter,View::AWEAP),
Move(_angle,_speed,Op::bullet),
Colis(C_ALIENSHOT,_force,"Bullet")
{
  X=_x;
  Y=_y;
  sangle=time=0;
  wave=NEW(Wave(x,y,angle,Mysprites::farmerdym),gobjects_mbn);
}

void Bullet::run(void)
{
  Myship* ship=NoNet::get_ship(Posit::x,Posit::y);
  if (!Visible::is_in_overscan()) 
  {
    wave->kill();
    KILLME
  }
  time+= KbdStat::time();
  if (time>150) 
  {
    plus_phase();
    sangle=normalize_angle(sangle+Mp::ANGLE16/2);
  }
  go();
  X=x+x_offset(sangle,12);
  Y=y+y_offset(sangle,12);
  wave->set(X,Y);

  Colis::check(-1,vx,vy,get_sprite(),get_phase());
  if (get_chl()==Posit::UP)
  {
    for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
    {
      if (C_CHECK_ALIENSH(temp))
      {
        wave->kill();
        E_SMALL;
        KILLME
      }
    }
  }
  LowColis::write_to_logged(x,y,vx,vy);
  log_to_view();
  Sound::run();
}

void Bullet::draw_object(Screen& screen,int plane)
{
  (void) plane;
  screen.put(x,y,get_sprite(),get_phase());
  Light::Ray(screen,&Mysprites::lihgtexpl,X,Y,3);
}

void Bullet::add_cmsg(Cmsg* temp,int _id)
{
  (void)_id;
  if (C_CHECK_ALIENSH(temp))
  {
    LowColis::add_cmsg(temp,_id);
  }
}

//ASPLASH
ASplash::ASplash(int _sx,int _sy,int _x,int _y,int _angle,Op::ObjPack& param,int force,Sprite& _sprite,int _plane):
Posit(_x,_y,Posit::UP),
Move(_angle,param.speed,param),
Colis(C_ALIENSHOT,force,"alien machine gun"),
Listmanager(OAWEAP),
Visible(_sprite,View::ACSHT)
{
  plane=_plane;
  if (!plane) plane=get_plane();
  X=_sx;
  Y=_sy;
  transform_speed();
  update_phase(get_dir());
}


void ASplash::run(void)
{
  if (!Visible::is_in_overscan())
  {
    KILLME
  }
  go();
  Colis::check(-1,vx,vy,get_sprite(),get_phase());
  for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
  {
    if (C_CHECK_ALIENSH(temp))
    {
      if (temp->x==X&&temp->y==Y) continue;
      NEW(Boom(x,y,Posit::UP),gobjects_mbn);
      KILLME
    }
  }

  LowColis::write_to_logged(x,y,vx,vy);

  update_phase(get_dir());
  log_to_view(plane);
  Sound::run();
}

void ASplash::add_cmsg(Cmsg* temp,int _id)
{
  (void)_id;
  if (C_CHECK_ALIENSH(temp))
  {
    LowColis::add_cmsg(temp,_id);
  }
}
//PLASK
Plask::Plask(int _x,int _y,int _angle,int _destroy):
Listmanager(OAWEAP),
LowColis(C_ALIENSHOT,_destroy),
Visible(View::NEXPL)
{
  x=_x;
  y=_y;
  angle=_angle;
  done=0;
}

void Plask::init(int _x,int _y)
{
  for (int i=0;i<8;i++)
  {
    body[i].x=_x<<8;
    body[i].y=_y<<8;
    body[i].vx=RAND%200-100;
    body[i].vy=RAND%200-100;
    body[i].time=RAND%400;
  }
  done=1;
}

void Plask::run(void)
{

  if (!done)
  {
    int vx=x_offset(angle,5);
    int vy=y_offset(angle,5);
    Cmsg *temp=NULL;
    for (;;x+=vx,y+=vy)
    {
      check(-1,x,y,vx,vy);
      temp=get_cmsg();
      if (temp&&C_CHECK_ALIENSH(temp))
      {
        init(x,y);
        break;
      }
      if (!Posit::is_in_overscan(0,x,y))
      {
        KILLME
      }
    }
  }

  int t=0;
  for (int i=0;i<8;i++)
  {
    body[i].x+=body[i].vx;
    body[i].y+=body[i].vy;
    body[i].time-=KbdStat::time();
    if (body[i].time>t) t=body[i].time;
  }
  if (t<=0) KILLME
  log_to_view(get_plane()+2);
}

void Plask::draw_object(Screen& screen,int plane)
{
  (void)plane;
  for (int i=0;i<8;i++)
  {
    if (body[i].time>0)
    {
      screen.plot(Tools::tool[Tools::LIGHT16][screen.check(body[i].x>>8,body[i].y>>8)],body[i].x>>8,body[i].y>>8);
    }
  }
}

//ALIENCHGUN
AlienChgun::AlienChgun(int _x,int _y,int _angle,int _destroy):
Listmanager(OAWEAP),
LowColis(C_ALIENSHOT,_destroy),
Visible(View::NEXPL)
{
  x=_x;
  y=_y;
  angle=_angle;
  done=0;
}

void AlienChgun::init(int _x,int _y)
{
  for (int i=0;i<MAX_ALIENCHGUNBODY;i++)
  {
    body[i].x=_x<<8;
    body[i].y=_y<<8;
    body[i].vx=RAND%400-200;
    body[i].vy=RAND%400-200;
    body[i].time=RAND%Mp::METRONQUALITY;
  }
  done=1;
}

void AlienChgun::run(void)
{
  if (!done)
  {
    int vx=x_offset(angle,5);
    int vy=y_offset(angle,5);
    Cmsg *temp=NULL;
    for (;;x+=vx,y+=vy)
    {
      check(-1,x,y,vx,vy);
      temp=get_cmsg();
      if (temp&&C_CHECK_ALIENSH(temp))
      {
        init(x,y);
        break;
      }
      if (!Posit::is_in_overscan(0,x,y))  KILLME
    }
  }

  int t=0;
  for (int i=0;i<MAX_ALIENCHGUNBODY;i++)
  {
    body[i].x+=body[i].vx;
    body[i].y+=body[i].vy;
    body[i].time-=KbdStat::time();
    if (body[i].time>t) t=body[i].time;
  }
  if (t<=0) KILLME
  log_to_view(get_plane()+2);
}

void AlienChgun::draw_object(Screen& screen,int plane)
{
  (void)plane;
  for (int i=0;i<MAX_ALIENCHGUNBODY;i++)
  {
    if (body[i].time>0)
      screen.plot(Tools::tool[Tools::LIGHT16][screen.check(body[i].x>>8,body[i].y>>8)],body[i].x>>8,body[i].y>>8);
  }
}
//MORPH
Morph::Morph(int _x,int _y,int _angle,int _bo_num,int _type):
Posit(_x,_y,Posit::UP),
Move(_angle,Op::morph.speed,Op::morph),
Colis(C_NEUTRAL,0,"morph"),
Listmanager(OAOBJ),
Shadow(Mysprites::bomb,View::AWEAP)
{
  shangle=blc=phs=mtime=time=0;
  bo_num=_bo_num;
  type=_type;
}

void Morph::wall(void)
{
  int t=wall_angle(1);
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
  }
}


void Morph::run(void)
{
  plus_phase();
  time+=KbdStat::time();
  if (time>2*Mp::METRONQUALITY)
  {
    mtime+=KbdStat::time();
    if (mtime>80)
    {
      phs++;
      if (phs<4)
      {
        mtime=0;
      }
    }
  }
  if (phs==4)
  {
    if (!blc)
    {
      if (type) NEW(Taran(x,y,bo_num,Posit::UP,1),gobjects_mbn);
      blc=1;
    }

  }
  if (phs==5)KILLME
  brake();
  wall();
  go();
  log_to_view();
  Myship* ship=NoNet::get_ship(Posit::x,Posit::y);
  shangle=get_dangle(ship->get_x()-x,ship->get_y()-y);
  shangle/=8;
}

void Morph::draw_object(Screen& screen,int plane)
{
  (void)plane;
  if (phs)
  {
    switch (phs)
    {
      case 0: screen.put(x,y,get_sprite(),get_phase());
              break;
      case 1: screen.put(x,y,get_sprite(),get_phase());
              if (type) screen.put(x,y,Mysprites::taran,shangle,Spr::with_tsphard,(void*)&tsp);
              break;
      case 2: screen.put(x,y,get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp);
              if (type) screen.put(x,y,Mysprites::taran,shangle,Spr::with_tspsoft,(void*)&tsp);
              break;
      case 3: screen.put(x,y,get_sprite(),get_phase(),Spr::with_tsphard,(void*)&tsp);
              if (type) screen.put(x,y,Mysprites::taran,shangle);
              break;
     default: if (type) screen.put(x,y,Mysprites::taran,shangle);
              break;
    }
  }
  else
  {
    screen.put(x,y,get_sprite(),get_phase());
  }
}

void Morph::draw_shadow(Screen& screen,int plane)
{
  (void)plane;
  int shx=x;
  int shy=y+height+Mp::SHADOWDIST;
  if (phs)
  {
    switch (phs)
    {
      case 1: screen.shape(shx,shy,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
              if (type) screen.shape(shx,shy,Mysprites::taran,0,Spr::with_tool,(void*)&Tools::tool[Tools::VLSHADOW]);
              break;
      case 2: screen.shape(shx,shy,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::LSHADOW]);
              if (type) screen.shape(shx,shy,Mysprites::taran,0,Spr::with_tool,(void*)&Tools::tool[Tools::LSHADOW]);
              break;
      case 3: screen.shape(shx,shy,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::VLSHADOW]);
              if (type) screen.shape(shx,shy,Mysprites::taran,0,Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
              break;
     default: if (type) screen.shape(shx,shy,Mysprites::taran,0,Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
              break;
    }
  }
  else
  {
    screen.shape(shx,shy,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
  }
}

