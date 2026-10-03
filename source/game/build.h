#ifndef __BUILDERS__
#define __BUILDERS__

//KLASA LEVIMP
 struct Parameters 
 {
   enum{ONAFTER=1,OFFAFTER,MID,ONBEFORE,OFFBEFORE};
   enum{ENEMYSHIP=1,CANNON,DESTROY,UNBLOCK,CAPSULE,GENERATOR,NETCANNON,NETBASE,NETLAND,BRICK,
         STARTPLACE,LANDPLACE,SECRET,DISABLE,ENABLE,MORPH,OTHER};
   int range;
   int smoke;
   int action;
   int disp_type;
   int serv_type;
   int colis_type;
   int time_delay;
   int frame_delay;
   int repeat_count;
   int sync_frame;
   int stuff_left;
   int change_type;
   int change_spr;
   char builder;
   int timer;
   int blocked;
   char is_object;
   char is_alien;
   char light;
   int count;
   char expl_sample,hit_sample,explode;
   char morph;
 };

const MAX_LANDPLACES=24;
const GENERATOR_STUFF=8989;

class LevImp
{
   friend class ImpExp;
   friend class Link;
   friend class GameManager;
   friend class World;
   friend class Update;
  private:
   static Buildable *builders[256];
   static int builders_indx;

   static int parameters_indx;
   static int maximum_crange;
   static int reg_type(char*,int);
   static int reg_name(char*,int);

   static int alien,mine,cannon,magnes,lcapsule,w0capsule,
              w1capsule,w2capsule,w3capsule,w4capsule,ccapsule,landplace,
              teleport,sswitch,unblock,door,destroy,machine,light,collect,photo,generator,
              base,mycannon,brick,buoy,raft,thief,smoke,secret,morphdest;
   static int initialized;
   static void regbuild_global(void);
   static void regbuild_local(void);
   static void regbuild_cancel(void);
   static char typesname[100];
   static int  get_sprite(char* txt);
   static void init_destroy(int);
   static void init_generator(int);
   static void init_netland(int);
   static void init_unblock(int);
   static void seek(void);
   static unsigned short *brickptr;
   static int             bricksize;
   static void randbrick(void);
  public:
   static World *world;
   static Parameters *parameters;
   static Text *types_text;
   struct Landplaces
   {
     int convert[MAX_LANDPLACES];
     int place[MAX_LANDPLACES];
     int size;
   };
   static Landplaces land;
   static void init(World *_world);
   static void quit(void);
   static Parameters *get_parameters(int type_num)
   {
     DBG_CHECK(initialized);
     DBG_CHECK(type_num>=0&&type_num<parameters_indx);
     return &parameters[type_num];
   }
   static int check_area(char* name,char* def,Text&);
   static int in_crange(int type,int x,int y);
   static int in_alienrange(int x,int y,int range);
   static int max_crange(void);
   static void report(void);
   static int  action[Level::max_size];
   enum {
          PNORMAL=0,
          P30TSP=1,
          P70TSP=2,
          PINVISIBLE=3,
          PSHADOW=4,
        };
   enum {
          PSRIGHT=1,
          PSLEFT=2,
          PSWAITRAND=3,
          PSPINGPONG=4,
          PSSROTRIGHT=5,
          PSSROTLEFT=6,
          PSYNCPINGPONG=7,
          PREPARE=8,
        };
   enum {
          PCNONE=0,
          PCWALL=1,
          PCWEAPON=2,
          PCKILL=3,
        };
   enum {
          SIMPLE1,
          SIMPLE2,
          SIMPLE3,
          TARAN,
          MBOSS1,
          MBOSS2,
          MBOSS3,
          MBOSS4,
          SIMPLE1INVISIBLE,
          SIMPLE2INVISIBLE,
          SIMPLE3INVISIBLE,
          TARANINVISIBLE,
          MBOSS1INVISIBLE,
          MBOSS2INVISIBLE,
          MBOSS3INVISIBLE,
          MBOSS4INVISIBLE,
          LAST,
        };
};

class Counter
{
   friend class LevImp;
   friend class MManager;
   static int global_ships;
   static int start_global_ships;
   static int global_secrets;
   static int start_global_secrets;
  public:
   enum {CANNON,SWARM,PLASMA,ROCKET,GUN,COBAIN,LIFE,GOLDENEYE,CLOAK,MEDKIT,IMMORTAL,NETMINE,UPGR_LAST};
   enum {SIMPLE1,SIMPLE2,SIMPLE3,SIMPLE4,MBOSS1,MBOSS2,MBOSS3,MBOSS4,CANNON_TOWER,SH_LAST};
   enum {DESTROY,COLLECT,LIGHT,NEUT_LAST};
   static int upgr[UPGR_LAST];
   static int ships[SH_LAST];
   static int neutral[NEUT_LAST];
   static int all_ships(void) {return global_ships<=0;}
   static int ships_count(void) {return global_ships;}
   static void present_alien(int lev_num);
   static void present_secret(void) {global_secrets--;DBG_CHECK(global_secrets>=0);}
   static void update_registry();
   static void run(Level*);
};

//BUILDERY WSZELAKIE
class AlienBuild : public Buildable
{virtual void builder(void);};

class MineBuild : public Buildable
{virtual void builder(void);};

class CannonBuild : public Buildable
{virtual void builder(void);};

class LandplaceBuild : public Buildable
{virtual void builder(void);};

class TeleportBuild : public Buildable
{virtual void builder(void);};

class SwitchBuild : public Buildable
{virtual void builder(void);};

class UnblockBuild : public Buildable
{virtual void builder(void);};

class LightBuild : public Buildable
{virtual void builder(void);};

class DoorBuild : public Buildable
{virtual void builder(void);};

class DestroyBuild : public Buildable
{virtual void builder(void);};

class MorphDestBuild : public Buildable
{virtual void builder(void);};

class CollectBuild : public Buildable
{virtual void builder(void);};

class CapsuleBuild : public Buildable
{virtual void builder(void);};

class GeneratorBuild : public Buildable
{virtual void builder(void);};

class BrickBuild : public Buildable
{virtual void builder(void);};

class BaseBuild : public Buildable
{virtual void builder(void);};

class MyCannonBuild : public Buildable
{virtual void builder(void);};

class RaftBuild : public Buildable
{virtual void builder(void);};

class BuoyBuild : public Buildable
{virtual void builder(void);};

class ThiefBuild : public Buildable
{virtual void builder(void);};

class SmokeBuild : public Buildable
{virtual void builder(void);};

class SecretBuild : public Buildable
{virtual void builder(void);};
//BUILDERY NIETWORZACE OBIEKTOW
class RotatRight : public Buildable
{virtual void builder(void);};

class RotatLeft : public Buildable
{virtual void builder(void);};

class RandomWait : public Buildable
{virtual void builder(void);};

class PingPong : public Buildable
{virtual void builder(void);};

class SRotatRight : public Buildable
{virtual void builder(void);};

class SRotatLeft : public Buildable
{virtual void builder(void);};

class SPingPong : public Buildable
{virtual void builder(void);};

class Prepare : public Buildable
{virtual void builder(void);};

#endif

/*MEMO
pola Lobjectu :
                                              _
 Obcych statkach                 user.short3 x |
                                               |-> poczatkowe pozycje x,y
                                 user.short4 y |
                                              _
 W grze sieciowej                user.short1   |-> id basy lotnisk i dzial
                                              _
 w obcych dzialach               user.short1   |-> kat dziala 
                                              _
 Wszystkie obiekty 'obj_         aux1          |-> pierwotny plan na ktorym staly
                                              _
 W minibosach i dzialach         user2.short2  |-> zycie po powrocie na level


  ?????                                       _
 W obiektach detsroy              aux1,aux2    |-> x,y pocztkowe

*/

