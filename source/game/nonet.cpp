#include "headers.h"

//KLASA NET
int NoNet::initialized=0;
int NoNet::levels_synchronized=0;
int NoNet::resources=0;
int NoNet::this_id=0;
int NoNet::serv_id=0;
int NoNet::users_number=0;
int NoNet::gametime=0;
int NoNet::maxfrags=0;
int NoNet::levels_number=0;
char NoNet::name_buffer[NoNet::max_users+1][256];
int NoNet::buffer_FOR[max_users+1];
int NoNet::levels_table[NoNet::max_levels];
int NoNet::levels_local[NoNet::max_levels];
char NoNet::levels_names[NoNet::max_levels][NoNet::max_levelname];
Myship *NoNet::ship_table[NoNet::max_users+1];
NoNet::Mode NoNet::mode=(NoNet::Mode)0;
NoNet::EndCond NoNet::endcond=(NoNet::EndCond)0;
NoNet::NetVersion NoNet::netversion=(NoNet::NetVersion)0;

void NoNet::init(void)
{
  DBG_CHECK(!initialized);
  initialized=1;
  mode=(NoNet::Mode)0;
  this_id=0;
  serv_id=0;

//  MESSAGE("NETWORK: version 1.00");
  if (Eem::is_network_mode())
  {
//    MESSAGE("NETWORK: network support enabled");
    if (RegData::GameMode==RegData::gm_Deathmatch)
    {
      mode=NETWORKMATCH;
      if (RegData::DMFinish==RegData::dm_Kill)
      {
        endcond=FRAGS;
        maxfrags=RegData::DMKills;
//        MESSAGE("NETWORK: deathmatch/%d frags limit",maxfrags);
      }
      else
      {
        endcond=TIMEUP;
        gametime=RegData::DMTime;
//        MESSAGE("NETWORK: deathmatch/%d minutes time limit",gametime);
      }
    }
    else
    {
      mode=NETWORKFLAG;
//      MESSAGE("NETWORK: basebuilding mode");
    }

    users_number=Eem::get_players_num();
    DBG_CHECK(users_number>=1);
    int cnt=0;
    for (int i=1;i<=users_number;i++)
    {
      if (Eem::is_present(i))
      {
        buffer_FOR[cnt++]=i;
      }
    }
    buffer_FOR[cnt]=0;
    DBG_CHECK(cnt>0&&cnt<=users_number);

    FOR_ALL_USERS(ID)
    {
      if (Eem::is_local(ID))
      {
        this_id=ID;
        break;
      }
    }
    DBG_CHECK(check_id(this_id));

  }
  else
  {
//    MESSAGE("NETWORK: network support disabled");
    mode=STANDARD;
    users_number=1;
    buffer_FOR[0]=1;
    buffer_FOR[1]=0;
    this_id=1;
    serv_id=1;
  }

  FOR_ALL_USERS(ID)
  {
    char *tmp=Eem::get_player_name(ID);
    if (mode!=STANDARD)
    {
//      MESSAGE("NETWORK: player %d found as \"%s\".",ID,tmp);
    }
    strncpy(name_buffer[ID],tmp,256);
    name_buffer[ID][255]='\0';
  }
//  MESSAGE("NETWORK: local player is \"%s\" with id %d.",name_buffer[this_id],this_id);

}

void NoNet::quit(void)
{
  initialized=0;
}

void NoNet::refresh(int internal_only)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  DBG_CHECK(users_number>=1);
  int check_buffer[max_users+1];
  memset((void*)check_buffer,0,sizeof(check_buffer));
  FOR_ALL_USERS(ID)
  {
    check_buffer[ID]=1;
  }
  if (!internal_only)
  {
    int cnt=0;
    for (int i=1;i<=users_number;i++)
    {
      if (Eem::is_present(i))
      {
        DBG_CHECK(check_buffer[i]);
        buffer_FOR[cnt++]=i;
      }
      else if (check_buffer[i])
      {
        if (i==serv_id)
        {
          serv_id=this_id;
//          MESSAGE("NETWORK: player %d \"%s\" (server) moved out of the game.",i,name_buffer[i]);
        }
        else
        {
//          MESSAGE("NETWORK: player %d \"%s\" moved out of the game.",i,name_buffer[i]);
        }
      }
    }
    buffer_FOR[cnt]=0;
    if (users_number!=Eem::get_players_num())
    {
      users_number=Eem::get_players_num();
    }
    DBG_CHECK(cnt>0&&cnt<=users_number);
  }
  else
  {
    if (!check_id(serv_id))
    {
      FOR_ALL_USERS(SID)
      {
        if (Eem::is_server(SID))
        {
          serv_id=SID;
          break;
        }
      }
      DBG_CHECK(check_id(serv_id));
      if (serv_id==-1)
      {
        FAILURE("Query for server id failed.");
      }
      if (this_id==serv_id)
      {
//        MESSAGE("NETWORK: local player found as server.");
      }
    }
  }
}

void NoNet::init_resources(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  DBG_CHECK(!resources);
  resources=1;
  for (int i=0;i<max_users+1;i++)
  {
    ship_table[i]=NULL;
  }
  FOR_ALL_USERS(ID)
  {
    if (ID>=LevImp::land.size)
    {
      FAILURE("Not enough start places in level.");
    }
    int shipx=world->get_level()[LevImp::land.place[LevImp::land.convert[ID]]].x;
    int shipy=world->get_level()[LevImp::land.place[LevImp::land.convert[ID]]].y;
    ship_table[ID]=NEW(Myship(shipx,shipy,Mysprites::ship,ID),ship_mbn);
  }
}

void NoNet::quit_resources(void)
{
  resources=0;
  for (int i=0;i<max_users+1;i++)
  {
    ship_table[i]=NULL;
  }
}

void NoNet::init_levels(void)
{
  DBG_CHECK(sizeof(NetInfo)<=max_netinfosize);
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  DBG_CHECK(!resources);
  DBG_CHECK(!levels_synchronized);
  levels_synchronized=1;
  memset(levels_table,0,sizeof(levels_table));
  
  if (NoNet::is_network_mode())
  {
    int myver=MYVERSION;
    NetInfo data[max_users+1];
    NetInfo mydata;
    memset((void*)data,0,sizeof(data));
    memset((void*)&mydata,0,sizeof(mydata));
    mydata.packet_size=sizeof(NetInfo);
    mydata.version=MYVERSION;
    mydata.debug_status=Mp::DEBUG;
    mydata.weapon_numb=6;
    mydata.inventory_numb=5;
    mydata.IQ_Bolca=76;
    mydata.level_mask=0x00;
    Text &levtxt=GameManager::level_descript;
    DBG_CHECK(levtxt.size("net_ident")==levtxt.size("net_order"));
    int listsize=levtxt.size("net_ident");
    for (int j=0;j<listsize;j++)
    {
      mydata.level_mask|=(unsigned)levtxt.value("net_ident",j);   
    }

    Eem::start();
    Eem::user_block_send((unsigned char*)&mydata,sizeof(mydata));
    
    int stop=1;
    int retry=0;
    int received[max_users+1];
    char buffer[max_netinfosize];
    memset((void*)received,0,sizeof(received));
    do
    {
      while (!Eem::read());
      NoNet::refresh();
      retry++;
      stop=1;
      FOR_ALL_USERS(ID)
      {
        if (!received[ID])
        {
          memset((void*)buffer,0,sizeof(buffer));
          unsigned datasize=sizeof(buffer);
          Eem::select(ID);
          if (Eem::user_block_receive((unsigned char*)buffer,&datasize))
          {
            DBG_CHECK(datasize<=sizeof(buffer));
            DBG_CHECK(datasize==((NetInfo*)buffer)->packet_size);
            memcpy((void*)&data[ID],(void*)buffer,sizeof(NetInfo));
            received[ID]=1;
            DBG_MESSAGE("NETWORK: Received info from player %d, retry %d",ID,retry);
          }
          else
          {
            stop=0;
          }
        }
      }
    } while (!stop);
    DBG_MESSAGE("NETWORK: Received info data from all players, retry %d",retry);
    NoNet::refresh();
    Eem::stop();

    unsigned levelmask=0xffffffff;
    int debugstat=-1;
    FOR_ALL_USERS(ID1)
    {
      if (debugstat!=-1&&data[ID1].debug_status!=debugstat)
      {
        FAILURE("NETWORK: Cannot run network mode due to debug mode on some computers");
      }
      NetInfo &d=data[ID1];
      DBG_MESSAGE("NETWORK: Player %d: psize %d, dinfo %d, version %d, lmask %d",ID1,d.packet_size,d.debug_status,d.version,d.level_mask);
      debugstat=d.debug_status;
      levelmask&=d.level_mask;
    }
    DBG_MESSAGE("NETWORK: All players have compatible games - proceeding with initialization");

    int result=-1;
    unsigned returnvalue[max_users+1];
    if (levelmask==0)
    {
      result=SORRYWINEOTU;
      DBG_MESSAGE("NETWORK: I can't play with others: no shared levels");
    }
    else
    {
      result=OKWECANPLAY;
      DBG_MESSAGE("NETWORK: I can play with others, shared levels: %d",levelmask);
    }
    Eem::start();
    Eem::user_block_send((unsigned char*)&result,sizeof(result));
    memset((void*)received,0,sizeof(received));
    memset(returnvalue,0,sizeof(returnvalue));
    stop=1;
    retry=0;
    do
    {
      while (!Eem::read());
      NoNet::refresh();
      retry++;
      stop=1;
      FOR_ALL_USERS(ID2)
      {
        if (!received[ID2])
        {
          unsigned size=sizeof(returnvalue[ID2]);
          Eem::select(ID2);
          if (Eem::user_block_receive((unsigned char*)&returnvalue[ID2],&size))
          {
            DBG_CHECK(size==sizeof(returnvalue[ID2]));
            received[ID2]=1;
            DBG_MESSAGE("NETWORK: Received %s result from player %d, retry %d",(returnvalue[ID2]==OKWECANPLAY)?"positive":"negative",ID2,retry);
          }
          else
          {
            stop=0;
          }
        }
      }
    } while (!stop);
    DBG_MESSAGE("NETWORK: Received answers from all players, retry %d",retry);
    NoNet::refresh();

    FOR_ALL_USERS(ID3)
    {
      if (returnvalue[ID3]!=OKWECANPLAY)
      {
        FAILURE2(RegData::NoCommonLevels,"Player %s's game refuses playing (no shared levels or incorrect version)",get_player_name(ID3));
      }
    }

    DBG_CHECK(max_levels==32);
    netversion=FIRSTFINAL;
    levels_number=0;
    unsigned t=0x0001;
    for (int i=0;i<max_levels;(i++,t<<=1))
    {
      if (levelmask&t)
      {
        int ok=0;
        int textsize=levtxt.size("net_ident");
        for (int k=0;k<textsize;k++)
        {
          if (((unsigned)levtxt.value("net_ident",k))==t)
          {
            strncpy(levels_names[levels_number],levtxt.string("net_order",k),max_levelname);  
            levels_names[levels_number][max_levelname-1]='\0';
            levels_table[levels_number]=t;
            levels_local[levels_number]=k;
            levels_number++;
            ok=1;
            break;
          }
        }
        DBG_CHECK(ok);
      }
    }
    DBG_CHECK(levels_number>0&&levels_number<=max_levels);

    Eem::stop();
  }
}

void NoNet::quit_levels(void)
{
  levels_synchronized=0;
}

char *NoNet::get_level_name(int level)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  DBG_CHECK(levels_synchronized);
  DBG_CHECK(level>=0&&level<levels_number);
  return levels_names[level];
}

int *NoNet::get_levels_table(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  DBG_CHECK(levels_synchronized);
  return levels_table;
}

int NoNet::get_levels_number(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  DBG_CHECK(levels_synchronized);
  return levels_number;
}

int NoNet::get_local_level_number(int level)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  DBG_CHECK(levels_synchronized);
  DBG_CHECK(level>=0&&level<levels_number);
  return levels_local[level];
}

Myship *NoNet::get_ship(int id)
{
  DBG_CHECK(initialized);
  DBG_CHECK(resources);
  DBG_CHECK(mode);
  DBG_CHECK(levels_synchronized);
  if (id==-1)
  {
    id=this_id;
  }
  DBG_CHECK(check_id(id));
  DBG_CHECK(ship_table[id]!=NULL);
  return ship_table[id];
}

Myship *NoNet::get_ship(int x,int y,int ignore_id)
{
  DBG_CHECK(initialized);
  DBG_CHECK(resources);
  DBG_CHECK(mode);
  DBG_CHECK(levels_synchronized);
  if (!NoNet::is_network_mode())
  {
    DBG_CHECK(ignore_id==0);
    return get_ship(this_id);
  }
  else
  {
    int result_id=0;
    int result_distance=-1;
    FOR_ALL_USERS(ID)
    {
      Myship *ship=get_ship(ID);
      DBG_CHECK(check_id(ID));
      DBG_CHECK(ship!=NULL);
      int distance=Posit::vect_len(x-ship->get_x(),y-ship->get_y());
      if (ID!=ignore_id&&(distance<result_distance||result_distance==-1))
      {
        result_distance=distance;
        result_id=ID;
      }
    }
    return ((result_id!=0)?get_ship(result_id):NULL);
  }
}

int NoNet::get_this_id(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  DBG_CHECK(check_id(this_id));
  return this_id;
}

int NoNet::get_server_id(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  DBG_CHECK(check_id(serv_id));
  return serv_id;
}

int NoNet::get_players_number(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  DBG_CHECK(users_number==Eem::get_players_num());
  return users_number;
}

int NoNet::check_id(int id)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  FOR_ALL_USERS(ID)
  {
    if (id==ID)
    {
      return 1;
    }
  }
  return 0;
}

int NoNet::is_network_mode(int _mode)
{
  DBG_CHECK(initialized);
  if (_mode==-1)
  {
    DBG_CHECK(mode);
    return mode&(NETWORKMATCH|NETWORKFLAG);
  }
  else
  {
    DBG_CHECK(mode);
    DBG_CHECK(_mode==NETWORKMATCH||_mode==NETWORKFLAG);
    return (_mode==mode);
  }
}

NoNet::EndCond NoNet::end_condition(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  DBG_CHECK(endcond);
  DBG_CHECK(is_network_mode());
  return endcond;
}


NoNet::NetVersion NoNet::play_version(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  DBG_CHECK(endcond);
  DBG_CHECK(is_network_mode());
  return netversion;
}

int NoNet::game_time(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  DBG_CHECK(endcond==TIMEUP);
  return gametime;
}

int NoNet::max_frags(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  DBG_CHECK(endcond==FRAGS);
  return maxfrags;
}

char *NoNet::get_player_name(int id)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  DBG_CHECK(check_id(id));
  return name_buffer[id];
}

char *NoNet::get_player_color(int id)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  DBG_CHECK(check_id(id));
  switch (id)
  {
    case 1: return "red";
            break;
    case 2: return "yellow";
            break;
    case 3: return "green";
            break;
    case 4: return "blue";
            break;
   default: DBG_CHECK(0);
            break;
  }
  return "someone";
}

int NoNet::is_alive(int id)
{
  DBG_CHECK(initialized);
  DBG_CHECK(mode);
  DBG_CHECK(check_id(id));
  return Eem::is_present(id);
}


