#include "headers.h"

static int _get_time(float time_seconds) {return (int)(time_seconds*Mp::METRONQUALITY);}
static int _get_turnspeed(float turn_speed) {return (int)(turn_speed*0x10000);}
static int _get_brkpwr(float speed) {return (int)(speed*0x10000);}
static int _get_angle(int angle) {return (angle*Mp::TURNQUALITY)/360;}

//KLASA MP
int   Mp::DEBUG;
int   Mp::DEBUGKEYS;
int   Mp::BUILDSPRITESMODE;
int   Mp::BEYONDTIME;
int   Mp::AUTOFLYMODE;
int   Mp::NETDEBUGLEVEL;
int   Mp::RANDDEBUGLEVEL;
int   Mp::SHIPDEBUGLEVEL;
int   Mp::MEMORYDEBUG;
int   Mp::SYNCFAIL;
int   Mp::SYNCFAILABORT;
int   Mp::OVERSCANX;
int   Mp::OVERSCANY;
int   Mp::MAXOVERSCAN;
int   Mp::DIRNUM;
int   Mp::COLISQUEUESIZE;
int   Mp::KILLQUEUESIZE;
int   Mp::TURNSPEED;
int   Mp::CLIMBSPEED;
int   Mp::PIXELSBETWEENPLANES;
int   Mp::SHADOWDIST;
int   Mp::LINKPRECISION;
int   Mp::HALFHEIGHT;
int   Mp::RADARDISTANCE;
int   Mp::SYNCFRAME;
int   Mp::TIMEDELAY;
int   Mp::FRAMEDELAY;
int   Mp::MINVX;
int   Mp::MINVY;
int   Mp::LOWVOICE;
int   Mp::RADARALIENRAY;
int   Mp::HIRESAVAIL;
int   Mp::BACKGROUND;
int   Mp::LOADSAMPLE;


void Mp::init(char* filename)
{
  Text text;
  text.load(filename);

  DEBUG=                 0;
  DEBUGKEYS=             0;
  AUTOFLYMODE=           0;
  BUILDSPRITESMODE=      0;
  NETDEBUGLEVEL=         0;
  RANDDEBUGLEVEL=        0;
  SHIPDEBUGLEVEL=        0;
  MEMORYDEBUG=           0;
  SYNCFAIL=              0;
  SYNCFAILABORT=         0;

  DEBUG=!Comm::production;
  if (DEBUG)
  {
    if (Cmd_line::get_int("check",0))
    {
      BUILDSPRITESMODE=1;
    }
    AUTOFLYMODE=             text.value("AUTOFLYMODE");
    MEMORYDEBUG=             text.value("MEMORYDEBUG");
    if (Cmd_line::get_int("memorydebug",0))
    {
      MEMORYDEBUG=1;
    }
    DEBUGKEYS=               text.value("DEBUGKEYS");
    if (Cmd_line::get_int("debugkeys",0))
    {
      DEBUGKEYS=1;
    }
    if (NoNet::is_network_mode())
    {
      NETDEBUGLEVEL=         text.value("NETDEBUGLEVEL");
      if (Cmd_line::get_int("netdebug",0))
      {
        NETDEBUGLEVEL=200;
      }
      RANDDEBUGLEVEL=        text.value("RANDDEBUGLEVEL");
      if (Cmd_line::get_int("randdebug",0))
      {
        RANDDEBUGLEVEL=200;
      }
      SHIPDEBUGLEVEL=        text.value("SHIPDEBUGLEVEL");
      if (Cmd_line::get_int("shipdebug",0))
      {
        SHIPDEBUGLEVEL=1000;
      }
      SYNCFAIL=             text.value("SYNCFAIL");
      if (Cmd_line::get_int("syncfail",0))
      {
        SYNCFAIL=1;
      }
      SYNCFAILABORT=        text.value("SYNCFAILABORT");
      if (Cmd_line::get_int("syncfailabort",0))
      {
        SYNCFAILABORT=1;
      }
    }
  }
  OVERSCANX=               text.value("OVERSCANX");
  OVERSCANY=               text.value("OVERSCANY");
  DBG_CHECK(2*(OVERSCANX/2)==OVERSCANX);
  DBG_CHECK(2*(OVERSCANY/2)==OVERSCANY);
  MAXOVERSCAN=(OVERSCANX>OVERSCANY)?OVERSCANX:OVERSCANY;
  DIRNUM=                  text.value("DIRNUM");
  COLISQUEUESIZE=          text.value("COLISQUEUESIZE");
  KILLQUEUESIZE=           text.value("KILLQUEUESIZE");
  TURNSPEED=               _get_turnspeed(text.fp_value("TURNSPEED"));
  CLIMBSPEED=              text.value("CLIMBSPEED");
  PIXELSBETWEENPLANES=     text.value("PIXELSBETWEENPLANES");
  SHADOWDIST=              text.value("SHADOWDIST");
  LINKPRECISION=           text.value("LINKPRECISION");
  SYNCFRAME=               text.value("SYNCFRAME");
  TIMEDELAY=               text.value("TIMEDELAY");
  FRAMEDELAY=              text.value("FRAMEDELAY");
  LOWVOICE=                text.value("LOWVOICE");
  BEYONDTIME=              _get_time(text.fp_value("BEYONDTIME"));
  RADARALIENRAY=           text.value("RADARALIENRAY");
  RADARDISTANCE=           text.value("RADARDISTANCE");
  BACKGROUND=              text.value("BACKGROUND");

  //UWAGA: te obliczenia trzeba zostawic na koncu i uwazac
  //aby wszystko liczyc w odpowiedniej kolejnosci.
  HALFHEIGHT=              PIXELSBETWEENPLANES/2;
  MINVX=                   (1<<16)/Mp::METRONQUALITY;
  MINVY=                   (1<<16)/Mp::METRONQUALITY;
  HIRESAVAIL=              (RegData::DataToLoad!=RegData::load_Lores);
  LOADSAMPLE=              RegData::UseDialogs;
}
//KLASA OP
Text Op::text;
Text Op::typetext;

Op::ObjPack  Op::obj_enemy[MAX_OBJTYPES];
Op::ObjPack  Op::obj_mboss[MAX_OBJTYPES];
Op::CannonPack  Op::def_cannon[MAX_CANNONTYPES];

Op::DefPack  Op::def_enemy[MAX_OBJTYPES];
Op::DefPack  Op::def_mboss[MAX_OBJTYPES];
Op::ObjPack  Op::obj_weaponary[MAX_WEAPON];
int  Op::weaponary_begin[MAX_WEAPON];
int  Op::weaponary_net_begin[MAX_WEAPON];



//MYSHIP
Op::ObjPack Op::myship;
int   Op::MYSHIPFLASHTIME;
int   Op::MYSHIPCLOAKTIME;
int   Op::MYSHIPSHIELDTIME;
int   Op::MYSHIPTURBOMAXSPEED;
int   Op::MYSHIPTURBOSTRAFESPEED;
int   Op::MYSHIPTURBOTURNSPEED;
int   Op::MYSHIPCOLSNDSPEED;
int   Op::MYSHIPDESTROYCOLSPEED;
int   Op::MYSHIPDESTROYCOLAMOUNT;
int   Op::MYSHIPENABLEFUTERKO;

//DISPINFO
int   Op::DISPINFOCOLORS;
int   Op::DISPINFOBEGCOLOR;
int   Op::DISPINFOORGX;
int   Op::DISPINFOORGY;
int   Op::DISPINFOWEAPX;
int   Op::DISPINFOWEAPY;
int   Op::DISPINFOWEAPSX;
int   Op::DISPINFOWEAPSY;
int   Op::DISPINFOWEAPINRECTX;
int   Op::DISPINFOWEAPINRECTY;
int   Op::DISPINFOWEAPSPACEX;
int   Op::DISPINFOLIFEX;
int   Op::DISPINFOLIFEY;
int   Op::DISPINFOMARGINX;
int   Op::DISPINFOMARGINY;
int   Op::DISPINFOLIFERECTSX;
int   Op::DISPINFOLIFERECTSY;
int   Op::DISPINFOLIFERECTSPACEX;
int   Op::DISPINFOINVX;
int   Op::DISPINFOINVY;
int   Op::DISPINFOINVSX;
int   Op::DISPINFOINVSY;
int   Op::DISPINFOINVINRECTX;
int   Op::DISPINFOINVINRECTY;
//WEAPON
int   Op::WEAPONALIENANGLE;
int   Op::WEAPONALIENRANGE;
int   Op::WEAPONDESTROYANGLE;
int   Op::WEAPONDESTROYRANGE;
int   Op::WEAPONMORPHPHASES;
int   Op::WEAPONAUTO1[10];
int   Op::WEAPONAUTO2[10];
int   Op::WEAPONAUTO3[10];
int   Op::WEAPONAUTO4[10];
int   Op::WEAPONAUTO5[10];
int   Op::WEAPONAUTO6[10];

//MYSPLASH
int   Op::MYSPLASHLIFETIME;
//FADE
int   Op::FADEFADETIME;
int   Op::FADETEXTY;
//USER
int   Op::USERLIFEUPGRAMOUNT;
int   Op::USERLIFEUPGRTIME;
//INVENTORY
int   Op::INVENTMAXMINE;
int   Op::INVENTMAXGOLDENEYE;
char *Op::INVENTDESCRIPT[24];

//SCRCATCH
Op::ObjPack Op::scrcatch;
int   Op::SCRCATCHRADIUS;
//COCA
Op::ObjPack Op::cocarocket;
//TARAN
int   Op::TARANFLYTIME;
int   Op::TARANATTACKTIME;
//SIMPLE1
int   Op::SIMPLE1APPROX;
int   Op::SIMPLE1TARGETTIME;
int   Op::SIMPLE1SHOOTTIME;
int   Op::SIMPLE1SHOOT;
//SIMPLE2
Op::ObjPack Op::simple2_splash;
int   Op::SIMPLE2SHOOTTIME;
int   Op::SIMPLE2SSPLASH;
//MBOSS1
Op::ObjPack Op::mb1splash;
int   Op::MB1TARGETTIME;
int   Op::MB1APROXPOINT;
int   Op::MB1SHFRQ;
//MBOSS2
Op::ObjPack Op::mboss2rocket;
int   Op::MB2AAP;
int   Op::MB2TTIME;
int   Op::MB2FLYTIME;
int   Op::MB2WALLCOLIS;
int   Op::MB2SHOOTCOUNT;
int   Op::MB2SHFRQ;
//MBOSS3
Op::ObjPack Op::mb3bullet;
int   Op::MBOSS3AAP;
int   Op::MBOSS3TTIME;
int   Op::MBOSS3FLYTIME;
int   Op::MBOSS3SHTIME;
int   Op::MBOSS3WALLCOLIS;
int   Op::MBOSS3FRQ;

//MBOSS4
int   Op::MBOSS4AAP;
int   Op::MBOSS4FLYTIME;
int   Op::MBOSS4SHTIME;
int   Op::MBOSS4TTIME;
int   Op::MBOSS4FRQ;
int   Op::MBOSS4TARANLIFETIME;
//CANNONROCKETROCKET
Op::ObjPack   Op::CANNONROCKET;
//LANDPLACE
int   Op::LANDDISTANCE;
int   Op::LANDSPEED;
int   Op::LANDTIME;
//TELEPORT
int   Op::TELEPORTSTARTDISTANCE;
int   Op::TELEPORTINUSEDISTANCE;
//DESTROY
int   Op::DESTROYLIFEHIGH;
int   Op::DESTROYLIFEMEDIUM;
int   Op::DESTROYLIFELOW;
int   Op::DESTROYIMMORTAL;
int   Op::DESTROYMORPHTIME;



//MAGNES
int   Op::MAGNESZEROPWR;
int   Op::MAGNESEDGEPWR;
int   Op::MAGNESLIFE;
//EXPL
int   Op::EXPLTIME;
//MYROCKET
int   Op::MYROCKETLIFETIME;
int   Op::MYROCKETIDLETIME;
int   Op::MYROCKETALIENANGLE;
int   Op::MYROCKETALIENRANGE;
int   Op::MYROCKETDESTROYANGLE;
int   Op::MYROCKETDESTROYRANGE;
//MYBOMB
int   Op::MYBOMBLIFETIME;
int   Op::MYBOMBSIZE;
int   Op::MYBOMBRADIUS;
int   Op::MYBOMBPOWER;
int   Op::MYBOMBSHAKETIME;
//SHIPFUCK
int   Op::SHIPFUCKTIME;
int   Op::SHIPFUCKPOWER;
//MYSWARMER
int   Op::MYSWARMERLIFETIME;
int   Op::MYSWARMERINITTIME;
int   Op::MYSWARMERSTARTDANGLE;
int   Op::MYSWARMERTRANSANGLE;
//MYLASER
int   Op::MYLASERLIFETIME;
//CAPSULE
int   Op::W1BEG[3];
int   Op::W1CNT[3];
int   Op::W2BEG[3];
int   Op::W2CNT[3];
int   Op::W3BEG[3];
int   Op::W3CNT[3];
int   Op::W4BEG[3];
int   Op::W4CNT[3];
int   Op::W5BEG[3];
int   Op::W5CNT[3];
int   Op::W6BEG[3];
int   Op::W6CNT[3];
int   Op::LIBEG[3];
int   Op::LICNT[3];
int   Op::MKBEG;
int   Op::MKCNT;
int   Op::IMBEG;
int   Op::IMCNT;
int   Op::CLBEG;
int   Op::CLCNT;
int   Op::NMBEG;
int   Op::NMCNT;
int   Op::GEBEG;
int   Op::GECNT;
int   Op::CHBEG;
int   Op::CHCNT;
//BRICK
Op::ObjPack Op::brick;


int   Op::max_weapon[MAX_WEAPON];
int   Op::weapon_power[MAX_WEAPON];
int   Op::weapon_ugr_max[MAX_WEAPON];
int   Op::weapon_ugr_med[MAX_WEAPON];
int   Op::weapon_ugr_min[MAX_WEAPON];
int   Op::weaponary_frq[MAX_WEAPON];
int   Op::shields[3];
int   Op::DESTSMOKEFRQ;
int   Op::DESTSMOKEAAP;
int   Op::DESTSMOKEPRC;
int   Op::DESTSMOKECH;
int   Op::OBJCTIVETIME;
int   Op::OBJCTIVEDOWNJUST;

//MYMINE
int   Op::MYMINERANGE;


//MINE
Op::ObjPack Op::MINE;
int  Op::MINENONACTIVETIME;

//CANNONSPLASH
Op::ObjPack Op::CANNONSPLASH;
int         Op::CANNONSPLASHFRQ;

 //NETCANNON
Op::ObjPack Op::NETCANNON1;
Op::ObjPack Op::NETCANNON2;
int    Op::NETCANNON1POWER;
int    Op::NETCANNON1PASSIVE;
int    Op::NETCANNON2PASSIVE;
int    Op::NETCANNON1ACTIVE;
int    Op::NETCANNON2ACTIVE;
int    Op::NETCANNON2POWER;

int    Op::BASELIFE;
int    Op::MAXSHIPBRICK;
int    Op::MAXLEVELBRICK;

int    Op::SHOTGUNCHPOWER;

//THIEF
Op::ObjPack    Op::THIEF;

//RAFT
Op::ObjPack    Op::RAFT;
int    Op::RAFTMINEPOWER;
int    Op::RAFTMINELIFE;

//SOUNDS
int    Op::R1;
int    Op::R2;
int    Op::MIN_VOL;
int    Op::NETR1;
int    Op::NETR2;
int    Op::NETMIN_VOL;


//COLLECT
int    Op::COLLECTSPEED;
int    Op::COLLECTRADIUS;
int    Op::COLLECTTIME;

//HEADER
int    Op::HEADERFRAMETIME;
int    Op::HEADERTEXTSPEED;

//SPLINTER
int    Op::SPLINTERTIME;
int    Op::SPLINTERSPEED1;
int    Op::SPLINTERSPEED2;
int    Op::SPLINTERSPEED3;

int    Op::SPLINTERFALL;
int    Op::SPLINTERPIX;

//BULLET
Op::ObjPack    Op::bullet;
//MORPH
Op::ObjPack    Op::morph;
//SPLINTER
Op::ObjPack    Op::splinter;
//SMOKE
Op::ObjPack    Op::smoke;

 //COUNTDOWN
int  Op::COUNTDOWNX;
int  Op::COUNTDOWNY;
int  Op::COUNTDOWNFRQ1;
int  Op::COUNTDOWNFRQ2;
int  Op::COUNTDOWNFRQ3;

 //STATISTICS
int Op::STATISTICSBONUS;

void Op::load_defpack(DefPack* pack,char* txt)
{
  float tf[12];
  int ti[12];
  char *tmp=NULL;
  
  typetext.area(txt);

  tmp=typetext.string("shields");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_OBJTYPES) pack[RPTCNT].shields=ti[RPTCNT];
  
  tmp=typetext.string("power");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_OBJTYPES) pack[RPTCNT].power=ti[RPTCNT];

  tmp=typetext.string("weapon");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_OBJTYPES) pack[RPTCNT].weapon=ti[RPTCNT];

  tmp=typetext.string("speed");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_OBJTYPES) pack[RPTCNT].speed=ti[RPTCNT];

  tmp=typetext.string("passive");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_OBJTYPES) pack[RPTCNT].passive=ti[RPTCNT];

  tmp=typetext.string("active");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_OBJTYPES) pack[RPTCNT].active=ti[RPTCNT];

  if (!strcmp("miniboss",txt))
  {
    tmp=typetext.string("shoot_frq");
    sscanf(tmp,"%f %f %f %f %f %f %f %f %f %f %f %f",&tf[0],&tf[1],&tf[2],&tf[3],&tf[4],&tf[5],&tf[6],&tf[7],&tf[8],&tf[9],&tf[10],&tf[11]);
    REPEAT(4) pack[RPTCNT].frq=_get_time(tf[RPTCNT]);
  }

  typetext.endarea();
}

void Op::load_objpack(ObjPack* pack,char* txt)
{
  float tf[12];
  int ti[12];
  char *tmp=NULL;

  typetext.area(txt);
  
  tmp=typetext.string("max_speed");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_OBJTYPES) pack[RPTCNT].max_speed=ti[RPTCNT];
  
  tmp=typetext.string("accel_pwr");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_OBJTYPES) pack[RPTCNT].accel_pwr=ti[RPTCNT];

  tmp=typetext.string("brake_pwr");
  sscanf(tmp,"%f %f %f %f %f %f %f %f %f %f %f %f",&tf[0],&tf[1],&tf[2],&tf[3],&tf[4],&tf[5],&tf[6],&tf[7],&tf[8],&tf[9],&tf[10],&tf[11]);
  REPEAT(MAX_OBJTYPES) pack[RPTCNT].brake_pwr=_get_brkpwr(tf[RPTCNT]);

  tmp=typetext.string("turn_speed");
  sscanf(tmp,"%f %f %f %f %f %f %f %f %f %f %f %f",&tf[0],&tf[1],&tf[2],&tf[3],&tf[4],&tf[5],&tf[6],&tf[7],&tf[8],&tf[9],&tf[10],&tf[11]);
  REPEAT(MAX_OBJTYPES) pack[RPTCNT].turn_speed=_get_turnspeed(tf[RPTCNT]);

  tmp=typetext.string("climb_speed");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_OBJTYPES) pack[RPTCNT].climb_speed=ti[RPTCNT];

  if (!strcmpi(txt,"objmboss"))
  {
    pack[0].dirs=pack[1].dirs=pack[2].dirs=pack[3].dirs=Mp::DIRNUM/2;
  }
  else
  {
    pack[0].dirs=pack[1].dirs=pack[2].dirs=pack[3].dirs=Mp::DIRNUM;
    pack[0].strafe_speed=pack[3].strafe_speed=pack[3].strafe_speed=pack[3].strafe_speed=0;
    pack[0].auto_pwr=pack[1].auto_pwr=pack[2].auto_pwr=pack[3].auto_pwr=_get_brkpwr(float(0));
  }

  typetext.endarea();

}

void Op::init_defpack(void)
{
  load_defpack(def_enemy,"enemy");
  load_defpack(def_mboss,"miniboss");
  load_cannonpack(def_cannon,"cannon");
}

void Op::init_objpack(void)
{
  load_objpack(obj_enemy,"objenemy");
  load_objpack(obj_mboss,"objmboss");
}

void Op::compute_params(ObjPack* opack,DefPack* dpack)
{
  for (int i=0;i<MAX_OBJTYPES;i++)
  {
    int param=dpack[i].speed;
    opack[i].life   =dpack[i].shields;
    opack[i].destroy=dpack[i].power;
    opack[i].weapon =dpack[i].weapon;
    opack[i].max_speed=(opack[i].max_speed*param)/100;
    opack[i].turn_speed=(opack[i].turn_speed*param)/100;
    opack[i].accel_pwr=(opack[i].accel_pwr*param)/100;
    opack[i].climb_speed=(opack[i].climb_speed*param)/100;
    opack[i].brake_pwr=(opack[i].brake_pwr*param)/100;
  }
}

void Op::init_objpack(ObjPack& ref)
{
  if (text.exist("speed")) ref.speed=text.value("speed");
  else ref.speed=0;

  if (text.exist("max_speed")) ref.max_speed=text.value("max_speed");
  else ref.max_speed=ref.speed;

  if (text.exist("accel_pwr")) ref.accel_pwr=text.value("accel_pwr");
  else ref.accel_pwr=0;

  if (text.exist("brake_pwr")) ref.brake_pwr=_get_brkpwr(text.fp_value("brake_pwr"));
  else  ref.brake_pwr=0;

  if (text.exist("auto_pwr")) ref.auto_pwr=_get_brkpwr(text.fp_value("auto_pwr"));
  else ref.auto_pwr=0;

  if (text.exist("strafe_speed")) ref.strafe_speed=text.value("strafe_speed");
  else ref.strafe_speed=0;

  if (text.exist("turn_speed")) ref.turn_speed=_get_turnspeed(text.fp_value("turn_speed"));
  else ref.turn_speed=Mp::TURNSPEED;

  if (text.exist("dirs")) ref.dirs=text.value("dirs");
  else  ref.dirs=Mp::DIRNUM;

  if (text.exist("climb_speed")) ref.climb_speed=text.value("climb_speed");
  else ref.climb_speed=Mp::CLIMBSPEED;

  if (text.exist("life")) ref.life=text.value("life");
  else ref.life=0;

  if (text.exist("destroy")) ref.destroy=text.value("destroy");
  else ref.destroy=0;
}

void Op::hero_params(void)
{
  int ti[12];
  char *tmp=NULL;

  if (NoNet::is_network_mode())
  {
    RegData::RespawnInfo &info=(NoNet::is_network_mode(NoNet::NETWORKMATCH))?RegData::RespawnDM:RegData::RespawnBB;
    MYSHIPENABLEFUTERKO=info.WeaponRecycling;
  }
  else
  {
    MYSHIPENABLEFUTERKO=0;
  }

  text.area("myship");
  init_objpack(myship);
  MYSHIPTURBOMAXSPEED=      text.value("turbomaxspeed");
  MYSHIPTURBOSTRAFESPEED=   text.value("turbostrafespeed");
  MYSHIPTURBOTURNSPEED=     _get_turnspeed(text.fp_value("turboturnspeed"));
  MYSHIPCOLSNDSPEED=        text.value("colsndspeed");
  MYSHIPDESTROYCOLSPEED=    text.value("destroycolspeed");
  MYSHIPDESTROYCOLAMOUNT=   text.value("destroycolamount");
  MYSHIPFLASHTIME=          _get_time(text.fp_value("flashtime"));
  MYSHIPCLOAKTIME=          _get_time(text.fp_value("cloaktime"));
  MYSHIPSHIELDTIME=         _get_time(text.fp_value("shieldtime"));
  text.endarea();

  typetext.area("hero");
  myship.max_speed=typetext.value("speed");
  myship.strafe_speed=typetext.value("strafe_speed");
  myship.turn_speed=_get_turnspeed(typetext.fp_value("rotate_speed"));
  myship.accel_pwr=typetext.value("acceleration");
  myship.brake_pwr=_get_brkpwr(typetext.fp_value("brakes"));
  myship.auto_pwr=_get_brkpwr(typetext.fp_value("auto_brakes"));
  myship.life=typetext.value("shields");
  myship.destroy=typetext.value("power");

  tmp=typetext.string("weapon_power");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_WEAPON) weapon_power[RPTCNT]=ti[RPTCNT];

  tmp=typetext.string("weapon_max");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_WEAPON) max_weapon[RPTCNT]=ti[RPTCNT];

  tmp=typetext.string("weapon_upgr_max");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_WEAPON) weapon_ugr_max[RPTCNT]=ti[RPTCNT];

  tmp=typetext.string("weapon_upgr_med");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_WEAPON) weapon_ugr_med[RPTCNT]=ti[RPTCNT];

  tmp=typetext.string("weapon_upgr_min");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_WEAPON) weapon_ugr_min[RPTCNT]=ti[RPTCNT];

  shields[0]=typetext.value("shields_upgr_min");
  shields[1]=typetext.value("shields_upgr_med");
  shields[2]=typetext.value("shields_upgr_max");

  typetext.endarea();
}

void Op::load_cannonpack(CannonPack* pack,char* txt)
{
  float tf[12];
  int ti[12];
  char *tmp=NULL;

  typetext.area(txt);

  tmp=typetext.string("shields");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_CANNONTYPES) pack[RPTCNT].life=ti[RPTCNT];

  tmp=typetext.string("weapon");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_CANNONTYPES) pack[RPTCNT].weaponpower=ti[RPTCNT];

  tmp=typetext.string("speed");
  sscanf(tmp,"%f %f %f %f %f %f %f %f %f %f %f %f",&tf[0],&tf[1],&tf[2],&tf[3],&tf[4],&tf[5],&tf[6],&tf[7],&tf[8],&tf[9],&tf[10],&tf[11]);
  REPEAT(MAX_CANNONTYPES) pack[RPTCNT].turn_speed=_get_turnspeed(tf[RPTCNT]);

  tmp=typetext.string("passive");
  sscanf(tmp,"%f %f %f %f %f %f %f %f %f %f %f %f",&tf[0],&tf[1],&tf[2],&tf[3],&tf[4],&tf[5],&tf[6],&tf[7],&tf[8],&tf[9],&tf[10],&tf[11]);
  REPEAT(MAX_CANNONTYPES) pack[RPTCNT].passive=_get_time(tf[RPTCNT]);

  tmp=typetext.string("active");
  sscanf(tmp,"%f %f %f %f %f %f %f %f %f %f %f %f",&tf[0],&tf[1],&tf[2],&tf[3],&tf[4],&tf[5],&tf[6],&tf[7],&tf[8],&tf[9],&tf[10],&tf[11]);
  REPEAT(MAX_CANNONTYPES) pack[RPTCNT].active=_get_time(tf[RPTCNT]);

  tmp=typetext.string("angle");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_CANNONTYPES) pack[RPTCNT].angle=_get_angle(ti[RPTCNT])/2;

  tmp=typetext.string("gun");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_CANNONTYPES) pack[RPTCNT].gun=ti[RPTCNT];

  typetext.endarea();
}

void Op::init_weaponary(void)
{
  float tf[12];
  int ti[12];
  char *tmp=NULL;

  typetext.area("weaponary");

  tmp=typetext.string("begin");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_WEAPON) weaponary_begin[RPTCNT]=ti[RPTCNT];

  tmp=typetext.string("net_begin");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_WEAPON) weaponary_net_begin[RPTCNT]=ti[RPTCNT];

  tmp=typetext.string("frq");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_WEAPON) weaponary_frq[RPTCNT]=ti[RPTCNT];

  tmp=typetext.string("max_speed");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_WEAPON) obj_weaponary[RPTCNT].max_speed=ti[RPTCNT];

  tmp=typetext.string("speed");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_WEAPON) obj_weaponary[RPTCNT].speed=ti[RPTCNT];

  tmp=typetext.string("accel_pwr");
  sscanf(tmp,"%d %d %d %d %d %d %d %d %d %d %d %d",&ti[0],&ti[1],&ti[2],&ti[3],&ti[4],&ti[5],&ti[6],&ti[7],&ti[8],&ti[9],&ti[10],&ti[11]);
  REPEAT(MAX_WEAPON) obj_weaponary[RPTCNT].accel_pwr=ti[RPTCNT];

  tmp=typetext.string("brake_pwr");
  sscanf(tmp,"%f %f %f %f %f %f %f %f %f %f %f %f",&tf[0],&tf[1],&tf[2],&tf[3],&tf[4],&tf[5],&tf[6],&tf[7],&tf[8],&tf[9],&tf[10],&tf[11]);
  REPEAT(MAX_WEAPON) obj_weaponary[RPTCNT].brake_pwr=_get_brkpwr(tf[RPTCNT]);

  tmp=typetext.string("turn_speed");
  sscanf(tmp,"%f %f %f %f %f %f %f %f %f %f %f %f",&tf[0],&tf[1],&tf[2],&tf[3],&tf[4],&tf[5],&tf[6],&tf[7],&tf[8],&tf[9],&tf[10],&tf[11]);
  REPEAT(MAX_WEAPON) obj_weaponary[RPTCNT].turn_speed=_get_turnspeed(tf[RPTCNT]);

  REPEAT(MAX_WEAPON) obj_weaponary[RPTCNT].dirs=Mp::DIRNUM;
  REPEAT(MAX_WEAPON) obj_weaponary[RPTCNT].auto_pwr=_get_brkpwr(float(0));
  REPEAT(MAX_WEAPON) obj_weaponary[RPTCNT].climb_speed=0;
  REPEAT(MAX_WEAPON) obj_weaponary[RPTCNT].strafe_speed=0;

  typetext.endarea();
}

void Op::init(char* filename)
{
  text.load(filename);
  typetext.load("set_parameters");
  init_defpack();
  init_objpack();
  init_weaponary();
  compute_params(obj_enemy,def_enemy);
  compute_params(obj_mboss,def_mboss);
  hero_params();

  //DISPINFO
  text.area("dispinfo");
  DISPINFOCOLORS=           text.value("COLORS");
  DISPINFOBEGCOLOR=         text.value("BEGCOLOR");
  DISPINFOORGX=             text.value("ORGX");
  DISPINFOORGY=             text.value("ORGY");
  DISPINFOWEAPX=            text.value("WEAPX");
  DISPINFOWEAPY=            text.value("WEAPY");
  DISPINFOWEAPSX=           text.value("WEAPSX");
  DISPINFOWEAPSY=           text.value("WEAPSY");
  DISPINFOWEAPINRECTX=      text.value("WEAPINRECTX");
  DISPINFOWEAPINRECTY=      text.value("WEAPINRECTY");
  DISPINFOWEAPSPACEX=       text.value("WEAPSPACEX");
  DISPINFOLIFEX=            text.value("LIFEX");
  DISPINFOLIFEY=            text.value("LIFEY");
  DISPINFOMARGINX=          text.value("MARGINX");
  DISPINFOMARGINY=          text.value("MARGINY");
  DISPINFOLIFERECTSX=       text.value("LIFERECTSX");
  DISPINFOLIFERECTSY=       text.value("LIFERECTSY");
  DISPINFOLIFERECTSPACEX=   text.value("LIFERECTSPACEX");
  DISPINFOINVX=             text.value("INVX");
  DISPINFOINVY=             text.value("INVY");
  DISPINFOINVSX=            text.value("INVSX");
  DISPINFOINVSY=            text.value("INVSY");
  DISPINFOINVINRECTX=       text.value("INVINRECTX");
  DISPINFOINVINRECTY=       text.value("INVINRECTY");
  text.endarea();

  //FADE
  text.area("fade");
  FADEFADETIME=        (int)(text.fp_value("FADETIME")*Mp::METRONQUALITY);
  FADETEXTY=           text.value("TEXTY");
  text.endarea();

  //USER
  text.area("user");
  USERLIFEUPGRAMOUNT=  text.value("LIFEUPGRAMOUNT");
  USERLIFEUPGRTIME=    (int)(text.fp_value("LIFEUPGRTIME")*Mp::METRONQUALITY);
  text.endarea();

  //INVENTORY
  text.area("inventory");
  INVENTMAXMINE=       text.value("MAXMINE");
  INVENTMAXGOLDENEYE=  text.value("MAXGOLDENEYE");
  int inv_size=text.size("DESCRIPT");
  DBG_CHECK(inv_size>0&&inv_size<24);
  int inv_i;
  for (inv_i=0;inv_i<24;inv_i++)
  {
    INVENTDESCRIPT[inv_i]=NULL;
  }
  for (inv_i=0;inv_i<inv_size;inv_i++)
  {
    INVENTDESCRIPT[inv_i]=text.string("DESCRIPT",inv_i);
  }
  text.endarea();

  //SCRCATCH
  text.area("scrcatch");
  init_objpack(scrcatch);
  SCRCATCHRADIUS=          text.value("radius");
  text.endarea();

  //WEAPON
  text.area("weapon");
  WEAPONALIENANGLE=        (text.value("ALIENANGLE")*Mp::TURNQUALITY)/360;
  WEAPONALIENRANGE=        text.value("ALIENRANGE");
  WEAPONDESTROYANGLE=      (text.value("DESTROYANGLE")*Mp::TURNQUALITY)/360;
  WEAPONDESTROYRANGE=      text.value("DESTROYRANGE");
  WEAPONMORPHPHASES=       text.value("MORPHPHASES");
  int size=0;

  size=sscanf(text.string("weap1_auto"),
              "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
               &WEAPONAUTO1[0],
               &WEAPONAUTO1[1],
               &WEAPONAUTO1[2],
               &WEAPONAUTO1[3],
               &WEAPONAUTO1[4],
               &WEAPONAUTO1[5],
               &WEAPONAUTO1[6],
               &WEAPONAUTO1[7],
               &WEAPONAUTO1[8],
               &WEAPONAUTO1[9]);
  CHECK(size>0&&size<10);
  WEAPONAUTO1[size]=0;

  size=sscanf(text.string("weap2_auto"),
              "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
               &WEAPONAUTO2[0],
               &WEAPONAUTO2[1],
               &WEAPONAUTO2[2],
               &WEAPONAUTO2[3],
               &WEAPONAUTO2[4],
               &WEAPONAUTO2[5],
               &WEAPONAUTO2[6],
               &WEAPONAUTO2[7],
               &WEAPONAUTO2[8],
               &WEAPONAUTO2[9]);
  CHECK(size>0&&size<10);
  WEAPONAUTO2[size]=0;

  size=sscanf(text.string("weap3_auto"),
              "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
               &WEAPONAUTO3[0],
               &WEAPONAUTO3[1],
               &WEAPONAUTO3[2],
               &WEAPONAUTO3[3],
               &WEAPONAUTO3[4],
               &WEAPONAUTO3[5],
               &WEAPONAUTO3[6],
               &WEAPONAUTO3[7],
               &WEAPONAUTO3[8],
               &WEAPONAUTO3[9]);
  CHECK(size>0&&size<10);
  WEAPONAUTO3[size]=0;

  size=sscanf(text.string("weap4_auto"),
              "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
               &WEAPONAUTO4[0],
               &WEAPONAUTO4[1],
               &WEAPONAUTO4[2],
               &WEAPONAUTO4[3],
               &WEAPONAUTO4[4],
               &WEAPONAUTO4[5],
               &WEAPONAUTO4[6],
               &WEAPONAUTO4[7],
               &WEAPONAUTO4[8],
               &WEAPONAUTO4[9]);
  CHECK(size>0&&size<10);
  WEAPONAUTO4[size]=0;

  size=sscanf(text.string("weap5_auto"),
              "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
               &WEAPONAUTO5[0],
               &WEAPONAUTO5[1],
               &WEAPONAUTO5[2],
               &WEAPONAUTO5[3],
               &WEAPONAUTO5[4],
               &WEAPONAUTO5[5],
               &WEAPONAUTO5[6],
               &WEAPONAUTO5[7],
               &WEAPONAUTO5[8],
               &WEAPONAUTO5[9]);
  CHECK(size>0&&size<10);
  WEAPONAUTO5[size]=0;

  size=sscanf(text.string("weap6_auto"),
              "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
               &WEAPONAUTO6[0],
               &WEAPONAUTO6[1],
               &WEAPONAUTO6[2],
               &WEAPONAUTO6[3],
               &WEAPONAUTO6[4],
               &WEAPONAUTO6[5],
               &WEAPONAUTO6[6],
               &WEAPONAUTO6[7],
               &WEAPONAUTO6[8],
               &WEAPONAUTO6[9]);
  CHECK(size>0&&size<10);
  WEAPONAUTO6[size]=0;

  text.endarea();

  //MYSPLASH
  text.area("mysplash");
  MYSPLASHLIFETIME=       (int)(text.fp_value("LIFETIME")*Mp::METRONQUALITY);
  text.endarea();

  //MYSWARMER
  text.area("myswarmer");
  MYSWARMERLIFETIME=       (int)(text.fp_value("LIFETIME")*Mp::METRONQUALITY);
  MYSWARMERINITTIME=       (int)(text.fp_value("INITTIME")*Mp::METRONQUALITY);
  MYSWARMERSTARTDANGLE=    (text.value("STARTDANGLE")*Mp::TURNQUALITY)/360;
  MYSWARMERTRANSANGLE=     (text.value("TRANSANGLE")*Mp::TURNQUALITY)/360;
  text.endarea();

  //MYROCKET
  text.area("myrocket");
  MYROCKETLIFETIME=       _get_time(text.fp_value("LIFETIME"));
  MYROCKETIDLETIME=       _get_time(text.fp_value("IDLETIME"));
  MYROCKETALIENANGLE=     (text.value("ALIENANGLE")*Mp::TURNQUALITY)/360;
  MYROCKETALIENRANGE=     text.value("ALIENRANGE");
  MYROCKETDESTROYANGLE=   (text.value("DESTROYANGLE")*Mp::TURNQUALITY)/360;
  MYROCKETDESTROYRANGE=   text.value("DESTROYRANGE");
  text.endarea();

  //MYBOMB
  text.area("mybomb");
  MYBOMBLIFETIME=         (int)(text.fp_value("LIFETIME")*Mp::METRONQUALITY);
  MYBOMBSIZE=             text.value("SIZE");
  MYBOMBRADIUS=           text.value("RADIUS");
  MYBOMBPOWER=            text.value("POWER");
  MYBOMBSHAKETIME=        (int)(text.fp_value("SHAKETIME")*Mp::METRONQUALITY);
  text.endarea();

  //SHIPFUCK
  text.area("rebeliant");
  SHIPFUCKTIME=           (int)(text.fp_value("TIME")*Mp::METRONQUALITY);
  SHIPFUCKPOWER=          text.value("POWER");
  text.endarea();

  //CAPSULE
  text.area("capsule");
  sscanf(text.string("wpn1_min"),"%d,%d",&W1BEG[0],&W1CNT[0]);
  sscanf(text.string("wpn1_med"),"%d,%d",&W1BEG[1],&W1CNT[1]);
  sscanf(text.string("wpn1_max"),"%d,%d",&W1BEG[2],&W1CNT[2]);
  sscanf(text.string("wpn2_min"),"%d,%d",&W2BEG[0],&W2CNT[0]);
  sscanf(text.string("wpn2_med"),"%d,%d",&W2BEG[1],&W2CNT[1]);
  sscanf(text.string("wpn2_max"),"%d,%d",&W2BEG[2],&W2CNT[2]);
  sscanf(text.string("wpn3_min"),"%d,%d",&W3BEG[0],&W3CNT[0]);
  sscanf(text.string("wpn3_med"),"%d,%d",&W3BEG[1],&W3CNT[1]);
  sscanf(text.string("wpn3_max"),"%d,%d",&W3BEG[2],&W3CNT[2]);
  sscanf(text.string("wpn4_min"),"%d,%d",&W4BEG[0],&W4CNT[0]);
  sscanf(text.string("wpn4_med"),"%d,%d",&W4BEG[1],&W4CNT[1]);
  sscanf(text.string("wpn4_max"),"%d,%d",&W4BEG[2],&W4CNT[2]);
  sscanf(text.string("wpn5_min"),"%d,%d",&W5BEG[0],&W5CNT[0]);
  sscanf(text.string("wpn5_med"),"%d,%d",&W5BEG[1],&W5CNT[1]);
  sscanf(text.string("wpn5_max"),"%d,%d",&W5BEG[2],&W5CNT[2]);
  sscanf(text.string("wpn6_min"),"%d,%d",&W6BEG[0],&W6CNT[0]);
  sscanf(text.string("wpn6_med"),"%d,%d",&W6BEG[1],&W6CNT[1]);
  sscanf(text.string("wpn6_max"),"%d,%d",&W6BEG[2],&W6CNT[2]);
  sscanf(text.string("life_min"),"%d,%d",&LIBEG[0],&LICNT[0]);
  sscanf(text.string("life_med"),"%d,%d",&LIBEG[1],&LICNT[1]);
  sscanf(text.string("life_max"),"%d,%d",&LIBEG[2],&LICNT[2]);
  sscanf(text.string("medikit"),"%d,%d",&MKBEG,&MKCNT);
  sscanf(text.string("immortal"),"%d,%d",&IMBEG,&IMCNT);
  sscanf(text.string("cloak"),"%d,%d",&CLBEG,&CLCNT);
  sscanf(text.string("netmine"),"%d,%d",&NMBEG,&NMCNT);
  sscanf(text.string("goldeneye"),"%d,%d",&GEBEG,&GECNT);
  sscanf(text.string("chaos"),"%d,%d",&CHBEG,&CHCNT);
  text.endarea();

  //BRICK
  text.area("brick");
  init_objpack(brick);
  text.endarea();

  //COCA
  text.area("simple3");
  text.area("shoot");
  init_objpack(cocarocket);
  text.endarea();
  text.endarea();
  //TARAN
  text.area("taran");
  TARANFLYTIME=            def_enemy[Op::TARAN].passive*Mp::METRONQUALITY;
  TARANATTACKTIME=         def_enemy[Op::TARAN].active*Mp::METRONQUALITY;
  text.endarea();
  //SIMPLE1
  text.area("simple1");
  SIMPLE1APPROX=           text.value("approx");
  SIMPLE1TARGETTIME=       (int)(text.fp_value("TARGETTIME")*Mp::METRONQUALITY);

  SIMPLE1SHOOTTIME=        def_enemy[Op::SIMPLE1].passive*Mp::METRONQUALITY;
  text.area("shoot");
  SIMPLE1SHOOT=            text.value("destroy");
  text.endarea();
  text.endarea();

  //SIMPLE2
  text.area("simple2");
  SIMPLE2SHOOTTIME=        def_enemy[Op::SIMPLE2].passive*Mp::METRONQUALITY;
  text.area("shoot");
  init_objpack(simple2_splash);
  text.endarea();
  text.endarea();

  //MBOSS1
  text.area("mboss1");
  MB1SHFRQ=                  (int)(text.fp_value("SHOOTFRQ")*Mp::METRONQUALITY);
  MB1TARGETTIME=             (int)(text.fp_value("TARGETTIME")*Mp::METRONQUALITY);
  MB1APROXPOINT=             text.value("APROXPOINT");
  text.area("shoot");
  init_objpack(mb1splash);
  text.endarea();

  text.endarea();
  //MBOSS2
  text.area("mboss2");
  MB2AAP=                    text.value("APROXPOINT");
  MB2TTIME=                  (int)(text.fp_value("TTIME")*Mp::METRONQUALITY);
  MB2FLYTIME=                def_mboss[Op::MBOSS2].passive*Mp::METRONQUALITY;
  MB2WALLCOLIS=              (int)(text.fp_value("WALLCOLIS")*Mp::METRONQUALITY);
  MB2SHFRQ=                  (int)(text.fp_value("SHOOTFRQ")*Mp::METRONQUALITY);
  MB2SHOOTCOUNT=             (int)(((float)def_mboss[Op::MBOSS2].active)/(text.fp_value("SHOOTFRQ")));
  text.area("rocket");
  init_objpack(mboss2rocket);
  text.endarea();
  text.endarea();

  //MBOSS3
  text.area("mboss3");
  MBOSS3AAP=                 text.value("APROXPOINT");
  MBOSS3TTIME=               (int)(text.fp_value("TTIME")*Mp::METRONQUALITY);
  MBOSS3FLYTIME=             def_mboss[Op::MBOSS3].passive*Mp::METRONQUALITY;
  MBOSS3SHTIME=              def_mboss[Op::MBOSS3].active*Mp::METRONQUALITY;
  MBOSS3WALLCOLIS=           (int)(text.fp_value("WALLCOLIS")*Mp::METRONQUALITY);
  MBOSS3FRQ=                 (int)(text.fp_value("SHOOTFRQ")*Mp::METRONQUALITY);
  text.area("shoot");
  init_objpack(mb3bullet);
  text.endarea();
  text.endarea();

  //MBOSS4
  text.area("mboss4");
  MBOSS4AAP=                text.value("APROXPOINT");
  MBOSS4FLYTIME=            def_mboss[Op::MBOSS4].passive*Mp::METRONQUALITY;;
  MBOSS4SHTIME=             def_mboss[Op::MBOSS4].active*Mp::METRONQUALITY;
  MBOSS4TTIME=              (int)(text.fp_value("TTIME")*Mp::METRONQUALITY);
  MBOSS4FRQ=                (int)(text.fp_value("SHOOTFRQ")*Mp::METRONQUALITY);
  text.area("shoot");
  MBOSS4TARANLIFETIME=      (int)(text.fp_value("timelife")*Mp::METRONQUALITY);
  text.endarea();
  text.endarea();


  //MYLASER
  text.area("mylaser");
  MYLASERLIFETIME=        (int)(text.fp_value("LIFETIME")*Mp::METRONQUALITY);
  text.endarea();

  //LANDPLACE
  text.area("landplace");
  LANDDISTANCE=            text.value("landdistance");
  LANDSPEED=               text.value("landspeed");
  LANDTIME=                (int)(text.fp_value("landtime")*Mp::METRONQUALITY);
  text.endarea();
  //TELEPORT
  text.area("teleport");
  TELEPORTSTARTDISTANCE=  text.value("startdistance");
  TELEPORTINUSEDISTANCE=  text.value("inusedistance");
  text.endarea();
  //CANNONROCKET
  text.area("CannonRocket");
  init_objpack(CANNONROCKET);
  text.endarea();
  //DESTROY
  typetext.area("destroy");
  DESTROYLIFEHIGH=         typetext.value("HIGH_SHIELDS");
  DESTROYLIFEMEDIUM=       typetext.value("MEDIUM_SHIELDS");
  DESTROYLIFELOW=          typetext.value("LOW_SHIELDS");
  DESTROYIMMORTAL=         99999999;
  typetext.endarea();
  text.area("destroy");
  DESTROYMORPHTIME=        (int)(text.fp_value("morph_time")*Mp::METRONQUALITY);
  text.endarea();
  //MAGNES
  MAGNESZEROPWR=           text.value("MAGNESZEROPWR");
  MAGNESEDGEPWR=           text.value("MAGNESEDGEPWR");
  MAGNESLIFE=              text.value("MAGNESLIFE");
  //EXPL
  EXPLTIME=                text.value("EXPLTIME");
  //SHOTGUN
  SHOTGUNCHPOWER=          text.value("SHOTGUNCHPOWER");
  //DESTSMOKE
  text.area("destroy_smoke");
  DESTSMOKEFRQ=            (int)(text.fp_value("frq")*Mp::METRONQUALITY);
  DESTSMOKEAAP=            text.value("aap");
  DESTSMOKEPRC=            text.value("disp_phases");
  DESTSMOKECH=             text.value("time_change");
  text.endarea();

  text.area("OBJECTIVE");
  OBJCTIVETIME=            (int)(text.fp_value("time")*Mp::METRONQUALITY);
  OBJCTIVEDOWNJUST=        text.value("downjust");
  text.endarea();

  text.area("MINE");
  init_objpack(MINE);
  MINENONACTIVETIME=       (int)(text.fp_value("non_active")*Mp::METRONQUALITY);
  text.endarea();

  text.area("MYMINE");
  MYMINERANGE=              text.value("range");
  text.endarea();

  text.area("netcannon1");
  init_objpack(NETCANNON1);
  NETCANNON1POWER=         text.value("power");
  NETCANNON1ACTIVE=        (int)(text.fp_value("active")*Mp::METRONQUALITY);
  NETCANNON1PASSIVE=       (int)(text.fp_value("passive")*Mp::METRONQUALITY);
  text.endarea();

  text.area("netcannon2");
  init_objpack(NETCANNON2);
  NETCANNON2POWER=         text.value("power");
  NETCANNON2ACTIVE=        (int)(text.fp_value("active")*Mp::METRONQUALITY);
  NETCANNON2PASSIVE=       (int)(text.fp_value("passive")*Mp::METRONQUALITY);
  text.endarea();

  text.area("CANNONSPLASH");
  init_objpack(CANNONSPLASH);
  CANNONSPLASHFRQ=         (int)(text.fp_value("FRQ")*Mp::METRONQUALITY);
  text.endarea();

  text.area("BASE");
  BASELIFE=              text.value("BASELIFE");
  MAXSHIPBRICK=          RegData::BricksPerShip;
  DBG_MESSAGE("BricksPerShip=%d",RegData::BricksPerShip);
  MAXLEVELBRICK=         RegData::BricksPerLevel;
  DBG_MESSAGE("BricksPerLevel=%d",RegData::BricksPerLevel);
  text.endarea();

  text.area("raft");
  init_objpack(RAFT);
  RAFTMINEPOWER=         text.value("buoy_power");
  RAFTMINELIFE=          text.value("buoy_life");
  text.endarea();

  text.area("thief");
  init_objpack(THIEF);
  text.endarea();

  text.area("sounds");
  R1=                    text.value("R1");
  R2=                    text.value("R2");
  MIN_VOL=               (text.value("MIN_VOL")*VOLUME_MAX)/100;
  NETR1=                 text.value("NETR1");
  NETR2=                 text.value("NETR2");
  NETMIN_VOL=            (text.value("NETMIN_VOL")*VOLUME_MAX)/100;
  text.endarea();

  text.area("collect");
  COLLECTSPEED=          text.value("SPEED");
  COLLECTRADIUS=         text.value("RADIUS");
  COLLECTTIME=           _get_time(text.fp_value("time"));
  text.endarea();

  text.area("header");
  HEADERFRAMETIME=       _get_time(text.fp_value("frametime"));
  HEADERTEXTSPEED=       text.value("textspeed");
  text.endarea();

  text.area("splinter");
  SPLINTERTIME=          _get_time(text.fp_value("time"));
  SPLINTERSPEED1=         text.value("speed1");
  SPLINTERSPEED2=         text.value("speed2");
  SPLINTERSPEED3=         text.value("speed3");
  SPLINTERFALL=          _get_time(text.fp_value("fall_time"));
  SPLINTERPIX=           text.value("pixels")*100;
  text.endarea();

  text.area("bullet");
  init_objpack(bullet);
  text.endarea();

  text.area("morph");
  init_objpack(morph);
  text.endarea();

  text.area("splinter");
  init_objpack(splinter);
  text.endarea();

  text.area("smoke");
  init_objpack(smoke);
  text.endarea();

   //COUNTDOWN
  text.area("countdown");
  COUNTDOWNX=                  text.value("x");
  COUNTDOWNY=                  text.value("y");
  COUNTDOWNFRQ1=               _get_time(text.fp_value("time1"));
  COUNTDOWNFRQ2=               _get_time(text.fp_value("time2"));
  COUNTDOWNFRQ3=               _get_time(text.fp_value("time3"));
  text.endarea();

  text.area("statistics");
  STATISTICSBONUS=             text.value("bonus");
  text.endarea();

}

//LAYOUT
Text Layout::text;
int  Layout::STATPANELX;
int  Layout::STATPANELY;

int  Layout::STATPLAYERNAMEX;
int  Layout::STATPLAYERLASTX;
int  Layout::STATPLAYERSUMX;
int  Layout::STATPLAYERY;
int  Layout::STATPLAYERH;

int  Layout::STATSPRITEX;
int  Layout::STATSPRITEY;
int  Layout::STATSPRITEW;
int  Layout::STATPOINTERW;
int  Layout::STATPOINTERX;
int  Layout::STATPOINTERY;

int  Layout::SINGLELEVElX;
int  Layout::SINGLELEVElY;
int  Layout::SINGLEBRICKX;
int  Layout::SINGLEBRICKY;
int  Layout::SINGLEBRICKW;
int  Layout::SINGLEPOINTERX;
int  Layout::SINGLEPOINTERY;

int  Layout::SINGLEPANELX;
int  Layout::SINGLEPANELY;
int  Layout::SINGLETEXTX;
int  Layout::SINGLETEXTY;

int  Layout::SINGLEANIMSPEED;
int  Layout::SINGLELIGHT1;
int  Layout::SINGLELIGHT2;
int  Layout::SINGLEDARK;

char* Layout::SINGLEREADY;
char* Layout::SINGLECOMPLETED;
char* Layout::SINGLENOTCOMPLETED;
int   Layout::SINGLESTATUSX;
int   Layout::SINGLESTATUSY;
int   Layout::SINGLELINEX;
int   Layout::SINGLELINEY;

int   Layout::SINGLE_MARGIN;
int   Layout::SINGLE_TOOLS[4];
int   Layout::SINGLE_TITLE;
int   Layout::SINGLE_STRIPEX;
int   Layout::SINGLE_STRIPEY;
int   Layout::NET_TOOLS[7];
int   Layout::SINGLE_ACTIVETIME;

Layout::Panel Layout::panel[4];
Layout::Panel Layout::netpanel[4];
Layout::Panel Layout::helppanel;



 void Layout::init(void)
 {
   text.load("layout");

   text.area("NetStatisctics");
   STATPANELX=              text.value("titleX");
   STATPANELY=              text.value("titleY");

   STATPLAYERNAMEX=         text.value("playerNAMEX");
   STATPLAYERLASTX=         text.value("playerLASTX");
   STATPLAYERSUMX=          text.value("playerSUMX");
   STATPLAYERY=             text.value("playerY");
   STATPLAYERH=             text.value("playerH");

   STATSPRITEX=             text.value("spriteX");
   STATSPRITEY=             text.value("spriteY");
   STATSPRITEW=             text.value("spriteW");
   STATPOINTERW=            text.value("pointerW");
   STATPOINTERX=            text.value("pointerX");
   STATPOINTERY=            text.value("pointerY");
   text.endarea();

   text.area("SingleStatisctics");

   SINGLELEVElX=            text.value("levelx");
   SINGLELEVElY=            text.value("levely");
   SINGLEBRICKX=            text.value("brickx");
   SINGLEBRICKY=            text.value("bricky");
   SINGLEBRICKW=            text.value("brickw");

   SINGLEPOINTERX=          text.value("pointerx");
   SINGLEPOINTERY=          text.value("pointery");

   SINGLEPANELX=            text.value("panelx");
   SINGLEPANELY=            text.value("panely");
   SINGLETEXTX=             text.value("textx");
   SINGLETEXTY=             text.value("texty");

   SINGLEANIMSPEED=         text.value("anim_speed");

   SINGLELIGHT1=            text.value("light1");
   SINGLELIGHT2=            text.value("light2");
   SINGLEDARK=              text.value("dark");

   SINGLEREADY=             text.string("ready");
   SINGLECOMPLETED=         text.string("completed");
   SINGLENOTCOMPLETED=      text.string("notcompleted");
   SINGLESTATUSX=           text.value("statusX");
   SINGLESTATUSY=           text.value("statusY");
   SINGLELINEX=             text.value("lineX");
   SINGLELINEY=             text.value("lineY");

   struct Panel
   {
     int x,y;
     int w,h;
     int mr1,mr2,mrw;
   };


   text.area("single_panels");
   sscanf(text.string("panel1"),"%d,%d,%d,%d",&panel[0].x,&panel[0].y,&panel[0].w,&panel[0].h);
   sscanf(text.string("panel2"),"%d,%d,%d,%d",&panel[1].x,&panel[1].y,&panel[1].w,&panel[1].h);
   sscanf(text.string("panel3"),"%d,%d,%d,%d,%d,%d,%d",&panel[2].x,&panel[2].y,&panel[2].w,&panel[2].h,&panel[2].mr1,&panel[2].mr2,&panel[2].mrw);
   sscanf(text.string("panel4"),"%d,%d,%d,%d",&panel[3].x,&panel[3].y,&panel[3].w,&panel[3].h);
   sscanf(text.string("tools"),"%d,%d,%d,%d",&SINGLE_TOOLS[0],&SINGLE_TOOLS[1],&SINGLE_TOOLS[2],&SINGLE_TOOLS[3]);
   SINGLE_TITLE          =text.value("title");
   SINGLE_MARGIN         =text.value("margin");
   SINGLE_STRIPEX        =text.value("stripeX");
   SINGLE_STRIPEY        =text.value("stripeY");
   SINGLE_ACTIVETIME     =(int)(text.fp_value("active_time")*Mp::METRONQUALITY);
   text.endarea();

   text.area("net_panels");
   sscanf(text.string("panel1"),"%d,%d,%d,%d",&netpanel[0].x,&netpanel[0].y,&netpanel[0].w,&netpanel[0].h);
   sscanf(text.string("panel2"),"%d,%d,%d,%d,%d,%d,%d",&netpanel[1].x,&netpanel[1].y,&netpanel[1].w,&netpanel[1].h,&netpanel[1].mr1,&netpanel[1].mr2,&netpanel[1].mrw);

   sscanf(text.string("panel3"),"%d,%d,%d,%d,%d,%d,%d",&netpanel[2].x,&netpanel[2].y,&netpanel[2].w,&netpanel[2].h,&netpanel[2].mr1,&netpanel[2].mr2,&netpanel[2].mrw);
   sscanf(text.string("panel4"),"%d,%d,%d,%d,%d",&netpanel[3].x,&netpanel[3].y,&netpanel[3].w,&netpanel[3].h,&netpanel[3].mr1,&netpanel[3].mr2);

   sscanf(text.string("tools"),"%d,%d,%d,%d,%d,%d,%d",&NET_TOOLS[0],&NET_TOOLS[1],&NET_TOOLS[2],&NET_TOOLS[3],&NET_TOOLS[4],&NET_TOOLS[5],&NET_TOOLS[6]);
   text.endarea();


   text.endarea();

   text.area("help");
   sscanf(text.string("panel"),"%d,%d,%d,%d,%d",&helppanel.x,&helppanel.y,&helppanel.w,&helppanel.h,&helppanel.mr1);
   text.endarea();

 }
