#include "headers.h"

unsigned  RegData::MinimalPlayers=1;
BOOL      RegData::UseCD = TRUE;
BOOL      RegData::UseDialogs = TRUE;
BOOL	    RegData::FirstDialogs = TRUE;
BOOL	    RegData::DialogsPresent(1);
unsigned  RegData::JoyDeadZone = RegData::def_JoyDead;
BOOL      RegData::__Loaded=FALSE;
BOOL	    RegData::HiresPresent(1);
GUID	    RegData::NewOrderGuid = {0xc4460804,0x69fd,0x11cf,{0x8a,0xe1,0x44,0x45,0x53,0x54,0x0,0x0}};
BOOL	    RegData::DisableDDraw(0);
int	      RegData::HiresMode = RegData::hires_Def;
int	      RegData::LoresMode = RegData::lores_Def;
BOOL	    RegData::DisableDSound(0);
BOOL	    RegData::DisableSound(0);
unsigned  RegData::SoundChannels(def_SoundChannels);
int	      RegData::MixingMode = RegData::mix_Def;
int	      RegData::VolumeAssignment = RegData::vol_CD;
unsigned  RegData::NetTimeout(RegData::def_NetTimeout);
unsigned  RegData::NetRetries(RegData::def_NetRetries);
GUID	    RegData::ProviderGuid;
unsigned  RegData::NetPlayers = RegData::def_NetPlayers;
unsigned  RegData::GameId = 0xFFFFFFFF;
char	    RegData::NetGameName[Net::game_name_len]	   = "Sample player's game";
char	    RegData::NetPlayerName[Net::player_name_len] = "Sample player";
int	      RegData::PlayMode = RegData::pm_Def;
int	      RegData::NetMode = RegData::nm_Def;
unsigned char RegData::GameInfo[Net::game_info_len] = "";
int	      RegData::GameMode = 0;
int	      RegData::DMFinish = RegData::def_DMFinish;
unsigned  RegData::DMKills(RegData::def_DMKills);
unsigned  RegData::DMTime(RegData::def_DMTime);
unsigned  RegData::BricksPerShip(RegData::def_BricksPerShip);
unsigned  RegData::BricksPerLevel(RegData::def_BricksPerLevel);
int	      RegData::DataToLoad = RegData::load_Def;
BOOL	    RegData::FirstHires = TRUE;
BOOL	    RegData::AutoJoin=RegData::def_AutoJoin;
unsigned char RegData::BPL[MAX_NET_PLAYERS] = {20, 40, 40, 40};
unsigned char RegData::DefBPL[MAX_NET_PLAYERS] = {20, 40, 40, 40};

RegData::RespawnInfo  RegData::DefRespawnBB = {RegData::def_PowerUpTime, RegData::def_ItemTime,
                                               RegData::defBB_WeaponRecycling,
					{
					  RegData::defBB_UpVulcan,
					  RegData::defBB_UpSwarmers,
					  RegData::defBB_UpPlasma,
					  RegData::defBB_UpMissile,
					  RegData::defBB_UpCanon,
					  RegData::defBB_UpBomb,
					  RegData::defBB_UpShield,
					  RegData::defBB_ShieldRecharge,
					  RegData::defBB_MagneticShock,
					  RegData::defBB_CloakingDevice,
					  RegData::defBB_Mine,
					  RegData::defBB_DoubledShield,
            RegData::defBB_Random
					}
			  };
RegData::RespawnInfo  RegData::DefRespawnDM = {RegData::def_PowerUpTime, RegData::def_ItemTime,
                                               RegData::defDM_WeaponRecycling,
					{
					  RegData::defDM_UpVulcan,
					  RegData::defDM_UpSwarmers,
					  RegData::defDM_UpPlasma,
					  RegData::defDM_UpMissile,
					  RegData::defDM_UpCanon,
					  RegData::defDM_UpBomb,
					  RegData::defDM_UpShield,
					  RegData::defDM_ShieldRecharge,
					  RegData::defDM_MagneticShock,
					  RegData::defDM_CloakingDevice,
					  RegData::defDM_Mine,
					  RegData::defDM_DoubledShield,
            RegData::defDM_Random
					}
		    };      		

RegData::RespawnInfo  RegData::RespawnBB;
RegData::RespawnInfo  RegData::RespawnDM;

RegData::ControlsInfo RegData::Controls;
RegData::ControlsInfo RegData::DefControls;
RegData::PlayersInfo  RegData::Players;

const char *RegData::RespawnStr[RESPAWN_ITEMS_NUM] =
{
  " vulcan ammo",
  " swarmers",
  " plasma gun ammo",
  " missiles",
  " cannon ammo",
  " grenades",
  " shield energy",
  " shield recharge",
  " em-shock",
  " cloaking device",
  " mines",
  " doubled shield",
  " rotating power-up"
};

const char *RegData::ControlsStr[CONTROLS_NUM] =
{
  " turn left",
  " turn right",
  " move forward",
  " move back",
  " primary fire",
  " secondary fire",
  " strafe left",
  " strafe right",
  " strafe on",
  " previous inventory item",
  " next inventory item",
  " use inventory item",
  " turbo on",
  " turbo lock",
  " weapon 1 - vulcan",
  " weapon 2 - swarmers",
  " weapon 3 - plasma gun",
  " weapon 4 - missiles",
  " weapon 5 - cannon",
  " weapon 6 - grenades",
  " previous weapon",
  " next weapon",
  " follow mouse cursor"
};

const char *RegData::ControlsSetStr[CONTROLS_SET_NUM] =
{
  "keyboard prefered",
  "mouse prefered",
  "joystick prefered"
};

#include <1cw_strg.h>

static char sPlayerPrefix[]  = "PLAYER";
static char sMousePrefix[]   = "M: ";
static char sKbdPrefix[]     = "K: ";
static char sJoyPrefix[]     = "J: ";

static char sec_players[]	          = "players";
static char key_mouse_follow[]	    = "mouse follow";
static char key_current_set[]	      = "current set";
static char key_enhanced_kbd[]	    = "enhanced kbd";
static char key_players_number[]    = "number of players";
static char key_current_player[]    = "current player";
static char key_skill[] 	          = "skill level";
static char val_skill_normal[]  	  = "normal";
static char val_skill_hard[]	      = "hard";
static char key_auto_join[]	        = "auto join";
static char key_spr_first_hires[]   = "first hires";
static char key_sos_first_dialogs[] = "first dialogs";
static char key_BPL[]               = "BPL";

//=============================================================================
void RegData::Init(void)
{
  if (Comm::production)
    MinimalPlayers = 2;
  else
    MinimalPlayers = 1;
  memset(GameInfo, 0, Net::game_info_len);
  DefControls.EnhancedKbd = def_EnhancedKbd;
  DefControls.CurrentSet  = def_CurrentSet;
  DefControls.Controls[ctlset_Kbd][ctl_MoveLeft]      = (scan_Kbd|Kbd::get_scan(Kbd::left));
  DefControls.Controls[ctlset_Kbd][ctl_MoveRight]     = (scan_Kbd|Kbd::get_scan(Kbd::right));
  DefControls.Controls[ctlset_Kbd][ctl_MoveForward]   = (scan_Kbd|Kbd::get_scan(Kbd::up));
  DefControls.Controls[ctlset_Kbd][ctl_MoveBack]      = (scan_Kbd|Kbd::get_scan(Kbd::down));
  DefControls.Controls[ctlset_Kbd][ctl_Fire1]	        = (scan_Kbd|Kbd::get_scan(Kbd::space));
  DefControls.Controls[ctlset_Kbd][ctl_Fire2]	        = (scan_Kbd|Kbd::get_scan(Kbd::lctrl));
  DefControls.Controls[ctlset_Kbd][ctl_StrafeLeft]    = (scan_Kbd|Kbd::get_scan(Kbd::keyz));
  DefControls.Controls[ctlset_Kbd][ctl_StrafeRight]   = (scan_Kbd|Kbd::get_scan(Kbd::keyx));
  DefControls.Controls[ctlset_Kbd][ctl_StrafeShift]   = (scan_Kbd|Kbd::get_scan(Kbd::lalt));
  DefControls.Controls[ctlset_Kbd][ctl_PrevInvent]    = (scan_Kbd|Kbd::get_scan(Kbd::gray_minus));
  DefControls.Controls[ctlset_Kbd][ctl_NextInvent]    = (scan_Kbd|Kbd::get_scan(Kbd::gray_plus));
  DefControls.Controls[ctlset_Kbd][ctl_UseInvent]     = (scan_Kbd|Kbd::get_scan(Kbd::enter));
  DefControls.Controls[ctlset_Kbd][ctl_TurboShift]    = (scan_Kbd|Kbd::get_scan(Kbd::lshift));
  DefControls.Controls[ctlset_Kbd][ctl_TurboLock]     = (scan_Kbd|Kbd::get_scan(Kbd::capslock));
  DefControls.Controls[ctlset_Kbd][ctl_Weapon1]       = (scan_Kbd|Kbd::get_scan(Kbd::keyb_1));
  DefControls.Controls[ctlset_Kbd][ctl_Weapon2]       = (scan_Kbd|Kbd::get_scan(Kbd::keyb_2));
  DefControls.Controls[ctlset_Kbd][ctl_Weapon3]       = (scan_Kbd|Kbd::get_scan(Kbd::keyb_3));
  DefControls.Controls[ctlset_Kbd][ctl_Weapon4]       = (scan_Kbd|Kbd::get_scan(Kbd::keyb_4));
  DefControls.Controls[ctlset_Kbd][ctl_Weapon5]       = (scan_Kbd|Kbd::get_scan(Kbd::keyb_5));
  DefControls.Controls[ctlset_Kbd][ctl_Weapon6]       = (scan_Kbd|Kbd::get_scan(Kbd::keyb_6));
  DefControls.Controls[ctlset_Kbd][ctl_PrevWeapon]    = (scan_Kbd|Kbd::get_scan(Kbd::pgup));
  DefControls.Controls[ctlset_Kbd][ctl_NextWeapon]    = (scan_Kbd|Kbd::get_scan(Kbd::pgdn));
  DefControls.Controls[ctlset_Kbd][ctl_GotoMouse]     = (scan_Mouse|Mouse::get_scan(Mouse::right_button));

  DefControls.Controls[ctlset_Mouse][ctl_MoveLeft]    = (scan_Kbd|Kbd::get_scan(Kbd::left));
  DefControls.Controls[ctlset_Mouse][ctl_MoveRight]   = (scan_Kbd|Kbd::get_scan(Kbd::right));
  DefControls.Controls[ctlset_Mouse][ctl_MoveForward] = (scan_Kbd|Kbd::get_scan(Kbd::up));
  DefControls.Controls[ctlset_Mouse][ctl_MoveBack]    = (scan_Kbd|Kbd::get_scan(Kbd::down));
  DefControls.Controls[ctlset_Mouse][ctl_Fire1]       = (scan_Mouse|Mouse::get_scan(Mouse::left_button));
  DefControls.Controls[ctlset_Mouse][ctl_Fire2]       = (scan_Kbd|Kbd::get_scan(Kbd::space));
  DefControls.Controls[ctlset_Mouse][ctl_StrafeLeft]  = (scan_Kbd|Kbd::get_scan(Kbd::keyz));
  DefControls.Controls[ctlset_Mouse][ctl_StrafeRight] = (scan_Kbd|Kbd::get_scan(Kbd::keyx));
  DefControls.Controls[ctlset_Mouse][ctl_StrafeShift] = (scan_Kbd|Kbd::get_scan(Kbd::lalt));
  DefControls.Controls[ctlset_Mouse][ctl_PrevInvent]  = (scan_Kbd|Kbd::get_scan(Kbd::gray_minus));
  DefControls.Controls[ctlset_Mouse][ctl_NextInvent]  = (scan_Kbd|Kbd::get_scan(Kbd::gray_plus));
  DefControls.Controls[ctlset_Mouse][ctl_UseInvent]   = (scan_Kbd|Kbd::get_scan(Kbd::enter));
  DefControls.Controls[ctlset_Mouse][ctl_TurboShift]  = (scan_Kbd|Kbd::get_scan(Kbd::lshift));
  DefControls.Controls[ctlset_Mouse][ctl_TurboLock]   = (scan_Kbd|Kbd::get_scan(Kbd::capslock));
  DefControls.Controls[ctlset_Mouse][ctl_Weapon1]     = (scan_Kbd|Kbd::get_scan(Kbd::keyb_1));
  DefControls.Controls[ctlset_Mouse][ctl_Weapon2]     = (scan_Kbd|Kbd::get_scan(Kbd::keyb_2));
  DefControls.Controls[ctlset_Mouse][ctl_Weapon3]     = (scan_Kbd|Kbd::get_scan(Kbd::keyb_3));
  DefControls.Controls[ctlset_Mouse][ctl_Weapon4]     = (scan_Kbd|Kbd::get_scan(Kbd::keyb_4));
  DefControls.Controls[ctlset_Mouse][ctl_Weapon5]     = (scan_Kbd|Kbd::get_scan(Kbd::keyb_5));
  DefControls.Controls[ctlset_Mouse][ctl_Weapon6]     = (scan_Kbd|Kbd::get_scan(Kbd::keyb_6));
  DefControls.Controls[ctlset_Mouse][ctl_PrevWeapon]  = (scan_Kbd|Kbd::get_scan(Kbd::pgup));
  DefControls.Controls[ctlset_Mouse][ctl_NextWeapon]  = (scan_Kbd|Kbd::get_scan(Kbd::pgdn));
  DefControls.Controls[ctlset_Mouse][ctl_GotoMouse]   = (scan_Mouse|Mouse::get_scan(Mouse::right_button));

  DefControls.Controls[ctlset_Joy][ctl_MoveLeft]      = (scan_Joy|Joy::get_scan(Joy::left));
  DefControls.Controls[ctlset_Joy][ctl_MoveRight]     = (scan_Joy|Joy::get_scan(Joy::right));
  DefControls.Controls[ctlset_Joy][ctl_MoveForward]   = (scan_Joy|Joy::get_scan(Joy::up));
  DefControls.Controls[ctlset_Joy][ctl_MoveBack]      = (scan_Joy|Joy::get_scan(Joy::down));
  DefControls.Controls[ctlset_Joy][ctl_Fire1]	        = (scan_Kbd|Kbd::get_scan(Kbd::space));
  DefControls.Controls[ctlset_Joy][ctl_Fire2]	        = (scan_Joy|Joy::get_scan(Joy::button1));
  DefControls.Controls[ctlset_Joy][ctl_StrafeLeft]    = (scan_Kbd|Kbd::get_scan(Kbd::keyz));
  DefControls.Controls[ctlset_Joy][ctl_StrafeRight]   = (scan_Kbd|Kbd::get_scan(Kbd::keyx));
  DefControls.Controls[ctlset_Joy][ctl_StrafeShift]   = (scan_Kbd|Kbd::get_scan(Kbd::lalt));
  DefControls.Controls[ctlset_Joy][ctl_PrevInvent]    = (scan_Kbd|Kbd::get_scan(Kbd::gray_minus));
  DefControls.Controls[ctlset_Joy][ctl_NextInvent]    = (scan_Kbd|Kbd::get_scan(Kbd::gray_plus));
  DefControls.Controls[ctlset_Joy][ctl_UseInvent]     = (scan_Kbd|Kbd::get_scan(Kbd::enter));
  DefControls.Controls[ctlset_Joy][ctl_TurboShift]    = (scan_Joy|Joy::get_scan(Joy::button2));
  DefControls.Controls[ctlset_Joy][ctl_TurboLock]     = (scan_Kbd|Kbd::get_scan(Kbd::capslock));
  DefControls.Controls[ctlset_Joy][ctl_Weapon1]       = (scan_Kbd|Kbd::get_scan(Kbd::keyb_1));
  DefControls.Controls[ctlset_Joy][ctl_Weapon2]       = (scan_Kbd|Kbd::get_scan(Kbd::keyb_2));
  DefControls.Controls[ctlset_Joy][ctl_Weapon3]       = (scan_Kbd|Kbd::get_scan(Kbd::keyb_3));
  DefControls.Controls[ctlset_Joy][ctl_Weapon4]       = (scan_Kbd|Kbd::get_scan(Kbd::keyb_4));
  DefControls.Controls[ctlset_Joy][ctl_Weapon5]       = (scan_Kbd|Kbd::get_scan(Kbd::keyb_5));
  DefControls.Controls[ctlset_Joy][ctl_Weapon6]       = (scan_Kbd|Kbd::get_scan(Kbd::keyb_6));
  DefControls.Controls[ctlset_Joy][ctl_PrevWeapon]    = (scan_Kbd|Kbd::get_scan(Kbd::pgup));
  DefControls.Controls[ctlset_Joy][ctl_NextWeapon]    = (scan_Kbd|Kbd::get_scan(Kbd::pgdn));
  DefControls.Controls[ctlset_Joy][ctl_GotoMouse]     = (scan_Mouse|Mouse::get_scan(Mouse::right_button));

  memset(&Players, 0, sizeof(Players));
  strncpy( Players.Names[0], "default", PLAYER_NAME_LEN-1);
  Players.Skills[0] = skill_Def;
  Players.Current = 0;
  Players.Quantity = 1;

  char uname[MAX_COMPUTERNAME_LENGTH];
  DWORD i=MAX_COMPUTERNAME_LENGTH;
  if ( !GetUserName( uname, &i ))
    strncpy(NetPlayerName, "<unknown>", Net::player_name_len-1);
  else
    strncpy(NetPlayerName, uname, Net::player_name_len-1);
}
//-----------------------------------------------------------------------------
void RegData::ToRegistry(int loader_save)
{
  if ((!__Loaded)&&(!loader_save))
    return;
  EncodeGameInfo();
  if (loader_save)
  {
    BricksPerShip = BPL[NetPlayers-1];
    Registry::set_binary (sec_eem, key_eem_game, (char*)(&NewOrderGuid), sizeof(GUID));
    Registry::set_binary (sec_eem, key_eem_provider, (char*)(&ProviderGuid),  sizeof(GUID));
    Registry::set_int	   (sec_eem, key_eem_game_id, GameId);
    Registry::set_int	   (sec_eem, key_eem_players_num, NetPlayers);
    Registry::set_int  	 (sec_eem, key_eem_timeout, NetTimeout);
    Registry::set_string (sec_eem, key_eem_game_name, NetGameName);
    Registry::set_string (sec_eem, key_eem_player_name, NetPlayerName);
    Registry::set_int	   (sec_eem, key_eem_timeout, NetTimeout);
    Registry::set_int  	 (sec_eem, key_eem_retries, NetRetries);
    Registry::set_int	   (sec_eem, key_auto_join, AutoJoin);
    Registry::set_int	   (sec_eem, key_eem_joy_dead, JoyDeadZone);
    if (PlayMode==pm_Multi)
      Registry::set_string (sec_eem, key_eem_play_mode, val_eem_play_multi);
    else if (PlayMode==pm_Single)
      Registry::set_string (sec_eem, key_eem_play_mode, val_eem_play_single);
    if (NetMode==nm_Client)
      Registry::set_string (sec_eem, key_eem_net_mode, val_eem_client);
    else if (NetMode==nm_Server)
      Registry::set_string(sec_eem, key_eem_net_mode, val_eem_server);
    Registry::set_binary(sec_eem, key_eem_game_info, (char*)GameInfo, Net::game_info_len);
    Registry::set_int (sec_sos, key_sos_disable_dsound, DisableDSound);
    Registry::set_int (sec_sos, key_sos_disable_sound, DisableSound);
    Registry::set_int (sec_sos, key_sos_mixing_mode, MixingMode);
    Registry::set_int (sec_sos, key_sos_volume, VolumeAssignment);
    Registry::set_int (sec_sos, key_sos_channels, SoundChannels);
    Registry::set_int (sec_sos, key_sos_use_CD, UseCD);
    Registry::set_int (sec_sos, key_sos_use_dialogs, UseDialogs);
    Registry::set_int (sec_spr, key_spr_disable_ddraw, DisableDDraw);
    Registry::set_int (sec_spr, key_spr_hires_mode, HiresMode);
    Registry::set_int (sec_spr, key_spr_lores_mode, LoresMode);
    Registry::set_int (sec_spr, key_spr_load_mode, DataToLoad);
    Registry::set_int (sec_spr, key_spr_first_hires, FirstHires);
    Registry::set_int (sec_sos, key_sos_first_dialogs, FirstDialogs);
  }
  char buf[80];
  int i;

// Players
  Registry::delete_section(sec_players);
  for (i = 0; i<Players.Quantity; i++)
  {
    sprintf(buf,"%d",i);
    Registry::set_string(sec_players, buf, Players.Names[i]);
    sprintf(buf,"%s:%s",sPlayerPrefix, Players.Names[i]);
    if (Players.Skills[i]==skill_Normal)
      Registry::set_string(buf, key_skill, val_skill_normal);
    else if (Players.Skills[i]==skill_Hard)
      Registry::set_string(buf, key_skill, val_skill_hard);
  }
  Registry::set_int (sec_players, key_players_number, Players.Quantity);
  Registry::set_int (sec_players, key_current_player, Players.Current);
  PlayerToRegistry(loader_save);
}
//-----------------------------------------------------------------------------
void RegData::PlayerToRegistry(int loader_save)
{
  char* sec_player = GetPlayerSection();
  char buf[80];
// Controls
  for (int i=0; i<CONTROLS_NUM; i++)
  {
    strcpy(buf, sMousePrefix);
    strcat(buf, ControlsStr[i]);
    Registry::set_int(sec_player, buf, Controls.Controls[ctlset_Mouse][i]);
    strcpy(buf, sKbdPrefix);
    strcat(buf, ControlsStr[i]);
    Registry::set_int(sec_player, buf, Controls.Controls[ctlset_Kbd][i]);
    strcpy(buf, sJoyPrefix);
    strcat(buf, ControlsStr[i]);
    Registry::set_int(sec_player, buf, Controls.Controls[ctlset_Joy][i]);
  }
  Registry::set_int(sec_player, key_current_set,  Controls.CurrentSet);
  Registry::set_int(sec_player, key_enhanced_kbd, Controls.EnhancedKbd);
  Registry::set_binary(sec_player, key_BPL, (char*)BPL, MAX_NET_PLAYERS);
  if (loader_save)
    Registry::set_binary(sec_player, key_eem_game_info, (char*)GameInfo, Net::game_info_len);
}
//-----------------------------------------------------------------------------
void RegData::FromRegistry(int loader_load)
{
  __Loaded = TRUE;
  RespawnBB = DefRespawnBB;
  RespawnDM = DefRespawnDM;
  const char *temp;
  unsigned size = sizeof(GUID);

  temp = Registry::get_string(sec_eem, key_eem_play_mode, val_eem_play_single);
  if (strcmpi(temp, val_eem_play_multi)==0)
    PlayMode = pm_Multi;
  else
    PlayMode = pm_Single;
  Registry::get_binary(sec_eem, key_eem_provider, (char*)(&ProviderGuid), &size);
  NetPlayers = Registry::get_int  (sec_eem, key_eem_players_num, NetPlayers);
  temp = Registry::get_string(sec_eem, key_eem_game_name, NetGameName);
  strcpy(NetGameName, temp);
  temp = Registry::get_string(sec_eem, key_eem_net_mode, val_eem_client);
  if (strcmpi(temp, val_eem_client)==0)
    NetMode = nm_Client;
  else
    NetMode = nm_Server;

  strncpy(NetPlayerName, Registry::get_string (sec_eem, 
          key_eem_player_name, NetPlayerName), Net::player_name_len-1);
  NetTimeout	     = Registry::get_int (sec_eem, key_eem_timeout, 3);
  NetRetries	     = Registry::get_int (sec_eem, key_eem_retries, 40);
  AutoJoin  	     = Registry::get_int (sec_eem, key_auto_join, AutoJoin);
  JoyDeadZone      = Registry::get_int (sec_eem, key_eem_joy_dead, JoyDeadZone);
  DisableDSound    = Registry::get_int (sec_sos, key_sos_disable_dsound, 0);
  DisableSound	   = Registry::get_int (sec_sos, key_sos_disable_sound, 0);
  MixingMode	     = Registry::get_int (sec_sos, key_sos_mixing_mode, mix_Def);
  VolumeAssignment = Registry::get_int (sec_sos, key_sos_volume, VolumeAssignment);
  SoundChannels    = Registry::get_int (sec_sos, key_sos_channels, def_SoundChannels);
  UseCD            = Registry::get_int (sec_sos, key_sos_use_CD, UseCD);
  UseDialogs       = Registry::get_int (sec_sos, key_sos_use_dialogs, UseDialogs);
  DisableDDraw	   = Registry::get_int (sec_spr, key_spr_disable_ddraw, 0);
  HiresMode	       = Registry::get_int (sec_spr, key_spr_hires_mode, hires_Def);
  LoresMode	       = Registry::get_int (sec_spr, key_spr_lores_mode, lores_Def);
  DataToLoad	     = Registry::get_int (sec_spr, key_spr_load_mode, DataToLoad);
  FirstHires	     = Registry::get_int (sec_spr, key_spr_first_hires, FirstHires);
  FirstDialogs     = Registry::get_int (sec_sos, key_sos_first_dialogs, FirstDialogs);

  size = Net::game_info_len;
  if (!loader_load)
  {
    if (Registry::get_binary (sec_eem, key_eem_game_info, (char*)GameInfo, &size))
      DecodeGameInfo();
    else
      DefaultGameInfo();
  }

  char buf[80];
  int i;
// Players
  Players.Quantity = Registry::get_int(sec_players, key_players_number, Players.Quantity);
  if (Players.Quantity==0)
    Players.Quantity = 1;
  for (i = 0; i<Players.Quantity; i++)
  {
    sprintf(buf,"%d",i);
    strcpy (Players.Names[i], Registry::get_string(sec_players, buf, Players.Names[i]));
    sprintf(buf,"%s:%s",sPlayerPrefix, Players.Names[i]);
    if (strcmpi(Registry::get_string(buf, key_skill, val_skill_normal), val_skill_normal)==0)
      Players.Skills[i]=skill_Normal;
    else
      Players.Skills[i]=skill_Hard;
  }
  Players.Current = Registry::get_int (sec_players, key_current_player, Players.Current);
  PlayerFromRegistry(loader_load);
  Vkey::reset_content();
  for (i = 0; i<CONTROLS_NUM; i++)
  {
    if (((Controls.Controls[Controls.CurrentSet][i])&scan_Kbd)!=0)
      Controls.VKeys[i] = Vkey::set(Vkey::dev_Kbd,
				       (unsigned char)(Controls.Controls[Controls.CurrentSet][i]),
				       (char*)ControlsStr[i]);
    else if (((Controls.Controls[Controls.CurrentSet][i])&scan_Mouse)!=0)
      Controls.VKeys[i] = Vkey::set(Vkey::dev_Mouse,
				       (unsigned char)(Controls.Controls[Controls.CurrentSet][i]),
				       (char*)ControlsStr[i]);
    else if (((Controls.Controls[Controls.CurrentSet][i])&scan_Joy)!=0)
      Controls.VKeys[i] = Vkey::set(Vkey::dev_Joy,
				       (unsigned char)(Controls.Controls[Controls.CurrentSet][i]),
				       (char*)ControlsStr[i]);
  }
}
//-----------------------------------------------------------------------------
void RegData::PlayerFromRegistry(int loader_load)
{
  char* sec_player = GetPlayerSection();
  char buf[80];
// Controls
  for (int i = 0; i < CONTROLS_NUM; i++)
  {
    strcpy(buf, sMousePrefix);
    strcat(buf, ControlsStr[i]);
    Controls.Controls[ctlset_Mouse][i] = Registry::get_int(sec_player, buf,
					      DefControls.Controls[ctlset_Mouse][i]);
    strcpy(buf, sKbdPrefix);
    strcat(buf, ControlsStr[i]);
    Controls.Controls[ctlset_Kbd][i] = Registry::get_int(sec_player, buf,
					      DefControls.Controls[ctlset_Kbd][i]);
    strcpy(buf, sJoyPrefix);
    strcat(buf, ControlsStr[i]);
    Controls.Controls[ctlset_Joy][i] = Registry::get_int(sec_player, buf,
					      DefControls.Controls[ctlset_Joy][i]);
  }
  Controls.CurrentSet  = Registry::get_int(sec_player, key_current_set,  DefControls.CurrentSet);
  Controls.EnhancedKbd = Registry::get_int(sec_player, key_enhanced_kbd, DefControls.EnhancedKbd);
  unsigned size = Net::game_info_len;
  if (loader_load)
  {
    if (Registry::get_binary (sec_player, key_eem_game_info, (char*)GameInfo, &size))
      DecodeGameInfo();
    else
      DefaultGameInfo();
  }
  size = MAX_NET_PLAYERS;
  BOOL res = Registry::get_binary(sec_player, key_BPL, (char*)BPL, &size);
  if (!res)
    memcpy(BPL, DefBPL, MAX_NET_PLAYERS);
}
//-----------------------------------------------------------------------------
void RegData::DecodeGameInfo(void)
{
  if (Bit::is((unsigned char*)GameInfo,0))
    GameMode = gm_BaseBuild;
  else
    GameMode = gm_Deathmatch;
  if (Bit::is((unsigned char*)GameInfo,1))
    DMFinish = dm_Time;
  else
    DMFinish = dm_Kill;
  DMKills = GameInfo[1];
  if ((DMKills < min_DMKills)||(DMKills > max_DMKills))
    DMKills = def_DMKills;
  DMTime  = GameInfo[2];
  if ((DMTime < min_DMTime)||(DMTime > max_DMTime))
    DMTime = def_DMTime;
  BricksPerShip   = GameInfo[3];
  if ((BricksPerShip < min_BricksPerShip)||(BricksPerShip > max_BricksPerShip))
    BricksPerShip = def_BricksPerShip;
  BricksPerLevel  = GameInfo[4];
  if ((BricksPerLevel < min_BricksPerLevel)||(BricksPerLevel > max_BricksPerLevel))
    BricksPerLevel = def_BricksPerLevel;
  if (Bit::is(&(GameInfo[5]),0))
    RespawnBB.WeaponRecycling = 1;
  else
    RespawnBB.WeaponRecycling = 0;
  if (Bit::is(&(GameInfo[5]),1))
    RespawnDM.WeaponRecycling = 1;
  else
    RespawnDM.WeaponRecycling = 0;
  BOOL BBdef = FALSE;
  BOOL DMdef = FALSE;
  RespawnBB.PowerUpTime = GameInfo[6];
  if ((RespawnBB.PowerUpTime < min_PowerUpTime)||(RespawnBB.PowerUpTime > max_PowerUpTime))
  {
    RespawnBB.PowerUpTime = def_PowerUpTime;
    BBdef = TRUE;
  }
  RespawnDM.PowerUpTime = GameInfo[7];
  if ((RespawnDM.PowerUpTime < min_PowerUpTime)||(RespawnDM.PowerUpTime > max_PowerUpTime))
  {
    RespawnDM.PowerUpTime = def_PowerUpTime;
    DMdef = TRUE;
  }
  RespawnBB.ItemTime	= GameInfo[8];
  if ((RespawnBB.ItemTime < min_ItemTime)||(RespawnBB.ItemTime > max_ItemTime))
  {
    RespawnBB.ItemTime = def_ItemTime;
    BBdef = TRUE;
  }
  RespawnDM.ItemTime	= GameInfo[9];
  if ((RespawnDM.ItemTime < min_ItemTime)||(RespawnDM.ItemTime > max_ItemTime))
  {
    RespawnDM.ItemTime = def_ItemTime;
    DMdef = TRUE;
  }
  if (!BBdef)
    for (int i=0; i<RESPAWN_ITEMS_NUM; i++)
      if (Bit::is(&(GameInfo[10]),i))
      	RespawnBB.Flags[i]=1;
      else
      	RespawnBB.Flags[i]=0;
  if (!DMdef)
    for (int i=0; i<RESPAWN_ITEMS_NUM; i++)
      if (Bit::is(&(GameInfo[12]),i))
      	RespawnDM.Flags[i]=1;
      else
      	RespawnDM.Flags[i]=0;
}
//-----------------------------------------------------------------------------
void RegData::EncodeGameInfo(void)
{
  memset(GameInfo, 0, Net::game_info_len);
  if (GameMode==gm_BaseBuild)
    Bit::set((unsigned char*)GameInfo,0);
  if (DMFinish==dm_Time)
    Bit::set((unsigned char*)GameInfo,1);
  GameInfo[1]=(unsigned char)DMKills;
  GameInfo[2]=(unsigned char)DMTime;
  GameInfo[3]=(unsigned char)BricksPerShip;
  GameInfo[4]=(unsigned char)BricksPerLevel;
  unsigned char wr = 0;
  if (RespawnBB.WeaponRecycling)
    Bit::set(&wr, 0);
  if (RespawnDM.WeaponRecycling)
    Bit::set(&wr, 1);
  GameInfo[5]=wr;
  GameInfo[6]=(unsigned char)RespawnBB.PowerUpTime;
  GameInfo[7]=(unsigned char)RespawnDM.PowerUpTime;
  GameInfo[8]=(unsigned char)RespawnBB.ItemTime;
  GameInfo[9]=(unsigned char)RespawnDM.ItemTime;
  for (int i=0; i<RESPAWN_ITEMS_NUM; i++)
  {
    if (RespawnBB.Flags[i]!=0)
      Bit::set(&(GameInfo[10]),i);
    if (RespawnDM.Flags[i]!=0)
      Bit::set(&(GameInfo[12]),i);
  }
}
//-----------------------------------------------------------------------------
void RegData::Quit(void)
{
}
//-----------------------------------------------------------------------------
static char current_player_sec[80];

char *RegData::GetPlayerSection(void)
{
  sprintf(current_player_sec,"%s:%s",sPlayerPrefix, Players.Names[Players.Current]);
  return(current_player_sec);
}
//-----------------------------------------------------------------------------
int RegData::GetPlayerSkill(void)
{
  return(Players.Skills[Players.Current]);
}
//-----------------------------------------------------------------------------
int RegData::GetCurrentControlSet(void)
{
  return (Controls.CurrentSet);
}
//-----------------------------------------------------------------------------
void RegData::SetPlayersInfo(RegData::PlayersInfo pi)
{
  ToRegistry(1);
  for (int i=0; i<Players.Quantity; i++)
  {
    BOOL found = FALSE;
    for (int k=0; k<pi.Quantity; k++)
      if (Players.Id[i]==pi.Id[k])
      {
	found = TRUE;
	break;
      }
    if (!found)
    {
      char buf[80];
      sprintf(buf,"%s:%s",sPlayerPrefix, Players.Names[i]);
      Registry::delete_section(buf);
    }
  }
  Players = pi;
  Controls = DefControls;
  PlayerFromRegistry(1);
}
//-----------------------------------------------------------------------------
void RegData::GetPlayersInfo(RegData::PlayersInfo *pi)
{
  for (int i=0; i<PLAYERS_NUM; i++)
    Players.Id[i] = i+1;
  *pi = Players;
}
//-----------------------------------------------------------------------------
void RegData::DefaultGameInfo(void)
{
  GameMode = gm_Def;
  DMFinish = dm_Def;
  DMKills  = def_DMKills;
  DMTime   = def_DMTime;
  BricksPerShip = def_BricksPerShip;
  BricksPerLevel = def_BricksPerLevel;
  RespawnBB = DefRespawnBB;
  RespawnDM = DefRespawnDM;
}
//-----------------------------------------------------------------------------
void RegData::SetCurrentControlSet(int cset)
{
  Controls.CurrentSet = cset;
  Vkey::reset_content();
  for (int i = 0; i<CONTROLS_NUM; i++)
  {
    if (((Controls.Controls[Controls.CurrentSet][i])&scan_Kbd)!=0)
      Controls.VKeys[i] = Vkey::set(Vkey::dev_Kbd,
				       (unsigned char)(Controls.Controls[Controls.CurrentSet][i]),
				       (char*)ControlsStr[i]);
    else if (((Controls.Controls[Controls.CurrentSet][i])&scan_Mouse)!=0)
      Controls.VKeys[i] = Vkey::set(Vkey::dev_Mouse,
				       (unsigned char)(Controls.Controls[Controls.CurrentSet][i]),
				       (char*)ControlsStr[i]);
    else if (((Controls.Controls[Controls.CurrentSet][i])&scan_Joy)!=0)
      Controls.VKeys[i] = Vkey::set(Vkey::dev_Joy,
				       (unsigned char)(Controls.Controls[Controls.CurrentSet][i]),
				       (char*)ControlsStr[i]);
  }
}
//=============================================================================
