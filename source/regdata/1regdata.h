#ifndef _1REGDATA_H_INCLUDED
#define _1REGDATA_H_INCLUDED

#ifndef EXCLUDE_LIBS
#  ifdef _DEBUG
#    pragma comment(lib,"1regdatd.lib")
#  else
#    pragma comment(lib,"1regdata.lib")
#  endif
#endif

#define RESPAWN_ITEMS_NUM 13 
#define CONTROLS_NUM      23 
#define CONTROLS_SET_NUM   3
#define PLAYERS_NUM       20
#define PLAYER_NAME_LEN   30
#define MAX_NET_PLAYERS    4

class RegData
{
private:
  static unsigned char    GameInfo[Net::game_info_len];
public:
  enum
  {
    NoCommonLevels        = 400,
    DiagnosticFailure     = 401,
    CopyProtection        = 402
  };
  enum
  {
    min_JoyDead           = 5,
    def_JoyDead           = 20,
    max_JoyDead           = 50,

    pm_Multi              = 0,      
    pm_Single             = 1,
    pm_Def                = pm_Single,

    nm_Client             = 0,         
    nm_Server             = 1,
    nm_Def                = nm_Client,

    hires_480             = 1,
    hires_400             = 2,
    hires_Def             = hires_480,

    lores_240             = 1,
    lores_200             = 2,
    lores_Def             = lores_240,
                     
    mix_16_22             = 2,
    mix_8_22              = 3,
    mix_16_11             = 4,
    mix_8_11              = 5,
    mix_Def               = mix_8_22,

    vol_CD                = MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC,
    vol_LineIn            = MIXERLINE_COMPONENTTYPE_SRC_LINE,
    vol_MIDI              = MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER,

    min_NetTimeout        = 1,
    def_NetTimeout        = 3,
    max_NetTimeout        = 10,

    min_NetRetries        = 5,
    def_NetRetries        = 40,
    max_NetRetries        = 250,

    min_NetPlayers        = 1,
    def_NetPlayers        = 2,
    max_NetPlayers        = 4,

    gm_Deathmatch         = 0,            // mozliwe wartosci GameMode
    gm_BaseBuild          = 1,
    gm_Def                = gm_Deathmatch,

    dm_Kill               = 0,            // kryterium zakonczenia deatchmatcha sa fragi
    dm_Time               = 1,            // kryterium zakonczenia deatchmatcha jest czas 
    dm_Def                = dm_Kill, 

    min_DMKills           = 1,
    max_DMKills           = 250,
    def_DMKills           = 8,

    min_DMTime            = 1,
    max_DMTime            = 250,
    def_DMTime            = 5,

    def_DMFinish          = dm_Kill,

    min_BricksPerShip     = 10,
    max_BricksPerShip     = 100,
    def_BricksPerShip     = 40,

    min_BricksPerLevel    = 1,
    max_BricksPerLevel    = 100,
    def_BricksPerLevel    = 25,

    resp_UpVulcan         = 0,
    resp_UpSwarmers       = 1,
    resp_UpPlasma         = 2,
    resp_UpMissile        = 3,
    resp_UpCanon          = 4,
    resp_UpBomb           = 5,
    resp_UpShield         = 6,
    resp_ShieldRecharge   = 7,
    resp_MagneticShock    = 8,
    resp_CloakingDevice   = 9,
    resp_Mine             = 10,
    resp_DoubledShield    = 11,
    resp_Random           = 13,

    min_PowerUpTime       = 10,
    max_PowerUpTime       = 255,
    def_PowerUpTime       = 150,

    min_ItemTime          = 10,
    max_ItemTime          = 255,
    def_ItemTime          = 90,

    defBB_UpVulcan        = 1,
    defBB_UpSwarmers      = 1,
    defBB_UpPlasma        = 1,
    defBB_UpMissile       = 1,
    defBB_UpCanon         = 1,
    defBB_UpBomb          = 1,
    defBB_UpShield        = 1,
    defBB_ShieldRecharge  = 1,
    defBB_MagneticShock   = 1,
    defBB_CloakingDevice  = 1,
    defBB_Mine            = 1,
    defBB_DoubledShield   = 1,
    defBB_Random          = 1,

    defBB_WeaponRecycling = 1,

    defDM_UpVulcan        = 1,
    defDM_UpSwarmers      = 1,
    defDM_UpPlasma        = 1,
    defDM_UpMissile       = 1,
    defDM_UpCanon         = 1,
    defDM_UpBomb          = 1,
    defDM_UpShield        = 1,
    defDM_ShieldRecharge  = 1,
    defDM_MagneticShock   = 1,
    defDM_CloakingDevice  = 1,
    defDM_Mine            = 1,
    defDM_DoubledShield   = 1,
    defDM_Random          = 1,

    defDM_WeaponRecycling = 1,

    ctl_MoveLeft          = 0,
    ctl_MoveRight         = 1,
    ctl_MoveForward       = 2,
    ctl_MoveBack          = 3,
    ctl_Fire1             = 4,
    ctl_Fire2             = 5,
    ctl_StrafeLeft        = 6,
    ctl_StrafeRight       = 7,
    ctl_StrafeShift       = 8,
    ctl_PrevInvent        = 9,
    ctl_NextInvent        = 10,
    ctl_UseInvent         = 11,
    ctl_TurboShift        = 12,
    ctl_TurboLock         = 13,
    ctl_Weapon1           = 14,
    ctl_Weapon2           = 15,
    ctl_Weapon3           = 16,
    ctl_Weapon4           = 17,
    ctl_Weapon5           = 18,
    ctl_Weapon6           = 19,
    ctl_NextWeapon        = 20,
    ctl_PrevWeapon        = 21,
    ctl_GotoMouse         = 22,

    ctlset_Kbd            = 0,
    ctlset_Mouse          = 1,
    ctlset_Joy            = 2,

    def_EnhancedKbd       = 1,
    def_CurrentSet        = 0,
    def_AutoJoin          = 1,

    scan_Mouse            = 0x01000000,
    scan_Kbd              = 0x02000000,
    scan_Joy              = 0x04000000,

    skill_Normal          = 1,
    skill_Hard            = 2,
    skill_Def             = skill_Normal,

    load_Hires            = 1,
    load_Lores            = 2,
    load_Both             = 3,
    load_Def              = load_Both,

    def_SoundChannels     = 8
  };
  struct PlayersInfo
  {
    char Names[PLAYERS_NUM][PLAYER_NAME_LEN];
    char Skills[PLAYERS_NUM];
    int  Current;
    int  Quantity;
    int  Id [PLAYERS_NUM];
  };
  struct RespawnInfo
  {
    unsigned PowerUpTime;                     // czas respawnu powerupow w sekundach
    unsigned ItemTime;                        // czas respawnu rzeczy z inwentory w sekundach
    int      WeaponRecycling;
    unsigned char Flags[RESPAWN_ITEMS_NUM];
  };
  struct ControlsInfo
  {
    unsigned int Controls[CONTROLS_SET_NUM][CONTROLS_NUM];
    unsigned int VKeys[CONTROLS_NUM];
    int CurrentSet;
    int EnhancedKbd;
  };
  static unsigned     MinimalPlayers;
  static BOOL         UseCD;
  static unsigned     JoyDeadZone;
  static BOOL         __Loaded;
  static unsigned char BPL[MAX_NET_PLAYERS];
  static unsigned char DefBPL[MAX_NET_PLAYERS];
  static BOOL         DisableDDraw;
  static int          HiresMode;
  static int          LoresMode;

  static int          FirstHires;
  static int          DataToLoad;
  static BOOL         HiresPresent;

  static int          FirstDialogs;
  static BOOL         UseDialogs;
  static BOOL         DialogsPresent;

  static BOOL         DisableDSound;
  static BOOL         DisableSound;
  static unsigned     SoundChannels;
  static int          MixingMode;
  static int          VolumeAssignment;
  static unsigned     NetTimeout;
  static unsigned     NetRetries;
  static GUID         NewOrderGuid;
  static GUID         ProviderGuid;
  static unsigned     NetPlayers;
  static unsigned     GameId;
  static char         NetGameName[Net::game_name_len];
  static char         NetPlayerName[Net::player_name_len];
  static int          PlayMode;              
  static int          NetMode;               
  static BOOL         AutoJoin;               

  static int          GameMode;            // rodzaj gry (deathmatch, base building) patrz enum
  static int          DMFinish;            // sposob zakonczenia gry w deathmatch (patrz enum)
  static unsigned     DMKills;             // ile fragow konczy deatchmatch (jezeli dm_Kill)
  static unsigned     DMTime;              // po jakim czasie (min) konczy sie deatchmatch (jezeli dm_Time)
  static unsigned     BricksPerShip;       // ile cegielek miesci sie w statku
  static unsigned     BricksPerLevel;      // ile cegielek lezy na calym levelu
  static RespawnInfo  RespawnBB;
  static RespawnInfo  RespawnDM;
  static RespawnInfo  DefRespawnBB;
  static RespawnInfo  DefRespawnDM;
  static const char  *RespawnStr[RESPAWN_ITEMS_NUM];

  static ControlsInfo DefControls;
  static ControlsInfo Controls;
  static const char  *ControlsStr[CONTROLS_NUM];
  static const char  *ControlsSetStr[CONTROLS_SET_NUM];
  static PlayersInfo  Players;

  static void PlayerToRegistry(int loader_save);
  static void PlayerFromRegistry(int loader_load);
  static void DecodeGameInfo(void);
  static void EncodeGameInfo(void);
  static void DefaultGameInfo(void);
  static void SetPlayersInfo(PlayersInfo pi);
  static void GetPlayersInfo(PlayersInfo *pi);

  static void Init (void);
  static void Quit (void);
  static void ToRegistry(int loader_save=0);      
  static void FromRegistry(int loader_load=0);
  static char *GetPlayerSection(void);
  static int  GetPlayerSkill(void);
  static int  GetCurrentControlSet(void);
  static void SetCurrentControlSet(int cset);
};

#endif