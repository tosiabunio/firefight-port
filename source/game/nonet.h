#ifndef __NET__
#define __NET__

//KLASA NET
class Myship;
class NoNet
{
  public:
   enum {max_users=4,max_levels=32,max_levelname=64,max_netinfosize=USER_BLOCK_DATA_SIZE};
   enum Mode
   {
     STANDARD      = 0x0001,
     NETWORKMATCH  = 0x0002,
     NETWORKFLAG   = 0x0004,
   };
   enum EndCond
   {
     FRAGS         = 0x0001,
     TIMEUP        = 0x0002,
   };
   enum NetVersion
   {
     FIRSTFINAL    = 0x0001,
     MYVERSION     = FIRSTFINAL,
   };
   enum ConnectAbility
   {
     OKWECANPLAY   = 0x0001,
     SORRYWINEOTU  = 0x0002,
   };
  private:
   struct NetInfo
   {
     unsigned packet_size;
     int version;
     int debug_status;
     int level_mask;
     int weapon_numb;
     int inventory_numb;
     int IQ_Bolca;
   };
   static int initialized;
   static int resources;
   static int levels_synchronized;
   static Mode mode;
   static EndCond endcond;
   static NetVersion netversion;
   static int gametime;
   static int maxfrags;
   static int this_id;
   static int serv_id;
   static int users_number;
   static int levels_table[max_levels];
   static int levels_local[max_levels];
   static int levels_number;
   static char levels_names[max_levels][max_levelname];
   static Myship *ship_table[max_users+1];
   static char name_buffer[max_users+1][256];
  public:
   static void init(void);
   static void quit(void);
   static void init_resources(void);
   static void quit_resources(void);
   static void init_levels(void);
   static void quit_levels(void);
   static void refresh(int internal_only=0);
   static int is_network_mode(int _mode=-1);
   static EndCond end_condition(void);
   static NetVersion play_version(void);
   static Myship *get_ship(int id=-1);
   static Myship *get_ship(int x,int y,int ignore_id=0);
   static int *get_levels_table(void);
   static int get_levels_number(void);
   static int get_local_level_number(int level);
   static char *get_level_name(int level);
   static int get_this_id(void);
   static int get_server_id(void);
   static int get_players_number(void);
   static int check_id(int id);
   static int is_alive(int id);
   static int game_time(void);
   static int max_frags(void);
   static char *get_player_name(int id);
   static char *get_player_color(int id);
   //FOR_ALL_USERS - do not use structures below:
   static int buffer_FOR[max_users+1];
};

#define FOR_ALL_USERS(user_id)  int user_id;for(LINEUNIQUE_DECLARE(indx);((user_id=NoNet::buffer_FOR[LINEUNIQUE(indx)])>0);LINEUNIQUE(indx)++)

#endif
