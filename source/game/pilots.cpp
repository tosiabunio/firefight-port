#include "headers.h"

int  Pilot::version=0x0006;
int  Pilot::initialized=0;
int  Pilot::freezed=0;
int  Pilot::no_pilot=0;
char Pilot::reg_section[Pilot::section_len];
char Pilot::reg_key[Pilot::key_len];
Pilot::Data Pilot::data;
Pilot::Data Pilot::freezed_data;

void Pilot::init(int _no_pilot)
{
  DBG_CHECK(sizeof(data.sysset_data)==(unsigned)SysSet::dump_size());
  DBG_CHECK(!initialized);
  initialized=1;
  freezed=0;
  no_pilot=_no_pilot;

  strncpy(reg_section,RegData::GetPlayerSection(),section_len);
  reg_section[section_len-1]='\0';
  if (no_pilot)
  {
    strncpy(reg_key,"d:-)",section_len);
  }
  else if (NoNet::is_network_mode())
  {
    strncpy(reg_key,"ntrk",section_len);
  }
  else if (RegData::GetPlayerSkill()==RegData::skill_Normal)
  {
    strncpy(reg_key,"easy",section_len);
  }
  else
  {
    strncpy(reg_key,"hard",section_len);
  }
  reg_key[key_len-1]='\0';
  
  if (no_pilot||!try_registry())
  {
    SysSet::set();
    memset((void*)&data,0,sizeof(data));
    data.version=version;
    data.skill=!(RegData::GetPlayerSkill()==RegData::skill_Normal);
    int is_net=NoNet::is_network_mode();
    int i;
    for (i=0;i<weap_num;i++)
    {
      if (!is_net)
      {
        data.user_data[i+WEAP0]=(int)Op::weaponary_begin[i];
      }
    }
    for (i=0;i<chat_text_num;i++)
    {
      data.chat_text[i][0]='\0';
    }
  }
}

void Pilot::quit(void)
{
  if (initialized&&!no_pilot&&!freezed)
  {
    DBG_CHECK(!Mp::BUILDSPRITESMODE);
    dump_to_registry(data);
  }
  initialized=0;
}

void Pilot::dump_to_registry(Data& d)
{
  DBG_CHECK(initialized);
  DBG_CHECK(!freezed);
  DBG_CHECK(!Mp::BUILDSPRITESMODE);
  Data to_code=d;
  DBG_CHECK(sizeof(to_code.sysset_data)==(unsigned)SysSet::dump_size());
  memcpy((void*)to_code.sysset_data,(void*)SysSet::dump_buffer(),sizeof(to_code.sysset_data));
  to_code.csum=count_csum(to_code);
  code(to_code);
  Registry::set_binary(reg_section,reg_key,(char*)&to_code,sizeof(Data));
}

int Pilot::try_registry(void)
{
  DBG_CHECK(initialized);

  unsigned data_size=sizeof(Data);
  if (!Registry::get_binary(reg_section,reg_key,(char*)&data,&data_size))
  {
//    MESSAGE("Reading pilot data from registry failed.");
    return 0;
  }
  if (!uncode(data))
  {
//    MESSAGE("Decoding pilot data failed.");
    return 0;
  }
  if (data.csum!=count_csum(data))
  {
//    MESSAGE("Pilot data in registry has invalid checksum.");
    return 0;
  }
  if (data_size!=sizeof(Data))
  {
//    MESSAGE("Pilot data in registry has invalid size.");
    return 0;
  }
  if (data.version!=version)
  {
//    MESSAGE("Pilot data in registry has invalid version.");
    return 0;
  }
  if (!SysSet::set((void*)data.sysset_data,sizeof(data.sysset_data)))
  {
//    MESSAGE("Incorrect system parameters in registry - assuming defaults.");
    SysSet::set();
  }
  return 1;
}

int Pilot::count_csum(Data& d)
{
  int curr_csum=d.csum;
  d.csum=0;
  int size=sizeof(Data);
  char *ptr=(char*)&d;
  int csum=0;
  for (int i=0;i<size;i++)
  {
    csum+=ptr[i];
  }
  d.csum=curr_csum;
  return csum;
}

void Pilot::code(Data& d)
{
  int size=sizeof(Data);
  char *ptr=(char*)&d;
  unsigned char code_value=0;
  unsigned char prev_value=0;
  for (int i=0;i<size;i++)
  {
    unsigned int result=(2*code_value+prev_value+11)%256;
    code_value=(unsigned char)result;
    ptr[i]^=code_value;
    prev_value=ptr[i];
  }
}

int Pilot::uncode(Data& d)
{
  int size=sizeof(Data);
  char *ptr=(char*)&d;
  unsigned char code_value=0;
  unsigned char prev_value=0;
  for (int i=0;i<size;i++)
  {              
    unsigned int result=(2*code_value+prev_value+11)%256;
    code_value=(unsigned char)result;
    prev_value=ptr[i];
    ptr[i]^=code_value;
  }
  return 1;
}

void Pilot::freeze(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(!freezed);
  freezed=1;
  freezed_data=data;
}

void Pilot::unfreeze(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(freezed);
  data=freezed_data;
  freezed=0;
}

int Pilot::get_level(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(data.curr_level>=0&&data.curr_level<levels_num);
  return data.curr_level;
}

void Pilot::set_level(int num)
{
  DBG_CHECK(initialized);
  DBG_CHECK(num>=0&&num<levels_num);
  data.curr_level=num;
}

int Pilot::get_skill(void)
{
  DBG_CHECK(initialized);
  return (data.skill||NoNet::is_network_mode());
}

void Pilot::set_skill(int skill)
{
  DBG_CHECK(initialized);
  data.skill=skill;
}

int& Pilot::level_status(int num)
{
  DBG_CHECK(initialized);
  DBG_CHECK(!NoNet::is_network_mode());
  DBG_CHECK(num>=0&&num<levels_num);
  return data.level_status[num];
}

int& Pilot::secrets(int num)
{
  DBG_CHECK(initialized);
  DBG_CHECK(!NoNet::is_network_mode());
  DBG_CHECK(num>=0&&num<levels_num);
  return data.secrets[num];
}

int& Pilot::secrets_num(int num)
{
  DBG_CHECK(initialized);
  DBG_CHECK(!NoNet::is_network_mode());
  DBG_CHECK(num>=0&&num<levels_num);
  return data.secrets_num[num];
}

int& Pilot::kills(int num)
{
  DBG_CHECK(initialized);
  DBG_CHECK(!NoNet::is_network_mode());
  DBG_CHECK(num>=0&&num<levels_num);
  return data.kills[num];
}

int& Pilot::kills_num(int num)
{
  DBG_CHECK(initialized);
  DBG_CHECK(!NoNet::is_network_mode());
  DBG_CHECK(num>=0&&num<levels_num);
  return data.kills_num[num];
}

int& Pilot::time(int num)
{
  DBG_CHECK(initialized);
  DBG_CHECK(!NoNet::is_network_mode());
  DBG_CHECK(num>=0&&num<levels_num);
  return data.time[num];
}


void Pilot::reckon_level(int num)
{
  DBG_CHECK(initialized);
  DBG_CHECK(!NoNet::is_network_mode());
  DBG_CHECK(num>=0&&num<levels_num);
  DBG_CHECK(!Mp::BUILDSPRITESMODE);
  data.level_status[num]=1;
  store_user_data();
  dump_to_registry(data);
}

void Pilot::store_user_data(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(!NoNet::is_network_mode());
  DBG_CHECK(!GameManager::get_testing());
  NoNet::get_ship()->get_user()->store_data();
}

char *Pilot::chat_buffer(int num)
{
  DBG_CHECK(initialized);
  DBG_CHECK(NoNet::is_network_mode());
  DBG_CHECK(num>=0&&num<chat_text_num);
  return data.chat_text[num];
}


