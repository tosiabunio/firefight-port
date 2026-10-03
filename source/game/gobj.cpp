#include "headers.h"

//FUNKCJE
int sign(int a)
{
  return (a<0)?-1:1;
}
//KLASA RAND
int Rand::initialized=0;
int Rand::data[Rand::size];
int Rand::indx=0;
int Rand::locked=0;
int Rand::debug_cnt=0;

NoQueue<Rand::Data> *Rand::queue=NULL;

void Rand::init(void)
{
  DBG_CHECK(!initialized);
  initialized=1;
  indx=0;
  locked=0;
  debug_cnt=0;
  if (Mp::RANDDEBUGLEVEL>0)
  {
    queue=NEW(NoQueue<Rand::Data>(Mp::RANDDEBUGLEVEL,"rand debug data",1),queue_mbn);
    DBG_CHECK(queue!=NULL);
    Data d;
    d.frame=0;
    d.value=0;
    d.counter=0;
    d.indx=0;
    strncpy(d.place,"*** not used yet ***",Data::place_buffer);
    for (int i=0;i<Mp::RANDDEBUGLEVEL;i++)
    {
      queue->add(&d);
    }
  }
  srand(0);
  for (int i=0;i<size;i++)
  {
    data[i]=::rand();
  }
  lock();
}

void Rand::quit(void)
{
  if (queue!=NULL)
  {
    DBG_CHECK(Mp::NETDEBUGLEVEL);
    delete queue;
    queue=NULL;
  }
  initialized=0;
}

void Rand::reinit(void)
{
  DBG_CHECK(initialized);
  indx=0;
  debug_cnt=0;
  DBG_MESSAGE("Random reinitialized");
}

void Rand::dump(void)
{
  if (queue!=NULL)
  {
    MESSAGE("RAND DEBUGINFO DUMP:");
    MESSAGE("f:frame,v:value,c:counter,x:context");
    while (!queue->empty())
    {
      char buff[150];
      Data *d=queue->head();
      sprintf(buff,"RAND (f:%d,v:%5d,c:%4d,x:%s)",d->frame,d->value,d->counter,d->place);
      MESSAGE(buff);
    }
  }
}

void Rand::lock(void)
{
  locked=1;
}

void Rand::unlock(void)
{
  locked=0;
}

int Rand::rand(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(!locked);
  DBG_CHECK_RANGE(indx,0,size-1);
  indx=(indx>=size-1)?0:indx+1;
  return data[indx];
}

int Rand::rand(char *place)
{
  debug_cnt++;
  int to_return=rand();
  if (Mp::RANDDEBUGLEVEL>0)
  {
    DBG_CHECK(queue!=NULL);
    queue->head();
    Data d;
    d.frame=KbdStat::local_frame();
    d.value=to_return;
    d.counter=debug_cnt;
    d.indx=curr_indx();
    strncpy(d.place,place,Data::place_buffer);
    queue->add(&d);
  }
  return to_return;
}

int Rand::curr_indx(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK_RANGE(indx,0,size-1);
  return indx;
}
//KLASA KILL
NoQueue<Object*> *Kill::queue=NULL;
int Kill::initialized=0;

void Kill::init(void)
{
  DBG_CHECK(!initialized);
  initialized=1;
  queue=NEW(NoQueue<Object*>(Mp::KILLQUEUESIZE,"kill"),queue_mbn);
}

void Kill::quit(void)
{
  if (queue!=NULL)
  {
    delete queue;
    queue=NULL;
  }
  initialized=0;
}

void Kill::run(void)
{
  DBG_CHECK(initialized);
  while (!queue->empty())
  {
    delete (*(queue->head()));
  }
}

//KLASA FASTALLOC
unsigned FastAlloc::objects_table[2]={1024,0};
unsigned FastAlloc::objects_number=512;
Fast_heap FastAlloc::my_heap;
int FastAlloc::initialized=0;

void FastAlloc::init(void)
{
  DBG_CHECK(!initialized);
  initialized=1;
  my_heap.init(objects_table,objects_number,"Main");
}

void FastAlloc::quit(void)
{
  my_heap.quit();
  initialized=0;
}

void *FastAlloc::operator new(size_t obj_size)
{
  DBG_CHECK(initialized);
  DBG_CHECK(obj_size<=objects_table[0]);
  void *ptr=my_heap.alloc();
  CHECK(ptr!=NULL);
  return ptr;
}

void FastAlloc::operator delete(void *ptr,size_t obj_size)
{
  if (ptr==NULL)
  {
    return;
  }
  DBG_CHECK(initialized);
  DBG_CHECK(obj_size<=objects_table[0]);
  my_heap.free(ptr);
}
//KLASA POSIT
int Posit::initialized=0;

void Posit::init(void)
{
  DBG_CHECK(!initialized);
  initialized=1;
}

void Posit::quit(void)
{
  initialized=0;
}

void Posit::_ctor(int _x,int _y,int _height,int _climb_speed)
{
  x=_x;
  y=_y;
  fractx=0;
  fracty=0;
  height=_height;
  kstat_per_pixel=Mp::METRONQUALITY/_climb_speed;
  counted_kstat_up=0;
  counted_kstat_down=0;
}

Posit::Posit(int _x,int _y,int _chl,int _climb_speed)
{
  DBG_CHECK(_chl==Posit::UP||_chl==Posit::DOWN);
  int _height=(_chl==Posit::UP)?Mp::PIXELSBETWEENPLANES:0;
  _ctor(_x,_y,_height,_climb_speed);
}

Posit::Posit(int _x,int _y,int _chl,Op::ObjPack& _ref)
{
  DBG_CHECK(_chl==Posit::UP||_chl==Posit::DOWN);
  int _height=(_chl==Posit::UP)?Mp::PIXELSBETWEENPLANES:0;
  _ctor(_x,_y,_height,_ref.climb_speed);
}

int Posit::is_in(int range,int _x,int _y,int id)
{
  if (id==-1)
  {
    FOR_ALL_USERS(ID)
    {
      int scrx=_x-world->get_x(ID);
      int scry=_y-world->get_y(ID);
      if (scrx>=-range&&scrx<world->get_sx()+range&&scry>=-range&&scry<world->get_sy()+range)
      {
        return 1;
      }
    }
    return 0;
  }
  else
  {
    int scrx=_x-world->get_x(id);
    int scry=_y-world->get_y(id);
    return (scrx>=-range&&scrx<world->get_sx()+range&&scry>=-range&&scry<world->get_sy()+range);
  }
}


int Posit::is_in_overscan(int range,int _x,int _y,int id)
{
  int left_edge=-Mp::OVERSCANX-range;
  int right_edge=world->get_sx()+Mp::OVERSCANX+range;
  int up_edge=-Mp::OVERSCANY-range;
  int down_edge=world->get_sy()+Mp::OVERSCANY+range;
  if (id==-1)
  {
    FOR_ALL_USERS(ID)
    {
      int scrx=_x-world->get_x(ID);
      int scry=_y-world->get_y(ID);
      if (scrx>=left_edge&&scrx<right_edge&&scry>=up_edge&&scry<down_edge)
      {
        return 1;
      }
    }
    return 0;
  }
  else
  {
    int scrx=_x-world->get_x(id);
    int scry=_y-world->get_y(id);
    return (scrx>=left_edge&&scrx<right_edge&&scry>=up_edge&&scry<down_edge);
  }
}


void Posit::go_up(void)
{
  if (height<Mp::PIXELSBETWEENPLANES)
  {
    int dheight(0);
    counted_kstat_up+=KbdStat::time();
    while (counted_kstat_up>=kstat_per_pixel&&height<Mp::PIXELSBETWEENPLANES)
    {
      counted_kstat_up-=kstat_per_pixel;
      height++;
      dheight++;
    }
    y-=dheight;
  }
}

void Posit::go_down(void)
{
  if (height>0)
  {
    int dheight(0);
    counted_kstat_down+=KbdStat::time();
    while (counted_kstat_down>=kstat_per_pixel&&height>0)
    {
      counted_kstat_down-=kstat_per_pixel;
      height--;
      dheight++;
    }
    y+=dheight;
  }
}

void Posit::move_me(int _angle,int _dist)
{
  DBG_CHECK(initialized);
  DBG_CHECK(_angle>=0&&_angle<Mp::TURNQUALITY);
  DBG_CHECK(_dist>0&&_dist<256);
  int dx=vxlut[_angle]*_dist;
  int dy=vylut[_angle]*_dist;
  int newx=(x<<16)+fractx+dx;
  int newy=(y<<16)+fracty+dy;
  x=(newx>>16);
  y=(newy>>16);
  fractx=newx&0xffff;
  fracty=newy&0xffff;
}
//KLASA TURN
Turn::Turn(int _angle,int _turn_speed,int _dirs)
{
  DBG_CHECK(_angle>=0&&_angle<Mp::TURNQUALITY);
  dirs=_dirs;
  angle=_angle;
  restangle=0;
  dirs_per_angle=(dirs<<16)/Mp::TURNQUALITY;
  angles_per_dir=Mp::TURNQUALITY/dirs;
  set_turn_speed(_turn_speed);
}

Turn::Turn(int _angle,Op::ObjPack& ref)
{
  DBG_CHECK(_angle>=0&&_angle<Mp::TURNQUALITY);
  dirs=ref.dirs;
  angle=_angle;
  restangle=0;
  dirs_per_angle=(dirs<<16)/Mp::TURNQUALITY;
  angles_per_dir=Mp::TURNQUALITY/dirs;
  set_turn_speed(ref.turn_speed);
}

int Turn::get_dir(int ddir)
{
  int _angle=normalize_angle(angle+angles_per_dir/2);
  int dir=(_angle*dirs_per_angle)>>16;
  if (ddir)
  {
    if (dir+ddir>=dirs)
    {
      return dir+ddir-dirs;
    }
    if (dir+ddir<0)
    {
      return dir+ddir+dirs;
    }
    return dir+ddir;
  }
  return dir;
}

int Turn::get_angle(int dangle)
{
  if (dangle)
  {
    int _angle=angle+dangle;
    if (_angle>=Mp::TURNQUALITY)
    {
      _angle-=Mp::TURNQUALITY;
    }
    else if (_angle<0)
    {
      _angle+=Mp::TURNQUALITY;
    }
    return _angle;
  }
  else
  {
    return angle;
  }
}

int Turn::get_dangle(int dx,int dy)
{
  //DBG_CHECK(!(!dx&&!dy));
  int sx=0;
  int sy=0;
  if (dx<0)
  {
    sx=-1;
    dx=-dx;
  }
  else
    sx=1;

  if (dy<0)
  {
    sy=-1;
    dy=-dy;
  }
  else
    sy=1;

  if (dy==0)
  {
    if (sx>0) return Mp::QUARTANGLE;
    if (sx<0) return Mp::HALFANGLE+Mp::QUARTANGLE;
  }
  int key=(dx<<16)/(dy);


  int num=Mp::EIGHTANGLE;
  int step=Mp::EIGHTANGLE;

  for(int t=0;t<=6;t++)
  {
    DBG_CHECK(num>=0&&num<Mp::TURNQUALITY);
    step>>=1;
    if (key<atanlut[num].wlt)
    {
      num-=step;
      continue;
    }
    if (key>atanlut[num].wlt)
    {
      num+=step;
      continue;
    }
    break;
  }

  /*
  int co=abs(atanlut[num].wlt-key);

  if (num==Mp::TURNQUALITY-1)
  else if (num==0)
  else
  {
    if (abs(abs(atanlut[num].wlt);)
  }
  */

  int retang=atanlut[num].angle;
  if (sx<0&&sy<0)
  {
    retang=Mp::TURNQUALITY-(Mp::HALFANGLE-retang);
  }
  else if(sx<0&&sy>0)
  {
    retang=Mp::HALFANGLE+(Mp::HALFANGLE-retang);
  }
  else if(sx>0&&sy<0)
  {
    retang=Mp::QUARTANGLE+(Mp::QUARTANGLE-retang);
  }
  if (retang>=Mp::TURNQUALITY) retang-=Mp::TURNQUALITY;
  if (retang<0)               retang+=Mp::TURNQUALITY;

  DBG_CHECK(retang>=0&&retang<Mp::TURNQUALITY);
  return retang;
}

int Turn::normalize_angle(int _angle)
{
  if (_angle<0)
  {
    _angle+=Mp::TURNQUALITY;
  }
  else if (_angle>=Mp::TURNQUALITY)
  {
    _angle-=Mp::TURNQUALITY;
  }
  DBG_CHECK(_angle>=0&&_angle<Mp::TURNQUALITY);
  return _angle;
}

void Turn::set_turn_speed(int _turn_speed)
{
  DBG_CHECK(_turn_speed>0);
  angles_per_time=(Mp::TURNQUALITY*_turn_speed)/Mp::METRONQUALITY;
}

void Turn::turn_left(void)
{
  int temp_angle=(angle<<16)+restangle;
  temp_angle-=KbdStat::time()*angles_per_time;
  angle=(temp_angle>>16);
  restangle=temp_angle&0xffff;
  if (angle<0)
  {
    angle+=Mp::TURNQUALITY;
  }
}

void Turn::turn_right(void)
{
  int temp_angle=(angle<<16)+restangle;
  temp_angle+=KbdStat::time()*angles_per_time;
  angle=(temp_angle>>16);
  restangle=temp_angle&0xffff;
  if (angle>=Mp::TURNQUALITY)
  {
    angle-=Mp::TURNQUALITY;
  }
}

int Turn::turn_point(int tox,int toy)
{
  int target=get_dangle(tox-x,toy-y);
  return turn_angle(target);
}

int Turn::turn_angle(int toangle)
{
  if (toangle==angle)
  {
    return 1;
  }
  if (toangle>angle)
  {
    if (toangle-angle<Mp::HALFANGLE)
    {
      turn_right();
      if (toangle<=angle)
      {
        angle=toangle;
        return 1;
      }
      else
      {
        return 0;
      }
    }
    else
    {
      int oldangle=angle;
      turn_left();
      if (angle-oldangle>0&&angle<=toangle)
      {
        angle=toangle;
        return 1;
      }
      else
      {
        return 0;
      }
    }
  }
  else
  {
    if (angle-toangle<=Mp::HALFANGLE)
    {
      turn_left();
      if (toangle>=angle)
      {
        angle=toangle;
        return 1;
      }
      else
      {
        return 0;
      }
    }
    else
    {
      int oldangle=angle;
      turn_right();
      if (angle-oldangle<0&&angle>=toangle)
      {
        angle=toangle;
        return 1;
      }
      else
      {
        return 0;
      }
    }
  }
}

int Turn::point_r_or_l(int tox,int toy)
{
  int tovect_x=tox-x;
  int tovect_y=toy-y;
  int myvect_x=Posit::x_offset(angle,Mp::MAXOFFSET);
  int myvect_y=Posit::y_offset(angle,Mp::MAXOFFSET);
  int direction=myvect_x*tovect_y-myvect_y*tovect_x;
  if (direction>0)
  {
    return 1;
  }
  else if (direction<0)
  {
    return -1;
  }
  else
  {
    int the_same_angle=((sign(tovect_x)==sign(myvect_x))&&(sign(tovect_y)==sign(myvect_y)));
    return (!the_same_angle);
  }
}

int Turn::angle_r_or_l(int toangle,int fromangle)
{
  if (fromangle==-1)
  {
    fromangle=angle;
  }
  if (toangle==fromangle)
  {
    return 0;
  }
  if (toangle>fromangle)
  {
    if (toangle-fromangle<Mp::HALFANGLE)
    {
      return 1;
    }
    else
    {
      return -1;
    }
  }
  else
  {
    if (fromangle-toangle<=Mp::HALFANGLE)
    {
      return -1;
    }
    else
    {
      return 1;
    }
  }
}

int Turn::sub_angle(int fromangle,int toangle)
{
  int t=0;
  int sign=0;
  if (fromangle>toangle)
  {
    sign=-1;
    t=fromangle-toangle;
  }
  else
  {
    sign=1;
    t=toangle-fromangle;
  }
  if (t>Mp::HALFANGLE)
  {
    sign=-sign;
    t=Mp::TURNQUALITY-t;
  }
  return t*sign;
}
//KLASA MOVE
Move::Move(int _angle,int _actual_speed,Op::ObjPack& ref)
:Turn(_angle,ref)
{
  max_speed=ref.max_speed;
  strafe_speed=ref.strafe_speed;
  accel_pwr=(ref.accel_pwr*0x10000)/Mp::METRONQUALITY;
  brake_pwr=ref.brake_pwr/Mp::METRONQUALITY;
  auto_pwr=ref.auto_pwr/Mp::METRONQUALITY;
  set_speed(_actual_speed);
}

void Move::go(void)
{
  int dx=vx*KbdStat::time();
  int dy=vy*KbdStat::time();
  int newx=(x<<16)+fractx+dx;
  int newy=(y<<16)+fracty+dy;
  x=(newx>>16);
  y=(newy>>16);
  fractx=newx&0xffff;
  fracty=newy&0xffff;
  DBG_CHECK(x>-100&&x<Level::max_sx+100);
  DBG_CHECK(y>-100&&y<Level::max_sy+100);
}


void Move::set_speed(int _speed,int _angle)
{
  if (_angle==-1)
  {
    _angle=angle;
  }
  if (_speed>max_speed)
  {
    _speed=max_speed;
  }
  else if (_speed<0)
  {
    _speed=0;
  }
  DBG_CHECK(_angle>=0&&_angle<Mp::TURNQUALITY);
  vx=(_speed*Posit::vxlut[_angle])/Mp::METRONQUALITY;
  vy=(_speed*Posit::vylut[_angle])/Mp::METRONQUALITY;
}

void Move::_set_speed(int _speed,int _angle)
{
  if (_angle==-1)
  {
    _angle=angle;
  }
  DBG_CHECK(_angle>=0&&_angle<Mp::TURNQUALITY);
  vx=(_speed*Posit::vxlut[_angle])>>16;
  vy=(_speed*Posit::vylut[_angle])>>16;
}

void Move::transform_speed(int _angle)
{
  if (_angle==-1)
  {
    _angle=angle;
  }
  DBG_CHECK(_angle>=0&&_angle<Mp::TURNQUALITY);
  _set_speed(_get_speed(),_angle);
}


void Move::update_speed(void)
{
  int old_speed=get_speed();
  if (old_speed>max_speed)
  {
    int decrease_ratio=(max_speed<<10)/old_speed;
    DBG_CHECK(decrease_ratio>0);
    vx=(vx*decrease_ratio)>>10;
    vy=(vy*decrease_ratio)>>10;
  }
}

void Move::brake(void)
{
  int brake=brake_pwr*KbdStat::time();
  vx-=((vx*brake)>>16);
  vy-=((vy*brake)>>16);
  if (abs(vx)<Mp::MINVX&&abs(vy)<Mp::MINVY)
  {
    vx=0;
    vy=0;
  }
}

void Move::auto_brake(void)
{
  int brake=auto_pwr*KbdStat::time();
  vx-=((vx*brake)>>16);
  vy-=((vy*brake)>>16);
  if (abs(vx)<Mp::MINVX&&abs(vy)<Mp::MINVY)
  {
    vx=0;
    vy=0;
  }
}

void Move::strafe(int _angle)
{
  DBG_CHECK(_angle>=0&&_angle<Mp::TURNQUALITY);
  vx+=(strafe_speed*Posit::vxlut[_angle])/Mp::METRONQUALITY;
  vy+=(strafe_speed*Posit::vylut[_angle])/Mp::METRONQUALITY;
  update_speed();
}

void Move::set_max_speed(int _max_speed)
{
  max_speed=_max_speed;
}

void Move::set_strafe_speed(int _strafe_speed)
{
  strafe_speed=_strafe_speed;
}

void Move::_kick_me(int _angle,int _accel)
{
  int accel=(int)((_accel*0x10000)/Mp::METRONQUALITY)*KbdStat::time();
  DBG_CHECK(accel>0);
  vx+=((accel>>8)*(Posit::vxlut[_angle]>>8))/Mp::METRONQUALITY;
  vy+=((accel>>8)*(Posit::vylut[_angle]>>8))/Mp::METRONQUALITY;
  update_speed();
}

void Move::kick_me(int _x,int _y,KickPower pow)
{
  int _angle=Turn::get_dangle(x-_x,y-_y);
  kick_me(_angle,pow);
}

void Move::kick_me(int _angle,KickPower pow)
{
  int accel=0;
  switch (pow)
  {
      case WEAK: accel=3000;
                 break;
    case MEDIUM: accel=6000;
                 break;
    case STRONG: accel=9000;
                 break;
    case OHFUCK: accel=12000;
                 break;
        default: DBG_CHECK(0);
                 break;
  }
  _kick_me(_angle,accel);
}
//KLASA VISIBLE
Sprite Visible::dummy;

Visible::Visible(View::Vprty _prty):sprite(Visible::dummy)
{
  no_sprite=1;
  prty=_prty;
}

Visible::Visible(Sprite& _sprite,View::Vprty _prty,int _phase):sprite(_sprite)
{
  no_sprite=0;
  prty=_prty;
  reset_range();
  phase=_phase;
}

void Visible::set_range(int _downrange,int _uprange)
{
  DBG_CHECK(!no_sprite);
  DBG_CHECK(_uprange>=0&&_uprange<sprite.phases);
  DBG_CHECK(_downrange>=0&&_downrange<sprite.phases);
  DBG_CHECK(_downrange<=_uprange);
  downrange=_downrange;
  uprange=_uprange;
}

void Visible::reset_range(void)
{
  DBG_CHECK(!no_sprite);
  DBG_CHECK(sprite.phases>0);
  downrange=0;
  uprange=sprite.phases-1;
}

int Visible::is_phase_in(int range,int _x,int _y,Phase& p,int id)
{
  if (id==-1)
  {
    FOR_ALL_USERS(ID)
    {
      int scrx=_x-world->get_x(ID);
      int scry=_y-world->get_y(ID);
      if (!(scrx+p.r<-range||scrx+p.l>=world->get_sx()+range||scry+p.d<-range||scry+p.u>=world->get_sy()+range))
      {
        return 1;
      }
    }
    return 0;
  }
  else
  {
    int scrx=_x-world->get_x(id);
    int scry=_y-world->get_y(id);
    return (!(scrx+p.r<-range||scrx+p.l>=world->get_sx()+range||scry+p.d<-range||scry+p.u>=world->get_sy()+range));
  }
}

int Visible::is_in(int range,int id)
{
  DBG_CHECK(!no_sprite);
  DBG_CHECK(phase<sprite.phases);
  DBG_CHECK(phase>=0);
  Phase &p=sprite[phase];
  return is_phase_in(range,x,y,p,id);
}

int Visible::is_in_overscan(int id)
{
  DBG_CHECK(!no_sprite);
  DBG_CHECK(phase<sprite.phases);
  DBG_CHECK(phase>=0);
  Phase &p=sprite[phase];
  int left_edge=-Mp::OVERSCANX;
  int right_edge=world->get_sx()+Mp::OVERSCANX;
  int up_edge=-Mp::OVERSCANY;
  int down_edge=world->get_sy()+Mp::OVERSCANY;
  if (id==-1)
  {
    FOR_ALL_USERS(ID)
    {
      int scrx=x-world->get_x(ID);
      int scry=y-world->get_y(ID);
      if (!(scrx+p.r<left_edge||scrx+p.l>=right_edge||scry+p.d<up_edge||scry+p.u>=down_edge))
      {
        return 1;
      }
    }
    return 0;
  }
  else
  {
    int scrx=x-world->get_x(id);
    int scry=y-world->get_y(id);
    return (!(scrx+p.r<left_edge||scrx+p.l>=right_edge||scry+p.d<up_edge||scry+p.u>=down_edge));
  }
}

#if HI_DEBUG
void Visible::draw_shadow(Screen& screen,int plane)
{
  (void)screen;
  (void)plane;
  FAILURE("Visible::draw_shadow() - you cannot call Visible::draw_shadow().");
}
#endif

//KLASA SHADOW
void Shadow::draw_shadow(Screen& screen,int plane)
{
  (void)plane;
  DBG_CHECK(!no_sprite);
  screen.shape(x,y+height+Mp::SHADOWDIST,get_sprite(),get_phase(),Spr::with_tool,(void*)&Tools::tool[Tools::SHADOW]);
}

//KLASA LISTMANAGER
List<Listmanager,Oprty,oprty_types> *Listmanager::list;
int Listmanager::initialized(0);

void Listmanager::init(void)
{
  typedef List<Listmanager,Oprty,oprty_types> _LIST;
  list=NEW(_LIST("Listmanager::list"),list_mbn);
  DBG_CHECK(!initialized);
  initialized=1;
}

void Listmanager::quit(void)
{
  Object *temp;
  temp=(Object*)list->reset();
  while (temp!=NULL)
  {
    delete temp;
    temp=(Object*)list->get_next();
  }
  int objects_numb=list->size();
  DBG_CHECK(objects_numb==0);
  initialized=0;
  if (list!=NULL)
  {
    delete list;
    list=NULL;
  }
}

Listmanager::Listmanager(Oprty prty)
{
  DBG_CHECK(initialized);
  list->add(this,prty);
}

Listmanager::~Listmanager(void)
{
  DBG_CHECK(initialized);
  list->remove(this);
}

void Listmanager::run(void)
{
  DBG_CHECK(initialized);
  for (Object* temp=(Object*)list->reset();temp!=NULL;temp=(Object*)list->get_next())
  {
    temp->run();
  }
}
//KLASA IMPEXP
Impexp::Impexp(int _level_num,World* _world)
{
  DBG_CHECK(_world);
  if (_level_num==-1)
  {
    no_in_level=1;
    lobject=NULL;
    level_plane=level_num=0;
    return;
  }
  else no_in_level=0;

  to_return=1;
  level_num=_level_num;
  level_plane=_world->get_level().oplane(level_num);
  _world->get_level()[level_num].active=0;
  lobject=&_world->get_level()[level_num];
  _world->get_level().update(level_num);
}

Impexp::~Impexp(void)
{
  if (no_in_level) return;
  if (to_return)
  {
    lobject->active=1;
    world->get_level().update(level_num);
  }
}

void Impexp::remove_from_level(void)
{
  to_return=0;
}
//KLASA LOWCOLIS
#define NIL -1

int LowColis::initialized(0);
unsigned char* LowColis::mem_for_cworld[NoNet::max_users+1];
Screen LowColis::cworld[NoNet::max_users+1];
Cmanager LowColis::cm[NoNet::max_users+1];
LowColis::Msg_unit *LowColis::messages=NULL;
NoQueue<int> *LowColis::free_msg_cells=NULL;
NoQueue<LowColis*> *LowColis::logged_objects=NULL;
#if HI_DEBUG
int LowColis::msg_size(0);
int LowColis::msg_used(0);
int LowColis::max_msg_used(0);
int LowColis::max_user_used(0);
char *LowColis::max_user_name=NULL;
#endif

void LowColis::init(void)
{
  DBG_CHECK(!initialized);
  initialized=1;
  messages=(LowColis::Msg_unit*)Heap::alloc(sizeof(LowColis::Msg_unit)*Mp::COLISQUEUESIZE,gobjects_mbn);
  free_msg_cells=NEW(NoQueue<int>(Mp::COLISQUEUESIZE,"LowColis::free_msg_cells",1),queue_mbn);
  logged_objects=NEW(NoQueue<LowColis*>(Mp::COLISQUEUESIZE,"LowColis::logged_objects"),queue_mbn);
  for (int i=0;i<Mp::COLISQUEUESIZE;i++)
  {
    messages[i].next=NIL;
    free_msg_cells->add(&i);
  }
#if HI_DEBUG
  msg_size=Mp::COLISQUEUESIZE;
  msg_used=0;
  max_msg_used=0;
  max_user_used=0;
  max_user_name=NULL;
#endif
  for (i=0;i<NoNet::max_users+1;i++)
  {
    mem_for_cworld[i]=NULL;
  }
}

void LowColis::quit(void)
{
#if HI_DEBUG
  for (int i=0;i<NoNet::max_users+1;i++)
  {
    CHECK(mem_for_cworld[i]==NULL);
  }
#endif
  if (free_msg_cells!=NULL)
  {
    delete free_msg_cells;
    free_msg_cells=NULL;
  }
  if (logged_objects!=NULL)
  {
    delete logged_objects;
    logged_objects=NULL;
  }
  if (messages!=NULL)
  {
    Heap::free((void*)messages);
    messages=NULL;
  }
#if HI_DEBUG
  if (max_msg_used>msg_size/2)
  {
    DBG_WARNING("[col] occupancy exceeded 50%");
  }
  DBG_MESSAGE("maximum occupancy: %d%%",(max_msg_used*100/msg_size));
  DBG_MESSAGE("maximum user occupancy: %d%% (%d messages)",(max_user_used*100/msg_size),max_user_used);
  DBG_MESSAGE("maximum user: \"%s\"",(max_user_name?max_user_name:"*** not specified ***"));
  initialized=0;
#endif
}

void LowColis::create_cworld(int id)
{
  DBG_CHECK(initialized);
  DBG_CHECK(NoNet::check_id(id)&&mem_for_cworld[id]==NULL);
  int tmp_sx=Mp::SX+2*Mp::OVERSCANX;
  int tmp_sy=Mp::SY+2*Mp::OVERSCANY;
  mem_for_cworld[id]=(unsigned char*)Heap::alloc(Screen::mem_required(tmp_sx,tmp_sy,Spr::collis),gobjects_mbn);
  cworld[id]=Screen((unsigned char*)mem_for_cworld[id],tmp_sx,tmp_sy,tmp_sx,Spr::collis);
  cm[id].init(cworld[id],256);
}

void LowColis::delete_cworld(int id)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mem_for_cworld[id]!=NULL);
  if (mem_for_cworld[id]!=NULL)
  {
    Heap::free(mem_for_cworld[id],FILE_LINE);
    mem_for_cworld[id]=NULL;
    cm[id].free();
  }
}

LowColis *LowColis::first_logged(void)
{
  LowColis** temp=logged_objects->see_first();
  if (temp!=NULL)
  {
    return (*temp);
  }
  return NULL;
}

LowColis *LowColis::next_logged(void)
{
  LowColis** temp=logged_objects->see_next();
  if (temp!=NULL)
  {
    return (*temp);
  }
  return NULL;
}

LowColis::LowColis(int wall,int my,int alien,int weapthrough,int objthrough,int myship,int killer,int killable,int collect,int destroy,char* user_name)
{
  DBG_CHECK(initialized);
  myinfo.WALL=wall;
  myinfo.MY=my;
  myinfo.ALIEN=alien;
  myinfo.WEAPTHROUGH=weapthrough;
  myinfo.OBJTHROUGH=objthrough;
  myinfo.MYSHIP=myship;
  myinfo.KILLER=killer;
  myinfo.KILLABLE=killable;
  myinfo.COLLECT=collect;
  myinfo.destroy=destroy;
  myinfo.x=0;
  myinfo.y=0;
  myinfo.vx=0;
  myinfo.vy=0;
#if HI_DEBUG
  user_msg_name=user_name;
  user_msg_used=0;
#else
  (void)user_name;
#endif
  user_msg_cursor=NIL;
}

LowColis::~LowColis(void)
{
  DBG_CHECK(initialized);
  reset_cmsg();
}

Cmsg* LowColis::get_cmsg(void)
{
  DBG_CHECK(initialized);
  if (user_msg_cursor!=NIL)
  {
    int curr=user_msg_cursor;
    user_msg_cursor=messages[user_msg_cursor].next;
    messages[curr].next=NIL;
    DBG_CHECK((--user_msg_used)>=0);
    DBG_CHECK((--msg_used)>=0);
    free_msg_cells->add(&curr);
    return &messages[curr].msg;
  }
  return NULL;
}

void LowColis::set_cinfo(int wall,int my,int alien,int weapthrough,int objthrough,int myship,int killer,int killable,int collect)
{
  DBG_CHECK(initialized);
  myinfo.WALL=wall;
  myinfo.MY=my;
  myinfo.ALIEN=alien;
  myinfo.WEAPTHROUGH=weapthrough;
  myinfo.OBJTHROUGH=objthrough;
  myinfo.MYSHIP=myship;
  myinfo.KILLER=killer;
  myinfo.KILLABLE=killable;
  myinfo.COLLECT=collect;
}

void LowColis::reset_cmsg(void)
{
  DBG_CHECK(initialized);
  while (get_cmsg());
  DBG_CHECK(user_msg_used==0);
}

void LowColis::add_cmsg(Cmsg* cmsg,int id)
{
  (void)id;
  DBG_CHECK(initialized);
  int *free=free_msg_cells->head();
  DBG_CHECK(free!=NULL);
  if (!free)
  {
    return;
  }
  messages[*free].msg=*cmsg;
  messages[*free].next=user_msg_cursor;
  user_msg_cursor=*free;
#if HI_DEBUG
  msg_used++;
  user_msg_used++;
  if (msg_used>max_msg_used)
  {
    max_msg_used=msg_used;
  }
  if (user_msg_used>max_user_used)
  {
    max_user_used=user_msg_used;
    max_user_name=user_msg_name;
  }
#endif
}

void LowColis::check_id(int id)
{
  DBG_CHECK(initialized);
  DBG_CHECK(NoNet::check_id(id));
  (void)id;
}

void LowColis::write(int id,int x,int y,int vx,int vy,Sprite& sprite,int phase,int mirror)
{
  DBG_CHECK(initialized);
  myinfo.x=x;
  myinfo.y=y;
  myinfo.vx=vx;
  myinfo.vy=vy;
  LowColis *temp=this;
  logged_objects->add(&temp);
  if (id==-1)
  {
    FOR_ALL_USERS(ID)
    {
      check_id(ID);
      cm[ID].write(x,y,sprite,phase,mirror,(void*)this);
    }
  }
  else
  {
    check_id(id);
    cm[id].write(x,y,sprite,phase,mirror,(void*)this);
  }
}

void LowColis::check(int id,int x,int y,int vx,int vy,Sprite& sprite,int phase,int mirror)
{
  DBG_CHECK(initialized);
  myinfo.x=x;
  myinfo.y=y;
  myinfo.vx=vx;
  myinfo.vy=vy;
  LowColis** temp=NULL;
  if (id==-1)
  {
    FOR_ALL_USERS(ID)
    {
      check_id(ID);
      for (temp=(LowColis**)cm[ID].check(x,y,sprite,phase,mirror);*temp;temp++)
      {
        add_cmsg((*temp)->respond(),ID);
        (*temp)->add_cmsg(respond(),ID);
      }
    }
  }
  else
  {
    check_id(id);
    for (temp=(LowColis**)cm[id].check(x,y,sprite,phase,mirror);*temp;temp++)
    {
      add_cmsg((*temp)->respond(),id);
      (*temp)->add_cmsg(respond(),id);
    }
  }
}

void LowColis::see(int id,int x,int y,Sprite& sprite,int phase,int mirror)
{
  DBG_CHECK(initialized);
  LowColis** temp=NULL;
  if (id==-1)
  {
    FOR_ALL_USERS(ID)
    {
      check_id(ID);
      for (temp=(LowColis**)cm[ID].check(x,y,sprite,phase,mirror);*temp;temp++)
      {
        add_cmsg((*temp)->respond(),ID);
      }
    }
  }
  else
  {
    check_id(id);
    for (temp=(LowColis**)cm[id].check(x,y,sprite,phase,mirror);*temp;temp++)
    {
      add_cmsg((*temp)->respond(),id);
    }
  }
}

void LowColis::write(int id,int x,int y,int vx,int vy)
{
  DBG_CHECK(initialized);
  myinfo.x=x;
  myinfo.y=y;
  myinfo.vx=vx;
  myinfo.vy=vy;
  LowColis *temp=this;
  logged_objects->add(&temp);
  if (id==-1)
  {
    FOR_ALL_USERS(ID)
    {
      check_id(ID);
      cm[ID].write(x,y,(void*)this);
    }
  }
  else
  {
    check_id(id);
    cm[id].write(x,y,(void*)this);
  }
}

void LowColis::write_to_logged(int x,int y,int vx,int vy)
{
  DBG_CHECK(initialized);
  myinfo.x=x;
  myinfo.y=y;
  myinfo.vx=vx;
  myinfo.vy=vy;
  LowColis *temp=this;
  logged_objects->add(&temp);
}

void LowColis::check(int id,int x,int y,int vx,int vy)
{
  DBG_CHECK(initialized);
  myinfo.x=x;
  myinfo.y=y;
  myinfo.vx=vx;
  myinfo.vy=vy;
  if (id==-1)
  {
    FOR_ALL_USERS(ID)
    {
      check_id(ID);
      if (Posit::is_in_overscan(0,x,y,ID))
      {
        LowColis* temp=(LowColis*)cm[ID].check(x,y);
        if (temp)
        {
          add_cmsg(temp->respond(),ID);
          temp->add_cmsg(respond(),ID);
        }
      }
    }
  }
  else
  {
    check_id(id);
    if (Posit::is_in_overscan(0,x,y,id))
    {
      LowColis* temp=(LowColis*)cm[id].check(x,y);
      if (temp)
      {
        add_cmsg(temp->respond(),id);
        temp->add_cmsg(respond(),id);
      }
    }
  }
}

Cmsg* LowColis::see(int id,int x,int y,int copy_to_queue)
{
  DBG_CHECK(initialized);
  DBG_CHECK(id!=-1);
  check_id(id);
  if (Posit::is_in_overscan(0,x,y,id))
  {
    LowColis* temp=(LowColis*)cm[id].check(x,y);
    if (temp)
    {
      if (copy_to_queue)
      {
        add_cmsg(temp->respond(),id);
      }
      return temp->respond();
    }
  }
  return NULL;
}

void LowColis::clean(void)
{
  DBG_CHECK(initialized);
  FOR_ALL_USERS(ID)
  {
    DBG_CHECK(mem_for_cworld[ID]!=NULL);
    cm[ID].reset();
  }
  logged_objects->flush();
}

void LowColis::origin(int id,int ox,int oy)
{
  DBG_CHECK(initialized);
  check_id(id);
  cworld[id].origin(ox,oy);
}

void LowColis::show_cworld(int id,Screen& screen,int posx,int posy)
{
  CHECK(initialized);
  cworld[id].copy(0,screen,posx,posy);
}
//KLASA COLIS
Colis::Colis(int wall,int my,int alien,int weapthrough,int objthrough,int updnthrough,int killer,int killable,int collect,int destroy,char *user_name)
:LowColis(wall,my,alien,weapthrough,objthrough,updnthrough,killer,killable,collect,destroy,user_name)
{
}

Colis::Colis(int wall,int my,int alien,int weapthrough,int objthrough,int updnthrough,int killer,int killable,int collect,Op::ObjPack& ref,char *user_name)
:LowColis(wall,my,alien,weapthrough,objthrough,updnthrough,killer,killable,collect,ref.destroy,user_name)
{
}

int Colis::wall_angle(int edge,int circle_size)
{

  //popraw to !!!!! (jak obiekt jest poza swiatem kolizji to dostaje 116
  
  CHECK(!NoNet::is_network_mode());
  int size=circle_size*5;
  Cmsg* temp=NULL;
  int _x;
  int _y;
  int dx=0;
  int dy=0;

  for (register int i=0;i<Mp::TURNQUALITY;i+=8)
  {
    _x=x+Posit::x_offset(i,size);
    _y=y+Posit::y_offset(i,size);
    if (Posit::is_in_overscan(0,_x,_y))
    {
      temp=LowColis::see(NoNet::get_this_id(),_x,_y,0);
      if (temp&&temp->WALL)
      {
        dx-=x_offset(i,size);
        dy-=y_offset(i,size);
      }
    }
    else
    {
      if (edge)
      {
        dx-=x_offset(i,Mp::MAXOFFSET);
        dy-=y_offset(i,Mp::MAXOFFSET);
      }
    }
  }
  if (!dx&&!dy)
  {
    return -1;
  }
  return (int)Turn::get_dangle((int)dx,(int)dy);
}

int Colis::wall_angle2(int circle_size)
{
  CHECK(!NoNet::is_network_mode());
  int size=circle_size*5;
  Cmsg* temp=NULL;
  int _x;
  int _y;
  int dx=0;
  int dy=0;
  int wll=0;
  int cnt=0;

  for (register int i=0;i<Mp::TURNQUALITY;i+=8)
  {
    cnt++;
    _x=x+Posit::x_offset(i,size);
    _y=y+Posit::y_offset(i,size);
    if (Posit::is_in_overscan(0,_x,_y))
    {
      temp=LowColis::see(NoNet::get_this_id(),_x,_y,0);
      if (temp&&temp->WALL)
      {
        dx-=x_offset(i,size);
        dy-=y_offset(i,size);
      }
    }
    else
    {
      dx-=x_offset(i,Mp::MAXOFFSET);
      dy-=y_offset(i,Mp::MAXOFFSET);
      wll++;
    }
  }
  if (cnt==wll)  return -2;
  if (!dx&&!dy)  return -1;
  return (int)Turn::get_dangle((int)dx,(int)dy);
}


//KLASA LINK
int Link::initialized(0);
unsigned short *Link::prv_link=NULL;

Link::Link(int _l_num,int _precision)
{
  l_num=_l_num;
  precision=_precision;
}

void Link::init(void)
{
  prv_link=(unsigned short*)Heap::alloc(sizeof(unsigned short)*world->get_level().size,gobjects_mbn);
  for (register int i=1;i<world->get_level().size;i++)
  {
    if (world->get_level()[i].link)
    {
      prv_link[world->get_level()[i].link]=i;
    }
  }
  initialized=1;
}

void Link::quit(void)
{
  if (prv_link!=NULL)
  {
    Heap::free(prv_link);
    prv_link=NULL;
  }
  initialized=0;
}

void Link::check_init(void)
{
  DBG_CHECK(initialized);
}

void Link::send_impulse(int lev_num)
{
  for (int i=world->get_level()[lev_num].link;i&&!world->get_level()[i].status&&i!=lev_num;i=world->get_level()[i].link)
  {
    if (!i)  break;
    if (LevImp::get_parameters(world->get_level()[i].type)->is_object)
      world->get_level()[i].status=1;
  }
}

//KLASA RADAR
NoQueue<Radar*> *Radar::dott;
unsigned char Radar::points[32];
unsigned char Radar::queue[32];
int     Radar::range;
int     Radar::R;
int     Radar::X;
int     Radar::Y;
int     Radar::TargetX;
int     Radar::TargetY;
List<Radar::GM_Rad,int,1> *Radar::tar;
int     Radar::cur_type[MAX_RADARCUR_TYPE];
int     Radar::last;
int     Radar::radarON;
int     Radar::time;
int     Radar::ttime;
int     Radar::phase;
int     Radar::lphase;
int     Radar::ltime;
Radar*  Radar::current;

Radar::Radar(int _tar,int _id,int _dx,int _dy)
{
  ray=Mp::RADARALIENRAY;
  dx=_dx;
  dy=_dy;
  if (_tar)
  {
    gm_rad.tar=1;
    gm_rad.id=_id;
    gm_rad.type=_tar;
    gm_rad.snd=0;
    gm_rad.pttr=this;
    tar->add(&gm_rad,0);
  }
  else
  {
    gm_rad.tar=0;
  }
  targeting=1;
  distance=Mp::RADARDISTANCE;
}

Radar::~Radar(void)
{
  if (gm_rad.tar) tar->remove(&gm_rad);
}

void Radar::init(void)
{
  typedef List<GM_Rad,int,1> _LIST;
  tar=NEW(_LIST("Radar::Tar"),list_mbn);

  R=Mp::TURNQUALITY/Mysprites::radar_point.phases;
  ttime=time=last=0;
  range=Mp::RADARDISTANCE/Tools::max_radartools;
  for(register int i=0;i<32;i++) points[i]=queue[i]=0;
  dott=NEW(NoQueue<Radar*>(70,"Radar Queue"),queue_mbn);
  memset((void*)cur_type,0,sizeof(int)*5);
  current=NULL;
  phase=lphase=ltime=TargetX=TargetY=0;
  Tools::init_radar_tool();
}

void Radar::done(void)
{
  if (tar!=NULL)
  {
    GM_Rad *temp;
    temp=(GM_Rad*)tar->reset();
    while (temp!=NULL)
    {
      tar->remove(temp);
      temp=(GM_Rad*)tar->get_next();
    }
    DBG_CHECK(tar->size()==0);
    delete tar;
    tar=NULL;
  }
  if (dott!=NULL)
  {
    delete dott;
    dott=NULL;
  }
  current=NULL;
}

void Radar::set_cur_type(int *ptr,int size)
{
  DBG_CHECK(size<MAX_RADARCUR_TYPE);
  int *t=ptr;
  for(int i=0;i<MAX_RADARCUR_TYPE;i++,t++)
  {
    if (i<size) cur_type[i]=*t;
    else cur_type[i]=0;
  }
}

void Radar::put(Screen& screen,int num,int _R)
{
  if (NoNet::is_network_mode())
  {
    if (_R>0)
      screen.shape(X,Y,Mysprites::radar_point,num,Spr::nomirror,(void*)&Tools::colors[Tools::NET_FIRST-1+_R]);
    else
      screen.shape(X,Y,Mysprites::radar_point,num,Spr::with_tool,(void*)&Tools::radar_tool[_R]);
    return;
  }
  screen.shape(X,Y,Mysprites::radar_point,num,Spr::with_tool,(void*)&Tools::radar_tool[_R]);
}

void Radar::radar_log(void)
{
  if (NoNet::get_ship()->is_alive())
  {
    Radar* temp=this;
    dott->add(&temp);
  }
}

void Radar::login(void)
{
  int _id;
  int t=Turn::get_dangle(x-X,y-Y)/R;
  if (NoNet::is_network_mode())
  {
    FOR_ALL_USERS(ID)
    {
      if (NoNet::get_ship(ID)==this)
      {
        _id=ID;
        break;
      }
    }
    points[t]=_id;
    return;
  }
  int s=(Tools::max_radartools-1)-Posit::vect_len(x-X,y-Y)/range;
  if (s<0) return;
  if (s>points[t]) points[t]=s;
}

void Radar::login(Screen& screen)
{
  int prc=life_percent();
  if (targeting)
  {
    if (prc>0)
    {
      screen.put(x,y,Mysprites::radar_scope,phase);
      screen.put(x,y,Mysprites::indicator,(Mysprites::indicator.phases-1)-(Mysprites::indicator.phases*prc/101));
      screen.shape(x,y,Mysprites::indicatore,(Mysprites::indicatore.phases-1)-(Mysprites::indicatore.phases*prc/101),Spr::with_tool,(void*)&Tools::tool[Tools::LSHADOW]);
    }
    else
      screen.put(x,y-Mp::PIXELSBETWEENPLANES,Mysprites::radar_landing,lphase);
  }
  int road=Posit::vect_len((x+dx)-X,(y+dy)-Y);
  if (road<180) return;
  int t=Turn::get_dangle(x-X+dx,y-Y+dy)/R;
  int s=(Tools::max_radartools-1)-road/(2*range);
  if (s<0) s=0;
  screen.shape(X,Y,Mysprites::radar_target,t,Spr::with_tool,(void*)&Tools::radar_tool[s]);
}

int Radar::check_cur_type(int type)
{
  for(int i=0;i<MAX_RADARCUR_TYPE;i++)
    if(type==cur_type[i]) return 1;
  return 0;
}

void Radar::compute(void)
{
  if ((!NoNet::is_network_mode()&&radarON)||NoNet::is_network_mode(NoNet::NETWORKFLAG))
  {
    GM_Rad* max=NULL;
    int     maxi=99999999;
    for(GM_Rad* tmp=tar->reset();tmp;tmp=tar->get_next())
    {
      if (check_cur_type(tmp->type))
      {
        if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
        {
          if (!(tmp->id==NoNet::get_this_id()))continue;
        }
        int t;
        if (maxi>(t=Posit::vect_len(X-(tmp->pttr->x+tmp->pttr->dx),Y-(tmp->pttr->y+tmp->pttr->dy))))
        {
          maxi=t;
          max=tmp;
        }
      }
    }
    if (max)
    {
      current=max->pttr;
      TargetX=max->pttr->x;
      TargetY=max->pttr->y;
      int prc=max->pttr->life_percent();
    }
    else
    {
      current=NULL;
      TargetX=0;
      TargetY=0;
    }
    if (current)
    {
      int dis=8*Posit::vect_len(TargetX-X,TargetY-Y);
    }
  }
  else
    current=NULL;
  ttime+=KbdStat::time();
  if (time>300)
  {
    if ((++phase)>=Mysprites::radar_scope.phases) phase=0;
    time=0;
  }
  ltime+=KbdStat::time();
  if (ltime>300)
  {
    if ((++lphase)>=Mysprites::radar_landing.phases) lphase=0;
    ltime=0;
  }
}

void Radar::draw(Screen& screen)
{
  for (Radar** temp=dott->see_first();temp;temp=dott->see_next())
    if (temp) (*temp)->login();

  for (register int i=0;i<32;i++)
  {
    put(screen,i,points[i]);
    points[i]=0;
  }
  if (current)  current->login(screen);
}

void Radar::get_target(int* tx,int* ty)
{
  GM_Rad* max=NULL;
  int     maxi=99999999;
  for(GM_Rad* tmp=tar->reset();tmp;tmp=tar->get_next())
  {
    if (check_cur_type(tmp->type))
    {
      int t;
      if (maxi>(t=Posit::vect_len(X-tmp->pttr->x,Y-tmp->pttr->y)))
      {
        maxi=t;
        max=tmp;
      }
    }
  }
  if (max)
  {
    *tx=max->pttr->x;
    *ty=max->pttr->y;
  }
}

void Radar::clean(void)
{
  dott->flush();
  time+=KbdStat::time();
}


//KLASA HANDLE
List<Handle,int,1> *Handle::mail_box=NULL;
int Handle::initialized(0);

void Handle::init(void)
{
  DBG_CHECK(!initialized);
  initialized=1;
  typedef List<Handle,int,1> _LIST;
  mail_box=NEW(_LIST("Handle::mail_box"),list_mbn);
}

void Handle::done(void)
{
  if (mail_box!=NULL)
  {
    if (!Game::get_failure_detection())
    {
      DBG_CHECK(mail_box->size()==0);
      delete mail_box;
    }
    mail_box=NULL;
  }
  initialized=0;
}

//SOUND
Sample Sound::sounds[MAX_GM_SOUNDS];
char   Sound::sounds_name[MAX_GM_SOUNDS][30];
int    Sound::sample_count;

void Sound::init(void)
{
  sample_count=0;
}

int Sound::load_sound(char* txt)
{
  DBG_CHECK(sample_count<MAX_GM_SOUNDS);
  for(int i=0;i<sample_count;i++)
  {
    if (!strcmpi(sounds_name[i],txt))
      return i;
  }
  strcpy(sounds_name[sample_count],txt);
  sounds[sample_count].load(txt);
  sample_count++;
  return sample_count-1;
}

void Sound::pl(Sample* smp,int _x,int _y)
{
  int srx=world->get_x()+Mp::SX/2;
  int sry=world->get_y()+Mp::SY/2;
  int dis=vect_len(srx-_x,sry-_y);
  int x=-(world->get_x()-_x+Mp::SX/2)*7;
  int vol;
  if(dis<Op::R1) vol=VOLUME_MAX;
  else if(dis>Op::R2) vol=Op::MIN_VOL;
  else vol=((Op::MIN_VOL-VOLUME_MAX)*(dis-Op::R1))/(Op::R2-Op::R1)+VOLUME_MAX;
  if (MManager::is_speaking()) vol=vol*Mp::LOWVOICE/100;
  //DBG_MESSAGE("dis=%d, R1=%d, R2=%d, volume=%d",dis,Op::R1,Op::R2,vol);
  smp->play(x,vol);
}


void Sound::netpl(Sample* smp,int _x,int _y)
{
  int srx=world->get_x()+Mp::SX/2;
  int sry=world->get_y()+Mp::SY/2;
  int dis=vect_len(srx-_x,sry-_y);
  int x=-(world->get_x()-_x+Mp::SX/2)*7;
  int vol;
  if(dis<Op::NETR1) vol=VOLUME_MAX;
  else if(dis>Op::NETR2) return;
  else vol=((Op::NETMIN_VOL-VOLUME_MAX)*(dis-Op::NETR1))/(Op::NETR2-Op::NETR1)+VOLUME_MAX;
  if (MManager::is_speaking()) vol=vol*Mp::LOWVOICE/100;
  //DBG_MESSAGE("dis=%d, R1=%d, R2=%d, volume=%d",dis,Op::R1,Op::R2,vol);
  smp->play(x,vol);
}


void Sound::mission_play(int smp)
{
  DBG_CHECK(sample_count>smp);
  sounds[smp].play(0,VOLUME_MAX);
}


void Sound::play(int smp,int _x,int _y)
{
  DBG_CHECK(sample_count>smp);
  if (NoNet::is_network_mode()) netpl(&sounds[smp],_x,_y);
  else  pl(&sounds[smp],_x,_y);
}

void Sound::done(void)
{
  for(int i=0;i<sample_count;i++) sounds[i].free();
}

Sound::Sound(void)
{
  smp=NULL;
}

void Sound::play(Sample& _sample,int pn)
{
  smp=&_sample;
  if (!pn)
  {
    smp->play(0,VOLUME_MAX);
    return;
  }
  if (NoNet::is_network_mode())  netpl(smp,x,y);
  else   pl(smp,x,y);
}

void Sound::st_play(Sample* smp,int vol)
{
  if(vol&&MManager::is_speaking())
    smp->play(0,VOLUME_MAX*Mp::LOWVOICE/100);
  else
    smp->play(0,VOLUME_MAX);
}

int Sound::playing(int smp)
{
  DBG_CHECK(0<=smp);
  DBG_CHECK(sample_count>smp);
  return sounds[smp].playing();
}

void Sound::stopsmp(int smp)
{
  DBG_CHECK(0<=smp);
  DBG_CHECK(sample_count>smp);
  sounds[smp].stop();
}

void Sound::run(void)
{
}

void Sound::callback(unsigned user)
{
}
