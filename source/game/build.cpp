#include "headers.h"

char* MINE         = "obj_mine*";
char* CANNON       = "obj_cannon*";
char* MAGNES       = "obj_magnes*";
char* CAPSULE      = "obj_capsule*";
char* DEST         = "obj_destroy*";
char* MACHINE      = "obj_machine*";
char* LIGHT        = "obj_light*";
char* TELEPORT     = "obj_teleport*";
char* GENERATOR    = "obj_generator*";
char* RAFT         = "obj_raft*";
char* THIEF        = "obj_thief*";
char* BUOY         = "obj_buoy*";
char* LANDPLACE    = "landplace*";
char* SSWITCH      = "switch*";
char* DOOR         = "door*";
char* UNBLOCK      = "obj_unblock*";
char* COLLECT      = "obj_collect*";
char* PHOTOS       = "obj_photo*";
char* ALIEN        = "obj_alien*";
char* BASE         = "base*";
char* MYCANNON     = "obj_mycannon*";
char* BRICK        = "obj_brick*";
char* START        = "start_place*";
char* SMOKE        = "obj_smoke*";
char* SECRET       = "obj_secret*";
char* MDEST        = "obj_morph*";

#define  TYPE_NAME world->get_level().type[i]
//KLASA LEVIMP
Buildable *LevImp::builders[256];
int LevImp::builders_indx(0);
int LevImp::maximum_crange(0);
Parameters *LevImp::parameters=NULL;
int LevImp::parameters_indx(0);
LevImp::Landplaces LevImp::land;

World *LevImp::world=NULL;
char  LevImp::typesname[100];
Text  *LevImp::types_text;
int   LevImp::alien(-1);
int   LevImp::mine(-1);
int   LevImp::cannon(-1);
int   LevImp::magnes(-1);
int   LevImp::lcapsule(-1);
int   LevImp::w0capsule(-1);
int   LevImp::w1capsule(-1);
int   LevImp::w2capsule(-1);
int   LevImp::w3capsule(-1);
int   LevImp::w4capsule(-1);
int   LevImp::ccapsule(-1);
int   LevImp::landplace(-1);
int   LevImp::teleport(-1);
int   LevImp::sswitch(-1);
int   LevImp::door(-1);
int   LevImp::destroy(-1);
int   LevImp::machine(-1);
int   LevImp::light(-1);
int   LevImp::unblock(-1);
int   LevImp::collect(-1);
int   LevImp::photo(-1);
int   LevImp::generator(-1);
int   LevImp::base(-1);
int   LevImp::mycannon(-1);
int   LevImp::brick(-1);
int   LevImp::bricksize;
int   LevImp::buoy(-1);
int   LevImp::raft(-1);
int   LevImp::thief(-1);
int   LevImp::smoke(-1);
int   LevImp::secret(-1);
int   LevImp::morphdest(-1);
unsigned short *LevImp::brickptr;
int LevImp::initialized(0);
int LevImp::action[Level::max_size];



//LEVIMP::INIT
void LevImp::init(World *_world)
{
  DBG_MESSAGE("INICJALIZACJA BILDEROW");
  DBG_CHECK(initialized==0);
  initialized=1;
  world=_world;


  alien=mine=cannon=magnes=lcapsule=w0capsule=w1capsule=w2capsule=
  w3capsule=w4capsule=ccapsule=landplace=sswitch=light=door=destroy=machine=
  unblock=collect=photo=teleport=generator=base=mycannon=brick=buoy=raft=thief=
  smoke=secret=morphdest=-1;


  parameters_indx=world->get_level().types_num;
  parameters=(Parameters*)Heap::alloc(sizeof(Parameters)*parameters_indx,builders_mbn);

  memset(builders,0,256*sizeof(Buildable*));
  memset(parameters,0,parameters_indx*sizeof(Parameters));
  memset(land.place,0,sizeof(land.place));
  memset(land.convert,0,sizeof(land.convert));
  memset(action,0,sizeof(action));
  land.size=1;

  int i;
  for (i=0;i<parameters_indx;i++)
  {

    DBG_MESSAGE("Obsluguje teraz typ '%s'.",world->get_level().type[i]);
    if (check_area(world->get_level().type[i],"type",*types_text))
    {
      char *_area=world->get_level().type[i];
      types_text->area(_area);
      int registered=reg_type((*types_text).string("type"),i);
      parameters[i].builder=registered;
#if HI_DEBUG
      if (!registered)
      {
        FAILURE("NIEROZPOZNANY <%s>: 'type' Nie rozpoznalem typu '%s'.",TYPE_NAME,(*types_text).string("type"));
      }
#endif
      (*types_text).endarea();
    }
    else
    {
      if (!(parameters[i].builder=reg_name(world->get_level().type[i],i)))
      {
        parameters[i].disp_type=(world->get_level().status[i])>>12;
        parameters[i].serv_type=(world->get_level().status[i]&0x0fff)>>8;
        parameters[i].colis_type=(world->get_level().status[i]&0x00ff)>>4;
        parameters[i].sync_frame=((world->get_level().status[i]&0x000f))*Mp::SYNCFRAME;

        parameters[i].frame_delay=((world->get_level().aux1[i])>>12)*Mp::FRAMEDELAY;
        parameters[i].time_delay=((world->get_level().aux1[i]&0x0fff)>>8)*Mp::TIMEDELAY;
        parameters[i].repeat_count=(world->get_level().aux1[i]&0x00ff)>>4;
        parameters[i].change_type=(world->get_level().aux1[i]&0x000f);

        parameters[i].change_spr=(world->get_level().aux2[i])>>12;
        if (parameters[i].change_type>7)
        {
          parameters[i].change_type=-(parameters[i].change_type-7);
        }
      }
    }

    world->get_level().status[i]=0;
    world->get_level().aux1[i]=0;
    world->get_level().aux2[i]=0;
  }
  maximum_crange=parameters[0].range;
  for (i=1;i<parameters_indx;i++)
  {
    if (parameters[i].range>maximum_crange)
    {
      maximum_crange=parameters[i].range;
    }
  }

  seek();
  Counter::run(&world->get_level());
}
//LEVIMP::QUIT
void LevImp::quit(void)
{
  initialized=0;
  if (parameters!=NULL)
  {
    Heap::free((void*)parameters);
    parameters=NULL;
  }
}

int  LevImp::get_sprite(char* name)
{
  for (int i=1;i<Level::max_sprites;i++)
  {
    if (world->get_level().sprite.is_loaded[i])
    {
      if (!strcmpi(world->get_level().sprite[i].name,name))
      {
        return i;
      }
    }
  }
  return 0;
}
//LEVIMP::CHECKAREA
int LevImp::check_area(char* name,char* def,Text& text)
{
  text.area(name);
  if (text.exist(def))
  {
    text.endarea();
    return 1;
  }
  text.endarea();
  return 0;
}
//LEVIMP::REG_TYPE


int LevImp::reg_type(char* type,int i)
{
  DBG_CHECK(initialized);


  if (!strncmp(type,LANDPLACE,strlen(LANDPLACE)-1))
  {
    if (landplace==-1)
    {
      landplace=builders_indx;
      builders[builders_indx++]=NEW(LandplaceBuild,builders_mbn);
    }
    builders[landplace]->bregister(world->get_level().type[i],world->get_level());
    if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
    {
      parameters[i].is_object=Parameters::STARTPLACE;
    }
    else if (NoNet::is_network_mode(NoNet::NETWORKMATCH))
    {
      parameters[i].is_object=Parameters::DISABLE;
    }
    else if (!NoNet::is_network_mode())
    {
      parameters[i].is_object=Parameters::LANDPLACE;
    }
    return 1;
  }

  else if (!strncmp(type,SSWITCH,strlen(SSWITCH)-1))
  {
    if (sswitch==-1)
    {
      sswitch=builders_indx;
      builders[builders_indx++]=NEW(SwitchBuild,builders_mbn);
    }
    builders[sswitch]->bregister(world->get_level().type[i],world->get_level());

    parameters[i].time_delay=(int)(Mp::METRONQUALITY*(*types_text).fp_value("standtime"));
    if (!(parameters[i].frame_delay=world->get_level().typenum((*types_text).string("door_type"))))
    {
      FAILURE("SWITCH <%s>: 'door_type' Nie ma w ledzie  drzwi typu '%s'",world->get_level().type[i],(*types_text).string("door_type"));
    }
    if (types_text->exist("door_type2"))
    {
      if (!(parameters[i].stuff_left=world->get_level().typenum((*types_text).string("door_type2"))))
      {
        FAILURE("SWITCH <%s>: 'door_type2' Nie ma w ledzie  drzwi typu '%s'",world->get_level().type[i],(*types_text).string("door_type2"));
      }

    }
    parameters[i].is_object=Parameters::OTHER;
    return 1;
  }
  else if (!strncmp(type,UNBLOCK,strlen(UNBLOCK)-1))
  {
    if (unblock==-1)
    {
      unblock=builders_indx;
      builders[builders_indx++]=NEW(UnblockBuild,builders_mbn);
    }
    builders[unblock]->bregister(world->get_level().type[i],world->get_level());

    parameters[i].is_object=Parameters::UNBLOCK;
    if (!strcmpi("visual",(*types_text).string("serv_type")))
    {
      int *t=&(parameters[i].blocked);
      parameters[i].blocked=0;

    }
    else if (!strcmpi(("collis"),(*types_text).string("serv_type")))
    {
      parameters[i].blocked=1;
    }
    else
    {
      FAILURE("UNBLOCK <%s>: 'serv_type' zly typ obslugi  (ma byc collis lub visual)",world->get_level().type[i]);
    }
    parameters[i].is_object=Parameters::OTHER;
    return 1;
  }

  else if (!strncmp(type,DOOR,strlen(DOOR)-1))
  {
    if (door==-1)
    {
      door=builders_indx;
      builders[builders_indx++]=NEW(DoorBuild,builders_mbn);
    }
    builders[door]->bregister(world->get_level().type[i],world->get_level());

    parameters[i].time_delay=(int)(Mp::METRONQUALITY*(*types_text).fp_value("delay"));
    parameters[i].disp_type=(*types_text).value("open");
    if (types_text->exist("serv_type"))
    {
      parameters[i].serv_type=(*types_text).value("serv_type");
    }
    else
    {
      parameters[i].serv_type=0;
    }
      parameters[i].is_object=Parameters::OTHER;
    return 1;
  }

  else if (!strncmp(type,DEST,strlen(DEST)-1))
  {
    if (destroy==-1)
    {
      destroy=builders_indx;
      builders[builders_indx++]=NEW(DestroyBuild,builders_mbn);
    }
    builders[destroy]->bregister(world->get_level().type[i],world->get_level());

    if (types_text->exist("colis")) parameters[i].stuff_left=(*types_text).value("colis");
    else parameters[i].stuff_left=0;

    if (types_text->exist("smoke")) parameters[i].smoke=(*types_text).value("smoke");
    else parameters[i].smoke=0;

    if (types_text->exist("light")) parameters[i].light=(*types_text).value("light");
    else parameters[i].light=1;

    parameters[i].range=50;

    char *lf=(*types_text).string("life");
    if (!strcmpi("HIGH_SHIELDS",lf))        parameters[i].sync_frame=Op::DESTROYLIFEHIGH;

    else if (!strcmpi("MEDIUM_SHIELDS",lf)) parameters[i].sync_frame=Op::DESTROYLIFEMEDIUM;

    else if (!strcmpi("LOW_SHIELDS",lf))    parameters[i].sync_frame=Op::DESTROYLIFELOW;

    else if (!strcmpi("IMMORTAL",lf))       parameters[i].sync_frame=Op::DESTROYIMMORTAL;
    else
      FAILURE("DESTROY <%s>: 'life' nie rozpoznalem '%s' (HIGH_SHIELDS,MEDIUM_SHIELDS,LOW_SHIELDS).",world->get_level().type[i],lf);

    if (types_text->exist("ch_sprite"))
    {
      if (!(parameters[i].change_spr=get_sprite((*types_text).string("ch_sprite"))))
        FAILURE("DESTROY <%s>: 'ch_sprite' nie ma sprajta '%s'",TYPE_NAME,(*types_text).string("ch_sprite"));
    }
    if (types_text->exist("shadow"))
    {
      if (!(parameters[i].blocked=get_sprite((*types_text).string("shadow"))))
        FAILURE("DESTROY <%s>: 'shadow' nie ma sprajta '%s'",TYPE_NAME,(*types_text).string("shadow"));
    }

    if (types_text->exist("anim_sprite"))
    {
      if(!(parameters[i].change_type=get_sprite((*types_text).string("anim_sprite"))))
        FAILURE("DESTROY <%s>: 'anim_sprite' nie ma sprajta '%s'",TYPE_NAME,(*types_text).string("anim_sprite"));
    }

    if (types_text->exist("ch_shadow"))
    {
      if (!(parameters[i].repeat_count=get_sprite((*types_text).string("ch_shadow"))))
        FAILURE("DESTROY <%s>: 'ch_shadow' nie ma sprajta '%s'",TYPE_NAME,(*types_text).string("ch_shadow"));
    }
    if (types_text->exist("anim_shadow"))
    {
      if (!(parameters[i].time_delay=get_sprite((*types_text).string("anim_shadow"))))
        FAILURE("DESTROY <%s>: 'anim_shadow' nie ma sprajta '%s'",TYPE_NAME,(*types_text).string("anim_shadow"));
    }

    if (types_text->exist("action"))
    {
      if (!strcmpi("disable_after",(*types_text).string("action"))) parameters[i].action=Parameters::OFFAFTER;
      else if (!strcmpi("disable_before",(*types_text).string("action"))) parameters[i].action=Parameters::OFFBEFORE;
      else if (!strcmpi("enable_after",(*types_text).string("action"))) parameters[i].action=Parameters::ONAFTER;
      else if (!strcmpi("enable_before",(*types_text).string("action"))) parameters[i].action=Parameters::ONBEFORE;
      else FAILURE ("DESTROY <%s>: 'active' nie rozpoznana akcja '%s'",TYPE_NAME,(char*)(*types_text).string("action"));
    }

    if (types_text->exist("visible")) parameters[i].disp_type=(*types_text).value("visible");
    else  parameters[i].disp_type=1;

    if (types_text->exist("morph"))
    {
      parameters[i].morph=(*types_text).value("morph");
      if (!parameters[i].change_spr) FAILURE ("DESTROY <%s>: 'morph' obiekt nie zmienia sprajta (ch_sprite)",TYPE_NAME);
    }
    else  parameters[i].morph=0;

#if HI_DEBUG
    if (parameters[i].morph&&parameters[i].change_type)
      FAILURE("DESTROY <%s>:ten obiek sie morfuje ,etykieta anim_sprite nie ma sensu(usun ja!!)",TYPE_NAME);
#endif

    if (types_text->exist("explode")) parameters[i].explode=(*types_text).value("explode");
    else parameters[i].explode=0;

    if (types_text->exist("explode_sample")) parameters[i].expl_sample=Sound::load_sound((*types_text).string("explode_sample"));
    else parameters[i].expl_sample=-1;

    if (types_text->exist("hit_sample")) parameters[i].hit_sample=Sound::load_sound((*types_text).string("hit_sample"));
    else parameters[i].hit_sample=-1;

    parameters[i].is_object=Parameters::DESTROY;
    parameters[i].frame_delay=(int)(Mp::METRONQUALITY*(*types_text).fp_value("frame_delay"));
    return 1;
  }
  else if (!strncmp(type,CAPSULE,strlen(CAPSULE)-1))
  {
    if (lcapsule==-1)
    {
      lcapsule=builders_indx;
      builders[builders_indx++]=NEW(CapsuleBuild,builders_mbn);
    }
    builders[lcapsule]->bregister(world->get_level().type[i],world->get_level());

    parameters[i].range=(*types_text).value("range");
    parameters[i].is_object=Parameters::CAPSULE;
    return 1;
  }
  else if (!strncmp(type,GENERATOR,strlen(GENERATOR)-1))
  {
    if (generator==-1)
    {
      generator=builders_indx;
      builders[builders_indx++]=NEW(GeneratorBuild,builders_mbn);
    }
    builders[generator]->bregister(world->get_level().type[i],world->get_level());

    parameters[i].range=80;
    parameters[i].serv_type=(int)((*types_text).fp_value("generate_frq")*Mp::METRONQUALITY);
    parameters[i].disp_type=(*types_text).value("object_count")-1;
    parameters[i].is_object=Parameters::GENERATOR;
    return 1;
  }
  else if (!strncmp(type,MYCANNON,strlen(MYCANNON)-1))
  {
    if (!NoNet::is_network_mode(NoNet::NETWORKFLAG)) return 1;
    if (mycannon==-1)
    {
      mycannon=builders_indx;
      builders[builders_indx++]=NEW(MyCannonBuild,builders_mbn);
    }
    builders[mycannon]->bregister(world->get_level().type[i],world->get_level());
    parameters[i].smoke=(*types_text).value("appear_level");
    parameters[i].light=(*types_text).value("cannon_type");
    parameters[i].blocked=((*types_text).value("angle")*Mp::TURNQUALITY)/2/360;
    parameters[i].is_object=Parameters::NETCANNON;
    return 1;
  }
  return 0;
}
//LEVIMP::REG_NAME
int LevImp::reg_name(char* type,int num)
{
  if (!strncmp(type,TELEPORT,strlen(TELEPORT)-1))
  {
    if (teleport==-1)
    {
      teleport=builders_indx;
      builders[builders_indx++]=NEW(TeleportBuild,builders_mbn);
    }
    builders[teleport]->bregister(world->get_level().type[num],world->get_level());
    parameters[num].range=100;
    parameters[num].builder=1;
    parameters[num].is_object=Parameters::OTHER;
    return 1;
  }
  else if (!strncmp(type,ALIEN,strlen(ALIEN)-1))
  {
    if (alien==-1)
    {
      alien=builders_indx;
      builders[builders_indx++]=NEW(AlienBuild,builders_mbn);
    }
    builders[alien]->bregister(world->get_level().type[num],world->get_level());
    parameters[num].range=350;
    parameters[num].is_alien=1;
    parameters[num].blocked=1;
    parameters[num].is_object=Parameters::ENEMYSHIP;
    return 1;
  }
  else if (!strncmp(type,LIGHT,strlen(LIGHT)-1))
  {
    if (light==-1)
    {
      light=builders_indx;
      builders[builders_indx++]=NEW(LightBuild,builders_mbn);
    }
    builders[light]->bregister(world->get_level().type[num],world->get_level());
    parameters[num].range=50;
    parameters[num].builder=1;
    parameters[num].is_object=Parameters::OTHER;
    return 1;
  }
  else if (!strncmp(type,SECRET,strlen(SECRET)-1))
  {
    if (secret==-1)
    {
      secret=builders_indx;
      builders[builders_indx++]=NEW(SecretBuild,builders_mbn);
    }
    builders[secret]->bregister(world->get_level().type[num],world->get_level());
    parameters[num].range=0;
    parameters[num].builder=1;
    parameters[num].is_object=Parameters::SECRET;
    return 1;
  }
  else if (!strncmp(type,MINE,strlen(MINE)-1))
  {
    if (mine==-1)
    {
      mine=builders_indx;
      builders[builders_indx++]=NEW(MineBuild,builders_mbn);
    }
    builders[mine]->bregister(world->get_level().type[num],world->get_level());
    parameters[num].is_object=Parameters::OTHER;
    parameters[num].range=100;
    return 1;
  }
  else if (!strncmp(type,RAFT,strlen(RAFT)-1))
  {
    if (raft==-1)
    {
      raft=builders_indx;
      builders[builders_indx++]=NEW(RaftBuild,builders_mbn);
    }
    builders[raft]->bregister(world->get_level().type[num],world->get_level());
    parameters[num].is_object=Parameters::OTHER;
    parameters[num].range=100;
    return 1;
  }
  else if (!strncmp(type,SMOKE,strlen(SMOKE)-1))
  {
    if (smoke==-1)
    {
      smoke=builders_indx;
      builders[builders_indx++]=NEW(SmokeBuild,builders_mbn);
    }
    builders[smoke]->bregister(world->get_level().type[num],world->get_level());
    parameters[num].is_object=Parameters::OTHER;
    parameters[num].range=0;
    return 1;
  }
  else if (!strncmp(type,BUOY,strlen(BUOY)-1))
  {
    if (buoy==-1)
    {
      buoy=builders_indx;
      builders[builders_indx++]=NEW(BuoyBuild,builders_mbn);
    }
    builders[buoy]->bregister(world->get_level().type[num],world->get_level());
    parameters[num].is_object=Parameters::ENABLE;
    parameters[num].range=100;
    return 1;
  }
  else if (!strncmp(type,MDEST,strlen(MDEST)-1))
  {
    if (morphdest==-1)
    {
      morphdest=builders_indx;
      builders[builders_indx++]=NEW(MorphDestBuild,builders_mbn);
    }
    builders[morphdest]->bregister(world->get_level().type[num],world->get_level());
    parameters[num].is_object=Parameters::MORPH;
    parameters[num].range=100;
    return 1;
  }
  else if (!strncmp(type,THIEF,strlen(THIEF)-1))
  {
    if (thief==-1)
    {
      thief=builders_indx;
      builders[builders_indx++]=NEW(ThiefBuild,builders_mbn);
    }
    builders[thief]->bregister(world->get_level().type[num],world->get_level());
    parameters[num].is_object=Parameters::OTHER;
    parameters[num].range=100;
    return 1;
  }
  else if (!strncmp(type,CANNON,strlen(CANNON)-1))
  {
    if (cannon==-1)
    {
      cannon=builders_indx;
      builders[builders_indx++]=NEW(CannonBuild,builders_mbn);
    }
    builders[cannon]->bregister(world->get_level().type[num],world->get_level());

    parameters[num].range=100;
    parameters[num].is_object=Parameters::CANNON;
    return 1;
  }

  else if (!strncmp(type,COLLECT,strlen(COLLECT)-1))
  {
    if (collect==-1)
    {
      collect=builders_indx;
      builders[builders_indx++]=NEW(CollectBuild,builders_mbn);
    }
    builders[collect]->bregister(world->get_level().type[num],world->get_level());
    parameters[num].builder=1;
    parameters[num].blocked=1;
    parameters[num].is_object=Parameters::CAPSULE;
    return 1;
  }
  else if (!strncmp(type,BRICK,strlen(BRICK)-1))
  {
    if (brick==-1)
    {
      brick=builders_indx;
      builders[builders_indx++]=NEW(BrickBuild,builders_mbn);
    }
    builders[brick]->bregister(world->get_level().type[num],world->get_level());
    parameters[num].builder=1;
    parameters[num].blocked=1;
    parameters[num].range=0;
    if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
      parameters[num].is_object=Parameters::BRICK;
    else parameters[num].is_object=Parameters::DISABLE;
    return 1;
  }
  else if (!strncmp(type,BASE,strlen(BASE)-1))
  {
    if (!NoNet::is_network_mode(NoNet::NETWORKFLAG))
    {
      parameters[num].is_object=Parameters::DISABLE;
      return 0;
    }
    if (base==-1)
    {
      base=builders_indx;
      builders[builders_indx++]=NEW(BaseBuild,builders_mbn);
    }
    builders[base]->bregister(world->get_level().type[num],world->get_level());
    if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
    {
      parameters[num].is_object=Parameters::NETBASE;
    }
    else parameters[num].is_object=Parameters::DISABLE;
    return 1;
  }
  else if (!strncmp(type,START,strlen(START)-1))
  {
    if (!NoNet::is_network_mode(NoNet::NETWORKFLAG)) parameters[num].is_object=Parameters::STARTPLACE;
    else parameters[num].is_object=Parameters::DISABLE;
    return 1;
  }
  return 0;
}


void LevImp::init_destroy(int num)
{
  Level* lev=&world->get_level();
  for(int i=(*lev)[num].link;i&&i!=num;i=(*lev)[i].link)
  {
    switch(parameters[(*lev)[i].type].is_object)
    {
      case Parameters::ENEMYSHIP:
         if (LevImp::action[i]&&!Comm::production) 
                                     WARNING("obiekt <%s,%d,x=%d,y=%d> uaktywniany na wiele sposobow",
                                         (*lev).type[(*lev)[i].type],
                                         i,(*lev)[i].x,(*lev)[i].y);
                LevImp::action[i]=1;

        break;
      case Parameters::DESTROY:
      case Parameters::CANNON:
      case Parameters::CAPSULE:
      default:
        (*lev)[i].aux1=(*lev)[i].x;
        (*lev)[i].aux2=(*lev)[i].y;
        (*lev)[i].active=0;
        (*lev).update(i);
        if (LevImp::action[i]&&!Comm::production) WARNING("obiekt <%s,%d,x=%d,y=%d> uaktywniany na wiele sposobow",
                                         (*lev).type[(*lev)[i].type],
                                         i,(*lev)[i].x,(*lev)[i].y);
        LevImp::action[i]=1;
        break;
    }
  }
}

void LevImp::init_unblock(int num)
{
  Level* lev=&world->get_level();
  for(int i=(*lev)[num].link;i&&i!=num;i=(*lev)[i].link)
  {
    if (LevImp::action[i]&&!Comm::production) WARNING("obiekt <%s,%d,x=%d,y=%d> uaktywniany na wiele sposobow",
                                         (*lev).type[(*lev)[i].type],
                                         i,(*lev)[i].x,(*lev)[i].y);
    LevImp::action[i]=1;
  }
}


void LevImp::init_generator(int num)
{
  Level* lev=&world->get_level();
  for(int i=(*lev)[num].link;i&&i!=num;i=(*lev)[i].link)
  {
    if (parameters[(*lev)[i].type].is_object!=Parameters::ENEMYSHIP)
      FAILURE("Do generatora dolinkowany obiekt inny niz statek.");
    (*lev)[i].aux2=GENERATOR_STUFF;
    if (LevImp::action[i]&&!Comm::production) WARNING("obiekt <%s,%d,x=%d,y=%d> uaktywniany na wiele sposobow",
                                         (*lev).type[(*lev)[i].type],
                                         i,(*lev)[i].x,(*lev)[i].y);
    LevImp::action[i]=1;
    (*lev)[i].active=0;
    (*lev).update(i);
  }
}

void LevImp::init_netland(int num)
{
   Level* lev=&world->get_level();
   int _id=2*NoNet::get_players_number();
   for(int t=1;t<=NoNet::get_players_number();t++)
   {
     if (land.convert[t]==land.size)
     {
       _id=t;
       break;
     }
   }
   if (NoNet::is_network_mode(NoNet::NETWORKFLAG)&&_id<=NoNet::get_players_number())
   {

     (*lev)[num].user.short1=_id;
     int i=(*lev)[num].link;
     if (!i) return;

     for(i=(*lev)[num].link;i&&i!=num;i=(*lev)[i].link)
     {
       (*lev)[i].user.short1=_id; //tu ma byc id
       if (i==(*lev)[num].link) continue;
       (*lev)[i].active=0;
       (*lev).update(i);
     }
   }
   else
   {
     (*lev)[num].active=0;
     (*lev).update(num);
     for(int i=(*lev)[num].link;i&&i!=num;i=(*lev)[i].link)
     {
       (*lev)[i].active=0;
       (*lev).update(i);
     }
   }
}

void LevImp::randbrick(void)
{
  if (bricksize>Mp::MAXBRICKS)
    FAILURE("Too many bricks on the level <%d>",bricksize);
  if (bricksize<Op::MAXLEVELBRICK) 
    FAILURE("Too few bricks on the level <%d>",bricksize);

  int counted=bricksize-Op::MAXLEVELBRICK;
  int size=bricksize;
  int num=0;
  for (int i=0;i<counted;i++)
  {
    DBG_CHECK(size>1);
    num=RAND%(size-1);
    world->get_level()[brickptr[num]].active=0;
    world->get_level().update(brickptr[num]);
    brickptr[num]=brickptr[size-1];
    size--;
  }
}


const int MAX_RANDLAND=4;

//LEVIMP::SEEK
void LevImp::seek(void)
{
  int capflag=NoNet::is_network_mode(NoNet::NETWORKFLAG);
  if (capflag)
  {
    brickptr=(unsigned short*) Heap::alloc(500*sizeof(unsigned short),"temporary buffer");
  }
  else brickptr=NULL;
  bricksize=0;

  if (NoNet::is_network_mode())
  {
    for(int t=1;t<=MAX_RANDLAND;t++)
    {
      int num=(RAND%MAX_RANDLAND)+1;
      while(land.convert[num])  num=(RAND%MAX_RANDLAND)+1;
      land.convert[num]=t;
    }
  }
  else
  {
    for(int t=1;t<=MAX_RANDLAND;t++)
    {
      land.convert[t]=t;
    }
  }
  int hard_level=Pilot::get_skill();
  for (unsigned short i=1;i<world->get_level().size;i++)
  {
    Lobject *o=&world->get_level()[i];
    /*
    if (o->status!=(short)CONVERTED) o->status=0;
    o->aux1=o->aux2=0;
    */
    if (!hard_level)
    {
      if (!(parameters[o->type].is_object==Parameters::CAPSULE)&&o->special)
      {
        int act=o->link;
        while (act)
        {
          if (world->get_level()[act].link==i)
          {
            world->get_level()[act].link=o->link;
            if (o->link==i) o->link=0;
            if (world->get_level()[act].link==act) world->get_level()[act].link=0;
            break;
          }
          act=world->get_level()[act].link;
        }

        o->link=o->active=0;
        world->get_level().update(i);
        continue;
      }
    }
    else
    {
      if (o->special&&parameters[o->type].is_object==Parameters::CAPSULE)
      {
        int act=o->link;
        while (act)
        {
          if (world->get_level()[act].link==i)
          {
            world->get_level()[act].link=o->link;
            if (o->link==i) o->link=0;
            if (world->get_level()[act].link==act) world->get_level()[act].link=0;
            break;
          }
          act=world->get_level()[act].link;
        }

        o->link=o->active=0;
        world->get_level().update(i);
        continue;
      }
    }
    if (parameters[o->type].is_object==Parameters::GENERATOR) init_generator(i);

    switch(parameters[o->type].action)
    {
      case Parameters::ONAFTER:    //schowanie
      case Parameters::ONBEFORE: init_destroy(i);
      break;
      case Parameters::OFFAFTER:
      case Parameters::OFFBEFORE:
      break;
      default :break;
    }
    parameters[o->type].count++;
    //BRICK
    if (capflag&&parameters[o->type].is_object==Parameters::BRICK)
    {
      brickptr[bricksize]=i;
      bricksize++;
    }
    //BEAS&STARTPLACE
    if (parameters[o->type].is_object==Parameters::STARTPLACE)
    {
      if (land.size>=MAX_LANDPLACES)
        FAILURE("Too many startplaces <%d>",MAX_LANDPLACES);
      if(NoNet::is_network_mode(NoNet::NETWORKFLAG)) init_netland(i);
      land.place[land.size]=i;
      land.size++;
    }
    //UNBLOCK
    if (parameters[o->type].is_object==Parameters::UNBLOCK) init_unblock(i); 
    //DISABLE
    if (parameters[o->type].is_object==Parameters::DISABLE)
    {
      o->active=0;
      world->get_level().update(i);
    }
  }
  if (capflag&&bricksize&&brickptr)
  {
    randbrick();
    Heap::free(brickptr);
  }
  if (NoNet::is_network_mode(NoNet::NETWORKFLAG)) DBG_CHECK(MAX_RANDLAND==land.size-1);
}
//LEVIMP::REGBUILD_GLOBAL
void LevImp::regbuild_global(void)
{
  DBG_CHECK(initialized);

  char type[100];
  for (int i=1;i<world->get_level().size;i++)
  {
    Lobject* o=&world->get_level()[i];
    if (parameters[o->type].builder&&parameters[o->type].is_object!=Parameters::DISABLE)
    {
      if (check_area(world->get_level().type[o->type],"type",*types_text))
      {
        types_text->area(world->get_level().type[o->type]);
        strcpy(type,(*types_text).string("type"));
      }
      else strcpy(type,world->get_level().type[o->type]);
      if ((!strncmp(type,BASE,strlen(BASE)-1))||(!strncmp(type,DOOR,strlen(DOOR)-1))||
          (!strncmp(type,SSWITCH,strlen(SSWITCH)-1))||(!strncmp(type,LANDPLACE,strlen(LANDPLACE)-1))||
          (!strncmp(type,BUOY,strlen(BUOY)-1)))
      {
        if (o->active) world->get_level().build(i,gamemanager->get_sublevel(),world->get_level().oplane(i));
      }
      types_text->endarea();
    }
  }
}
//LEVIMP::REGBUILD_LOCAL
void LevImp::regbuild_local(void)
{
  DBG_CHECK(initialized);

  int rotatr=builders_indx;
  builders[builders_indx++]=NEW(RotatRight,builders_mbn);
  int rotatl=builders_indx;
  builders[builders_indx++]=NEW(RotatLeft,builders_mbn);
  int wait=builders_indx;
  builders[builders_indx++]=NEW(RandomWait,builders_mbn);
  int ping=builders_indx;
  builders[builders_indx++]=NEW(PingPong,builders_mbn);
  int syncrr=builders_indx;
  builders[builders_indx++]=NEW(SRotatRight,builders_mbn);
  int syncrl=builders_indx;
  builders[builders_indx++]=NEW(SRotatLeft,builders_mbn);
  int syncpp=builders_indx;
  builders[builders_indx++]=NEW(SPingPong,builders_mbn);
  int prp=builders_indx;
  builders[builders_indx++]=NEW (Prepare,builders_mbn);

  for (int i=0;i<world->get_level().types_num;i++)
  {
    switch (get_parameters(i)->serv_type)
    {
      case PSRIGHT:        builders[rotatr]->bregister(world->get_level().type[i],world->get_level());
                           break;
      case PSLEFT:         builders[rotatl]->bregister(world->get_level().type[i],world->get_level());
                           break;
      case PSWAITRAND:     builders[wait]->bregister(world->get_level().type[i],world->get_level());
                           break;
      case PSPINGPONG:     builders[ping]->bregister(world->get_level().type[i],world->get_level());
                           break;
      case PSSROTRIGHT:    builders[syncrr]->bregister(world->get_level().type[i],world->get_level());
                           break;
      case PSSROTLEFT:     builders[syncrl]->bregister(world->get_level().type[i],world->get_level());
                           break;
      case PSYNCPINGPONG:  builders[syncpp]->bregister(world->get_level().type[i],world->get_level());
                           break;
      case PREPARE:        builders[prp]->bregister(world->get_level().type[i],world->get_level());
                           break;

    }
  }
}
//LEVIMP::REGBUILD_CANCEL
void LevImp::regbuild_cancel(void)
{
  DBG_CHECK(initialized);
  while (builders_indx>0)
  {
    builders_indx--;
    delete builders[builders_indx];
    builders[builders_indx]=NULL;
  }
}
//LEVIMP::IN_CRANGE
int LevImp::in_crange(int type,int x,int y)
{
  DBG_CHECK(initialized);
  return Posit::is_in_overscan(LevImp::get_parameters(type)->range,x,y);
}

int LevImp::in_alienrange(int x,int y,int range)
{
  return Posit::is_in(range,x,y);
}
//LEVIMP::REPORT
void LevImp::report(void)
{
}
//LEVIMP::MAX_CRANGE
int LevImp::max_crange(void)
{
  DBG_CHECK(initialized);
  return maximum_crange;
}


//COUNTER
int  Counter::upgr[UPGR_LAST];
int  Counter::ships[SH_LAST];
int  Counter::neutral[NEUT_LAST];
int  Counter::global_ships=0;
int  Counter::start_global_ships=0;
int  Counter::global_secrets=0;
int  Counter::start_global_secrets=0;

void Counter::update_registry(void)
{
  Pilot::time(Pilot::get_level())=Game::get_timer()/Mp::METRONQUALITY;
  Pilot::kills(Pilot::get_level())=(start_global_ships-global_ships);
  Pilot::secrets(Pilot::get_level())=(start_global_secrets-global_secrets);
  Pilot::secrets_num(Pilot::get_level())=start_global_secrets;
  Pilot::kills_num(Pilot::get_level())=start_global_ships;
  DBG_MESSAGE("missiosn time %d",Pilot::time(Pilot::get_level()));
  DBG_MESSAGE("ships in mission  %d",Pilot::kills_num(Pilot::get_level()));
  DBG_MESSAGE("ships killed      %d",Pilot::kills(Pilot::get_level()));
  DBG_MESSAGE("secret in mission %d",Pilot::secrets_num(Pilot::get_level()));
  DBG_MESSAGE("secret found      %d",Pilot::secrets(Pilot::get_level()));
}
void Counter::present_alien(int lev_num)
{
  DBG_MESSAGE("alien number  <%s,%d,x=%d,y=%d>",world->get_level().type[world->get_level()[lev_num].type]
                                               ,lev_num
                                               ,world->get_level()[lev_num].x
                                               ,world->get_level()[lev_num].y);
  global_ships--;
  DBG_CHECK(global_ships>=0);
}



void Counter::run(Level* level)
{
  memset(upgr,0,sizeof(upgr));
  memset(ships,0,sizeof(ships));
  memset(neutral,0,sizeof(neutral));
  int ships_life=0;
  global_ships=0;
  start_global_ships=0;
  global_secrets=0;
  start_global_secrets=0;
  for (int num=1;num<level->size;num++)
  {
    if (((*level)[num].special&&!Pilot::get_skill())||
      ((*level)[num].status==((short)CONVERTED))||
      ((*level)[num].aux2==GENERATOR_STUFF))
                            continue;
    switch (LevImp::get_parameters((*level)[num].type)->is_object)
    {
      case Parameters::CAPSULE:
        {
          switch ((*level)[num].phase)
          {
             case 0: upgr[CANNON]+=Capsule::get_amount(Capsule::WEAP0,0); break;
             case 1: upgr[CANNON]+=Capsule::get_amount(Capsule::WEAP0,1); break;
             case 2: upgr[CANNON]+=Capsule::get_amount(Capsule::WEAP0,2); break;

             case 3: upgr[SWARM]+=Capsule::get_amount(Capsule::WEAP1,0); break;
             case 4: upgr[SWARM]+=Capsule::get_amount(Capsule::WEAP1,1); break;
             case 5: upgr[SWARM]+=Capsule::get_amount(Capsule::WEAP1,2); break;

             case 6: upgr[PLASMA]+=Capsule::get_amount(Capsule::WEAP2,0); break;
             case 7: upgr[PLASMA]+=Capsule::get_amount(Capsule::WEAP2,1); break;
             case 8: upgr[PLASMA]+=Capsule::get_amount(Capsule::WEAP2,2); break;

             case 9: upgr[ROCKET]+=Capsule::get_amount(Capsule::WEAP3,0); break;
            case 10: upgr[ROCKET]+=Capsule::get_amount(Capsule::WEAP3,1); break;
            case 11: upgr[ROCKET]+=Capsule::get_amount(Capsule::WEAP3,2); break;

            case 12: upgr[GUN]+=Capsule::get_amount(Capsule::WEAP4,0); break;
            case 13: upgr[GUN]+=Capsule::get_amount(Capsule::WEAP4,1); break;
            case 14: upgr[GUN]+=Capsule::get_amount(Capsule::WEAP4,2); break;

            case 15: upgr[COBAIN]+=Capsule::get_amount(Capsule::WEAP5,0); break;
            case 16: upgr[COBAIN]+=Capsule::get_amount(Capsule::WEAP5,1); break;
            case 17: upgr[COBAIN]+=Capsule::get_amount(Capsule::WEAP5,2); break;

            case 18: upgr[LIFE]+=Capsule::get_amount(Capsule::LIFE,0); break;
            case 19: upgr[LIFE]+=Capsule::get_amount(Capsule::LIFE,1); break;
            case 20: upgr[LIFE]+=Capsule::get_amount(Capsule::LIFE,2); break;

            case 21: upgr[MEDKIT]++;break;
            case 22: upgr[CLOAK]++; break;
            case 23: upgr[IMMORTAL]++;break;
            case 26: upgr[NETMINE]++;break;
            case 27: upgr[GOLDENEYE]++;break;
          }
        }
        break;
      case Parameters::ENEMYSHIP:
        {
          DBG_MESSAGE("Counted alien number  <%s,%d,x=%d,y=%d>",(*level).type[(*level)[num].type],num,(*level)[num].x,(*level)[num].y);
          Counter::global_ships++;
          switch((*level)[num].phase)
          {
            case LevImp::SIMPLE1:  ships[SIMPLE1]++;upgr[CANNON]+=Capsule::get_amount(Capsule::WEAP0,0); ships_life+=Op::obj_enemy[(*level)[num].phase].life;break;
            case LevImp::SIMPLE2:  ships[SIMPLE2]++;upgr[PLASMA]+=Capsule::get_amount(Capsule::WEAP2,0); ships_life+=Op::obj_enemy[(*level)[num].phase].life;break;
            case LevImp::SIMPLE3:  ships[SIMPLE3]++;upgr[ROCKET]+=Capsule::get_amount(Capsule::WEAP3,0); ships_life+=Op::obj_enemy[(*level)[num].phase].life;break;
            case LevImp::TARAN:    ships[SIMPLE4]++;upgr[LIFE]+=Capsule::get_amount(Capsule::LIFE,0); ships_life+=Op::obj_enemy[(*level)[num].phase].life;break;
            case LevImp::MBOSS1:   ships[MBOSS1]++;upgr[PLASMA]+=Capsule::get_amount(Capsule::WEAP2,1); ships_life+=Op::obj_mboss[(*level)[num].phase].life;break;
            case LevImp::MBOSS2:   ships[MBOSS2]++;upgr[ROCKET]+=Capsule::get_amount(Capsule::WEAP3,1); ships_life+=Op::obj_mboss[(*level)[num].phase].life;break;
            case LevImp::MBOSS3:   ships[MBOSS3]++;upgr[CANNON]+=Capsule::get_amount(Capsule::WEAP0,1); ships_life+=Op::obj_mboss[(*level)[num].phase].life;break;
            case LevImp::MBOSS4:   ships[MBOSS4]++;upgr[LIFE]+=Capsule::get_amount(Capsule::LIFE,1); ships_life+=Op::obj_mboss[(*level)[num].phase].life;break;
            case LevImp::SIMPLE1INVISIBLE:  ships[SIMPLE1]++;ships[SIMPLE1]++;upgr[CANNON]+=Capsule::get_amount(Capsule::WEAP0,0); ships_life+=Op::obj_enemy[0].life;break;
            case LevImp::SIMPLE2INVISIBLE:  ships[SIMPLE2]++;ships[SIMPLE2]++;upgr[PLASMA]+=Capsule::get_amount(Capsule::WEAP2,0); ships_life+=Op::obj_enemy[1].life;break;
            case LevImp::SIMPLE3INVISIBLE:  ships[SIMPLE3]++;ships[SIMPLE3]++;upgr[ROCKET]+=Capsule::get_amount(Capsule::WEAP3,0); ships_life+=Op::obj_enemy[2].life;break;
            case LevImp::TARANINVISIBLE:    ships[SIMPLE4]++;ships[SIMPLE4]++;upgr[LIFE]+=Capsule::get_amount(Capsule::LIFE,0); ships_life+=Op::obj_enemy[3].life;break;
          }
        }

        break;
      case Parameters::CANNON:   DBG_MESSAGE("Counted alien number  <%s,%d,x=%d,y=%d>",(*level).type[(*level)[num].type],num,(*level)[num].x,(*level)[num].y);
                                 Counter::global_ships++;
                                 ships[CANNON_TOWER]++;
                                 ships_life+=Op::def_cannon[0].life;
                                 break;

      case Parameters::DESTROY:       neutral[DESTROY]++;    break;
      case Parameters::SECRET:        global_secrets++;    break;

      case Parameters::GENERATOR:     global_ships+=LevImp::get_parameters((*level)[num].type)->disp_type;
                                      DBG_MESSAGE("Generator ships %d",LevImp::get_parameters((*level)[num].type)->disp_type);
                                      ships_life+=Op::obj_enemy[1].life*LevImp::get_parameters((*level)[num].type)->disp_type;
                                      break;
    }
  }
  if (Mp::BUILDSPRITESMODE)
  {
    FILE* file;
    char temp_name[1024];
    sprintf(temp_name,"%sfirefght.rap",Comm::pref_path);  // port: was in TEMP

    static int first_time=1;
    if(first_time)
    {
      file=fopen(temp_name,"w");
      fprintf(file,"  level    wpp1 wpp2 wpp3 wpp4 wpp5 wpp6 wpn1 wpn2 wpn3 wpn4 wpn5 wpn6 life en1 en2 en3 en4 mb1 mb2 mb3 mb4 can dest secr\n");

      fclose(file);
      first_time=0;
    }
    file=fopen(temp_name,"a+");
    fprintf(file,"%6s%d    %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %3d %3d %3d %3d %3d %3d %3d %3d %3d %4d %d\n",
      GameManager::volume,
      gamemanager->get_sublevel(),
      (ships_life)?((upgr[CANNON]*_CSPLASH)*100/ships_life):0,
      (ships_life)?(upgr[SWARM]*_CSWARMERS)*100/ships_life:0,
      (ships_life)?(upgr[PLASMA]*_CLASER)*100/ships_life:0,
      (ships_life)?(upgr[ROCKET]*_CSPLASH)*100/ships_life:0,
      (ships_life)?(upgr[GUN]*_CSHOTGUN)*100/ships_life:0,
      (ships_life)?(upgr[COBAIN]*_CMYMINE)*100/ships_life:0,
      upgr[CANNON],
      upgr[SWARM],
      upgr[PLASMA],
      upgr[ROCKET],
      upgr[GUN],
      upgr[COBAIN],
      upgr[LIFE],
      ships[SIMPLE1],
      ships[SIMPLE2],
      ships[SIMPLE3],
      ships[SIMPLE4],
      ships[MBOSS1],
      ships[MBOSS2],
      ships[MBOSS3],
      ships[MBOSS4],
      ships[CANNON_TOWER],
      neutral[DESTROY],
      global_secrets
    );
    fclose(file);
  }
  start_global_ships=global_ships;
  DBG_MESSAGE("alien ships=%d",start_global_ships);
  start_global_secrets=global_secrets;
}


//ALIENBUILD
void AlienBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);

  switch (bo_phase())
  {
    case LevImp::SIMPLE1:if (Generator::initializing||LevImp::in_crange(bo_type(),bo_x(),bo_y()))
           {
             if (GameManager::mission_object) return;
             if(Generator::initializing) bo_setstatus(2);
             NEW(Simple1(bo_x(),bo_y(),bo_num,Simple1::NORMAL,0),gobjects_mbn);
           }
           break;
    case LevImp::SIMPLE2:if (Generator::initializing||LevImp::in_crange(bo_type(),bo_x(),bo_y()))
           {
             if (GameManager::mission_object) return;
             if(Generator::initializing) bo_setstatus(2);
             NEW(Simple1(bo_x(),bo_y(),bo_num,Simple1::SPECIAL,0),gobjects_mbn);
           }
           break;
    case LevImp::SIMPLE3:if (Generator::initializing||LevImp::in_crange(bo_type(),bo_x(),bo_y()))
           {
             if (GameManager::mission_object) return;
             if(Generator::initializing) bo_setstatus(2);
             NEW(Simple3(bo_x(),bo_y(),bo_num,0),gobjects_mbn);
           }
           break;
    case LevImp::TARAN:if (Generator::initializing||LevImp::in_crange(bo_type(),bo_x(),bo_y()))
           {
             if (GameManager::mission_object) return;
             if(Generator::initializing) bo_setstatus(2);
             NEW(Taran(bo_x(),bo_y(),bo_num,Posit::DOWN,0,0),gobjects_mbn);
           }
           break;
    case LevImp::MBOSS1:if (LevImp::in_crange(bo_type(),bo_x(),bo_y())||GameManager::mission_object)
           {
             NEW(MBoss1(bo_x(),bo_y(),bo_num,Mysprites::mboss1,Op::MBOSS1,0,GameManager::mission_object),gobjects_mbn);
           }
           break;
    case LevImp::MBOSS2:if (LevImp::in_crange(bo_type(),bo_x(),bo_y())||GameManager::mission_object)
           {
             NEW(MBoss1(bo_x(),bo_y(),bo_num,Mysprites::mboss2,Op::MBOSS2,0,GameManager::mission_object),gobjects_mbn);
           }
           break;
    case LevImp::MBOSS3:if (LevImp::in_crange(bo_type(),bo_x(),bo_y())||GameManager::mission_object)
           {
             NEW(MBoss1(bo_x(),bo_y(),bo_num,Mysprites::mboss3,Op::MBOSS3,0,GameManager::mission_object),gobjects_mbn);
           }
           break;
    case LevImp::MBOSS4:if (LevImp::in_crange(bo_type(),bo_x(),bo_y())||GameManager::mission_object)
           {
             NEW(MBoss1(bo_x(),bo_y(),bo_num,Mysprites::mboss4,Op::MBOSS4,0,GameManager::mission_object),gobjects_mbn);
           }
           break;
    case LevImp::SIMPLE1INVISIBLE:if (Generator::initializing||LevImp::in_crange(bo_type(),bo_x(),bo_y()))
           {
             if (GameManager::mission_object) return;
             if(Generator::initializing) bo_setstatus(2);
             NEW(Simple1(bo_x(),bo_y(),bo_num,Simple1::NORMAL,1),gobjects_mbn);
           }
           break;
    case LevImp::SIMPLE2INVISIBLE:if (Generator::initializing||LevImp::in_crange(bo_type(),bo_x(),bo_y()))
           {
             if (GameManager::mission_object) return;
             if(Generator::initializing) bo_setstatus(2);
             NEW(Simple1(bo_x(),bo_y(),bo_num,Simple1::SPECIAL,1),gobjects_mbn);
           }
           break;
    case LevImp::SIMPLE3INVISIBLE:if (Generator::initializing||LevImp::in_crange(bo_type(),bo_x(),bo_y()))
           {
             if (GameManager::mission_object) return;
             if(Generator::initializing) bo_setstatus(2);
             NEW(Simple3(bo_x(),bo_y(),bo_num,1),gobjects_mbn);
           }
           break;
    case LevImp::TARANINVISIBLE:if (Generator::initializing||LevImp::in_crange(bo_type(),bo_x(),bo_y()))
           {
             if (GameManager::mission_object) return;
             if(Generator::initializing) bo_setstatus(2);
             NEW(Taran(bo_x(),bo_y(),bo_num,Posit::DOWN,0,1),gobjects_mbn);
           }
           break;


    case LevImp::MBOSS1INVISIBLE:if (LevImp::in_crange(bo_type(),bo_x(),bo_y())||GameManager::mission_object)
           {
             NEW(MBoss1(bo_x(),bo_y(),bo_num,Mysprites::mboss1,Op::MBOSS1,1,GameManager::mission_object),gobjects_mbn);
           }
           break;
    case LevImp::MBOSS2INVISIBLE:if (LevImp::in_crange(bo_type(),bo_x(),bo_y())||GameManager::mission_object)
           {
             NEW(MBoss1(bo_x(),bo_y(),bo_num,Mysprites::mboss2,Op::MBOSS2,1,GameManager::mission_object),gobjects_mbn);
           }
           break;
    case LevImp::MBOSS3INVISIBLE:if (LevImp::in_crange(bo_type(),bo_x(),bo_y())||GameManager::mission_object)
           {
             NEW(MBoss1(bo_x(),bo_y(),bo_num,Mysprites::mboss3,Op::MBOSS3,1,GameManager::mission_object),gobjects_mbn);
           }
           break;
    case LevImp::MBOSS4INVISIBLE:if (LevImp::in_crange(bo_type(),bo_x(),bo_y())||GameManager::mission_object)
           {
             NEW(MBoss1(bo_x(),bo_y(),bo_num,Mysprites::mboss4,Op::MBOSS4,1,GameManager::mission_object),gobjects_mbn);
           }
           break;

#if HI_DEBUG
      default: FAILURE("Faza sprajta opisujaca obj_alien jest za duza (nie ma takiego statku (phase==%d,posx==%d,posy==%d).",bo_phase(),bo_x(),bo_y());
#endif
  }
}
//MINA BUILDER
void MineBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  if (LevImp::in_crange(bo_type(),bo_x(),bo_y()))
  {
    int mine=-1;
    if (!strcmpi(world->get_level().type[world->get_level()[bo_num].type],"obj_mine1")) mine=Mine::STATIC;
    else if (!strcmpi(world->get_level().type[world->get_level()[bo_num].type],"obj_mine2")) mine=Mine::TRACER;
    DBG_CHECK(mine!=-1);
    if (Generator::initializing) bo_setstatus(2);
    NEW(Mine(bo_x(),bo_y(),Posit::UP,*bo_spr,bo_num,mine),gobjects_mbn);
  }
}
//SWITCH BUILDER
void SwitchBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  NEW(Switch(bo_x(),bo_y(),Posit::UP,*bo_spr,bo_num),gobjects_mbn);
}
//UNBLOCK BUILDER
void UnblockBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  NEW(Unblock(bo_x(),bo_y(),*bo_spr,bo_num),gobjects_mbn);
}
//LIGHT
void LightBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  if (LevImp::in_crange(bo_type(),bo_x(),bo_y()))
  {
    NEW(Light(bo_x(),bo_y(),bo_aux1(),bo_phase(),bo_num),gobjects_mbn);
  }
}
//DOOR BUILDER
void DoorBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  NEW(Door(bo_x(),bo_y(),Posit::UP,*bo_spr,bo_num,bo_plane ),gobjects_mbn);
}
//DESTROY BUILDER
void DestroyBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  if (GameManager::mission_object)
    NEW(Destroy(bo_x(),bo_y(),Posit::UP,*bo_spr,bo_num,bo_aux1(),bo_aux2()),gobjects_mbn);
  else
  {
    if (LevImp::in_crange(bo_type(),bo_x(),bo_y()))
    {
      if (bo_aux2()==-1)
       NEW(Destroy(bo_x(),bo_y(),Posit::UP,*bo_spr,bo_num,bo_aux1(),1),gobjects_mbn);
      else
      {
        NEW(Destroy(bo_x(),bo_y(),Posit::UP,*bo_spr,bo_num,bo_aux1(),0),gobjects_mbn);
      }
    }

  }
}
//CANNON BUILDER
void CannonBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  if (LevImp::in_crange(bo_type(),bo_x(),bo_y()))
  {
    Lobject* o=&world->get_level()[bo_num];
    if (o->user.short2==CINDERSCANNON)
    {
      NEW(Cinders(bo_x(),bo_y(),Mysprites::cinder_cannon,bo_num,Cinders::CANNON,world->get_active_plane()),gobjects_mbn);
      return;
    }
    else if (o->user.short2==CINDERSPULSE)
    {
      NEW(Cinders(bo_x(),bo_y(),Mysprites::cinder_pulse,bo_num,Cinders::PULSE,world->get_active_plane()),gobjects_mbn);
      return;
    }

    int t=-1;
    if      (!strcmpi(world->get_level().type[world->get_level()[bo_num].type],"obj_cannon1")) t=0;
    else if (!strcmpi(world->get_level().type[world->get_level()[bo_num].type],"obj_cannon2")) t=1;
    else if (!strcmpi(world->get_level().type[world->get_level()[bo_num].type],"obj_cannon3")) t=2;
    else if (!strcmpi(world->get_level().type[world->get_level()[bo_num].type],"obj_cannon4")) t=3;
    else if (!strcmpi(world->get_level().type[world->get_level()[bo_num].type],"obj_cannon5")) t=4;
    else if (!strcmpi(world->get_level().type[world->get_level()[bo_num].type],"obj_cannon6")) t=5;
    else if (!strcmpi(world->get_level().type[world->get_level()[bo_num].type],"obj_cannon7")) t=6;
    else if (!strcmpi(world->get_level().type[world->get_level()[bo_num].type],"obj_cannon8")) t=7;
    else if (!strcmpi(world->get_level().type[world->get_level()[bo_num].type],"obj_cannon9")) t=8;
    else if (!strcmpi(world->get_level().type[world->get_level()[bo_num].type],"obj_cannon10")) t=9;
    else if (!strcmpi(world->get_level().type[world->get_level()[bo_num].type],"obj_cannon11")) t=10;
    else if (!strcmpi(world->get_level().type[world->get_level()[bo_num].type],"obj_cannon12")) t=11;
    DBG_CHECK(t!=-1);
    NEW(ACannon(bo_x(),bo_y(),*bo_spr,bo_num,world->get_active_plane(),&Op::def_cannon[t]),gobjects_mbn);
  }
}
//LANDPLACE BUILDER
void LandplaceBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  NEW(Landplace(bo_x(),bo_y(),*bo_spr,bo_phase(),bo_num),gobjects_mbn);
}
//TELEPORT BUILDER
void TeleportBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  if (LevImp::in_crange(bo_type(),bo_x(),bo_y()))
  {
    NEW(Teleport(bo_x(),bo_y(),bo_num,(bo_phase()>0)),gobjects_mbn);
  }
}
//COLLECT BUILDER
void CollectBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  NEW(Collect(bo_x(),bo_y(),*bo_spr,bo_num),gobjects_mbn);
}
//CAPSULE BUILDER
#define CAPSULE(type,power) NEW(LevelCapsule(bo_x(),bo_y(),Posit::UP,bo_num,Capsule::type,power),gobjects_mbn);
void CapsuleBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  if (LevImp::in_crange(bo_type(),bo_x(),bo_y()))
  {
    switch (bo_phase())
    {
       case 0: CAPSULE(WEAP0,0);
               break;
       case 1: CAPSULE(WEAP0,1);
               break;
       case 2: CAPSULE(WEAP0,2);
               break;
       case 3: CAPSULE(WEAP1,0);
               break;
       case 4: CAPSULE(WEAP1,1);
               break;
       case 5: CAPSULE(WEAP1,2);
               break;
       case 6: CAPSULE(WEAP2,0);
               break;
       case 7: CAPSULE(WEAP2,1);
               break;
       case 8: CAPSULE(WEAP2,2);
               break;
       case 9: CAPSULE(WEAP3,0);
               break;
      case 10: CAPSULE(WEAP3,1);
               break;
      case 11: CAPSULE(WEAP3,2);
               break;
      case 12: CAPSULE(WEAP4,0);
               break;
      case 13: CAPSULE(WEAP4,1);
               break;
      case 14: CAPSULE(WEAP4,2);
               break;
      case 15: CAPSULE(WEAP5,0);
               break;
      case 16: CAPSULE(WEAP5,1);
               break;
      case 17: CAPSULE(WEAP5,2);
               break;
      case 18: CAPSULE(LIFE,0);
               break;
      case 19: CAPSULE(LIFE,1);
               break;
      case 20: CAPSULE(LIFE,2);
               break;
      case 21: CAPSULE(MEDIKIT,0);
               break;
      case 22: CAPSULE(CLOAK,0);
               break;
      case 23: CAPSULE(IMMORT,0);
               break;
      case 26: CAPSULE(NETMINE,0);
               break;
      case 27: CAPSULE(GOLDENEYE,0);
               break;
      case 28: CAPSULE(ALTERING,0);
               break;
      case 29: CAPSULE(ALTERING,1);
               break;
      case 30: CAPSULE(ALTERING,2);
               break;
      case 31: CAPSULE(CHAOS,0);
               break;
      case 24: break;
      case 25: break;
#if HI_DEBUG
      default: FAILURE("Weapon upgrade sprite: invalid phase (phase==%d,posx==%d,posy==%d).",bo_phase(),bo_x(),bo_y());
#endif
    }
  }
}
#undef CAPSULE

void GeneratorBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  if (LevImp::in_crange(bo_type(),bo_x(),bo_y()))
    NEW(Generator(bo_x(),bo_y(),*bo_spr,bo_num),gobjects_mbn);
}

void BrickBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  if (NoNet::is_network_mode(NoNet::NETWORKFLAG))
  {
    if (LevImp::in_crange(bo_type(),bo_x(),bo_y()))
    {
      NEW(Brick(bo_x(),bo_y(),Posit::UP,0,0,bo_num),gobjects_mbn);
    }
  }
  else
  {
    bo_setactive(0);
    world->get_level().update(bo_num);
  }
}

void MorphDestBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  NEW(MorphDest(bo_x(),bo_y(),*bo_spr,bo_num,bo_aux1()),gobjects_mbn);
}

void BaseBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  NEW(Base(bo_x(),bo_y(),*bo_spr,bo_num,bo->user.short1),gobjects_mbn);
}

void MyCannonBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  NEW(MyCannon(bo_x(),bo_y(),*bo_spr,bo_num,bo_plane,bo->user.short1,LevImp::get_parameters(bo_type())->light,LevImp::get_parameters(bo_type())->smoke),gobjects_mbn);
}

void RaftBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  DBG_CHECK(!NoNet::is_network_mode());
  NEW(Raft(bo_x(),bo_y(),*bo_spr,bo_num,NoNet::get_this_id()),gobjects_mbn);
}

void BuoyBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  DBG_CHECK(!NoNet::is_network_mode());
  NEW(Buoy(bo_x(),bo_y(),*bo_spr,bo_num,LevImp::world),gobjects_mbn);
}

void ThiefBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  DBG_CHECK(!NoNet::is_network_mode());
  NEW(Thief(bo_x(),bo_y(),*bo_spr,bo_num),gobjects_mbn);
}


void SecretBuild::builder(void)
{
  DBG_CHECK(!NoNet::is_network_mode());
  if (Posit::is_in(-20,bo_x(),bo_y()))
  {
    Counter::present_secret();
    NoNet::get_ship()->play_ship_sample(Mysound::speech_secretplace,Myship::SPEECH);
    MSGTXT(GAMETXT("SECRET_PLACE"),NoNet::get_this_id());
    bo_setactive(0);
  }
}

void SmokeBuild::builder(void)
{
  DBG_CHECK(Game::get_work_mode()!=Game::DISPLAYING);
  if (!LevImp::in_crange(bo_type(),bo_x(),bo_y())) return;
  switch(bo_phase())
  {
     case 0: NEW(LevelSmoke(bo_x(),bo_y(),bo_num,LevelSmoke::SMALL,bo_plane),gobjects_mbn); break;
     case 1: NEW(LevelSmoke(bo_x(),bo_y(),bo_num,LevelSmoke::BIGB,bo_plane),gobjects_mbn); break;
     case 2: NEW(LevelSmoke(bo_x(),bo_y(),bo_num,LevelSmoke::BIGW,bo_plane),gobjects_mbn); break;
    default: DBG_FAILURE("obj_smoke<%s,%d> za duza faza dymu",world->get_level().name[bo_type()],bo_num);
  }

}
//ROTATRIGHT BUILDER
void RotatRight::builder(void)
{
  bo_setstatus(bo_status()+KbdStat::time());
  if (bo_aux1()==1)
  {
    if (bo_status()>=LevImp::get_parameters(bo_type())->time_delay)
    {
      bo_setaux1(0);
      bo_setstatus(0);
      bo_setaux2(bo_aux2()+1);
      if (bo_aux2()>=LevImp::get_parameters(bo_type())->repeat_count)
      {
        bo_settype(bo_type()+LevImp::get_parameters(bo_type())->change_type);
        bo_setaux2(0);
      }
    }
    if(LevImp::get_parameters(bo_type())->time_delay) return ;
  }
  if ((bo_status()>=LevImp::get_parameters(bo_type())->frame_delay))
  {
    DBG_CHECK(bo_phasesnum());
    bo_setphase((bo_phase()+1)%(world->get_level().sprite[bo_sprite()].phases));
    bo_setstatus(0);
    //czy ostatania
    if (bo_phase()==world->get_level().sprite[bo_sprite()].phases-1)
    {
      bo_setaux1(1);
    }
  }
}
//ROTATLEFT BUILDER
void RotatLeft::builder(void)
{
  bo_setstatus(bo_status()+KbdStat::time());
  if (bo_aux1()==1)
  {
    if (bo_status()>=LevImp::get_parameters(bo_type())->time_delay)
    {
      bo_setaux1(0);
      bo_setstatus(0);
      bo_setaux2(bo_aux2()+1);
      if (bo_aux2()>=LevImp::get_parameters(bo_type())->repeat_count)
      {
        bo_settype(bo_type()+LevImp::get_parameters(bo_type())->change_type);
        bo_setaux2(0);
      }
    }
    if(LevImp::get_parameters(bo_type())->time_delay) return ;
  }
  if ((bo_status()>=LevImp::get_parameters(bo_type())->frame_delay))
  {
    bo_setphase((bo_phase()-1>=0)?(bo_phase()-1):(world->get_level().sprite[bo_sprite()].phases-1));
    bo_setstatus(0);
    if (!bo_phase())
    {
      bo_setaux1(1);
    }
  }
}
//RANDOM BUILDER
void RandomWait::builder(void)
{
  bo_setstatus(bo_status()+KbdStat::time());
  if (!bo_aux1())
  {
    bo_setaux2(bo_aux2()+1);
    if (bo_aux2()>=LevImp::get_parameters(bo_type())->repeat_count)
    {
      bo_settype(bo_type()+LevImp::get_parameters(bo_type())->change_type);
      bo_setaux2(0);
    }
  }

  if (bo_status()>=bo_aux1())
  {
    bo_setaux1(0);
    bo_setstatus(0);
    bo_settype(bo_type()+LevImp::get_parameters(bo_type())->change_type);
  }
}
//PINGPONG BUILDER
void PingPong::builder(void)
{
  bo_setstatus(bo_status()+KbdStat::time());
  if (bo_aux1()>1)
  {
    if (bo_status()>=LevImp::get_parameters(bo_type())->time_delay)
    {
      if (bo_aux1()==2)bo_setaux1(1);
      else bo_setaux1(0);
      bo_setstatus(0);
      bo_setaux2(bo_aux2()+1);
      if (bo_aux2()>=LevImp::get_parameters(bo_type())->repeat_count)
      {
        bo_settype(bo_type()+LevImp::get_parameters(bo_type())->change_type);
        bo_setaux2(0);
      }
    }
    if(!LevImp::get_parameters(bo_type())->time_delay) return ;
  }
  if(bo_aux1()==0)
  {
    if (bo_status()>=LevImp::get_parameters(bo_type())->frame_delay)
    {
      DBG_CHECK(bo_phasesnum());
      bo_setphase((bo_phase()+1)%(bo_phasesnum()));
      bo_setstatus(0);
      //czy ostatania
      if (bo_phase()==world->get_level().sprite[bo_sprite()].phases-1)
      {
        bo_setaux1(2);
      }
    }
  }
  if (bo_aux1()==1)
  {
    if (bo_status()>=LevImp::get_parameters(bo_type())->frame_delay)
    {
      bo_setphase((bo_phase()-1>=0)?(bo_phase()-1):(bo_phasesnum()-1));
      bo_setstatus(0);
      //czy ostatania
      if (!bo_phase())
      {
        bo_setaux1(3);
      }
    }

  }
}
//SROTATRIGHT BUILDER
void SRotatRight::builder(void)
{
  if (!bo_status())
  {
    //waux1 pierwsza faza
    bo_setaux1(bo_phase());
    bo_setstatus(1);
    bo_setaux2(bo_aux2()+1);
  }
  DBG_CHECK(bo_phasesnum());

  bo_setphase((Game::get_timer()/(LevImp::get_parameters(bo_type())->sync_frame+1)+bo_aux1())%bo_phasesnum());
  if (bo_phase()==bo_aux1())
  {
    bo_setaux2(bo_aux2()+1);
    if (bo_aux2()>=LevImp::get_parameters(bo_type())->repeat_count)
    {
      bo_settype(bo_type()+LevImp::get_parameters(bo_type())->change_type);
      bo_setaux2(0);
    }
  }
}
//SROTATLEFT BUILDER
void SRotatLeft::builder(void)
{
  if (!bo_status())
  {
    bo_setaux1(bo_phase());
    bo_setstatus(1);
    bo_setaux2(bo_aux2()+1);
  }
  DBG_CHECK(bo_phasesnum());

  bo_setphase((world->get_level().sprite[bo_sprite()].phases-1)-(Game::get_timer()/(LevImp::get_parameters(bo_type())->sync_frame+1)+bo_aux1())%bo_phasesnum());
  if (bo_phase()==bo_aux1())
  {
    bo_setaux2(bo_aux2()+1);
    if (bo_aux2()>=LevImp::get_parameters(bo_type())->repeat_count)
    {
      bo_settype(bo_type()+LevImp::get_parameters(bo_type())->change_type);
      bo_setaux2(0);
    }
  }
}
//SPINGPONG BUILDER
void SPingPong::builder(void)
{
  if (!bo_status())
  {
    bo_setaux1(bo_phase());
    bo_setstatus(1);
    bo_setaux2(bo_aux2()+1);
  }
  DBG_CHECK(bo_phasesnum());
  int temp=(Game::get_timer()/(LevImp::get_parameters(bo_type())->sync_frame+1)+bo_aux1())%(2*bo_phasesnum());
  if (temp>=bo_phasesnum())
  {
    bo_setphase(2*bo_phasesnum()-temp-1);
  }
  else
  {
    bo_setphase(temp);
  }
}
//PREPARE BUILDER
void Prepare::builder(void)
{
  bo_settype(bo_type()+LevImp::get_parameters(bo_type())->change_type);
}

