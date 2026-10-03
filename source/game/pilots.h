#ifndef __PILOTS__
#define __PILOTS__

//KLASA PILOT
class Pilot
{
   friend class User;
  public:
   enum {chat_text_num=10,chat_text_len=256,levels_num=30,weap_num=6,section_len=256,key_len=256};
   enum 
   {
    _FIRST,
     WEAP0,
     WEAP1,
     WEAP2,
     WEAP3,
     WEAP4,
     WEAP5,
     MEDIKIT,
     CLOAK,
     IMMORT,
     GOLDENEYE,
    _LAST,
   };
  private:
   static int initialized;
   static int no_pilot;
   static int freezed;
   static int version;
   struct Data
   {
     int csum;
     int version;
     int curr_level;
     int skill;
     int level_status[levels_num];
     int secrets[levels_num];
     int secrets_num[levels_num];
     int kills[levels_num];
     int kills_num[levels_num];
     int time[levels_num];
     int user_data[_LAST];
     char chat_text[chat_text_num][chat_text_len];
     int sysset_data[SysSet::_LAST];
   };
   // Saved as raw bytes (port: in a pilot file, was a registry value).
   static_assert(sizeof(Data)==3388, "pilot record layout differs from the original");
   static Data data;
   static Data freezed_data;
   static char reg_section[section_len];
   static char reg_key[key_len];
   static int  try_registry(void);
   static void code(Data& d);
   static int  uncode(Data& d);
   static int  count_csum(Data& d);
   static void dump_to_registry(Data& d);
   static void store_user_data(void);
  public:
   static void init(int _no_pilot=0);
   static void quit(void);
   static int  get_level(void);
   static void set_level(int num);
   static int  get_skill(void);
   static void set_skill(int skill);
   static void freeze(void);
   static void unfreeze(void);
   static int& level_status(int num);
   static int& secrets(int num);
   static int& secrets_num(int num);
   static int& kills(int num);
   static int& kills_num(int num);
   static int& time(int num);
   static void reckon_level(int num);
   static char *chat_buffer(int num);
};

#endif



