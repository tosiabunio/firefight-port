#ifndef __PARAMS__
#define __PARAMS__

//KLASA MP
class Mp
{
  public:
   enum
   {
     TURNQUALITY=256,
     HALFANGLE=128,
     QUARTANGLE=64,
     EIGHTANGLE=32,
     ANGLE16=16,
     MAXOFFSET=25,
     METRONQUALITY=1024,
     SX=320,
     SY=200,
     ONEBYVYTOVXRATIO=112993,
     MAXBRICKS=200,
   };
   static void  init(char* filename);
   static int   DEBUG;
   static int   DEBUGKEYS;
   static int   NETDEBUGLEVEL;
   static int   RANDDEBUGLEVEL;
   static int   SHIPDEBUGLEVEL;
   static int   MEMORYDEBUG;
   static int   SYNCFAIL;
   static int   SYNCFAILABORT;
   static int   MINVX;
   static int   MINVY;
   static int   BEYONDTIME;
   static int   BUILDSPRITESMODE;
   static int   AUTOFLYMODE;
   static int   DIRNUM;
   static int   OVERSCANX;
   static int   OVERSCANY;
   static int   MAXOVERSCAN;
   static int   COLISQUEUESIZE;
   static int   KILLQUEUESIZE;
   static int   TURNSPEED;
   static int   CLIMBSPEED;
   static int   PIXELSBETWEENPLANES;
   static int   HALFHEIGHT;
   static int   SHADOWDIST;
   static int   LINKPRECISION;
   static int   SYNCFRAME;
   static int   TIMEDELAY;
   static int   FRAMEDELAY;
   static int   LOWVOICE;
   static int   RADARALIENRAY;
   static int   RADARDISTANCE;
   static int   HIRESAVAIL;
   static int   BACKGROUND;
   static int   LOADSAMPLE;
};
//KLASA OP
const MAX_OBJTYPES=4;
const MAX_CANNONTYPES=12;
const MAX_WEAPON=6;

#define _SPLASH    Op::obj_weaponary[0]
#define _SWARMERS  Op::obj_weaponary[1]
#define _LASER     Op::obj_weaponary[2]
#define _MYMINE    Op::obj_weaponary[5]
#define _ROCKET    Op::obj_weaponary[3]
#define _BOMB      Op::obj_weaponary[5]
#define _SHOTGUN   Op::obj_weaponary[4]

#define _CCHGUN     Op::weapon_power[0]
#define _CSWARMERS  Op::weapon_power[1]
#define _CLASER     Op::weapon_power[2]
#define _CROCKET    Op::weapon_power[3]
#define _CSHOTGUN   Op::weapon_power[4]
#define _CMYMINE    Op::weapon_power[5]

#define _CSPLASH    Op::NETCANNON1POWER

class Op
{
  public:
   static Text text;
   static Text typetext;
   struct ObjPack
   {
     int   max_speed;
     int   strafe_speed;
     int   accel_pwr;
     int   brake_pwr;
     int   auto_pwr;
     int   turn_speed;
     int   dirs;
     int   climb_speed;
     int   destroy;
     int   weapon;
     int   life;
     int   speed;
   };
   struct CannonPack
   {
     int   turn_speed;
     int   angle;
     int   gun;
     int   life;
     int   active;
     int   passive;
     int   weaponpower;
   };
   struct DefPack
   {
     int   shields;
     int   power;
     int   weapon;
     int   speed;
     int   passive;
     int   active;
     int   frq;
   };
  private:
   static void load_objpack(ObjPack* pack,char* txt);
   static void load_defpack(DefPack* pack,char* txt);
   static void load_cannonpack(CannonPack* pack,char* txt);
   static void init_objpack(void);
   static void init_defpack(void);
   static void compute_params(ObjPack* pack,DefPack* dpack);
   static void init_objpack(ObjPack& ref);
   static void init_weaponary(void);
   static void hero_params(void);
  public:
   static void init(char* filename);
   enum {SIMPLE1,SIMPLE2,SIMPLE3,TARAN};
   enum {MBOSS1,MBOSS2,MBOSS3,MBOSS4};

   static ObjPack     obj_enemy[MAX_OBJTYPES];
   static ObjPack     obj_mboss[MAX_OBJTYPES];

   static DefPack     def_enemy[MAX_OBJTYPES];
   static DefPack     def_mboss[MAX_OBJTYPES];
   static CannonPack  def_cannon[MAX_CANNONTYPES];
   static ObjPack     obj_weaponary[MAX_WEAPON];

   //MYSHIP
   static ObjPack myship;
   static int   max_weapon[MAX_WEAPON];
   static int   weapon_power[MAX_WEAPON];
   static int   weapon_ugr_max[MAX_WEAPON];
   static int   weapon_ugr_med[MAX_WEAPON];
   static int   weapon_ugr_min[MAX_WEAPON];
   static int   weaponary_frq[MAX_WEAPON];
   static int   weaponary_begin[MAX_WEAPON];
   static int   weaponary_net_begin[MAX_WEAPON];

   static int   shields[3];
   static int   MYSHIPFLASHTIME;
   static int   MYSHIPTURBOMAXSPEED;
   static int   MYSHIPTURBOSTRAFESPEED;
   static int   MYSHIPTURBOTURNSPEED;
   static int   MYSHIPCOLSNDSPEED;
   static int   MYSHIPDESTROYCOLSPEED;
   static int   MYSHIPDESTROYCOLAMOUNT;
   static int   MYSHIPCLOAKTIME;
   static int   MYSHIPSHIELDTIME;
   static int   MYSHIPENABLEFUTERKO;
   //DISPINFO
   static int   DISPINFOCOLORS;
   static int   DISPINFOBEGCOLOR;
   static int   DISPINFOORGX;
   static int   DISPINFOORGY;
   static int   DISPINFOWEAPX;
   static int   DISPINFOWEAPY;
   static int   DISPINFOWEAPSX;
   static int   DISPINFOWEAPSY;
   static int   DISPINFOWEAPINRECTX;
   static int   DISPINFOWEAPINRECTY;
   static int   DISPINFOWEAPSPACEX;
   static int   DISPINFOLIFEX;
   static int   DISPINFOLIFEY;
   static int   DISPINFOMARGINX;
   static int   DISPINFOMARGINY;
   static int   DISPINFOLIFERECTSX;
   static int   DISPINFOLIFERECTSY;
   static int   DISPINFOLIFERECTSPACEX;
   static int   DISPINFOINVX;
   static int   DISPINFOINVY;
   static int   DISPINFOINVSX;
   static int   DISPINFOINVSY;
   static int   DISPINFOINVINRECTX;
   static int   DISPINFOINVINRECTY;
   //FADE
   static int   FADEFADETIME;
   static int   FADETEXTY;

   //USER
   static int   USERLIFEUPGRAMOUNT;
   static int   USERLIFEUPGRTIME;
   //INVENTORY
   static int   INVENTMAXMINE;
   static int   INVENTMAXGOLDENEYE;
   static char *INVENTDESCRIPT[24];
   //SCRCATCH
   static ObjPack scrcatch;
   static int   SCRCATCHRADIUS;
   //WEAPON
   static int   WEAPONALIENANGLE;
   static int   WEAPONALIENRANGE;
   static int   WEAPONDESTROYANGLE;
   static int   WEAPONDESTROYRANGE;
   static int   WEAPONMORPHPHASES;
   static int   WEAPONAUTO1[10];
   static int   WEAPONAUTO2[10];
   static int   WEAPONAUTO3[10];
   static int   WEAPONAUTO4[10];
   static int   WEAPONAUTO5[10];
   static int   WEAPONAUTO6[10];
   //MYSPLASH
   static int   MYSPLASHLIFETIME;
   //MYSWARMER
   static int   MYSWARMERLIFETIME;
   static int   MYSWARMERINITTIME;
   static int   MYSWARMERSTARTDANGLE;
   static int   MYSWARMERTRANSANGLE;
   //MYROCKET
   static int   MYROCKETLIFETIME;
   static int   MYROCKETIDLETIME;
   static int   MYROCKETALIENANGLE;
   static int   MYROCKETALIENRANGE;
   static int   MYROCKETDESTROYANGLE;
   static int   MYROCKETDESTROYRANGE;
   //MYBOMB
   static int   MYBOMBLIFETIME;
   static int   MYBOMBSIZE;
   static int   MYBOMBRADIUS;
   static int   MYBOMBPOWER;
   static int   MYBOMBSHAKETIME;
   //SHIPFUCK
   static int   SHIPFUCKTIME;
   static int   SHIPFUCKPOWER;
   //COCA
   static ObjPack cocarocket;
   //TARAN
   static int   TARANFLYTIME;
   static int   TARANATTACKTIME;
   //SIMPLE1
   static int   SIMPLE1APPROX;
   static int   SIMPLE1TARGETTIME;
   static int   SIMPLE1SHOOTTIME;
   static int   SIMPLE1SHOOT;
   //SIMPLE2
   static ObjPack simple2_splash;
   static int   SIMPLE2SSPLASH;
   static int   SIMPLE2SHOOTTIME;
   //MBOSS1
   static ObjPack mb1splash;
   static int   MB1APROXPOINT;
   static int   MB1TARGETTIME;
   static int   MB1SHFRQ;
   //MBOSS2
   static ObjPack mboss2rocket;
   static int   MB2AAP;
   static int   MB2TTIME;
   static int   MB2FLYTIME;
   static int   MB2WALLCOLIS;
   static int   MB2SHOOTCOUNT;
   static int   MB2SHFRQ;
   //MBOSS3
   static ObjPack mb3bullet;
   static int   MBOSS3AAP;
   static int   MBOSS3TTIME;
   static int   MBOSS3FLYTIME;
   static int   MBOSS3SHTIME;
   static int   MBOSS3WALLCOLIS;
   static int   MBOSS3FRQ;
   //MBOSS4
   static int   MBOSS4AAP;
   static int   MBOSS4FLYTIME;
   static int   MBOSS4SHTIME;
   static int   MBOSS4TTIME;
   static int   MBOSS4FRQ;
   static int   MBOSS4TARANLIFETIME;
   //CANNONROCKET
   static ObjPack CANNONROCKET;
   //MYLASER
   static int   MYLASERLIFETIME;

   //LANDPLACE
   static int   LANDDISTANCE;
   static int   LANDSPEED;
   static int   LANDTIME;
   //TELEPORT
   static int   TELEPORTSTARTDISTANCE;
   static int   TELEPORTINUSEDISTANCE;
   //DESTROY
   static int   DESTROYLIFEHIGH;
   static int   DESTROYLIFEMEDIUM;
   static int   DESTROYLIFELOW;
   static int   DESTROYIMMORTAL;
   static int   DESTROYMORPHTIME;
   //MAGNES
   static int   MAGNESZEROPWR;
   static int   MAGNESEDGEPWR;
   static int   MAGNESLIFE;
   //EXPLOSION
   static int   EXPLTIME;
   //CAPSULE
   static int   W1BEG[3];
   static int   W1CNT[3];
   static int   W2BEG[3];
   static int   W2CNT[3];
   static int   W3BEG[3];
   static int   W3CNT[3];
   static int   W4BEG[3];
   static int   W4CNT[3];
   static int   W5BEG[3];
   static int   W5CNT[3];
   static int   W6BEG[3];
   static int   W6CNT[3];
   static int   LIBEG[3];
   static int   LICNT[3];
   static int   MKBEG;
   static int   MKCNT;
   static int   IMBEG;
   static int   IMCNT;
   static int   CLBEG;
   static int   CLCNT;
   static int   NMBEG;
   static int   NMCNT;
   static int   GEBEG;
   static int   GECNT;
   static int   CHBEG;
   static int   CHCNT;
   //BRICK
   static ObjPack brick;
   //DESTSMOKE
   static int   DESTSMOKEFRQ;
   static int   DESTSMOKEAAP;
   static int   DESTSMOKEPRC;
   static int   DESTSMOKECH;
   //OBJCTIVE
   static int   OBJCTIVETIME;
   static int   OBJCTIVEDOWNJUST;
   //MINE
   static ObjPack MINE;
   static int  MINENONACTIVETIME;
   //MYMINE
   static int MYMINERANGE;
   //THIEF
   static ObjPack THIEF;
   //RAFT
   static ObjPack RAFT;
   static int RAFTMINEPOWER;
   static int RAFTMINELIFE;

   //NETCANNON
   static ObjPack NETCANNON1;
   static int     NETCANNON1POWER;
   static int     NETCANNON1PASSIVE;
   static int     NETCANNON1ACTIVE;

   static ObjPack NETCANNON2;
   static int     NETCANNON2POWER;
   static int     NETCANNON2PASSIVE;
   static int     NETCANNON2ACTIVE;
   //CANNONSPLASH
   static ObjPack CANNONSPLASH;
   static int     CANNONSPLASHFRQ;

   static int     BASELIFE;
   static int     SHOTGUNCHPOWER;
   static int     MAXSHIPBRICK;
   static int     MAXLEVELBRICK;

   //SOUNDS
   static int     R1;
   static int     R2;
   static int     MIN_VOL;

   static int     NETR1;
   static int     NETR2;
   static int     NETMIN_VOL;

   //COLLECT
   static int    COLLECTSPEED;
   static int    COLLECTRADIUS;
   static int    COLLECTTIME;

   //HEADER
   static int    HEADERFRAMETIME;
   static int    HEADERTEXTSPEED;

   //SPLINTER
   static int SPLINTERTIME;
   static int SPLINTERSPEED1;
   static int SPLINTERSPEED2;
   static int SPLINTERSPEED3;
   static int SPLINTERFALL;
   static int SPLINTERPIX;
   //BULLET
   static ObjPack bullet;
   //MORPH
   static ObjPack morph;
   //SPLINTER
   static ObjPack splinter;
   //SMOKE
   static ObjPack smoke;

   //COUNTDOWN
   static int  COUNTDOWNX;
   static int  COUNTDOWNY;
   static int  COUNTDOWNFRQ1;
   static int  COUNTDOWNFRQ2;
   static int  COUNTDOWNFRQ3;

   //NETPARAMS
   static int STATISTICSBONUS;

};

class Layout
{
  public :
   static Text text;
   static void init(void);
   static int  STATPANELX;
   static int  STATPANELY;

   static int  STATPLAYERNAMEX;
   static int  STATPLAYERLASTX;
   static int  STATPLAYERSUMX;
   static int  STATPLAYERY;
   static int  STATPLAYERH;

   static int  STATSPRITEX;
   static int  STATSPRITEY;
   static int  STATSPRITEW;

   static int  STATPOINTERW;
   static int  STATPOINTERX;
   static int  STATPOINTERY;

   static int  SINGLELEVElX;
   static int  SINGLELEVElY;
   static int  SINGLEBRICKX;
   static int  SINGLEBRICKY;
   static int  SINGLEBRICKW;

   static int  SINGLEPOINTERX;
   static int  SINGLEPOINTERY;

   static int  SINGLEPANELX;
   static int  SINGLEPANELY;
   static int  SINGLETEXTX;
   static int  SINGLETEXTY;
   static int  SINGLEANIMSPEED;

   static int  SINGLELIGHT1;
   static int  SINGLELIGHT2;
   static int  SINGLEDARK;

   static char* SINGLEREADY;
   static char* SINGLECOMPLETED;
   static char* SINGLENOTCOMPLETED;
   static int   SINGLESTATUSX;
   static int   SINGLESTATUSY;
   static int   SINGLELINEX;
   static int   SINGLELINEY;

   struct Panel
   {
     int x,y;
     int w,h;
     int mr1,mr2,mrw;
   };
   static Panel panel[4];
   static int   SINGLE_TOOLS[4];
   static int   SINGLE_MARGIN;
   static int   SINGLE_TITLE;
   static int   SINGLE_STRIPEX;
   static int   SINGLE_STRIPEY;
   static int   SINGLE_ACTIVETIME;

   static Panel netpanel[4];
   static int   NET_TOOLS[7];

   static Panel helppanel;
};


#endif


