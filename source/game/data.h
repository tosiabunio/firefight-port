#ifndef __MYSPRITE__
#define __MYSPRITE__

//KLASA MYSPRITES
//Klasa ta powinna byc wykorzystywana do ladowania zewnetrznych spritow.
const MAX_SPRSPLINTER=15;
const MAX_SZZZZ=3;

class Mysprites
{
   static int back_phase;
   static Sprite patern;
  public:
   static void init(void);
   static void quit(void);
   static int sprnum(char *);
   static void put_pattern(Screen &screen,int shaded=0,int phase=-1);
   static void change_pattern(void);
   static Sprite *back;
   static Sprite spot;
   static Sprite ship;
   static Sprite ship_tower;
   static Sprite ship_tower_point;
   static Sprite ship_exhaust1;
   static Sprite ship_exhaust2;
   static Sprite ship_border;
   static Sprite ship_border_strong;
   static Sprite ship_shield;
   static Sprite ship_skin;
   static Sprite weap1fire;
   static Sprite shotgunfire;
   static Sprite header;
   static Sprite snail;
   static Sprite ordering;

   static Sprite disp_figuressm;
   static Sprite disp_figuresbig;

   static Sprite mybomb;
   static Sprite myrocket;
   static Sprite myswarmer;
   static Sprite expl;
   static Sprite boom;
   static Sprite fireball;
   static Sprite flame;
   static Sprite cannonsplash;
   static Sprite cannonpoint1;
   static Sprite cannonpoint2;

   static Sprite arocket;
   static Sprite asplash;
   static Sprite bomb;
   static Sprite dymki;
   static Sprite destsmoke;
   static Sprite mine;
   static Sprite staticmine[NoNet::max_users];

   //Explode
   static Sprite expl_dest;
   static Sprite expl_dest_small;
   static Sprite expl_rocket;
   static Sprite expl_mine;
   static Sprite expl_air;
   static Sprite expl_laser;
   static Sprite expl_swarm;

   static Sprite kulka;
   static Sprite strzal;
   static Sprite mbosssplinter;

   static Sprite capsule;
   static Sprite brick;
   static Sprite radar_point;
   static Sprite lihgtexpl;
   static Sprite radar_target;
   static Sprite radar_scope;
   static Sprite radar_landing;

   static Sprite photo_cross;
   static Sprite indicator;
   static Sprite indicatore;

   static Sprite telep_u;
   static Sprite telep_d;

   static Sprite fog;
   static Sprite night;
   static Sprite cloud;
   static Sprite collect_up;

   static Sprite dym;
   static Sprite farmerdym;
   static Sprite taran;
   static Sprite simple1;
   static Sprite simple2;
   static Sprite simple3;
   static Sprite mboss1;
   static Sprite mboss2;
   static Sprite mboss3;
   static Sprite mboss4;

   static Sprite generat;
   static Sprite mboss3_fire_left;
   static Sprite mboss3_fire_right;
   static Sprite persons;
   static Sprite missions_pics_dark;
   static Sprite missions_pics_nor;
   static Sprite active_mission;
   static Sprite backscreen;
   static Sprite titlescreen;
   static Sprite press;
   static Sprite copy;

   static void load_splinter(void);
   static void free_splinter(void);
   static int splinters_count(void) {return splinter_num;}
   static Sprite splinter[MAX_SPRSPLINTER];
   static int splinter_num;
   //LAYOUT
   static Sprite frame;
   //NET
   static Sprite level_pictures;
   static Sprite pointer;
   static Sprite cinder_cannon;
   static Sprite cinder_pulse;

   static void load_title(void);
   static void free_title(void);
};

const int MAXSPLINTER=6;
const int MAXTHUNDER=2;
const int MAXEXPLODE=6;
//Mysound
class Mysound
{
  public:
   static void init(void);
   static void quit(void);
   //speech samples (weapon's and upgrade's names)
   static Sample speech_vulcan;
   static Sample speech_swarmers;
   static Sample speech_plasma;
   static Sample speech_missiles;
   static Sample speech_cannon;
   static Sample speech_grenade;
   static Sample speech_emshock;
   static Sample speech_dshield;
   static Sample speech_mine;
   static Sample speech_cloak;
   static Sample speech_shield;
   static Sample speech_chaosupgrade;
   //speech samples (collected and selected)
   static Sample speech_collected;
   static Sample speech_activated;
   static Sample speech_selected;
   //speech samples (network names)
   static Sample speech_colours[NoNet::max_users+1];
   static Sample speech_player;
   //speech samples (chat and message speeches)
   static Sample speech_chatreqby;
   static Sample speech_chatterm;
   static Sample speech_incomingmsg;
   //speech samples (net kill messages)
   static Sample speech_youkilled;
   static Sample speech_youbeenkilledby;
   static Sample speech_youkilledyourself;
   static Sample speech_haskilled;
   static Sample speech_haskilledhimself;
   //speech samples (leaving net game)
   static Sample speech_leftgame;
   //speech samples (ship state)
   static Sample speech_shldlow;
   static Sample speech_shldcritical;
   static Sample speech_shldrecharged;
   static Sample speech_cloakactive;
   static Sample speech_cloak10deactive;
   static Sample speech_cloakdeactive;
   static Sample speech_dshieldactive;
   static Sample speech_dshield10deactive;
   static Sample speech_dshielddeactive;
   static Sample speech_outofammo;
   //speech samples (level situation)
   static Sample speech_alldestroyed;
   static Sample speech_youclearland;
   static Sample speech_clearland;
   static Sample speech_secretplace;
   //speech samples (end condition)
   static Sample speech_misncomplete;
   static Sample speech_misnfailed;
   static Sample speech_timeisup;
   static Sample speech_reachfrag;
   static Sample speech_youreachfrag;
   static Sample speech_you;
   static Sample speech_are;
   static Sample speech_is;
   static Sample speech_thewinner;
   //speech samples (beaming)
   static Sample speech_beamsuccess;
   static Sample speech_beaminterrupt;
   //speech samples (warnings of enemies)
   static Sample speech_incomemissile;
   static Sample speech_incomefire;
   static Sample speech_incomeenemy;
   static Sample speech_largeshiprange;
   static Sample speech_largeshipdetect;
   static Sample speech_underattack;
   static Sample speech_basethreat;
   //speech samples (net header)
   static Sample speech_youarefirst;
   static Sample speech_youaresecond;
   static Sample speech_youarethird;
   static Sample speech_youarefourth;
   static Sample speech_youarelast;
   //speech samples (damage)
   static Sample speech_fuelleak;
   static Sample speech_hulldamage;
   static Sample speech_cargounstable;
   static Sample speech_explosion;
   static Sample speech_coolingfail;
   //fx samples (weapons fx)
   static Sample ship_chgun;
   static Sample ship_swarm;
   static Sample ship_plasma;
   static Sample ship_homing;
   static Sample ship_shotgun;            
   static Sample ship_cobain;
   static Sample ship_mine;
   static Sample ship_emshock;
   //fx samples (ship events)
   static Sample ship_wallcollis;
   static Sample ship_shipcollis;
   static Sample ship_lifeincrease;
   static Sample ship_lifedecrease;
   static Sample ship_turboon;
   static Sample ship_turbooff;
   static Sample ship_weap_collected;
   static Sample ship_life_collected;
   static Sample ship_invent_collected;
   static Sample ship_brick_collected;
   static Sample ship_start;
   static Sample ship_land;
   static Sample ship_strongmorph_on;
   static Sample ship_strongmorph_off;
   //fx samples (beaming and respawning)
   static Sample respawning;
   static Sample beaming;


   static Sample arocket;
   static Sample asplash;
   static Sample karabin;
   static Sample storm[MAXTHUNDER];
   static Sample brick;
   static Sample taran;
   static Sample bullet;
   static Sample cannonrocket;
   static Sample generat;
   static Sample explode[MAXEXPLODE];
   static Sample mine;
   static Sample mineactive;

   static Sample mboss1_fire;
   static Sample mboss2_fire;
   static Sample mboss3_fire;
   static Sample mboss4_fire;

   static Sample splinter[MAXSPLINTER];
   static Sample spl_ship1;
   static Sample spl_ship2;
   static Sample spl_ship3;
   static Sample options_updown;
   static Sample options_switch;
   static Sample options_na;
   static Sample options_accept;
   static Sample options_fx;
   static Sample options_init;
   static Sample options_quit;
   static Sample cheat_on;
   static Sample cheat_off;
   static Sample mission_failed;
   static Sample mission_completed;
   static Sample buffer_overflow;
   static Sample ringing;
   static Sample countdown;
   static Sample logo[4];
   static Sample titlemove;
   //szzzzzzzzzzzzzz
   static Sample szzzz[MAX_SZZZZ];
   static Sample szzzzend[MAX_SZZZZ];
};
#endif
