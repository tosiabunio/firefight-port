#include "headers.h"

//KLASA GAMEMANAGER
char GameManager::volume[100];
int  GameManager::volume_number;
int  GameManager::sublevel_number;
int  GameManager::quit;
int  GameManager::quit_timer;
int  GameManager::restart;
int  GameManager::mission_object;
int  GameManager::textloaded(0);
int  GameManager::testing(0);
int  GameManager::all_ships(0);
int  GameManager::last_field(-1);


List<Field,int,1> *GameManager::necessary;
List<Field,int,1> *GameManager::bonus;

Text GameManager::weather_text;
Text GameManager::level_descript;
Text *GameManager::mission_text;

Text GameManager::missions[30];
Text GameManager::types[30];
Text GameManager::net_missions[10];
Text GameManager::net_types[10];
Text GameManager::game_text;

int  GameManager::text_count;
int  GameManager::text_num(0);
int  GameManager::fadeing;
char GameManager::lastobj[40];



GameManager::GameManager(int _net_mode):
Handle(GMANAGER)
{
  typedef List<Field,int,1> _LIST;
  necessary=NEW(_LIST("GameManager::necessary"),list_mbn);
  bonus=NEW(_LIST("GameManager::bonus"),list_mbn);
  net_mode=_net_mode;
  text_num=0;
}

//
GameManager::~GameManager(void)
{
  clean_list(necessary);
  delete necessary;
  clean_list(bonus);
  delete bonus;
}
//
void GameManager::start(void)
{
  char mis[1024],type[1024];
  weather_text.load("weather");
  level_descript.load("level_descript");
  game_text.load("game_text");

  int i;
  for (i=0;i<level_descript.size("Missions_order");i++)
  {
    sprintf(mis,"%s_MISSION",level_descript.string("Missions_order",i));
    sprintf(type,"%s_TYPES",level_descript.string("Missions_order",i));

    missions[i].load(mis);
    types[i].load(type);
  }

  for (i=0;i<level_descript.size("Net_order");i++)
  {
    sprintf(mis,"%s_MISSION",level_descript.string("Net_order",i));
    sprintf(type,"%s_TYPES",level_descript.string("Net_order",i));

    net_missions[i].load(mis);
    net_types[i].load(type);
  }
  textloaded=1;
}
//
void GameManager::stop(void)
{
  if (!textloaded) return;
  int i;
  for (i=0;i<level_descript.size("Missions_order");i++)
  {
    missions[i].free();
    types[i].free();
  }

  for (i=0;i<level_descript.size("Net_order");i++)
  {
    net_missions[i].free();
    net_types[i].free();
  }
  weather_text.free();
  level_descript.free();
}
//
void GameManager::reload_text(void)
{
  char mis[1024];
  char type[1024];
  if (!net_mode)
  {
    sprintf(mis,"%s_MISSION",level_descript.string("missions_order",Pilot::get_level()));
    sprintf(type,"%s_TYPES",level_descript.string("missions_order",Pilot::get_level()));
    missions[Pilot::get_level()].free();
    missions[Pilot::get_level()].load(mis);
    types[Pilot::get_level()].free();
    types[Pilot::get_level()].load(type);
  }
  else
  {
    sprintf(mis,"%s_MISSION",level_descript.string("Net_order",Pilot::get_level()));
    sprintf(type,"%s_TYPES",level_descript.string("Net_order",Pilot::get_level()));
    net_missions[Pilot::get_level()].free();
    net_missions[Pilot::get_level()].load(mis);
    net_types[Pilot::get_level()].free();
    net_types[Pilot::get_level()].load(type);
  }
}
//
void GameManager::update_params(int net_mode)
{
  prologue();

  char hej[1024];
  if (!net_mode)
  {
    sprintf(hej,level_descript.string("missions_order",Pilot::get_level()));
  }
  else
  {
    DBG_CHECK(Pilot::get_level()<=level_descript.size("net_order"));
    sprintf(hej,"%s",level_descript.string("net_order",Pilot::get_level()));
  }
  mission_object=0;
  char hej2[1024];
  strcpy(hej2,hej);
  hej2[strlen(hej2)-1]='\0';
  strcpy(volume,hej2);

  sublevel_number=hej[strlen(hej)-1]-'0'-1;
}
//
void GameManager::load_level(void)
{
  clean_up();
  strcpy(type,volume);

  if (!net_mode)
  {
    mission_text=&(missions[Pilot::get_level()]);
    LevImp::types_text=&(types[Pilot::get_level()]);
  }
  else
  {
    mission_text=&(net_missions[Pilot::get_level()]);
    LevImp::types_text=&(net_types[Pilot::get_level()]);
  }



  dx=(*mission_text).value("dx");
  dy=(*mission_text).value("dy");
  strcpy(weather_type,(*mission_text).string("weather"));
  text_num=0;
}
//
void GameManager::load_events(void)
{
  mission_object=1;
  load(necessary,"CheckPoints",*mission_text);
  if ((*mission_text).exist("Bonus")) load(bonus,"Bonus",*mission_text);
  mission_object=0;
  text_num=0;
}
//
void GameManager::init_stuff(void)
{
  if (!NoNet::is_network_mode(NoNet::NETWORKFLAG))
  {
    Field *temp=necessary->reset();
    if (temp)
    {
      //TEXT
      if ((temp->text.textsize||!Mp::LOADSAMPLE)&&!NoNet::is_network_mode())
      {
        strcpy(lastobj,temp->text.textobj);
        MManager::add_notext(temp,1);
      }
      //SOUNDS
      if (temp->sample!=-1)
      {
        Sound::play(temp->sample);
        temp->sample=-1;
      }
      //RADAR
      if (temp->radar)  Radar::set_cur_type(temp->type,temp->tsize);
      else Radar::set_cur_type(0,0);
    }
    if (!(*mission_text).exist("radar")) Radar::radarON=1;
    else Radar::radarON=(*mission_text).value("radar");
  }
  else
  {
    Radar::radarON=1;
  }
  fadeing=0;
  quit_timer=0;
  last_field=-1;
  restart=quit=0;

}
//
void GameManager::weather(Level* level)
{
  if (!strncmp("smoke",weather_type,strlen("smoke")))
  {
    NEW(Fog(weather_type,level),gobjects_mbn);

  }
  else if(!strcmpi("night",weather_type)||!strcmpi("fog",weather_type))
  {
    NEW(Night(weather_type),gobjects_mbn);
  }
  else if (!strcmpi("snow1",weather_type))
  {
    NEW(Snow(0),gobjects_mbn);
  }
  else if (!strcmpi("snow2",weather_type))
  {
    NEW(Snow(1),gobjects_mbn);
  }
  else if (!strcmpi("snow3",weather_type))
  {
    NEW(Snow(2),gobjects_mbn);
  }
  else if (!strcmpi("snow4",weather_type))
  {
    NEW(Snow(3),gobjects_mbn);
  }
  else if (!strcmpi("rain1",weather_type))
  {
    NEW(Rain(0),gobjects_mbn);
  }
  else if (!strcmpi("rain2",weather_type))
  {
    NEW(Rain(1),gobjects_mbn);
  }
  else if (!strcmpi("rain3",weather_type))
  {
    NEW(Rain(2),gobjects_mbn);
  }
  else if (!strcmpi("rain4",weather_type))
  {
    NEW(Rain(3),gobjects_mbn);
  }
  else if (!strncmp("clouds",weather_type,strlen("clouds")))
  {
    NEW(Cloud(weather_type,level),gobjects_mbn);
  }
  else if (strncmp("none",weather_type,strlen("none")))
  {
    FAILURE("Nie rozpoznany typ pogogy misji");
  }
}
//
void GameManager::clean_list(List<Field,int,1> *list)
{
  Field *temp=(Field*)list->reset();
  while (temp!=NULL)
  {
    list->remove(temp);
    if (temp)
    {
      temp->clean();
      delete temp;
      temp=(Field*)list->get_next();
    }
  }
  DBG_CHECK(list->size()==0);
}
//
void GameManager::clean_up(void)
{
  clean_list(necessary);
  clean_list(bonus);
  all_ships=0;
}
//
int GameManager::end_level(void)
{
  Field* temp;
  temp=necessary->reset();
  if (!temp)
  {
    DBG_MESSAGE("Misja ukonczona");
    if (!testing)
    {
      Counter::update_registry();
      Pilot::reckon_level(Pilot::get_level());
      int num=GameManager::level_descript.size("Missions_order");
      int i;
      for(i=0;i<num;i++)
      {
        if (!Pilot::level_status(i)) break;
      }
      if (i==num&&(Pilot::get_level()+1)==num) 
      {
        //Januszku!!!
        //W tym miejscu sa grane zakonczenie (z napisami jak w kinie) i/lub (w zaleznosci od wersji)
        //ordering information. Header::footer() to zakonczenie z napisami, natomiast Header::ordering()
        //to (jak sama nazwa wskazuje) ordering info. Wykomentuj ta ktora chcesz (np. obydwie?!),
        //zmieni ich kolejnosc, itp., itd. Jezeli wszystko jest O.K. (jaj se nie daje uciac - przerabiam
        //Grzesiowy kod) to nie powinno byc zadnych problemow...
#ifndef SHAREWARE        
        Header::footer();
#else
        Header::footer();
        Header::ordering();
#endif
      }
    }
    clean_up();
    return 0;
  }
  else
  {
    DBG_MESSAGE("Misja jeszcze nie ukonczona");
    clean_up();
    return 1;
  }
}
//
void GameManager::type_event(Field* ptr)
{
  if (!ptr->chtype) return;
  ChType *temp=ptr->chtype;
  for (int i=0;i<ptr->size;i++,temp++)
  {
    switch (temp->what)
    {
      case DISP_TYPE    : LevImp::get_parameters(temp->type)->disp_type=temp->count; break;
      case SERV_TYPE    : if(LevImp::get_parameters(temp->type)->colis_type!=4)
                          {
                            LevImp::builders[temp->count]->bregister(world->get_level().type[temp->type],world->get_level());break;
                          }
                          LevImp::get_parameters(temp->type)->serv_type=temp->count;break;

      case COLIS_TYPE   : LevImp::get_parameters(temp->type)->colis_type=temp->count;break;
      case FRAME_DELAY  : LevImp::get_parameters(temp->type)->frame_delay=temp->count; break;
      case TIME_DELAY   : LevImp::get_parameters(temp->type)->time_delay=temp->count; break;
      case REPEAT_COUNT : LevImp::get_parameters(temp->type)->repeat_count=temp->count; break;
      case SYNC_FRAME   : LevImp::get_parameters(temp->type)->sync_frame=temp->count; break;
      case CHANGE_TYPE  : LevImp::get_parameters(temp->type)->change_type=temp->count; break;
      case BLOCKED      : LevImp::get_parameters(temp->type)->blocked=temp->count; break;
#if HI_DEBUG
      default:FAILURE ("GameManager::type_event - something wrong with parameters.");
#endif
    }
  }
}
//
void GameManager::handle_event(Field* ptr)
{
  if (!ptr->handle) return;
  ChType *temp=ptr->handle;
  for (int i=0;i<ptr->hsize;i++,temp++)
  {
    DBG_MESSAGE("post message '%s' to '%d'",world->get_level().type[temp->what],temp->type);
    Handle::post(temp->type,temp->what);
  }
}
//
int GameManager::check_cur_type(int type)
{
  Field *tmp=necessary->reset();
  if (!tmp) return 0;
  int *t=tmp->type;
  for(int i=0;t&&i<tmp->tsize;i++,t++)
  {
    if(type==*t) return 1;
  }
  return 0;
}
//
int GameManager::check_message(int type,Field* ptr)
{
  int *tmp=ptr->type;
  for(int i=0;i<ptr->tsize;i++,tmp++)
  {
    if (type==*tmp) return 1;
  }
  return 0;
}
//
void GameManager::HandleEvent(int message)
{
  if (NoNet::is_network_mode(NoNet::NETWORKFLAG)) return;
  Field *temp=necessary->reset();
  int i=0;
  int curr_num=0;
  for (temp=necessary->reset();temp;temp=necessary->get_next(),i++)
  {
    if(check_message(message,temp)) //to ten event
    {
      temp->count--;
      DBG_MESSAGE("CheckPoint  event's count has been decreased,actual c=%d",temp->count);
      if (temp->count<=0)   //teraz event spelniony
      {
        if (temp->quit)
        {
          if (necessary->size()==1)  quit=1;
          else  return;
        }
        curr_num=temp->num;
        if (temp->set_radar!=-1) Radar::radarON=temp->set_radar;
        type_event(temp);
        handle_event(temp);
        if (temp->next) active_action(temp);
        if (temp->quit_timer) Countdown::off();
        if (temp->timer) NEW(Countdown(temp->timer),gamemanager_mbn);
        if (temp->rebel)  ShipFucker::start();
        if (temp->quit_rebel)  ShipFucker::stop();
        //SOUNDS
        if (temp->sample!=-1)
        {
          Sound::play(temp->sample);
          temp->sample=-1;
        }

        if (temp->shake_screen)
        {
          FOR_ALL_USERS(ID)
          {
            Myship* ship=NoNet::get_ship(ID);
            ship->shake_screen(2*Mp::METRONQUALITY);
          }
        }
        necessary->remove(temp);
        temp->clean();
        delete temp;
        temp=NULL;

#if HI_DEBUG
        DBG_MESSAGE("event has been completed");
        if ((temp=necessary->reset()))
        {
          DBG_MESSAGE("there are %d check points to complete:",necessary->size());
          DBG_MESSAGE("count=%d",temp->count);
        }
        else
        {
          DBG_MESSAGE("all check points have been completed");
        }
#endif
        temp=necessary->reset();
        if (temp)
        {
          if (i==0)
          {
            if (temp->radar) Radar::set_cur_type(temp->type,temp->tsize);
            else Radar::set_cur_type(0,0);
            if ((temp->text.textsize||!Mp::LOADSAMPLE)&&!NoNet::is_network_mode())
            {
              int last=0;
              if (last_field==curr_num-1) last=1;
              strcpy(lastobj,temp->text.textobj);
              MManager::add_notext(temp,last);
            }
          }
        }
        else
        {
          Radar::set_cur_type(0,0);
        }
        last_field=curr_num;
      }
      return;
    }
  }


  for (temp=bonus->reset();temp;temp=bonus->get_next())
  {
    if(check_message(message,temp)) //to ten event
    {
      temp->count--;
      DBG_MESSAGE("Bonus event's count has been decreased,actual c=%d",temp->count);
      if (temp->count<=0)   //teraz event spelniony
      {
        if (temp->text.textsize)   MManager::add_notext(temp,1);

        if (temp->quit)  quit=1;
        if (temp->quit_timer) Countdown::off();
        if (temp->timer) NEW(Countdown(temp->timer),gamemanager_mbn);
        if (temp->rebel)  ShipFucker::start();
        if (temp->quit_rebel)  ShipFucker::stop();
        if (temp->sample!=-1)
        {
          Sound::play(temp->sample);
          temp->sample=-1;
        }

        if (temp->shake_screen)
        {
          FOR_ALL_USERS(ID)
          {
            Myship* ship=NoNet::get_ship(ID);
            ship->shake_screen(2*Mp::METRONQUALITY);
          }
        }

        if (temp->set_radar!=-1) Radar::radarON=temp->set_radar;

        type_event(temp);
        handle_event(temp);
        active_action(temp);

        if (temp->del)
        {
          temp->clean();
          bonus->remove(temp);
          delete temp;
          temp=NULL;
          DBG_MESSAGE("Bonus Event has been completed");
        }
        else
        {
          temp->count=temp->init_count;
          DBG_MESSAGE("Bonus Event has been completed,but no deleted");
        }
      }
      break;
    }
  }
  if (quit==1&&necessary->reset())
  {
    set_restart(GameManager::KILL);
    quit=0;
  }

}
//
int GameManager::compute_count(Field* ptr)
{
  int sum=0,*tmp=ptr->type;
  for(int i=0;i<ptr->tsize;i++,tmp++)
  {
    sum+=LevImp::get_parameters(*tmp)->count;
  }
  return sum;
}
//
void GameManager::load(List<Field,int,1> *list,char* section,Text& text)
{
  DBG_MESSAGE("%s",section);
  char* tmp,*field;
  if (!text.exist(section))
  {
#if HI_DEBUG
    FAILURE("GameManager nie ma listy CheckPoint'ow",section);
#endif
  }
  int hej=0;
  for(int i=0;i<text.size(section);i++)
  {
    field=text.string(section,i);

    //teraz wczytujemy poszczegolne pola
    Field *temp=NEW(Field,gamemanager_mbn);
    text.area(field);

    temp->num=i;
    if (text.exist("type"))
    {
      temp->tsize=text.size("type");
      temp->type=(int*)Heap::alloc(temp->tsize*sizeof(int),"Gamemanager::field");
      int* ptr=temp->type;
      for(int i=0;i<temp->tsize;i++,ptr++)
      {
        tmp=text.string("type",i);
        if (!(*ptr=world->get_level().typenum(tmp)))
        {
#if HI_DEBUG
          FAILURE("GameManager::load - there is no '%s' in level.",tmp);
#endif
        }
        DBG_MESSAGE("type '%s' loaded in event",tmp);
      }
    }
#if HI_DEBUG
    else   FAILURE("GameManager::load - no label 'type' in area '%s'.",field);
#endif

    if (text.exist("count"))
    {
      if (!strcmpi(text.string("count"),"all"))  temp->count=compute_count(temp);
      else   temp->init_count=temp->count=text.value("count");
      DBG_MESSAGE("event count =%d",(int)temp->count);
    }
#if HI_DEBUG
    else  FAILURE("GameManager::load - no label 'count' in mission definition.");
#endif
    //radar
    if(text.exist("radar"))  temp->radar=1;
    else temp->radar=0;

    if(text.exist("set_radar")) temp->set_radar=text.value("set_radar");
    else temp->set_radar=-1;

    if(text.exist("nodelete"))  temp->del=0;
    else temp->del=1;

    if(text.exist("shake_screen"))  temp->shake_screen=1;
    else temp->shake_screen=0;
    //Action
    if(text.exist("QUIT"))
    {
      temp->quit=text.value("QUIT");
    }
    else temp->quit=0;

    if (!strcmpi(section,"CheckPoints"))
    {
      load_action(text,temp,1);
    }
    else
      load_action(text,temp,0);

    load_chtype(text,temp);
    load_handle(text,temp);
    read_action(temp,&text,field);
    if (text.exist("sample")) temp->sample=Sound::load_sound(text.string("sample"));
    else temp->sample=-1;


    list->add_end(temp,0);
    text.endarea();
    hej=i+1;

    if (!strcmpi(section,"CheckPoints"))
    {
      active_build(temp->type,temp->tsize);
    }
  }
  DBG_MESSAGE("read %d events from %s",hej,section);
}
//
void GameManager::active_action(Field* fld)
{
  if (fld->enableaction)
  {
    int* ptr=fld->enableaction;
    for(int i=0;i<fld->enablesize;i++,ptr++)
    {
      Lobject *o=&world->get_level()[*ptr];
      o->active=1;
      if (LevImp::get_parameters(o->type)->builder)
        world->get_level().build(*ptr,world->get_level().olevel(*ptr),world->get_level().oplane(*ptr));
      world->get_level().update(*ptr);
    }
  }

  if (fld->disableaction)
  {
    int* ptr=fld->disableaction;
    for(int i=0;i<fld->disablesize;i++,ptr++)
    {
      world->get_level()[*ptr].active=0;
      world->get_level().update(*ptr);
    }
  }
}
//
void GameManager::read_actionsection(int** action,int* size,Text* text,char *txt,int en)
{
  int count=0;
  int i;
  for (i=0;i<(*text).size(txt);i++)
  {
    int num=world->get_level().typenum((*text).string(txt,i));
    if (!num) DBG_FAILURE("<%s> nie ma obiektu %s na pozimie",txt,(*text).string(txt,i));
    count+=LevImp::parameters[num].count;
    DBG_MESSAGE("Na event uaktywniam %d obiektow %s",count,(*text).string(txt,i));
  }
  int *ptr=(*action)=(int*)Heap::alloc(sizeof(int)*count,"field->action");
  *size=count;
  int amount=0;

  for (i=0;i<(*text).size(txt);i++)
  {
    int type=world->get_level().typenum((*text).string(txt,i));
    Lobject* o;
    for (int j=1;j<world->get_level().size;j++)
    {
      o=&world->get_level()[j];
      if (o->type==type&&o->status!=(short)CONVERTED&&(!(!Pilot::get_skill()&&o->special)))
      {
        if (en)
        {
          o->active=0;
          world->get_level().update(j);
        }
        if (LevImp::action[j]&&!Comm::production) WARNING("obiekt <%s,%d,x=%d,y=%d> uaktywniany na wiele sposobow",
                                             world->get_level().type[world->get_level()[j].type],
                                             j,world->get_level()[j].x,world->get_level()[j].y);
        LevImp::action[j]=1;

        amount++;
        *ptr=j;
        ptr++;
      }
    }
  }
  DBG_CHECK(amount==count);
}
//
void GameManager::read_action(Field* fld,Text* text,char *area)
{
  (void) area;
  if ((*text).exist("enable_action") )
    read_actionsection(&fld->enableaction,&fld->enablesize,text,"enable_action",1);
  else
  {
    fld->enableaction=NULL;
    fld->enablesize=0;
  }

  if ((*text).exist("disable_action") )
    read_actionsection(&fld->disableaction,&fld->disablesize,text,"disable_action",0);
  else
  {
    fld->disableaction=NULL;
    fld->disablesize=0;
  }
}
//
void GameManager::active_build(int* ptr,int size)
{
  int i=0,type,*tmp=ptr;
  char hej2[100],hej[100];

  for (int b=0;b<size;b++,tmp++)
  {
    type=*tmp;
    strcpy(hej2,world->get_level().type[type]);
    if (!LevImp::get_parameters(type)->builder) continue;
    for (int j=1;j<world->get_level().size;j++)
    {
      strcpy(hej,world->get_level().type[world->get_level()[j].type]);
      if (world->get_level()[j].type==type&&world->get_level()[j].active)
      {
        world->get_level().build(j,world->get_level().olevel(j),world->get_level().oplane(j));
        i++;
      }
    }
    DBG_MESSAGE("%d active builders from game manager object <%s>",i,world->get_level().type[type]);
  }
}
//
int GameManager::code(int type,int event)
{
  (void)event;
  return type;
}
//
void GameManager::load_action(Text &text,Field* temp,int check_point)
{
  if (check_point&&!NoNet::is_network_mode())
  {
    if (text.exist("OBJ_TEXT")) 
    {
      strcpy(temp->text.textobj,text.string("OBJ_TEXT"));
      if (!GameManager::game_text.exist(temp->text.textobj))
        FAILURE("Nie ma etykiety %s w text.txt",temp->text.textobj);
    }
    else FAILURE("Nie ma etykiety OBJ_TEXT event'cie");
  }
  if(text.exist("DISPL_TEXT")&&Mp::LOADSAMPLE)
  {
    int textsize=text.size("DISPL_TEXT");
    DBG_CHECK(textsize/3<=MAX_FIELDTEXT);
    temp->text.textsize=textsize/3;
    char *txt;
    char *sound;
    char *kind;
    int j=0;
    for (int i=0;i<textsize;i+=3,j++)
    {
      txt=text.string("DISPL_TEXT",i);
      if (!GameManager::game_text.exist(txt))
        FAILURE("Nie ma etykiety %s w text.txt",txt);
      sound=text.string("DISPL_TEXT",i+1);
      kind=text.string("DISPL_TEXT",i+2);
      strcpy(temp->text.text[j],txt);
      temp->text.sound[j]=(char)Sound::load_sound(sound);
      if (!strcmpi(kind,"JAXON")) temp->text.kind[j]=Field::JAXON;
      else if (!strcmpi(kind,"RILEY")) temp->text.kind[j]=Field::RILEY;
      else if (!strcmpi(kind,"STACY")) temp->text.kind[j]=Field::STACY;
      else if (!strcmpi(kind,"SHADOW")) temp->text.kind[j]=Field::SHADOW;
      else
        FAILURE("Invalid <DISPL_TEXT>");
    }
  }
  else temp->text.textsize=0;

  if (text.exist("TIMER")) temp->timer=(text.value("TIMER"))*Mp::METRONQUALITY;
  else temp->timer=0;

  if (text.exist("REBEL")) temp->rebel=1;
  else temp->rebel=0;

  if (text.exist("REBEL_QUIT")) temp->quit_rebel=1;
  else temp->quit_rebel=0;


  if (text.exist("QUIT_TIMER")) temp->quit_timer=text.value("QUIT_TIMER");
  else temp->quit_timer=0;

  if (text.exist("PHOTO")) temp->photo=text.value("PHOTO");
  else   temp->photo=0;
}
//
void GameManager::load_handle(Text &text,Field* temp)
{
  if (text.exist("handle"))
  {
    temp->hsize=text.size("handle");

    ChType* our=temp->handle=(ChType*)Heap::alloc(temp->hsize*sizeof(ChType),gamemanager_mbn);

    for (int i=0;i<temp->hsize;i++,our++)
    {
      char message[80],ID[80];
      char *my=text.string("handle",i);
      sscanf(text.string("handle",i),"%s %s",ID,message);
      our->what=world->get_level().typenum(message);
      if (!our->what)
        FAILURE("GameManager::load_handle - wrong level type '%s'",message);


      if (!strcmpi(ID,"DOOR"))
      {
        our->type=HDOORS;
      }
      else if (!strcmpi(ID,"DESTROY_OBJ"))
      {
        active_build(&our->what,1);
        our->type=DESTROY_OBJ;
      }
      else
      {
        FAILURE("GameManager::load_handle - wrong ID type '%s'",ID);
      }

      our->count=-1;
    }
  }
  else temp->handle=NULL;
}
//
void GameManager::load_chtype(Text &text,Field* temp)
{
  if (text.exist("CH_TYPE"))
  {
    temp->size=text.size("CH_TYPE");

    ChType* our=temp->chtype=(ChType*)Heap::alloc(temp->size*sizeof(ChType),gamemanager_mbn);

    for (int i=0;i<temp->size;i++,our++)
    {
      int t=0;
      char type[80],what[80];
      int count;
      char *my=text.string("CH_TYPE",i);
      sscanf(text.string("CH_TYPE",i),"%s %s %d",type,what,&count);

      if (!(our->type=world->get_level().typenum(type)))
      {
        FAILURE("GameManager::load - there is no '%s' type in level.",type);
      }
      our->count=count;

      if (!strcmpi(what,"DISP_TYPE"))
      {
        our->what=DISP_TYPE;
        t++;
        continue;
      }

      if (!strcmpi(what,"SERV_TYPE"))
      {
        our->what=SERV_TYPE;
        t++;
        continue;
      }

      if (!strcmpi(what,"BLOCKED"))
      {
        our->what=BLOCKED;
        t++;
        continue;
      }

      if (!strcmpi(what,"COLIS_TYPE"))
      {
        our->what=COLIS_TYPE;
        t++;
        continue;
      }

      if (!strcmpi(what,"FRAME_DELAY"))
      {
        our->what=FRAME_DELAY;
        t++;
        continue;
      }

      if (!strcmpi(what,"TIME_DELAY"))
      {
        our->what=TIME_DELAY;
        t++;
        continue;
      }

      if (!strcmpi(what,"REPEAT_COUNT"))
      {
        our->what=REPEAT_COUNT;
        t++;
        continue;
      }

      if (!strcmpi(what,"SYNC_FRAME"))
      {
        our->what=SYNC_FRAME;
        t++;
        continue;
      }
      if (!strcmpi(what,"CHANGE_TYPE"))
      {
        our->what=CHANGE_TYPE;
        t++;
        continue;
      }

#if HI_DEBUG
      if (!t)
      {
        FAILURE ("GameManager::load - invalid CH_TYPE parameter.");
      }
#endif
    }
  }
  else temp->chtype=NULL;
}
//NEXT
void GameManager::next_level(void)
{
  clean_list(necessary);
  clean_list(bonus);
  int levels_numb=GameManager::level_descript.size("Missions_order");
  int curr_level=Pilot::get_level();
  if (curr_level<levels_numb-1)
  {
    Pilot::set_level(curr_level+1);
  }
  else
  {
    if (Mp::BUILDSPRITESMODE)
    {
      MESSAGE("BUILD_SPRITES last level processed successfully");
      throw TerminateGame();
    }
    else
    {
      Header::footer();
    }
  }
}
//poczatek
void GameManager::prologue(void)
{
}
//
void GameManager::slow_down(void)
{
}
const int MAX_BASE=4;
//
void GameManager::check_restart(void)
{
  int t=0;
  if (NoNet::is_network_mode(NoNet::NETWORKMATCH))
  {
    if (NoNet::end_condition()==NoNet::FRAGS)
    {
      if (!fadeing)
      {
        FOR_ALL_USERS(id)
        {
          if (Statistics::get(id)>=NoNet::max_frags())
          {
            SManager::flush();
            char buf[1024];
            Myship *s=NoNet::get_ship(world->get_current_displayed());
            if (world->get_current_displayed()==id)
            {
              s->play_ship_sample(Mysound::speech_youreachfrag,Myship::SPEECH);
              sprintf(buf,"%s",GAMETXT("YOU_NET_END_FRAGS"));
            }
            else
            {
              s->play_ship_sample(Mysound::speech_colours[id],Myship::SPEECH);
              s->play_ship_sample(Mysound::speech_reachfrag,Myship::SPEECH);
              sprintf(buf,"%s %s",NoNet::get_player_color(id),GAMETXT("NET_END_FRAGS"));
            }
            SManager::lock();
            Fade::start(Fade::ONLYIN,buf);
            quit_timer=3*Mp::METRONQUALITY;
            fadeing=1;
            break;
          }
        }
      }
      else 
      {
        t=Fade::is_faded();
        quit_timer-=KbdStat::time();
      }
      if (t&&quit_timer<=0) 
      {
        throw TerminateMission();
      }
    }
    else if (NoNet::end_condition()==NoNet::TIMEUP)
    {
      if (!fadeing)
      {
        if (Game::get_timer()>=NoNet::game_time()*Mp::METRONQUALITY*60)
        {
          SManager::flush();
          Myship *s=NoNet::get_ship(world->get_current_displayed());
          s->play_ship_sample(Mysound::speech_timeisup,Myship::SPEECH);
          SManager::lock();
          Fade::start(Fade::ONLYIN,GAMETXT("NET_TIMEUP"));
          quit_timer=2*Mp::METRONQUALITY;
          fadeing=1;
        }
      }
      else 
      {
        t=Fade::is_faded();
        quit_timer-=KbdStat::time();
      }
      if (t&&quit_timer<=0) 
      {
        throw TerminateMission();
      }
    }
  }
  else if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
  {
    if (!fadeing)
    {
      FOR_ALL_USERS(id)
      {
        if (Statistics::get(id)>=Op::MAXSHIPBRICK&&NoNet::get_ship(id)->get_landed())
        {
          SManager::flush();
          char buf[1024];
          Myship *s=NoNet::get_ship(world->get_current_displayed());
          if (world->get_current_displayed()==id)
          {
            s->play_ship_sample(Mysound::speech_you,Myship::SPEECH);
            s->play_ship_sample(Mysound::speech_are,Myship::SPEECH);
            s->play_ship_sample(Mysound::speech_thewinner,Myship::SPEECH);
            sprintf(buf,"%s",GAMETXT("YOU_NET_ENDBASEB"));
          }
          else
          {
            s->play_ship_sample(Mysound::speech_colours[id],Myship::SPEECH);
            s->play_ship_sample(Mysound::speech_is,Myship::SPEECH);
            s->play_ship_sample(Mysound::speech_thewinner,Myship::SPEECH);
            sprintf(buf,"%s %s",NoNet::get_player_color(id),GAMETXT("NET_ENDBASEB"));
          }
          SManager::lock();
          Fade::start(Fade::ONLYIN,buf);
          quit_timer=3*Mp::METRONQUALITY;
          fadeing=1;
          break;
        }
      }
    }
    else 
    {
      t=Fade::is_faded();
      quit_timer-=KbdStat::time();
    }
    if (t&&quit_timer<=0) 
    {
      throw TerminateMission();
    }
  }
  else if (restart)
  {
    DBG_CHECK(!NoNet::is_network_mode());
    if (!fadeing)
    {
      SManager::flush();
      Sound::st_play(&Mysound::speech_misnfailed);
      SManager::lock();
      Fade::start(Fade::ONLYIN,GAMETXT("RESTARTING"));
      quit_timer=2*Mp::METRONQUALITY;
      fadeing=1;
    }
    else 
    {
      t=Fade::is_faded();
      quit_timer-=KbdStat::time();
    }
    if (t&&quit_timer<=0) 
    {
      Update::realase(world,gamemanager);
    }
  }
  else if (quit)
  {
    if (!fadeing)
    {
      SManager::flush();
      Field* temp;
      temp=necessary->reset();
      if (!temp)
      {
        Sound::st_play(&Mysound::speech_misncomplete);
        Fade::start(Fade::ONLYIN,GAMETXT("MISSION_COMPLETED"));
      }
      else
      {
        Sound::st_play(&Mysound::speech_misnfailed);
        Fade::start(Fade::ONLYIN,GAMETXT("MISSION_FAILED"));
      }
      SManager::lock();
      quit_timer=2*Mp::METRONQUALITY;
      DBG_MESSAGE("ships count after level=%d",Counter::ships_count());
      fadeing=1;
    }
    else 
    {
      t=Fade::is_faded();
      quit_timer-=KbdStat::time();
    }
    if (t&&quit_timer<=0) 
    {
      throw TerminateMission();
    }
  }


  if (!NoNet::is_network_mode())
  {
    if (Counter::all_ships())
    {
       Field* last=NULL;
       for (Field* temp=necessary->reset();temp;temp=necessary->get_next())
         last=temp;
       if (!all_ships)
       {
         Myship *s=NoNet::get_ship();
         s->play_ship_sample(Mysound::speech_alldestroyed,Myship::SPEECH);
         MSGTXT(GAMETXT("ALL_SHIPS_KILLED"),NoNet::get_this_id());
         all_ships=1;
       }
       if (last&&(last->enableaction||last->disableaction))
       {
         active_action(last);
         if (last->disableaction!=NULL)
         {
           Heap::free(last->disableaction);
           last->disableaction=NULL;
         }

         if (last->enableaction!=NULL)
         {
           Heap::free(last->enableaction);
           last->enableaction=NULL;
         }
         last->enableaction=NULL;
         last->disableaction=NULL;
       }
    }
  }
}
//
void GameManager::read_all_text(void)
{
  int i=0;
  Field *temp;
  for (temp=necessary->reset();temp;temp=necessary->get_next(),i++)
  {
    if (i==text_num)
    {
      text_num++;
      if (temp->text.textsize)
      {
        MManager::add_notext(temp,1);
        strcpy(lastobj,temp->text.textobj);
        return;
      }
    }
  }
  i--;
  for (temp=bonus->reset();temp;temp=bonus->get_next(),i++)
  {
    if (i==text_num)
    {
      text_num++;
      if (temp->text.textsize)
      {
        MManager::add_notext(temp,1);
        return;
      }
    }
  }
}
//FIELD
void Field::clean(void)
{
  if (chtype!=NULL)
  {
    Heap::free(chtype);
    chtype=NULL;
  }

  if (handle!=NULL)
  {
    Heap::free(handle);
    handle=NULL;
  }

  if (disableaction!=NULL)
  {
    Heap::free(disableaction);
    disableaction=NULL;
  }

  if (enableaction!=NULL)
  {
    Heap::free(enableaction);
    enableaction=NULL;
  }

  if (type!=NULL)
  {
    Heap::free(type);
    type=NULL;
  }
}
