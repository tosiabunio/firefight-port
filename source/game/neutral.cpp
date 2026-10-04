#include "headers.h"

//KLASA EXPLODE
Explode::Explode(int _x,int _y,int chl,int splinter,int _type,Sprite &sprexp,int force,int sound):
Posit(_x,_y,chl),
Listmanager(ONEUTRAL),
Visible(sprexp,View::NEXPL)
{
  if (force!=DISABLE)
  {
    FOR_ALL_USERS(ID)
    {
      Myship* ship=NoNet::get_ship(ID);
      ship->kick_ship(x,y);
    }
  }
  phsair=time=flag=trace=0;
  type=_type;
  back=3<<8;
  if (sound) play(Mysound::explode[RAND%MAXEXPLODE]);
  ratio1=get_phases_num()/3;
  ratio2=get_phases_num()/3*2;
  if (!splinter) splptr=NULL;
  else  splptr=NEW(Splinters(x,y,splinter,get_chl()),gobjects_mbn);
}

void Explode::run(void)
{
  time+=KbdStat::time();
  if (time>Op::EXPLTIME)
  {
    back+=384;
    if (!is_last_phase()) plus_phase();
    else
    {
      flag=1;
      phsair++;
    }
    if (get_phase()>=get_phases_num()-3&&splptr) splptr->kill();
    time=0;
  }
  if (type!=NOLIGHT) Night::add_light(x,y);

  if (type==AIR) log_to_view();
  else log_to_view(get_plane()+2);

  if(is_last_phase()&&(back>>8)>=30) KILLME
  Sound::run();
}


void Explode::draw_object(Screen& screen,int plane)
{
  (void)plane;
  int _back=back>>8;
  if ((type!=NOLIGHT||type==AIR)&&SysSet::get(SysSet::DETAIL_LEVEL))
  {
    if ((int)_back<Mysprites::lihgtexpl.phases)
      Light::Ray(screen,&Mysprites::lihgtexpl,x,y,_back);
    else
      Light::Ray(screen,&Mysprites::lihgtexpl,x,y,(2*(Mysprites::lihgtexpl.phases-1))-_back);
  }
  if (type==AIR)
  {
    if (SysSet::get(SysSet::DETAIL_LEVEL))
    {
      if (get_phase()-2>=0&&phsair<=2)
        screen.put(x,y,get_sprite(),get_phase()-2+phsair,Spr::with_tsphard,(void*)&tsp);
      if (get_phase()-1>=0&&phsair<=1)
        screen.put(x,y,get_sprite(),get_phase()-1+phsair,Spr::with_tspsoft,(void*)&tsp);
    }
    if (phsair==0)
      screen.put(x,y,get_sprite(),get_phase());
    return;
  }

  if (flag) return;

  if (type!=TSP)
  {
    if (get_phase()>ratio2)
    {
      screen.put(x,y,get_sprite(),get_phase(),Spr::with_tsphard,(void*)&tsp);
      return;
    }
    if (get_phase()>ratio1)
    {
      screen.put(x,y,get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp);
      return;
    }
    screen.put(x,y,get_sprite(),get_phase());
  }
  else
    screen.put(x,y,get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp);
}
//FIRE
Fire::Fire(void):
Listmanager(ONEUTRAL),
Visible(View::NEXPL)
{
  explode=(Fire::Pair*)Heap::alloc(sizeof(Pair)*50,gobjects_mbn);
  for (int i=0;i<50;i++)
  {
    explode[i].x=RAND%Mp::SX;
    explode[i].y=RAND%Mp::SY;
    explode[i].count=RAND%Mysprites::expl_air.phases-Mysprites::expl_air.phases;
  }
  wx=world->get_x();
  wy=world->get_y();
  time=0;
  FOR_ALL_USERS(ID)
  {
    NoNet::get_ship(ID)->shake_screen(5*Mp::TURNQUALITY);
  }
}

Fire::~Fire(void)
{
  if (explode!=NULL)
  {
    Heap::free((void*)explode);
    explode=NULL;
  }
}


void Fire::run(void)
{
  int oldX=wx,oldY=wy;
  wx=world->get_x();
  wy=world->get_y();
  time+=KbdStat::time();
  int _y=wy-oldY;
  int _x=wx-oldX;

  for (int i=0;i<50;i++)
  {

    explode[i].count++;

    explode[i].x-=_x;
    explode[i].y-=_y;


    if (explode[i].x<0)       explode[i].x+=Mp::SX;
    if (explode[i].x>Mp::SX)  explode[i].x-=Mp::SX;

    if (explode[i].y<0)       explode[i].y+=Mp::SY;
    if (explode[i].y>Mp::SY)  explode[i].y-=Mp::SY;

    if (explode[i].count>=Mysprites::expl_air.phases)
    {
      explode[i].x=RAND%Mp::SX;
      explode[i].y=RAND%Mp::SY;
      explode[i].count=0;
    }
  }
  if (!(RAND%10)) Sound::st_play(&Mysound::explode[RAND%MAXEXPLODE]);
  log_to_view(8);
  if (time>3*Mp::METRONQUALITY)
    if (!NoNet::get_ship()->is_immortal()) gamemanager->set_restart(GameManager::KILL);

  if (time>5*Mp::METRONQUALITY)  KILLME
}

void Fire::draw_object(Screen& screen,int plane)
{
  (void)plane;
  int ox=screen.ox;
  int oy=screen.oy;
  screen.origin(0,0);

  for (int i=0;i<50;i++)
  {
    if (explode[i].count>0)
    {
      screen.put(explode[i].x,explode[i].y,Mysprites::expl_air,explode[i].count);
    }
  }
  screen.origin(ox,oy);
}



//KLASA BOOM
Boom::Boom(int _x,int _y,int chl):
Posit(_x,_y,chl),
Listmanager(ONEUTRAL),
Visible(Mysprites::boom,View::NEXPL)
{
  dtime=0;
}

void Boom::run(void)
{
  dtime+=KbdStat::time();
  if (dtime>20)
  {
    plus_phase();
    dtime=0;
  }
  log_to_view();
  if (is_last_phase()) KILLME
}

//SPLINTERS
const int SPL_INITMAX=25;
Splinters::Spl Splinters::init_table_down[MAX_INITSPL];
Splinters::Spl Splinters::init_table_up[MAX_INITSPL];
int   Splinters::initcounter(0);


Splinters::Splinters(int _x,int _y,int _init,int _chl) :
Posit(_x,_y,_chl),
Listmanager(ONEUTRAL),
Shadow(View::NEXPL)
{
  memset(table,0,sizeof(table));
  counter=killflag=0;
  flg=1;
  if (_init) for(int i=0;i<3*_init;i++) add_splinter();
}

void Splinters::init(void)
{
  initcounter=0;
  int i;
  for (i=0;i<MAX_INITSPL;i++)
  {
    Spl* s=&init_table_up[i];

    s->x=(RAND%15-7)<<8;
    s->y=(RAND%15-7)<<8;
    switch (RAND%3)
    {
      case 0:
              s->vx=RAND%Op::SPLINTERSPEED1-Op::SPLINTERSPEED1/2;
              s->vy=RAND%Op::SPLINTERSPEED1-Op::SPLINTERSPEED1/2;
              break;
      case 1:
              s->vx=RAND%Op::SPLINTERSPEED2-Op::SPLINTERSPEED2/2;
              s->vy=RAND%Op::SPLINTERSPEED2-Op::SPLINTERSPEED2/2;
              break;
      case 2:
              s->vx=RAND%Op::SPLINTERSPEED3-Op::SPLINTERSPEED3/2;
              s->vy=RAND%Op::SPLINTERSPEED3-Op::SPLINTERSPEED3/2;
              break;
    }
    s->vh=-(RAND%100);
    s->h=Mp::PIXELSBETWEENPLANES<<8;

    s->time=RAND%Op::SPLINTERTIME+50;
    s->spr=RAND%Mysprites::splinters_count();
    s->phs=0;
  }

  for (i=0;i<MAX_INITSPL;i++)
  {
    Spl* s=&init_table_down[i];

    s->x=(RAND%15-7)<<8;
    s->y=(RAND%15-7)<<8;
    switch (RAND%3)
    {
      case 0:
              s->vx=RAND%Op::SPLINTERSPEED1-Op::SPLINTERSPEED1/2;
              s->vy=RAND%Op::SPLINTERSPEED1-Op::SPLINTERSPEED1/2;
              break;
      case 1:
              s->vx=RAND%Op::SPLINTERSPEED2-Op::SPLINTERSPEED2/2;
              s->vy=RAND%Op::SPLINTERSPEED2-Op::SPLINTERSPEED2/2;
              break;
      case 2:
              s->vx=RAND%Op::SPLINTERSPEED3-Op::SPLINTERSPEED3/2;
              s->vy=RAND%Op::SPLINTERSPEED3-Op::SPLINTERSPEED3/2;
              break;
    }
    s->h=0;
    s->vh=(RAND%Op::SPLINTERPIX)+256;

    s->time=RAND%Op::SPLINTERTIME+50;
    s->spr=RAND%Mysprites::splinters_count();
    s->phs=0;
  }
}

void Splinters::add_splinter(void)
{
  DBG_CHECK(initcounter<MAX_INITSPL&&initcounter>=0);
  DBG_CHECK(counter<MAX_SPL&&counter>=0);
  Spl* s=&table[counter];
  Spl* it=NULL;


  if (get_chl()==Posit::UP)
    it=&init_table_up[initcounter];
  else
    it=&init_table_down[initcounter];

  memcpy(s,it,sizeof(Spl));

  s->x+=(get_down_x())<<8;
  s->y+=(get_down_y())<<8;

  initcounter++;
  if (initcounter>=MAX_INITSPL) initcounter=0;
  counter++;
  if (counter>=MAX_SPL) counter=0;
}
void Splinters::run(void)
{
  flg=-flg;
  if (flg<0&&!killflag) add_splinter();
  int t=0;

  Spl* s=table;
  int chl=get_chl();
  int tm=KbdStat::time();

  for (int i=0;i<MAX_SPL;i++,s++)
  {
    if (s->time>0)
    {
      s->x+=s->vx;
      s->y+=s->vy;
      s->h+=s->vh;
      s->time-=tm;
      s->vh-=20;

      if (s->h<0)
      {
        s->h=0;
        s->vh=(RAND%Op::SPLINTERPIX);
      }

      s->phs++;
      if (s->phs>=Mysprites::splinter[s->spr].phases) s->phs=0;

      if (s->time>t) t=s->time;
    }
  }
  if (t<=0&&killflag) KILLME

  if (chl==Posit::UP) log_to_view();
  else log_to_view(get_plane()+1);
}

void Splinters::draw_object(Screen& screen,int plane)
{
  (void)plane;
  if (!SysSet::get(SysSet::DETAIL_LEVEL)) return;
  Spl* spl=table;
  for (int i=0;i<MAX_SPL;i++,spl++)
  {
    if (spl->time>0)
    {
      if (spl->time<200)
      {
        screen.put (spl->x>>8,(spl->y>>8)-(spl->h>>8),Mysprites::splinter[spl->spr],spl->phs,Spr::with_tsphard,(void*)&tsp);
      }
      else if (spl->time<400)
      {
        screen.put (spl->x>>8,(spl->y>>8)-(spl->h>>8),Mysprites::splinter[spl->spr],spl->phs,Spr::with_tspsoft,(void*)&tsp);
      }
      else
      {
        screen.put (spl->x>>8,(spl->y>>8)-(spl->h>>8),Mysprites::splinter[spl->spr],spl->phs);
      }
    }
  }
}

void Splinters::draw_shadow(Screen& screen,int plane)
{
  (void)plane;
  if (!SysSet::get(SysSet::DETAIL_LEVEL)) return;
  Spl* spl=table;
  for (int i=0;i<MAX_SPL;i++,spl++)
  {
    if (spl->time>0)
    {
      if (spl->time<200)
      {
        screen.shape (spl->x>>8,spl->y>>8,Mysprites::splinter[spl->spr],spl->phs,Spr::with_tool,(void*)&Tools::tool[Tools::VLSHADOW]);
      }
      else if (spl->time<400)
      {
        screen.shape (spl->x>>8,spl->y>>8,Mysprites::splinter[spl->spr],spl->phs,Spr::with_tool,(void*)&Tools::tool[Tools::LSHADOW]);
      }
      else
      {
        screen.shape (spl->x>>8,spl->y>>8,Mysprites::splinter[spl->spr],spl->phs,Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
      }
    }
  }
}

//SPLINTER
Splinter::Splinter(int _x,int _y,int _chl,Sprite& _sprite,int _stand) :
Posit(_x,_y,_chl),
Move(0,Op::splinter.speed,Op::splinter),
Listmanager(ONEUTRAL),
Shadow(_sprite,View::NEXPL)
{
  stand=_stand;
  if (!stand)
  {
    set_speed(RAND%200);
    angle=RAND%256;
    transform_speed();
  }
  time=0;
  count=0;
}

void Splinter::run(void)
{
  if (!Visible::is_in_overscan())
  {
    KILLME
  }
  if (time>150||(stand&&time>0))
  {
    plus_phase();
    time=0;
    count++;
  }
  time+=KbdStat::time();
  if (!stand) go();
  log_to_view();
  if (stand)
  {
    if (is_last_phase()) KILLME
  }
  else
  {
    if (count>14+RAND%5) KILLME
  }
}

void Splinter::draw_object(Screen& screen,int plane)
{
  (void) plane;
  switch (stand)
  {
    case 0:
    {
      if (count>10)
      {
        screen.put(x,y,get_sprite(),get_phase(),Spr::with_tsphard,(void*)&tsp);
        return ;
      }
      if (count>5)
      {
        screen.put(x,y,get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp);
        return ;
      }
      screen.put(x,y,get_sprite(),get_phase());
    }
    break;
    case 1:
      screen.shape(x,y,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::LIGHT10]);
      break;
    case 2:
      screen.put(x,y,get_sprite(),get_phase(),Spr::with_tsphard,(void*)&tsp);
      break;
  }
}

void Splinter::draw_shadow(Screen& screen,int plane)
{
  (void)plane;
  if (!stand)
    screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
  else
    screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::LSHADOW]);
}

//SMOKE
Smoke::Smoke(int _x,int _y,Sprite& _sprite):
Posit(_x,_y,Posit::UP),
Move(0,Op::smoke.speed,Op::smoke),
Listmanager(ONEUTRAL),
Visible(_sprite,View::NEXPL)
{
  time=0;
}

void Smoke::run(void)
{
  time+=KbdStat::time();
  if (time>500)
  {
    if (is_last_phase())
    {
      KILLME
    }
    time=0;
    plus_phase();
  }
  go();
  log_to_view();
}

void Smoke::draw_object(Screen& screen,int plane)
{
  (void)plane;
  screen.shape(x,y,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::LIGHT4]);
}
//SWITCH
Switch::Switch(int _x,int _y,int _chl,Sprite& _sprite,int _level_num) :
Posit(_x,_y,_chl),
Listmanager(OWALL),
Colis(C_NWALL,0),
Shadow(_sprite,View::ASTAT),
Handle(HDOORS),
Impexp(_level_num)
{
  buttom=world->get_level()[_level_num].status;
  btime=0;
}

void Switch::HandleEvent(int message)
{
  if ((message==LevImp::get_parameters(lobject->type)->frame_delay)||(message==LevImp::get_parameters(lobject->type)->stuff_left))
  {
    DBG_MESSAGE("switch %s get message from %s",world->get_level().type[lobject->type],world->get_level().type[message]);
    if (!btime) btime=LevImp::get_parameters(lobject->type)->time_delay;
  }
}

void Switch::run(void)
{

  if (buttom)
  {
    if (!is_last_phase())  plus_phase();
  }
  else
  {
    if (!is_first_phase()) minus_phase();
  }

  if (!btime)
  {
    for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
    {
      if (temp->MY)
      {
        DBG_MESSAGE("switch %s has posted 'ON' to GM and to HDOORS",world->get_level().type[lobject->type]);
        Handle::post(GMANAGER,GameManager::code(lobject->type,ON));
        Handle::post(HDOORS,LevImp::get_parameters(lobject->type)->frame_delay);
        int t=LevImp::get_parameters(lobject->type)->stuff_left;
        if (t) Handle::post(HDOORS,t);
        lobject->status=0;
        reset_cmsg();
        break;
      }
    }
    buttom=0;
  }
  else
  {
    btime-=KbdStat::time();
    reset_cmsg();
    buttom=1;
  }
  if (btime<0) btime=0;
  Colis::write(-1,0,0,get_sprite(),get_phase());
  log_to_view();
}

//BACKGROUND
Background::Background(void):
Posit(0,0,Posit::DOWN),
Listmanager(OFIRST),
Visible(*Mysprites::back,View::MYFIRST)
{
  Sprite &tmp=*Mysprites::back;
  sx=tmp[0].r-tmp[0].l;
  sy=tmp[0].d-tmp[0].u;
  bx=320*2/sx;
  by=200*2/sy;
  back=(unsigned char*)Heap::alloc(sizeof(unsigned char)*(bx*by),gobjects_mbn);
  xsize=bx*sx;
  ysize=by*sy;

  for (int ix=0;ix<bx;ix++)
  {
    for (int jy=0;jy<by;jy++)
    {
      back[ix+jy*bx]=RAND%tmp.phases;
    }
  }
  mx=world->get_level().plane[gamemanager->get_sublevel()][1].mulx;
  my=world->get_level().plane[gamemanager->get_sublevel()][1].muly;
  wx=(int)((double)mx*world->get_x());  // port: x87 kept the product exact
  wy=(int)((double)my*world->get_y());  // port: x87 kept the product exact
}

Background::~Background(void)
{
  if (back!=NULL)
  {
    Heap::free((void*)back);
    back=NULL;
  }
}

void Background::run(void)
{
  int oldX=wx,oldY=wy;
  wx=(int)((double)mx*world->get_x());  // port: x87 kept the product exact
  wy=(int)((double)my*world->get_y());  // port: x87 kept the product exact

  y+=wy-oldY;
  x+=wx-oldX;

  if (x>=xsize) x=(int)x%xsize;
  if (y>=ysize) y=(int)y%ysize;
  if (x<0)
  {
    x=(int)x%xsize;
    x+=xsize;
  }
  if (y<0)
  {
    y=(int)y%ysize;
    y+=ysize;
  }
  Visible::log_to_view(0);
}

void Background::look_at(Screen& screen,int x,int y,int dx,int dy)
{
  for (int ix=x/sx;ix<bx&&ix*sx-x<dx;ix++)
  {
    for (int jy=y/sy;jy<by&&jy*sy-y<dy;jy++)
    {
      if (ix>=0&&ix<bx&&jy>=0&&jy<by)
        screen.put(ix*sx-x,jy*sy-y,get_sprite(),back[ix+jy*bx]);
    }
  }
}

void Background::look_at(Screen &screen,int x,int y,int dx,int dy,Tool *tool)
{
  for (int ix=x/sx;ix<bx&&ix*sx-x<dx;ix++)
  {
    for (int jy=y/sy;jy<by&&jy*sy-y<dy;jy++)
    {
      if (ix>=0&&ix<bx&&jy>=0&&jy<by)
        screen.put(ix*sx-x,jy*sy-y,get_sprite(),back[ix+jy*bx],Spr::with_tool,(void*)tool);
    }
  }
}


void Background::draw_object(Screen& screen,int plane)
{
  DBG_CHECK(!plane);
  if (!SysSet::get(SysSet::DETAIL_LEVEL))
  {
    screen.cls(world->get_cls_color());
    return;
  }
  (void)plane;
  if (!Mp::BACKGROUND) return;
  if (Snow::initialized)
  {
    Snow::draw_snow(screen,this,x,y);
    return;
  }
  int scr_orgx=screen.ox;
  int scr_orgy=screen.oy;
  screen.origin(0,0);
  look_at(screen,x,y,Mp::SX,Mp::SY);
  look_at(screen,x-xsize,y,Mp::SX,Mp::SY);
  look_at(screen,x,y-ysize,Mp::SX,Mp::SY);
  look_at(screen,x-xsize,y-ysize,Mp::SX,Mp::SY);
  screen.origin(scr_orgx,scr_orgy);
}

//KLASA FOG
Fog::Fog(char* type,Level* level):
Posit(0,0,Posit::UP),
Listmanager(ONEUTRAL),
Visible(Mysprites::fog,View::NEXPL)
{
  GameManager::weather_text.area(type);
  plane=GameManager::weather_text.value("plane");
  time=GameManager::weather_text.value("time");
  vx=GameManager::weather_text.value("vx");
  vy=GameManager::weather_text.value("vy");

  int begin=GameManager::weather_text.value("begin");
  int step=(begin)/get_phases_num();
  int r=GameManager::weather_text.value("R"),g=GameManager::weather_text.value("G"),b=GameManager::weather_text.value("B");

  GameManager::weather_text.endarea();

  if (get_phases_num()>=30) FAILURE("Nigth::Night  phases more than tools");
  for(int i=0;i<get_phases_num();i++)
  {
    tools[i]=NEW(Tool,"Fog tools");
    tools[i]->glass(Color(r,g,b),begin-(i*step));
  }

  count=0;
  xsize=get_spr_phase(0).r-get_spr_phase(0).l;
  ysize=get_spr_phase(0).d-get_spr_phase(0).u;

  mx=level->plane[gamemanager->get_sublevel()][plane].mulx;
  my=level->plane[gamemanager->get_sublevel()][plane].muly;
  first=1;
}

Fog::~Fog(void)
{
  for(int i=0;i<get_phases_num();i++)
  {
    if (tools[i]) delete tools[i];
  }
}


void Fog::run(void)
{
  if (first)
  {
    wx=(int)((double)mx*world->get_x());  // port: x87 kept the product exact
    wy=(int)((double)my*world->get_y());  // port: x87 kept the product exact
    first=0;
  }
  int oldX=wx,oldY=wy;
  int t=world->get_x();
  wx=(int)((double)mx*world->get_x());  // port: x87 kept the product exact
  t=world->get_y();
  wy=(int)((double)my*world->get_y());  // port: x87 kept the product exact

  y-=wy-oldY;
  x-=wx-oldX;

  count++;

  if (count>=time)
  {
    x+=vx;
    y+=vy;
    count=0;
  }


  if (x>=xsize) x=(int)x%xsize;
  if (y>=ysize) y=(int)y%ysize;
  if (x<0)      x+=xsize;
  if (y<0)      y+=ysize;

  Visible::log_to_view(plane);
}


void Fog::draw_object(Screen& screen,int plane)
{
  if (!SysSet::get(SysSet::DETAIL_LEVEL)) return;
  if (Video::mode_flag==Spr::hires)
  {
    xsize=get_spr_phase(0).hires.r-get_spr_phase(0).hires.l;
    ysize=get_spr_phase(0).hires.d-get_spr_phase(0).hires.u;
  }
  else
  {
    xsize=get_spr_phase(0).lores.r-get_spr_phase(0).lores.l;
    ysize=get_spr_phase(0).lores.d-get_spr_phase(0).lores.u;
  }

  int ox=screen.ox;
  int oy=screen.oy;
  screen.origin(0,0);

  (void)plane;
  for(int i=0;i<get_phases_num();i++)
  {
    screen.shape(x,y,get_sprite(),i,Spr::with_tool,(void*)tools[i]);
    screen.shape(x-xsize,y,get_sprite(),i,Spr::with_tool,(void*)tools[i]);
    screen.shape(x,y-ysize,get_sprite(),i,Spr::with_tool,(void*)tools[i]);
    screen.shape(x-xsize,y-ysize,get_sprite(),i,Spr::with_tool,(void*)tools[i]);
  }
  screen.origin(ox,oy);
}

//NIGHT
int Night::light_level;
int Night::distance_param;
int Night::light_mult;

Night::Night(char* type):
Listmanager(ONEUTRAL),
Visible(Mysprites::night,View::NEXPL)
{
  light_level=0;
  GameManager::weather_text.area(type);
  distance_param=GameManager::weather_text.value("light_edge");
  light_mult=GameManager::weather_text.value("light_param");
  int begin=GameManager::weather_text.value("begin");
  int step=(begin)/get_phases_num();

  int r=GameManager::weather_text.value("R"),g=GameManager::weather_text.value("G"),b=GameManager::weather_text.value("B");
  GameManager::weather_text.endarea();
  status=0;
  if (get_phases_num()>=30) FAILURE("Nigth::Night  phases more than tools");
  for(int i=0;i<get_phases_num();i++)
  {
    tools[i]=NEW(Tool,"Night tool");
    tools[i]->glass(Color(r,g,b),begin-(i*step));
  }
}

Night::~Night(void)
{
  for(int i=0;i<get_phases_num();i++)
  {
    if (tools[i]) delete tools[i];
  }
}

void Night::run(void)
{
  olevel=light_level;
  int param=olevel/(distance_param/light_mult);
  if (param<status)
  {
    status--;
  }
  if (param>status)
  {
    status++;
  }
  light_level=0;
  log_to_view(8);
}

void Night::draw_object(Screen& screen,int plane)
{
  if (!SysSet::get(SysSet::DETAIL_LEVEL)) return;
  (void)plane;
  int ox=screen.ox;
  int oy=screen.oy;
  screen.origin(0,0);


  for(int i=0;i<get_phases_num()-status;i++)
  {
    screen.shape(Mp::SX/2,Mp::SY/2,get_sprite(),i,Spr::with_tool,(void*)tools[i+status]);
  }
  screen.origin(ox,oy);
}

//RAIN
Rain::Rain(int type):
Listmanager(ONEUTRAL),
Visible(View::NEXPL)
{
  size=100;
  GameManager::weather_text.area("rain");

  dottsize=GameManager::weather_text.value("flake_count");
  movesize=GameManager::weather_text.value("move_count");
  weather_change=(int)(GameManager::weather_text.fp_value("weather_time_change")*Mp::METRONQUALITY);
  weather_stay=(int)(GameManager::weather_text.fp_value("weather_time_stay")*Mp::METRONQUALITY);
  weather_stay=weather_stay/weather_change/2;
  movesize2=movesize/2;
  dott=(Rain::Pair*)Heap::alloc(sizeof(Pair)*dottsize,gobjects_mbn);
  rnd=(Rain::Wkt*)Heap::alloc(sizeof(Wkt)*dottsize,gobjects_mbn);
  move=(Rain::Wkt*)Heap::alloc(sizeof(Wkt)*movesize,gobjects_mbn);
  color=Video::closest(Color(GameManager::weather_text.value("R"),GameManager::weather_text.value("G"),GameManager::weather_text.value("B")));

  int i;
  for (i=0;i<dottsize;i++)
  {
    dott[i].x=rnd[i].x=RAND%Mp::SX;
    dott[i].y=rnd[i].y=RAND%Mp::SY;//-movesize2;
    dott[i].count=RAND%movesize;
  }

  make_tool();
  int vx=GameManager::weather_text.value("vx");
  int vy=GameManager::weather_text.value("vy");
  for (i=0;i<movesize;i++)
  {
    move[i].x=vx;
    move[i].y=vy;
  }
  wx=-1000000;
  wy=-1000000;
  GameManager::weather_text.endarea();

  counter=weather_level=time=ltime=lflag=0;
  switch(type)
  {
    case 0: ch_sign=-1; break;
    case 1: ch_sign=1; break;
    case 2: weather_level=weather_stay; ch_sign=1; break;
    case 3: weather_level=weather_stay/2; ch_sign=-1;break;
  }
  // na calego weather_time=weather_change
  rand_time=(5*Mp::METRONQUALITY+(RAND%100)*100);
  y=Mp::SY/2;
  x=Mp::SX/2;
}


Rain::~Rain(void)
{
  if (dott!=NULL)
  {
    Heap::free((void*)dott);
    dott=NULL;
  }
  if (move!=NULL)
  {
    Heap::free((void*)move);
    move=NULL;
  }
  if (rnd!=NULL)
  {
    Heap::free((void*)rnd);
    rnd=NULL;
  }
  if (tool[0])
  {
    delete tool[0];
    delete tool[1];
  }
}

void Rain::weather_service(void)
{
  weather_time+=KbdStat::time();
  if (weather_time>weather_change)
  {
    weather_level+=ch_sign;
    if (weather_level<-weather_stay||weather_level>weather_stay+dottsize)  ch_sign=-ch_sign;
    weather_time=0;
  }
}

void Rain::make_tool(void)
{
  tool[0]=NEW(Tool,"Rain tool");
  tool[1]=NEW(Tool,"Rain tool");
  int white=Video::closest(Color::cwhite);
  int black=Video::closest(Color::cblack);
  for (int i=Video::min_color;i<Video::max_color;i++)
  {
    int t=(Video::palette[i].rval()+Video::palette[i].gval()+Video::palette[i].bval())/3;
    if (t<25)
    {
      tool[0]->conv[i]=(char)white;
    }
    else
    {
      tool[0]->conv[i]=(char)black;
    }
    if (t<18)
    {
      tool[1]->conv[i]=(char)black;
    }
    else
    {
      tool[1]->conv[i]=(char)white;
    }
  }
  tool[0]->brightness(200);
  tool[1]->brightness(40);
}

void Rain::run(void)
{
  weather_service();
  if (wx==-1000000&&wy==-1000000)
  {
    wx=world->get_x();
    wy=world->get_y();
  }
  int oldX=wx,oldY=wy;
  wx=world->get_x();
  wy=world->get_y();

  int _y=wy-oldY;
  int _x=wx-oldX;
  Pair* t=NULL;
  for (int i=0;i<dottsize;i++)
  {
    t=&dott[i];
    t->x+=move[t->count].x-_x;
    t->y+=move[t->count].y-_y;
    if (t->x<0)  t->x+=Mp::SX;
    if (t->x>=Mp::SX)  t->x-=Mp::SX;

    if (t->y<0)  t->y+=Mp::SY;
    if (t->y>=Mp::SY)  t->y-=Mp::SY;

    t->count++;
    if (t->count>=movesize)
    {
      t->x=rnd[i].x;
      t->y=rnd[i].y;
      t->count=0;
    }
  }
  time+=KbdStat::time();
  if (ltime>0)
  {
    ltime-=KbdStat::time();
  }
  int rnd=RAND%MAXTHUNDER;
  if (ltime>0&&weather_level>dottsize)
  {
     if (SysSet::get(SysSet::DETAIL_LEVEL))  play(Mysound::storm[rnd],0);
  }
  if (time>rand_time)
  {
    rand_time=(5*Mp::METRONQUALITY+(RAND%100)*100);
    time=0;
    ltime=150;
  }

  if (lflag)ltime=1;
  counter=1;
  log_to_view(8);
}

void Rain::draw_object(Screen& screen,int plane)
{
  if (!SysSet::get(SysSet::DETAIL_LEVEL)) return;
  (void)plane;
  int ox=screen.ox;
  int oy=screen.oy;
  screen.origin(0,0);

  int tsize=weather_level;
  if (weather_level<0) tsize=0;
  if (weather_level>=dottsize) tsize=dottsize;

  screen.origin(0,0);
  //screen.print(160,185,"weather=%d,ltime=%d,time=%d",weather_level,ltime,time);


  if (tool[0]&&ltime>0&&weather_level>dottsize&&counter)
  {
    if (ltime>75)
    {
      screen.rectangle(*tool[0],0,0,Mp::SX,Mp::SY);
      lflag=1;
    }
    else
    {
      screen.rectangle(*tool[1],0,0,Mp::SX,Mp::SY);
      lflag=0;
    }
    counter=0;
  }
  else
  {
    for (int i=0;i<tsize;i++)
    {
      screen.plot(color,dott[i].x,dott[i].y);
    }
  }
  screen.origin(ox,oy);
}



//SNOW
Tool *Snow::tool[MAX_SNOWTOOL];
int    Snow::initialized(0);
int    Snow::weather_level;
int    Snow::dottsize;
int    Snow::weather_stay;
int    Snow::tool_change;
int    Snow::weather_time;
int    Snow::weather_change;
int    Snow::stay_time;
int    Snow::ch_sign;
int    Snow::tool_level;

Snow::Snow(int type):
Listmanager(ONEUTRAL),
Visible(View::NEXPL)
{
  initialized=1;
  GameManager::weather_text.area("snow");
  dottsize=GameManager::weather_text.value("flake_count");
  movesize=GameManager::weather_text.value("move_count");
  weather_change=(int)(GameManager::weather_text.fp_value("weather_time_change")*Mp::METRONQUALITY);
  int ttime=Mp::METRONQUALITY/30;
  weather_change=(weather_change/ttime)*ttime;
  if (!weather_change) weather_change=ttime;
  weather_stay=(int)(GameManager::weather_text.fp_value("weather_time_stay")*Mp::METRONQUALITY);
  tool_change=(int)(GameManager::weather_text.fp_value("tool_time_change")*Mp::METRONQUALITY);
  weather_stay=weather_stay/weather_change/2;
  movesize2=movesize/2;
  dott=(Snow::Pair*)Heap::alloc(sizeof(Pair)*dottsize,gobjects_mbn);
  rnd=(Snow::Wkt*)Heap::alloc(sizeof(Wkt)*dottsize,gobjects_mbn);
  move=(Snow::Wkt*)Heap::alloc(sizeof(Wkt)*movesize,gobjects_mbn);
  color=Video::closest(Color(GameManager::weather_text.value("R"),GameManager::weather_text.value("G"),GameManager::weather_text.value("B")));

  int i;
  for (i=0;i<dottsize;i++)
  {
    dott[i].x=rnd[i].x=RAND%Mp::SX;
    dott[i].y=rnd[i].y=RAND%Mp::SY;//-movesize2;
    dott[i].count=RAND%movesize;
  }

  for (i=0;i<movesize;i++)
  {
    move[i].x=RAND%3-1;
    move[i].y=1;
  }
  wx=-1000000;
  wy=-1000000;
  init_tool(&GameManager::weather_text);
  GameManager::weather_text.endarea();
  for (int p=0;p<dottsize;p++)
  {
    if (dott[p].x<0||dott[p].x>Mp::SX) p++;
    if (dott[p].y<0||dott[p].y>Mp::SY) p++;
  }
  stay_time=weather_level=weather_time=time=ltime=lflag=0;
  switch(type)
  {
    case 0: ch_sign=-1; break;
    case 1: ch_sign=1; break;
    case 2: weather_level=weather_stay;
            ch_sign=1;
            stay_time=weather_change*dottsize-tool_change*MAX_SNOWTOOL;
            break;
    case 3: weather_level=weather_stay/2;
            ch_sign=-1;
//            stay_time
            break;
  }

}


Snow::~Snow(void)
{
  if (dott!=NULL)
  {
    Heap::free((void*)dott);
    dott=NULL;
  }
  if (move!=NULL)
  {
    Heap::free((void*)move);
    move=NULL;
  }
  if (rnd!=NULL)
  {
    Heap::free((void*)rnd);
    rnd=NULL;
  }
  for (int i=0;i<MAX_SNOWTOOL;i++) delete tool[i];
  initialized=0;
}

void Snow::init_tool(Text *text)
{
  int r,g,b,begin,step;
  begin=(*text).value("Tool_begin");

  step=((*text).value("Tool_end")-begin)/MAX_SNOWTOOL;
  sscanf((*text).string("Tool_RGB"),"%d %d %d",&r,&g,&b);
  for(int i=0;i<MAX_SNOWTOOL;i++)
  {
    tool[i]=NEW(Tool,"Snow tool");
    tool[i]->glass(Color(r,g,b),begin+(i*step));
  }
}

void Snow::draw_2plane(Screen& screen,Lobject *o)
{
  DBG_CHECK(initialized);
  if (tool_level>=0)
   screen.put(o->x,o->y,world->get_level().sprite[o->sprite], o->phase,Spr::with_tool,(void*)tool[(tool_level-3<0)?0:tool_level-3]);
  else
    screen.put(o->x,o->y,world->get_level().sprite[o->sprite], o->phase);
}

void Snow::draw_snow(Screen& screen,Background* back,int x,int y)
{
  DBG_CHECK(initialized);
  int scr_orgx=screen.ox;
  int scr_orgy=screen.oy;
  screen.origin(0,0);
  if (tool_level<=0)
  {
    back->look_at(screen,x,y,Mp::SX,Mp::SY);
    back->look_at(screen,x-back->xsize,y,Mp::SX,Mp::SY);
    back->look_at(screen,x,y-back->ysize,Mp::SX,Mp::SY);
    back->look_at(screen,x-back->xsize,y-back->ysize,Mp::SX,Mp::SY);
  }
  else
  {
    DBG_CHECK(tool_level>=0);
    back->look_at(screen,x,y,Mp::SX,Mp::SY,tool[tool_level]);
    back->look_at(screen,x-back->xsize,y,Mp::SX,Mp::SY,tool[tool_level]);
    back->look_at(screen,x,y-back->ysize,Mp::SX,Mp::SY,tool[tool_level]);
    back->look_at(screen,x-back->xsize,y-back->ysize,Mp::SX,Mp::SY,tool[tool_level]);
  }
  screen.origin(scr_orgx,scr_orgy);
}

void Snow::weather_service(void)
{
  weather_time+=KbdStat::time();

  if (weather_level<dottsize)
  {
    if (weather_level>0)
      stay_time+=ch_sign*KbdStat::time();
    else stay_time=0;
  }

  if (weather_time>weather_change)
  {
    weather_level+=ch_sign;
    if (weather_level<-weather_stay||weather_level>weather_stay+dottsize)  ch_sign=-ch_sign;
    weather_time=0;
  }

  int t=2*weather_change*dottsize-tool_change*MAX_SNOWTOOL;
  if (stay_time>t)
  {
    tool_level=(stay_time-t)/tool_change;
  }
  else tool_level=-1;
  if (tool_level>=MAX_SNOWTOOL)tool_level=MAX_SNOWTOOL-1;
}


void Snow::run(void)
{
  weather_service();
  if (wx==-1000000&&wy==-1000000)
  {
    wx=world->get_x();
    wy=world->get_y();
  }
  int oldX=wx,oldY=wy;
  wx=world->get_x();
  wy=world->get_y();

  int _y=wy-oldY;
  int _x=wx-oldX;

  for (int i=0;i<dottsize;i++)
  {
    Pair* t=&dott[i];
    t->x+=move[t->count].x-_x;
    t->y+=move[t->count].y-_y;
    if (t->x<0)  t->x+=Mp::SX;
    if (t->x>=Mp::SX)  t->x-=Mp::SX;

    if (t->y<0)  t->y+=Mp::SY;
    if (t->y>=Mp::SY)  t->y-=Mp::SY;

    t->count++;
    if (t->count>=movesize)
    {
      t->x=rnd[i].x;
      t->y=rnd[i].y;
      t->count=0;
    }
  }
  time+=KbdStat::time();
  if (ltime>0) ltime-=KbdStat::time();
  if (time>5*Mp::METRONQUALITY+RAND%10000)
  {
    x=world->get_x()+Mp::SX/2;
    y=world->get_y()+Mp::SY/2;
    time=0;
    ltime=150;
  }
  if (lflag) ltime=1;
  log_to_view(8);
}

void Snow::draw_object(Screen& screen,int plane)
{
  if (!SysSet::get(SysSet::DETAIL_LEVEL)) return;
  (void)plane;
  int ox=screen.ox;
  int oy=screen.oy;
  int tsize=weather_level;
  if (weather_level<0) tsize=0;
  if (weather_level>=dottsize) tsize=dottsize;

  screen.origin(0,0);
  //screen.print(5,185,"weather=%d,sty_time=%d,level=%d,tool=%d,all=%d",weather_level,stay_time,tool_level,weather_change*dottsize,tool_change*MAX_SNOWTOOL);
  for (int i=0;i<tsize;i++) screen.plot(color,dott[i].x,dott[i].y);
  screen.origin(ox,oy);
}

//CLOUDS
Cloud::Cloud(char* type,Level* level):
Listmanager(ONEUTRAL),
Visible(Mysprites::cloud,View::NEXPL)
{
  GameManager::weather_text.area(type);

  overscan=GameManager::weather_text.value("overscan");
  time=GameManager::weather_text.value("time");
  max=GameManager::weather_text.value("max");
  plane=GameManager::weather_text.value("plane");
  GameManager::weather_text.endarea();
  cloud=(Cloud::Elm*)Heap::alloc(sizeof(Elm)*max,gobjects_mbn);
  for (int i=0;i<max;i++)
  {
    cloud[i].x=RAND%(Mp::SX+2*overscan)-overscan;
    cloud[i].y=RAND%(Mp::SY+2*overscan)-overscan;
    if ((RAND%2))
    {
      cloud[i].vx=(float)1.2;
      cloud[i].vy=(float)1.2;
    }
    else
    {
      cloud[i].vx=(float)1.7;
      cloud[i].vy=(float)1.7;

    }
    cloud[i].spr=RAND%get_phases_num();
  }
  first=1;
  count=0;
  mx=level->plane[gamemanager->get_sublevel()][8].mulx;
  my=level->plane[gamemanager->get_sublevel()][8].muly;

}

Cloud::~Cloud(void)
{
  if (cloud!=NULL)
  {
    Heap::free((void*)cloud);
    cloud=NULL;
  }
}

void Cloud::run(void)
{
  if (first)
  {
    wx=(int)((double)mx*world->get_x());  // port: x87 kept the product exact
    wy=(int)((double)my*world->get_y());  // port: x87 kept the product exact
    first=0;
  }

  int oldX=wx,oldY=wy;
  wx=(int)((double)mx*world->get_x());  // port: x87 kept the product exact
  wy=(int)((double)my*world->get_y());  // port: x87 kept the product exact

  int _y=wy-oldY;
  int _x=wx-oldX;
  count++;
  for (int i=0;i<max;i++)
  {
    Elm* t=&cloud[i];
    t->x-=(int)(t->vx*_x);
    t->y-=(int)(t->vy*_y);
    if (t->x<-overscan)  t->x+=Mp::SX+2*overscan;
    if (t->x>Mp::SX+overscan)  t->x-=Mp::SX+2*overscan;

    if (t->y<-overscan)  t->y+=Mp::SY+2*overscan;
    if (t->y>Mp::SY+overscan)  t->y-=Mp::SY+2*overscan;
  }
  log_to_view(plane);
}

void Cloud::draw_object(Screen& screen,int plane)
{
  if (!SysSet::get(SysSet::DETAIL_LEVEL)) return;
  int ox=screen.ox;
  int oy=screen.oy;
  screen.origin(0,0);

  (void)plane;
  for (int i=0;i<max;i++)
  {
    Elm* t=&cloud[i];
    screen.put(t->x,t->y,get_sprite(),t->spr,Spr::with_tsphard,(void*)&tsp);
  }
  screen.origin(ox,oy);
}

//DOOR
Door::Door(int _x,int _y,int _chl,Sprite& _sprite,int _level_num,int _plane):
Posit(_x,_y,_chl),
Visible(_sprite,View::BUILD),
Listmanager(OWALL),
Colis(C_NWALL,0),
Handle(HDOORS),
Impexp(_level_num)
{
  plane=_plane;
  init=close=LevImp::get_parameters(lobject->type)->disp_type;
  if (close)
  {
    set_phase(get_phases_num()/2-1);
  }
  else
  {
    set_phase(0);
  }
  prepare=blocked=unactive=0;
  time=LevImp::get_parameters(lobject->type)->time_delay;
}

void Door::HandleEvent(int message)
{
  if (message==lobject->type)
  {
    DBG_MESSAGE("%s get 'opening' message from %s",world->get_level().type[lobject->type],world->get_level().type[message]);
    if (blocked==2)
    {
      if (!LevImp::get_parameters(lobject->type)->time_delay) unactive=1;
      time=LevImp::get_parameters(lobject->type)->time_delay;
      prepare=0;
      return;
    }
    if (!blocked)
    {
      if (!unactive)close=(close)?0:1;
      if (!LevImp::get_parameters(lobject->type)->time_delay) unactive=1;
      time=LevImp::get_parameters(lobject->type)->time_delay;
    }
    else prepare=1;
  }
}

void Door::run(void)
{

  if (close!=init)
  {
    time-=KbdStat::time();
    if (time<=0)
    {
      if (!blocked)
      {
        if (!unactive)close=(close)?0:1;
        if (!LevImp::get_parameters(lobject->type)->time_delay)
        unactive=1;
      }
      else
      {
        prepare=1;
      }
      time=LevImp::get_parameters(lobject->type)->time_delay;
    }
  }
  if (!close)
  {
    LowColis::set_cinfo(1,0,0,0,0,0,0,0,0);
    if (get_phase())
    {
      minus_phase();
    }
  }
  else
  {
    if (get_phase()<get_phases_num()/2-1)
    {
      plus_phase();
    }
    LowColis::set_cinfo(0,0,0,1,1,0,0,0,0);
  }

  blocked=0;
  Colis::write(-1,0,0,get_sprite(),get_phase());
  for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
  {
    if (temp->MYSHIP||temp->ALIEN)
    {
      blocked=2;
    }
  }

  if (!blocked&&prepare)
  {
    prepare=0;
    if (!unactive)close=(close)?0:1;
    if (!LevImp::get_parameters(lobject->type)->time_delay)
    unactive=1;
    time=LevImp::get_parameters(lobject->type)->time_delay;
  }
  reset_cmsg();


  log_to_view(plane);

  log_to_view(plane+1);
}

void Door::draw_object(Screen& screen,int _plane)
{
  if (plane==_plane)
  {
    switch (LevImp::get_parameters(lobject->type)->serv_type)
    {
      case 0: screen.put(x,y,get_sprite(),get_phase()); break;
      case 1: screen.put(x,y,get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp); break;
      case 2: screen.put(x,y,get_sprite(),get_phase(),Spr::with_tsphard,(void*)&tsp); break;
    }
  }
  else
  {
    switch (LevImp::get_parameters(lobject->type)->serv_type)
    {
      case 0: screen.put(x,y,get_sprite(),get_phase()+get_phases_num()/2); break;
      case 1: screen.put(x,y,get_sprite(),get_phase()+get_phases_num()/2,Spr::with_tspsoft,(void*)&tsp); break;
      case 2: screen.put(x,y,get_sprite(),get_phase()+get_phases_num()/2,Spr::with_tsphard,(void*)&tsp); break;
    }
  }
  //screen.print(x,y,"blocked=%d,time=%d,unactive=%d,prepare=%d",blocked,time,unactive,prepare);
}

//MORPHDESTROY

MorphDest::MorphDest(int _x,int _y,Sprite& _sprite,int _level_num,int _plane):
Posit(_x,_y,Posit::UP),
Visible(_sprite,View::BUILD),
Listmanager(OSTAT),
Impexp(_level_num),
Colis(C_DEST(0,0,1),0),
Radar(world->get_level()[_level_num].type),
Life(200)
{
  object_mission=GameManager::mission_object;
  plane=_plane;
  time=mlevel=0;
  prg=(get_life()/get_phases_num());
  num=get_life()/prg-1;
}


void MorphDest::run(void)
{
  for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
  {
    if (C_CHECK_DESTROY(temp)) decrease(temp->destroy);
  }
  Colis::write(-1,0,0,get_sprite(),get_phase());

  if (get_life()<num*prg&&!mlevel)
  {
    num--;
    mlevel=1;
  }

  if (mlevel)
  {
    time+=KbdStat::time();
    if (time>150)
    {
      mlevel++;
      time=0;
    }
    if (mlevel>3)
    {
      time=mlevel=0;
      plus_phase();
    }
  }

  if (get_life()<=0&&num<=0)
  {
    lock_targeting();
    E_BIG;
    Handle::post(GMANAGER,GameManager::code(lobject->type,DESTROY));
    remove_from_level();
    KILLME
  }
  log_to_view();
}

void MorphDest::draw_object(Screen& screen,int plane)
{
  switch (mlevel)
  {
    case 0:  screen.put(x,y,get_sprite(),get_phase());
             break;
    case 1:  screen.put(x,y,get_sprite(),get_phase());
             screen.put(x,y,get_sprite(),get_phase()+1,Spr::with_tsphard,(void*)&tsp);
             break;
    case 2:  screen.put(x,y,get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp);
             screen.put(x,y,get_sprite(),get_phase()+1,Spr::with_tspsoft,(void*)&tsp);
             break;
    case 3:  screen.put(x,y,get_sprite(),get_phase(),Spr::with_tsphard,(void*)&tsp);
             screen.put(x,y,get_sprite(),get_phase()+1);
             break;
  }
}


//DESTROY
#define Destroy_shadow      LevImp::get_parameters(lobject->type)->blocked
#define Destroy_ch_shadow   LevImp::get_parameters(lobject->type)->repeat_count
#define Destroy_anim_shadow LevImp::get_parameters(lobject->type)->time_delay
#define Destroy_anim        LevImp::get_parameters(lobject->type)->change_type
#define Destroy_smoke       LevImp::get_parameters(lobject->type)->smoke
#define Destroy_light       LevImp::get_parameters(lobject->type)->light
#define Destroy_visible     LevImp::get_parameters(lobject->type)->disp_type
#define Destroy_morph       LevImp::get_parameters(lobject->type)->morph
#define Destroy_ch_spr      LevImp::get_parameters(lobject->type)->change_spr
#define Destroy_life        LevImp::get_parameters(world->get_level()[_level_num].type)->sync_frame


int Destroy::destcount(0);

Destroy::Destroy(int _x,int _y,int _chl,Sprite& _sprite,int _level_num,int _plane,int status):
Posit(_x,_y,Posit::DOWN),
Shadow(_sprite,View::BUILD),
Listmanager(OSTAT),
Impexp(_level_num),
Colis(C_DEST((_plane>=world->get_active_plane())?1:LevImp::get_parameters(world->get_level()[_level_num].type)->stuff_left,(_plane>=world->get_active_plane())?1:LevImp::get_parameters(world->get_level()[_level_num].type)->stuff_left,(Destroy_life!=Op::DESTROYIMMORTAL)?!status:0),0),
Handle(DESTROY_OBJ),
Radar((status)?0:world->get_level()[_level_num].type),
Life(Destroy_life)
{
  object_mission=GameManager::mission_object;
  plane=_plane;
  blocked=status;
  time=0;
  lmorph_level=lmorph_time=0;
  lmorph_phase1=RAND%get_phases_num();
  lmorph_phase2=RAND%get_phases_num();
  active=world->get_active_plane();
  fr_delay=LevImp::get_parameters(lobject->type)->frame_delay;
  global_timer=morph_time=morph_phase=morph_level=send=blow=anim_phase=0;
  if (blocked&&Destroy_smoke)
    smoke=NEW(Destsmoke(x,y,plane,0,Destroy_smoke),gobjects_mbn);
  else smoke=NULL;

  global_time=Mp::BEYONDTIME;
  destcount++;
}

Destroy::~Destroy(void)
{
  destcount--;
}

void Destroy::run(void)
{
  if (blocked)
  {
    if (!Visible::is_in(100))
    {
      if (smoke) smoke->kill();
      smoke=NULL;
      KILLME
    }
  }
  else
  {
    if (!Visible::is_visible())
    {
      if ((global_time-=KbdStat::time())<0)
      {
        if (!object_mission&&get_life()>0) return;
        if (!object_mission&&get_life()>0)
        {
          if (smoke) smoke->kill();
          smoke=NULL;
          KILLME
        }
      }
    }
    else  global_time=Mp::BEYONDTIME;
  }

  time+=KbdStat::time();
  if (time>fr_delay)
  {
    plus_phase();
    if (get_life()<=0)
    {
      anim_phase++;
      if (Destroy_morph)
      {
        morph_phase++;
        if (morph_phase>=world->get_level().sprite[Destroy_ch_spr].phases) morph_phase=0;
      }
    }
    time=0;
  }

  if(Destroy_visible||blocked) log_to_view(plane,1);

  if (blocked&&Destroy_light) Night::add_light(x,y);
  if(!blocked)
  {
    for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
    {
      if (get_life()==Op::DESTROYIMMORTAL) continue;
      if (C_CHECK_DESTROY(temp))
      {
        if (LevImp::get_parameters(lobject->type)->hit_sample!=-1)
        Sound::play(LevImp::get_parameters(lobject->type)->hit_sample,x);
        if (!send)
        {
          if (LevImp::get_parameters(lobject->type)->action>Parameters::MID&&LevImp::get_parameters(lobject->type)->action) servis_link();
          send=1;
        }
        decrease(temp->destroy);
      }
    }
    if (get_life()<=0)
    {
      lock_targeting();
      int anim=LevImp::get_parameters(lobject->type)->change_type;
      int snd =LevImp::get_parameters(lobject->type)->expl_sample;
      int expl=LevImp::get_parameters(lobject->type)->explode;
      if (!blow)
      {
        if(expl&&snd==-1)
        {
          switch(expl)
          {
            case 1: E_BIG;break;
            case 2: E_MED;break;
          }
        }
        else
        {
          if (expl)
          {
            switch(expl)
            {
              case 1: NEW(Explode(x,y,get_chl(),8,Explode::NORMALL,Mysprites::expl_dest,Explode::ENABLE,0),gobjects_mbn); break;
              case 2: NEW(Explode(x,y,get_chl(),8,Explode::NORMALL,Mysprites::expl_dest_small,Explode::ENABLE,0),gobjects_mbn);break;
            }
          }
        }

        if (snd!=-1) Sound::play(LevImp::get_parameters(lobject->type)->expl_sample,x,y);
        blow=1;
      }

      if (anim&&(anim_phase!=world->get_level().sprite[anim].phases-1)) return;
      if (Destroy_morph)
      {
        if(!blocked||(plane>=active)) Colis::write(-1,0,0,get_sprite(),get_phase());
        morph_time+=KbdStat::time();
        if (morph_time>100)
        {
          morph_level++;
          morph_time=0;
        }
        if (morph_level<3) return;
      }

      Handle::post(GMANAGER,GameManager::code(lobject->type,DESTROY));
      if (LevImp::get_parameters(lobject->type)->action<Parameters::MID&&LevImp::get_parameters(lobject->type)->action) servis_link();
      int t=LevImp::get_parameters(world->get_level()[level_num].type)->change_spr;
      if (t)
      {
        lobject->aux2=-1;
        lobject->sprite=t;
        NEW(Destroy(x,y,Posit::UP,world->get_level().sprite[t],level_num,plane,1),gobjects_mbn);
      }
      remove_from_level();
      if(!blocked||(plane>=active)) Colis::write(-1,0,0,get_sprite(),get_phase());
      KILLME
    }
  }
  else reset_cmsg();
  if(!blocked||(plane>=active)) Colis::write(-1,0,0,get_sprite(),get_phase());
}

void Destroy::HandleEvent(int message)
{
  if (message==lobject->type)
  {
    decrease(get_life()+100);
  }
}

void Destroy::servis_link(void)
{
  Level* lev=&world->get_level();
  for(int i=(*lev)[level_num].link;i&&i!=level_num;i=(*lev)[i].link)
  {
    int t=LevImp::get_parameters(lobject->type)->action;
    if (t==Parameters::ONAFTER||t==Parameters::ONBEFORE)
    {
      switch(LevImp::get_parameters((*lev)[i].type)->is_object)
      {
        case Parameters::ENEMYSHIP:
          (*lev)[i].status=1;
          break;

        case Parameters::DESTROY:
        case Parameters::CANNON:
        case Parameters::CAPSULE:
        default :
            (*lev)[i].active=1;
            (*lev).update(i);
            break;
      }
    }
    else
    {
      switch(LevImp::get_parameters((*lev)[i].type)->is_object)
      {
        case Parameters::ENEMYSHIP: Handle::post(DESTROY_OBJ,(*lev)[i].type);
                                    (*lev)[i].aux2=-200;
                                    break;
        case Parameters::DESTROY:
        case Parameters::CANNON:    Handle::post(DESTROY_OBJ,(*lev)[i].type );
        default:
          (*lev)[i].active=0;
          (*lev).update(i);
          break;
      }
    }
  }
}

void Destroy::draw_shadow(Screen& screen,int plane)
{
  if (Destroy_ch_shadow||Destroy_shadow||Destroy_anim_shadow)
  {
    if (get_life()>0)
    {
      if (blocked)
      {
        if (Destroy_ch_shadow) screen.shape(x,y,world->get_level().sprite[Destroy_ch_shadow],anim_phase,Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
      }
      else
      {
        if (Destroy_shadow)    screen.shape(x,y,world->get_level().sprite[Destroy_shadow],anim_phase,Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
      }
    }
    else
    {
      if (Destroy_anim_shadow) screen.shape(x,y,world->get_level().sprite[Destroy_anim_shadow],anim_phase,Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
    }
  }
}

void Destroy::draw_object(Screen& screen,int plane)
{
  (void)plane;

  if (get_life()>0)
  {
    if (blocked)
    {
       int tmp1=(get_phase()-1<0)? get_phases_num()-1:get_phase()-1;
       int tmp2=(get_phase()-2<0)? get_phases_num()-2:get_phase()-2;
       if (tmp2<0) tmp2=0;
       screen.put(x,y,get_sprite(),get_phase());
       if (SysSet::get(SysSet::DETAIL_LEVEL))
       {
         screen.put(x,y,get_sprite(),tmp1,Spr::with_tspsoft,(void*)&tsp);
         screen.put(x,y,get_sprite(),tmp2,Spr::with_tsphard,(void*)&tsp);
       }

    }
    else screen.put(x,y,get_sprite(),get_phase());
  }
  else
  {
    if (LevImp::get_parameters(lobject->type)->change_type)
    {
      screen.put(x,y,world->get_level().sprite[LevImp::get_parameters(lobject->type)->change_type],anim_phase);
      return;
    }
    else if (Destroy_morph)
    {
      switch (morph_level)
      {
        case 0:  screen.put(x,y,get_sprite(),get_phase());
                 screen.put(x,y,world->get_level().sprite[Destroy_ch_spr],morph_phase,Spr::with_tsphard,(void*)&tsp);
                 break;
        case 1:  screen.put(x,y,get_sprite(),get_phase(),Spr::with_tspsoft,(void*)&tsp);
                 screen.put(x,y,world->get_level().sprite[Destroy_ch_spr],morph_phase,Spr::with_tspsoft,(void*)&tsp);
                 break;
        case 2:  screen.put(x,y,get_sprite(),get_phase(),Spr::with_tsphard,(void*)&tsp);
                 screen.put(x,y,world->get_level().sprite[Destroy_ch_spr],morph_phase);
                 break;
        default :  screen.put(x,y,world->get_level().sprite[Destroy_ch_spr],morph_phase);
                 break;
      }
      return;
    }
    else screen.put(x,y,get_sprite(),get_phase());
  }
}


//
AlienSmoke::AlienSmoke(int _x,int _y,int _angle,Sprite& spr,int _power,int _plane):
Posit(_x,_y,Posit::UP),
Move(_angle,_ROCKET.speed,_ROCKET),
Listmanager(OMWEAP),
Visible(spr,View::MYWEAP)
{
  memset(table,get_phases_num(),sizeof(Wkt)*MAX_WAVE);
  del=time=last=0;
  Lx=x;
  Ly=y;
  power=_power;
  plane=_plane;
  sstop=0;
}


void AlienSmoke::add(void)
{
  if (sstop) return;
  Lx=table[last].x=x+RAND%16-8;
  Ly=table[last].y=y+RAND%16-8;
  table[last].time=table[last].phase=0;
  last++;
  if (last>=MAX_WAVE) last=0;
}

void AlienSmoke::run(void)
{
  int tm=0;
  time+=tm=KbdStat::time();
  if (power)
  {
    if (power<50&&(RAND%power<6)) add();
  }
  else add();
  int t=0,change=0;
  if (time>20)
  {
    int i;
    for(i=0;i<MAX_ALIENWAVE;i++)
    {
      if (table[i].phase<get_phases_num())
      {
        table[i].time+=tm;
        if (table[i].time>150)
        {
          table[i].phase++;
          table[i].time=0;
        }
        table[i].y--;
        change=1;
      }
      t=1;
    }
    time=0;
  }
  if (plane<0) log_to_view();
  else log_to_view(plane);
//  if (del&&t&&!change) KILLME
}

void AlienSmoke::draw_object(Screen& screen,int plane)
{
  (void)plane;
  for(int i=0;i<MAX_ALIENWAVE;i++)
  {
    if (table[i].phase<get_phases_num())
      screen.shape(table[i].x,table[i].y,get_sprite(),table[i].phase,Spr::with_tool,(void*)(&Tools::tool[Tools::ALIENSMOKE]));
  }
}

//Destsmoke

Destsmoke::Destsmoke(int _x,int _y,int _plane,int lev_num,int _type):
Posit(_x,_y,Posit::UP),
Listmanager(ONEUTRAL),
Visible(Mysprites::destsmoke,View::NEXPL)
{
  type=_type;
  memset(table,0,sizeof(Destsmoke::Wkt)*MAX_SMOKE);
  last=0;
  plane=_plane;
  time=0;
}

void Destsmoke::add(void)
{
  table[last].x=x+RAND%Op::DESTSMOKEAAP-Op::DESTSMOKEAAP/2;
  table[last].y=y+RAND%Op::DESTSMOKEAAP-Op::DESTSMOKEAAP/2;
  table[last].time=0;
  table[last].level=get_phases_num();
  last++;
  if (last>=MAX_SMOKE) last=0;
}

void Destsmoke::run(void)
{
  time+=KbdStat::time();
  int t=0,change=0;
  if (time>Op::DESTSMOKEFRQ)
  {
    add();
    time=0;
  }
  int tm=KbdStat::time();
  for(int i=0;i<MAX_SMOKE;i++)
  {
    if (table[i].level>0)
    {
      table[i].time+=tm;
      table[i].x+=(RAND%3-1);
      table[i].y-=(RAND%2);
      if(table[i].time>Op::DESTSMOKECH)
      {
        table[i].time=0;
        table[i].level--;
        if (Op::DESTSMOKEPRC>table[i].level) table[i].level=0;
      }
    }
  }
  log_to_view(plane);
}

void Destsmoke::kill(void)
{
  KILLME
}


void Destsmoke::draw_object(Screen& screen,int plane)
{
  if (!SysSet::get(SysSet::DETAIL_LEVEL)) return;
  int phs=get_phases_num();
  Sprite* spr=&get_sprite();
  for (int index=0;index<MAX_SMOKE;index++)
  {
    if (table[index].level>0)
    {
      int t=get_phases_num()-table[index].level;
      for (int i=0;i<table[index].level;i++,t++)
      {
        switch (type)
        {
          case BLACK: screen.shape(table[index].x,table[index].y,*spr,phs-i-1,Spr::with_tool,(void*)&Tools::smokeblack_tool[t]); break;
          case WHITE: screen.shape(table[index].x,table[index].y,*spr,phs-i-1,Spr::with_tool,(void*)&Tools::smokewhite_tool[t]); break;
        }
      }
    }
  }
}
//LEVELSMOKE
LevelSmoke::LevelSmoke(int _x,int _y,int _level_num,int _type,int _plane):
Posit(_x,_y,Posit::UP),
Listmanager(OSTAT),
Impexp(_level_num)
{
  type=_type;
  plane=_plane;
  big_smoke=NULL;
  small_smoke=NULL;
  switch(_type)
  {
    case  BIGW: big_smoke=NEW(Destsmoke(x,y,plane,0,1),gobjects_mbn);
                small_smoke=NULL;
                break;
    case  BIGB: big_smoke=NEW(Destsmoke(x,y,plane,0,2),gobjects_mbn);
                small_smoke=NULL;
                break;
    case SMALL: small_smoke=NEW(AlienSmoke(x,y,0,Mysprites::farmerdym,0),gobjects_mbn);
                big_smoke=NULL;
                break;
  }
}

void LevelSmoke::run(void)
{
  if(!Posit::is_in_overscan())
  {
    if (small_smoke) small_smoke->kill();
    if (big_smoke)   big_smoke->kill();
    KILLME
  }
}





//LIGHT
Light::Light(int _x,int _y,int _plane,int _power,int _lev_num):
Posit(_x,_y,Posit::UP),
Listmanager(ONEUTRAL),
Visible(View::LIGHT),
Impexp(_lev_num)
{
  power=_power;
  if (power>=Mysprites::lihgtexpl.phases)
  {
    power=Mysprites::lihgtexpl.phases;
  }
  if (power<0)
  {
    power=0;
  }
  plane=_plane;
}

void Light::run(void)
{
  if (!Posit::is_in(Mp::MAXOVERSCAN*2)) KILLME
  Night::add_light(x,y);
  log_to_view(plane);
}

void Light::draw_object(Screen& screen,int plane)
{
  (void)plane;
  if (SysSet::get(SysSet::DETAIL_LEVEL)) Ray(screen,&Mysprites::lihgtexpl,x,y,power);
}


void Light::Ray(Screen& screen,Sprite* spr,int x,int y,int power)
{
  DBG_CHECK(!(spr->phases-1<power));
  int j=power;
  for (int i=0;i<power;i++,j--)
  {
    screen.shape(x,y,*spr,i,Spr::with_tool,(void*)&Tools::tool[Tools::LIGHT1+j]);
  }
}
//COUNTDOWN
int Countdown::disable(0);

Countdown::Countdown(int _time):
Listmanager(OSTAT),
Visible(View::INFO)
{
  stime=disable=0;
  time=_time;
  starttime=time/3;
}

void Countdown::run(void)
{
  if (disable) KILLME
  time-=KbdStat::time();
  stime+=KbdStat::time();
  int tm=0;
  if (time>40*Mp::METRONQUALITY) tm=Op::COUNTDOWNFRQ1;
  else  if (time>20*Mp::METRONQUALITY) tm=Op::COUNTDOWNFRQ2;
  else  tm=Op::COUNTDOWNFRQ3;
  if (stime>=tm)
  {                
    Sound::st_play(&Mysound::countdown);
    stime-=tm;
  }
  if (time<=0)
  {
    NEW(Fire,gobjects_mbn);
    KILLME
  }
  log_to_view(9);
}

void Countdown::draw_object(Screen& screen,int plane)
{
  (void)plane;
  int ox=screen.ox;
  int oy=screen.oy;
  screen.origin(0,0);
  char cnt[1024];
  sprintf(cnt,"TIME %d",time/Mp::METRONQUALITY);
  int cntsx=41;
  int cntsy=Op::DISPINFOINVSY;
  int cntx=Op::DISPINFOWEAPX;
  int cnty=Op::DISPINFOINVY+Op::DISPINFOINVSY+2;
  int cntinx=cntsx/2;
  int cntiny=Op::DISPINFOINVINRECTY;
  Tool& tool=Tools::tool[Tools::DISPINFO];
  screen.rectangle(tool,cntx,cnty,cntsx,cntsy);
  Print::print(screen,Print::TFONT,Print::_FLAST,1+cntx+cntinx-Print::get_dx(Print::TFONT,cnt)/2,cnty+cntiny,cnt);
  Print::print(screen,Print::TFONT,Print::FINVOVR,cntx+cntinx-Print::get_dx(Print::TFONT,cnt)/2,cnty+cntiny,cnt);
  screen.origin(ox,oy);
}

//UNBLOCK
Unblock::Unblock(int _x,int _y,Sprite& _sprite,int _level_num):
Posit(_x,_y,Posit::UP),
Listmanager(OSTAT),
Visible(_sprite,View::ASTAT),
Colis(C_NEUTRAL,0,"unblock"),
Radar((GameManager::mission_object)?world->get_level()[_level_num].type:0),
Impexp(_level_num)
{
  time=first=0;
  mission_obj=GameManager::mission_object;
  change=LevImp::get_parameters(lobject->type)->change_spr;
  our=1;
  ship=NoNet::get_ship(Posit::x,Posit::y);
  lock_targeting();
}

void Unblock::run(void)
{
  ship=NoNet::get_ship(Posit::x,Posit::y);
  Parameters *temp;
  temp=LevImp::get_parameters(change);

  if (!is_in_overscan()&&!mission_obj)
  {
    KILLME
  }
  //czy dziala przez kolizje
  int blocked;
  switch (LevImp::get_parameters(lobject->type)->blocked)
  {
    case 0:
           {
             if (Posit::distance(ship->get_x(),ship->get_y())<350)
             {
               first++;
               blocked=0;
             }
             else
             {
               blocked=1;
             }
             if (first==1)
             {
               Link::send_impulse(level_num);
               Handle::post(GMANAGER,GameManager::code(lobject->type,ON));
               if (mission_obj) KILLME
             }
             break;
           }
    case 1:
           {
             Colis::write(-1,0,0,get_sprite(),get_phase());
             blocked=1;
             for (Cmsg* temp=get_cmsg();temp;temp=get_cmsg())
             {
               if (temp->MYSHIP)
               {
                 first++;
                 blocked=0;
               }
             }

             if (first==1)
             {
               Link::send_impulse(level_num);
               Handle::post(GMANAGER,GameManager::code(lobject->type,ON));
               if (mission_obj) KILLME
             }
             break;
           }
  }
}

//COLLECT
Collect::Collect(int _x,int _y,Sprite& _sprite,int _level_num):
Posit(_x,_y,Posit::DOWN),
Visible(_sprite,View::ASTAT),
Listmanager(OSTAT),
LowColis(C_NEUTRAL,0,"collect"),
Impexp(_level_num),
Radar(world->get_level()[_level_num].type,0,0,-Mp::PIXELSBETWEENPLANES)
{
  coll_time=phs_target=kill=snd=pipe_time=pipe_phs=timer=landtime=ship_landing=0;
  ship=NoNet::get_ship(Posit::x,Posit::y);
  Sx=ship->get_x();
  Sy=ship->get_y();
}

void Collect::run(void)
{
  ship=NoNet::get_ship(Posit::x,Posit::y);
  DBG_CHECK(ship!=NULL);
  Sx=ship->get_x();
  Sy=ship->get_y();
  if (Posit::distance(ship->get_down_x(),ship->get_down_y())<=Op::COLLECTRADIUS&&ship->get_speed()<=Op::COLLECTSPEED)
  {
    landtime+=KbdStat::time();
    ship_landing=1;
    if (!snd)
    {
      ship->play_ship_sample(Mysound::beaming,Myship::INTERNAL);
      snd=1;
    }
  }
  else
  {
    if (snd)
    {
      Mysound::beaming.stop();
      if (!kill&&landtime>Op::COLLECTTIME/4)
      {
        ship->play_ship_sample(Mysound::speech_beaminterrupt,Myship::SPEECH);
        MSGTXT(GAMETXT("BEAMINTERRUPT"),0);
      }
      ship_landing=0;
      landtime=0;
      snd=0;
    }
  }


  pipe_time+=KbdStat::time();

  if (pipe_time>=30)
  {
    if (ship_landing)
    {
      if (!kill&&++pipe_phs>36) pipe_phs=36;
    }
    else
    {
      if (!kill&&--pipe_phs<-1) pipe_phs=-1;
    }
    if (kill)
    {
      pipe_phs++;
      Myship* ship=NoNet::get_ship(Posit::x,Posit::y);
      move_me(Turn::get_dangle(ship->get_x()-x,ship->get_y()-y),2);
    }
    pipe_time=0;
  }

  if (!kill&&Posit::distance(ship->get_down_x(),ship->get_down_y())<=2*Op::COLLECTRADIUS&&ship->get_speed()<=Op::COLLECTSPEED)
  {
    ship->beaming_attract(get_up_x(),get_up_y());
  }

  if (landtime>Op::COLLECTTIME&&!kill)
  {
    ship->play_ship_sample(Mysound::speech_beamsuccess,Myship::SPEECH);
    MSGTXT(GAMETXT("BEAMOK"),0);
    Handle::post(GMANAGER,GameManager::code(lobject->type,TAKE));
    Link::send_impulse(level_num);
    kill=1;
    lock_targeting();
  }

  if (kill&&pipe_phs>=Mysprites::collect_up.phases)
  {
    remove_from_level();
    KILLME
  }

  coll_time+=KbdStat::time();
  if (coll_time>=150)
  {
    if ((++phs_target)>=Mysprites::radar_landing.phases) phs_target=0;
    coll_time=0;
  }

  timer+=KbdStat::time();
  if (timer>=150)
  {
    timer=0;
    plus_phase();
  }
  log_to_view();
  log_to_view(8);
}

void Collect::draw_object(Screen& screen,int plane)
{
  if (plane!=8)
  {
    if (!kill) screen.put(x,y,get_sprite(),get_phase());
    if (pipe_phs>=0) screen.put(x,y,Mysprites::collect_up,pipe_phs);
  }
  else
  {
    if (!kill)
    {
      Myship* ship=NoNet::get_ship(Posit::x,Posit::y);
      int radar=Radar::check_cur_type(lobject->type)&&Radar::enable();

      if (!radar&&Posit::distance(ship->get_down_x(),ship->get_down_y())<=2*Op::COLLECTRADIUS)
           screen.put(x,y-Mp::PIXELSBETWEENPLANES,Mysprites::radar_landing,phs_target);
    }
  }
}

//KLASA WALL
int Wall::initialized(0);

Wall::Wall(void):
Listmanager(OFIRST)
{
  DBG_CHECK(!initialized);
  initialized=1;
  normal=NEW(NormalWall(),gobjects_mbn);
  weapon=NEW(WeaponWall(),gobjects_mbn);
  killer=NEW(KillerWall(),gobjects_mbn);

}

Wall::~Wall(void)
{
  if (normal!=NULL)
  {
    Stale_memory::object_deleted(normal);  // port
    delete normal;
    normal=NULL;
  }
  if (weapon!=NULL)
  {
    Stale_memory::object_deleted(weapon);  // port
    delete weapon;
    weapon=NULL;
  }
  if (killer!=NULL)
  {
    Stale_memory::object_deleted(killer);  // port
    delete killer;
    killer=NULL;
  }
  initialized=0;
}

void Wall::fill(int id,int x,int y,int sx,int sy)
{
  char planes_str[1024];
  sprintf(planes_str,"%d%d",gamemanager->get_sublevel(),world->get_active_plane());
  world->get_level().look_atnb(x,y,sx,sy,planes_str);
  LowColis::origin(id,world->get_level().buf[1].orgx,world->get_level().buf[1].orgy);
  for (unsigned short* i=world->get_level().buf[1].object; *i; i++)
  {
    Lobject *o=&world->get_level()[*i];
    switch (LevImp::get_parameters(o->type)->colis_type)
    {
#if !HI_DEBUG
      default:
#endif
      case LevImp::PCNONE:   break;
      case LevImp::PCWALL:   normal->run(id,o->x,o->y,world->get_level().sprite[o->sprite],o->phase,o->mirror);
                             break;
      case LevImp::PCWEAPON: weapon->run(id,o->x,o->y,world->get_level().sprite[o->sprite],o->phase,o->mirror);
                             break;
      case LevImp::PCKILL:   killer->run(id,o->x,o->y,world->get_level().sprite[o->sprite],o->phase,o->mirror);
                             break;
    }
  }
}

void Wall::run(void)
{
  FOR_ALL_USERS(ID)
  {
    fill(ID,world->get_x(ID)-Mp::OVERSCANX,world->get_y(ID)-Mp::OVERSCANY,world->get_sx()+(Mp::OVERSCANX<<1),world->get_sy()+(Mp::OVERSCANY<<1));
    LowColis::origin(ID,-(world->get_x(ID)-Mp::OVERSCANX),-(world->get_y(ID)-Mp::OVERSCANY));
  }
}
//KLASA NORMALWALL
NormalWall::NormalWall(void):
LowColis(C_NWALL,0,"normal wall")
{
}

void NormalWall::run(int id,int x,int y,Sprite& sprite,int phase,int mirror)
{
  write(id,x,y,0,0,sprite,phase,mirror);
}
//KLASA WEAPONWALL
WeaponWall::WeaponWall(void):
LowColis(C_WWALL,0,"weapon wall")
{
}

void WeaponWall::run(int id,int x,int y,Sprite& sprite,int phase,int mirror)
{
  write(id,x,y,0,0,sprite,phase,mirror);
}
//KLASA KILLERWALL
KillerWall::KillerWall(void):
LowColis(C_KWALL,10,"killer wall")
{
}

void KillerWall::run(int id,int x,int y,Sprite& sprite,int phase,int mirror)
{
  write(id,x,y,0,0,sprite,phase,mirror);
}
//KLASA FADE
int Fade::fade_time[NoNet::max_users+1];
char Fade::text_buf[NoNet::max_users+1][Fade::text_len];
int Fade::mode[NoNet::max_users+1];
int Fade::type[NoNet::max_users+1];

void Fade::init(void)
{
  for (int i=0;i<NoNet::max_users+1;i++)
  {
    fade_time[i]=0;
    mode[i]=IDLE;
    type[i]=BOTH;
  }
}

void Fade::quit(void)
{
}

void Fade::start(int _type,char *text,int id)
{
  DBG_CHECK((id==0)||NoNet::check_id(id));
  DBG_CHECK((_type==ONLYIN)||(_type==ONLYOUT)||(_type==BOTH));
  DBG_CHECK(strlen(text)<text_len);
  type[id]=_type;
  strncpy(text_buf[id],text,text_len);
  text_buf[id][text_len-1]='\0';
  switch (type[id])
  {
    case ONLYIN:
      case BOTH: fade_time[id]=0;
                 mode[id]=FADEIN;
                 break;
   case ONLYOUT: fade_time[id]=2*Op::FADEFADETIME;
                 mode[id]=FADEOUT;
                 break;
  }
}

void Fade::_run(int id)
{
  DBG_CHECK((id==0)||NoNet::check_id(id));
  switch (mode[id])
  {
    case FADEIN: if (fade_time[id]<Op::FADEFADETIME)
                 {
                   fade_time[id]+=KbdStat::time();
                   if (fade_time[id]>Op::FADEFADETIME)
                   {
                     fade_time[id]=Op::FADEFADETIME;
                     if (type[id]!=ONLYIN)
                     {
                       mode[id]=FADEOUT;
                     }
                   }
                 }
                 break;
   case FADEOUT: if (fade_time[id]>0)
                 {
                   fade_time[id]-=KbdStat::time();
                   if (fade_time[id]<0)
                   {
                     fade_time[id]=0;
                     mode[id]=IDLE;
                   }
                 }
                 break;
      case IDLE: break;
  }
}

int Fade::is_faded(int id)
{
  DBG_CHECK((id==0)||NoNet::check_id(id));
  return (fade_time[id]>=Op::FADEFADETIME);
}

void Fade::run(void)
{
  _run(0);
  FOR_ALL_USERS(ID)
  {
    _run(ID);
  }
}

void Fade::_draw(Screen& screen,int id)
{
  DBG_CHECK((id==0)||NoNet::check_id(id));
  if (fade_time[id]>0)
  {
    int ox=screen.ox;
    int oy=screen.oy;
    screen.origin(0,0);
    int sx=world->get_sx();
    int sy=world->get_sy();
    int bsx=sx;
    int bsy=(int)((sy*fade_time[id])/(Op::FADEFADETIME+(Op::FADEFADETIME/2)));
    screen.rectangle(Color::black,0,0,bsx,bsy);
    screen.rectangle(Color::black,0,sy-bsy,bsx,bsy);
    if (id==0||fade_time[0]<=0)
    {
      int textx=(sx/2)-(Print::get_dx(Print::TSPRITE,text_buf[id])/2);
      int texty=Op::FADETEXTY;
      Print::print(screen,Print::TSPRITE,Print::FWARNING,textx,texty,text_buf[id]);
    }
    screen.origin(ox,oy);
  }
}

void Fade::draw(Screen& screen)
{
  _draw(screen,world->get_current_displayed());
  _draw(screen,0);
}

int Fade::in_use(int id)
{
  DBG_CHECK((id==0)||NoNet::check_id(id));
  return (mode[id]!=IDLE);
}
//KLASA GENERATOR
int Generator::initializing(0);
#define  GeneratorGenFrq     LevImp::get_parameters(lobject->type)->serv_type
#define  GeneratorObjCount   LevImp::get_parameters(lobject->type)->disp_type


Generator::Generator(int _x,int _y,Sprite& _sprite,int _level_num):
Posit(_x,_y,Posit::DOWN),
Listmanager(OSTAT),
Impexp(_level_num)
{
  phs_time=time=0;
  if (lobject->aux2==1000) return;
  for(int i=lobject->link;i!=level_num;i=world->get_level()[i].link)
  {
    world->get_level()[i].aux1=world->get_level()[i].x;
    world->get_level()[i].aux2=world->get_level()[i].y;
  }
  lobject->aux2=1000;
  obj_count=GeneratorObjCount;
}

int Generator::generate(void)
{
  if (lobject->link)
  {
    initializing=1;
    int status=0;
    int i;
    for(i=lobject->link;i!=level_num;i=world->get_level()[i].link)
      if (world->get_level()[i].status<2) status++;
    if (!status) return 0;
    status=RAND%status+1;

    for(i=lobject->link;i!=level_num;i=world->get_level()[i].link)
    {
#if HI_DEBUG
      if (!LevImp::get_parameters(world->get_level()[i].type)->builder)
         FAILURE("Gnenerator dolinkowany obiekt '%s' nie jest obiektem gry",world->get_level().type[world->get_level()[i].type]);
#endif
      if (world->get_level()[i].status<2)
      {
        status--;
        if (!status)
        {
          Lobject* o=&world->get_level()[i];
          o->status=2;
          NEW(SplinterGen(o->x,o->y,Mysprites::generat,i),gobjects_mbn);
          obj_count--;
        }
      }
    }
    initializing=0;
  }
  return 1;
}

void Generator::run(void)
{
  if (LevImp::in_crange(lobject->type,x,y))
  {
    time+=KbdStat::time();
    if (time>GeneratorGenFrq)
    {
      time=0;
      generate();
    }
  }
  if (obj_count<=0)
  {
    remove_from_level();
    KILLME
  }
}

//SPLINTERGEN
SplinterGen::SplinterGen(int _x,int _y,Sprite& _sprite,int _level_num):
Posit(_x,_y,Posit::DOWN),
Listmanager(OSTAT),
Visible(_sprite,View::NEXPL)
{
  lev_num=_level_num;
  go=time=0;
  play(Mysound::generat);
}

void SplinterGen::run(void)
{
  time+=KbdStat::time();
  if (time>100)
  {
    plus_phase();
    time=0;
  }
  if (!go&&get_phase()>=get_phases_num()/2)
  {
    go=1;
    world->get_level().build(lev_num,world->get_level().olevel(lev_num),world->get_level().oplane(lev_num));
  }
  log_to_view();
  if (is_last_phase()) KILLME
}

//CINDERS
Cinders::Cinders(int _x,int _y,Sprite& _sprite,int _level_num,int _type,int _plane):
Posit(_x,_y,Posit::DOWN),
Listmanager(OSTAT),
Colis(C_NWALL,0,"cinder"),
Visible(_sprite,View::ASTAT),
Impexp(_level_num)
{
  time=0;
  type=_type;
  plane=_plane;
  smoke=NEW(AlienSmoke(x,y-5,0,Mysprites::farmerdym,0,plane),gobjects_mbn);
}


void Cinders::run(void)
{
  time+=KbdStat::time();
  if (!Visible::is_visible()) 
  {
    if (type==CANNON)lobject->user.short2=CINDERSCANNON;
    if (type==PULSE) lobject->user.short2=CINDERSPULSE;
    smoke->kill();
    KILLME
  }
  Colis::write(-1,0,0,get_sprite(),get_phase());
  reset_cmsg();
  if (time>200)
  {
    plus_phase();
    time=0;
  }
  log_to_view(plane);
}














