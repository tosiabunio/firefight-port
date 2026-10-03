#include "headers.h"

Message MManager::table[Max_mmessage];
Message MManager::systable[MAX_SYSTEXT];
int     MManager::X;
int     MManager::Y;
MManager::Mission_msg  MManager::notext[MAX_NOTEXT];
int MManager::size;
int MManager::serv_message;
int MManager::last_smp;
int MManager::fls;
int MManager::syscount;
int MManager::first(0);
int MManager::objective(0);
int MManager::rand;
Sample *MManager::smp;
int MManager::smpflag;



void MManager::init(int _y)
{
  X=0;
  Y=10;
  smpflag=1;
  objective=first=syscount=fls=size=0;
  serv_message=-1;
  last_smp=-1;
  smp=&Mysound::szzzzend[0];
  for (int t=0;t<Max_mmessage;t++) table[t].time=0;
}


void MManager::flush(void)
{
  for(int i=1;i<Max_mmessage;i++) table[i].time=0;
  objective=0;
  smpflag=1;
  //if (smp) smp->stop();
}

void MManager::flush_notxt(void)
{
  flush();
  if (last_smp>=0) Sound::stopsmp(last_smp);
  fls=size=0;
  last_smp=serv_message=-1;
  objective=0;
  if (smp) smp->stop();
}

void MManager::add_objective(char* txt)
{
  if (is_speaking()) return;
  //flush_notxt();
  flush();
  char tmp1[1024];
  char tmp2[1024];
  char tmp3[1024];
  char tmp[1024];
  objective=1;
  Myship* ship=NoNet::get_ship(NoNet::get_this_id());
  int tx=Level::max_sx/20;
  int ty=Level::max_sy/26;
  int shp=100;
  int sec=100;
  if (Counter::start_global_ships)
    shp=((Counter::start_global_ships-Counter::global_ships)*100)/Counter::start_global_ships;
  if (Counter::start_global_secrets)
    sec=((Counter::start_global_secrets-Counter::global_secrets)*100)/Counter::start_global_secrets;

  sprintf(tmp,"%s",GameManager::level_descript.string("Missions_order",Pilot::get_level()));

  sprintf(tmp1,"*%d. %s (%s)",
    Pilot::get_level()+1,
    GAMETXT(tmp,0),
    Pilot::get_skill() ?"challenge" :"normal"
    );
  
  sprintf(tmp2,"*%s",txt);

  sprintf(tmp3,"*kills %d%%%%, secrets %d%%%%, sector %c%d",shp,sec,
    (char)(ship->get_y()/ty+'a'),
    ship->get_x()/tx+1
    );

  int _x1=(Mp::SX-Print::get_dx(Print::TSPRITE,tmp1))/2;
  int _x2=(Mp::SX-Print::get_dx(Print::TSPRITE,tmp2))/2;
  int _x3=(Mp::SX-Print::get_dx(Print::TSPRITE,tmp3))/2;

  int _y1=Op::OBJCTIVEDOWNJUST-((Print::get_dy(Print::TSPRITE,tmp1)+2)*3);
  int _y2=Op::OBJCTIVEDOWNJUST-((Print::get_dy(Print::TSPRITE,tmp2)+2)*2);
  int _y3=Op::OBJCTIVEDOWNJUST-((Print::get_dy(Print::TSPRITE,tmp3)+2));

  add_string(tmp1,Op::OBJCTIVETIME/2,_x1,_y1,0,-1);
  add_string(tmp2,Op::OBJCTIVETIME/2,_x2,_y2,0,-1);
  add_string(tmp3,Op::OBJCTIVETIME/2,_x3,_y3,0,-1);
}


void MManager::add_notext(Field* fld,int full)
{
  if (NoNet::is_network_mode()) return;
  flush();
  serv_message=-1;
  objective=0;
  if (last_smp>=0)
  {
    Sound::stopsmp(last_smp);
    last_smp=-1;
  }
  if ((Mp::LOADSAMPLE))
  {
    size=fld->text.textsize;
    DBG_CHECK(fld->text.textsize<MAX_NOTEXT);
    for (int i=0;i<fld->text.textsize;i++)
    {
      strcpy(notext[i].string,fld->text.text[i]);
      notext[i].sound=fld->text.sound[i];
      notext[i].kind=fld->text.kind[i];
      notext[i].full=full;
    }
  }
  else
  {
    size=1;
    strcpy(notext[0].string,GameManager::lastobj);
    notext[0].sound=-1;
    notext[0].kind=1;
    notext[0].full=1;
  }
}


void MManager::serv_mission_msg(void)
{
  int rnd=RAND%MAX_SZZZZ;
  if (NoNet::is_network_mode()) return;
  if (!first&&Fade::in_use()) return;
  else first=1;
  if (size>0)
  {
    if ((!Mp::LOADSAMPLE)&&!NoNet::is_network_mode())
    {
      flush();
      int fnt=10;
      int dx=0;
      char tmp[1024];
      DBG_CHECK(strlen(GAMETXT(notext[0].string,0))<99);
      sprintf(tmp,"*%s",GAMETXT(notext[0].string,0));
      dx=(Mp::SX-Print::get_dx(Print::TSPRITE,tmp))/2;
      add_string(tmp,Op::OBJCTIVETIME/4,dx,Op::OBJCTIVEDOWNJUST-GameManager::game_text.size(notext[0].string)-fnt,0,-1);
      size--;
    }
    else              
    {
      if (serv_message==-1||!Sound::playing(notext[serv_message].sound))
      {
        if (smpflag==2)
        {
           smp=&Mysound::szzzzend[rnd];
           Sound::st_play(smp);
           smpflag=0;
        } 
        else
        {
          if (smp&&!smp->playing())
          {
            flush();
            if (last_smp>=0) 
            {
              Sound::stopsmp(last_smp);
              last_smp=-1;
            }
            serv_message++;
            if (size>0)
            {
              fls=1;
              if (size-1>0&&notext[serv_message].kind==notext[serv_message+1].kind) 
                smpflag=0;
              else 
                smpflag=2;

              int fnt=10;
              int dx=0;
              char tmp[1024];

              int t=0;
              int hash=0;
              for(int i=0;i<GameManager::game_text.size(notext[serv_message].string);i++,t++)
              {
                int flg=0;
                DBG_CHECK(strlen(GAMETXT(notext[serv_message].string,i))<99);

                if (*(GAMETXT(notext[serv_message].string,i))=='#') flg=1;
                if (!notext[serv_message].full&&flg)
                {
                  hash=1;
                  break;
                }

                sprintf(tmp,"*%s",GAMETXT(notext[serv_message].string,i)+flg);
                dx=(Mp::SX-Print::get_dx(Print::TSPRITE,tmp))/2;
                int _w=Mysprites::persons[0].r+3;
                add_string(tmp,Op::OBJCTIVETIME,_w,(Op::OBJCTIVEDOWNJUST-GameManager::game_text.size(notext[serv_message].string)*fnt)+fnt*t,0,notext[serv_message].kind);
              }
              if (!hash)
              {
                last_smp=notext[serv_message].sound;
                Sound::mission_play(notext[serv_message].sound);
              }
            }
            else  serv_message=-1;
            size--;
          }
        }
      }
    }
  }
  else
  {
    serv_message=-1;
  }
  if (fls&&last_smp>=0&&!Sound::playing(last_smp))
  {
    flush();
    fls=0;
    smp=&Mysound::szzzzend[rnd];
    Sound::st_play(smp);
  }
}

int MManager::is_speaking(void)
{
  if (objective||!Mp::LOADSAMPLE) return 0;
  for (int i=1;i<Max_mmessage;i++)
    if (table[i].time>0) return 1;
  return 0;
}

void MManager::add_sysstring(char* txt,int id,int col)
{
  if (id!=NoNet::get_this_id()&&id!=0)  return;
  DBG_CHECK(strlen(txt)<250);
  strcpy(systable[syscount].string,txt);
  systable[syscount].time=5*Mp::METRONQUALITY;
  systable[syscount].kind=col;
  if ((++syscount)>=MAX_SYSTEXT) syscount=0;
}


void MManager::add_string(char* txt,int time,int _x,int _y,int type,int kind)
{
  DBG_CHECK(strlen(txt)<250);
  int numb;
  if (type)
  {
    numb=0;
  }
  else
  {
    int i,min=0,mnumb=1;
    for (i=1;i<Max_mmessage;i++)
    {
      if (!table[i].time)
      {
        numb=i;
        break;
      }
      if (min>table[i].time)
      {
        min=table[i].time;
        mnumb=i;
      }
    }
    if (i==Max_mmessage)
    {
      numb=mnumb;
    }
  }
  strcpy(table[numb].string,txt);
  table[numb].x=_x;
  table[numb].y=_y;
  table[numb].time=time;
  table[numb].kind=kind;
}


void MManager::display_string(Screen& screen,int x,int y,char *text,int col)
{
  DBG_CHECK(strlen(text)<256);
  char buf[256];
  strncpy(buf,text,256);
  buf[256-1]='\0';
  _strupr(buf);                            
  screen.ink=Color::black;
  screen.print(x+1,y,buf);
  switch (col)
  {
    case 0: screen.ink=(Color::white);break;
    case 1: screen.ink=(Tools::colors[Tools::NET_FIRST]);break;
    case 2: screen.ink=(Tools::colors[Tools::NET_FIRST+1]);break;
    case 3: screen.ink=(Tools::colors[Tools::NET_FIRST+2]);break;
    case 4: screen.ink=(Tools::colors[Tools::NET_FIRST+3]);break;
    default: DBG_CHECK(0);break;
  }
  screen.print(x,y,buf);
}


void MManager::display(Screen& screen)
{
  int ox=screen.ox;
  int oy=screen.oy;
  screen.origin(0,0);
  int t=syscount-1;
  int count=0;
  for (int j=0;j<MAX_SYSTEXT;j++)
  {
    if ((++t)>=MAX_SYSTEXT) t=0;
    if (systable[t].time>0)
    {
      int _x=Print::get_dx(Print::TFONT,systable[t].string);
      _x=Mp::SX/2-_x/2;
      int _y=15+(Print::get_dy(Print::TFONT,systable[t].string)+2)*count;
//      int _y=15+10*count;
      display_string(screen,_x,_y,systable[t].string,systable[t].kind);
      count++;
    }
  }

  int _x=1;
  int _y=160;
  if ((!objective&&Mp::LOADSAMPLE)&&!NoNet::is_network_mode())
  {
    if (table[1].time>0)                                  
    {                        
      int phs=0;
      if (table[1].time>Op::OBJCTIVETIME-200) phs=2;
      else if (table[1].time>Op::OBJCTIVETIME-400) phs=1;
      else 
      {
        if ((rand%15)<1) phs=1;
        else phs=0;
      }

      switch(table[1].kind)
      {                                     
        case Field::STACY: screen.shape(_x+Print::sh_offset_x,_y+Print::sh_offset_y,Mysprites::persons,2,Spr::with_tool,(void*)&Print::tools[Print::_FLAST]);
                           screen.put(_x,_y,Mysprites::persons,2*3+phs);
                  break;
        case Field::RILEY: screen.shape(_x+Print::sh_offset_x,_y+Print::sh_offset_y,Mysprites::persons,1,Spr::with_tool,(void*)&Print::tools[Print::_FLAST]);
                           screen.put(_x,_y,Mysprites::persons,1*3+phs);
                  break;
        case Field::JAXON: screen.shape(_x+Print::sh_offset_x,_y+Print::sh_offset_y,Mysprites::persons,0,Spr::with_tool,(void*)&Print::tools[Print::_FLAST]);
                           screen.put(_x,_y,Mysprites::persons,0*3+phs);
                  break;
        case Field::SHADOW:screen.shape(_x+Print::sh_offset_x,_y+Print::sh_offset_y,Mysprites::persons,3,Spr::with_tool,(void*)&Print::tools[Print::_FLAST]);
                           screen.put(_x,_y,Mysprites::persons,3*3+phs);
                  break;
      }
    }
  }
  for (int i=1;i<Max_mmessage;i++)
  {
    if (table[i].time>0)
    {
      switch(table[i].kind)
      {
        case -1:             Print::print(screen,Print::TSPRITE,Print::FWARNING,table[i].x,table[i].y,table[i].string);
                  break;
        case Field::RILEY:   Print::print(screen,Print::TSPRITE,Print::FRILEY,table[i].x,table[i].y,table[i].string);
                  break;
        case Field::STACY:   Print::print(screen,Print::TSPRITE,Print::FSTACY,table[i].x,table[i].y,table[i].string);
                  break;
        case Field::JAXON:   Print::print(screen,Print::TSPRITE,Print::FJAXON,table[i].x,table[i].y,table[i].string);
                  break;
        case Field::SHADOW:  Print::print(screen,Print::TSPRITE,Print::FSHADOW,table[i].x,table[i].y,table[i].string);
                  break;
      }
    }
  }
  screen.origin(ox,oy);
}

void MManager::run(void)
{
  if (gamemanager->get_restart()) flush_notxt();
  for (int i=0;i<Max_mmessage;i++)
  {
    table[i].time-=KbdStat::time();
    if (table[i].time<0) table[i].time=0;
  }

  for (i=0;i<MAX_SYSTEXT;i++)
  {
    systable[i].time-=KbdStat::time();
    if (systable[i].time<0) systable[i].time=0;
  }
  rand=RAND;
  serv_mission_msg();
}

// SMANAGER
int      SManager::count;
int      SManager::head;
int      SManager::end;
int      SManager::last_playing;
int      SManager::blocked(0);

Sample*  SManager::stable[MAX_SMSAMPLE];


void SManager::init(void)
{
  memset(stable,0,sizeof(stable));
  end=count=head=0;
  last_playing=-1;
  blocked=0;
}

void SManager::run(void)
{
  if (last_playing!=-1)
  {
    if (stable[head]&&!stable[head]->playing()) 
    {
      last_playing=-1;
      stable[head]=NULL;
      if (++head>=MAX_SMSAMPLE) head=0;
      count--;
    }
  }

  if (last_playing==-1)
  {
    if (count>0&&stable[head])
    {
      Sound::st_play(stable[head],1);
      last_playing=head;
    }
  }
}


void SManager::add_sample(Sample* s1,Sample* s2,Sample* s3,Sample* s4,Sample* s5)
{
  if (blocked) return;
  if (count<MAX_SMSAMPLE&&s1)
  {
    stable[end]=s1;
    count++;
    if (++end>=MAX_SMSAMPLE) end=0;
  }

  if (count<MAX_SMSAMPLE&&s2)
  {
    stable[end]=s2;
    count++;
    if (++end>=MAX_SMSAMPLE) end=0;
  }
  
  if (count<MAX_SMSAMPLE&&s3)
  {
    stable[end]=s3;
    count++;
    if (++end>=MAX_SMSAMPLE) end=0;
  }
  if (count<MAX_SMSAMPLE&&s4)
  {
    stable[end]=s4;
    count++;
    if (++end>=MAX_SMSAMPLE) end=0;
  }
  if (count<MAX_SMSAMPLE&&s5)
  {
    stable[end]=s5;
    count++;
    if (++end>=MAX_SMSAMPLE) end=0;
  }
}

int  SManager::playing(void)
{
  return count;
}


void SManager::flush(void)
{
  if (last_playing!=-1)  
  {
    count=1;
    if (head>=MAX_SMSAMPLE-1) end=0;
    else end=head+1;
  }
  else
  {
    count=0;
    end=head;
  }
}
