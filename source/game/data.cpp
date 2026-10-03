#include "headers.h"

Sprite Mysprites::staticmine[NoNet::max_users];
Sprite Mysprites::patern;
Sprite *Mysprites::back;
Sprite Mysprites::spot;
Sprite Mysprites::ship;
Sprite Mysprites::ship_tower;
Sprite Mysprites::ship_tower_point;
Sprite Mysprites::ship_exhaust1;
Sprite Mysprites::ship_exhaust2;
Sprite Mysprites::ship_border;
Sprite Mysprites::ship_border_strong;
Sprite Mysprites::ship_shield;
Sprite Mysprites::ship_skin;
Sprite Mysprites::weap1fire;
Sprite Mysprites::disp_figuressm;
Sprite Mysprites::disp_figuresbig;
Sprite Mysprites::mybomb;
Sprite Mysprites::myrocket;
Sprite Mysprites::arocket;
Sprite Mysprites::myswarmer;
Sprite Mysprites::boom;
Sprite Mysprites::fireball;
Sprite Mysprites::flame;
Sprite Mysprites::kulka;
Sprite Mysprites::strzal;
Sprite Mysprites::cannonsplash;
Sprite Mysprites::cannonpoint1;
Sprite Mysprites::cannonpoint2;
Sprite Mysprites::shotgunfire;
Sprite Mysprites::telep_u;
Sprite Mysprites::telep_d;
Sprite Mysprites::collect_up;
Sprite Mysprites::fog;
Sprite Mysprites::night;
Sprite Mysprites::cloud;
Sprite Mysprites::header;
Sprite Mysprites::ordering;

Sprite Mysprites::expl_mine;
Sprite Mysprites::expl_dest_small;
Sprite Mysprites::expl_dest;
Sprite Mysprites::expl_rocket;
Sprite Mysprites::expl_air;
Sprite Mysprites::expl_laser;
Sprite Mysprites::expl_swarm;

Sprite Mysprites::photo_cross;
Sprite Mysprites::dymki;
Sprite Mysprites::asplash;
Sprite Mysprites::bomb;
Sprite Mysprites::capsule;
Sprite Mysprites::brick;
Sprite Mysprites::lihgtexpl;
Sprite Mysprites::radar_point;
Sprite Mysprites::radar_target;
Sprite Mysprites::radar_scope;
Sprite Mysprites::radar_landing;
Sprite Mysprites::mine;
Sprite Mysprites::mbosssplinter;
Sprite Mysprites::indicator;
Sprite Mysprites::indicatore;
Sprite Mysprites::dym;
Sprite Mysprites::farmerdym;
Sprite Mysprites::destsmoke;
Sprite Mysprites::taran; ;
Sprite Mysprites::simple1;
Sprite Mysprites::simple2;
Sprite Mysprites::simple3;
Sprite Mysprites::mboss1;
Sprite Mysprites::mboss2;                                                     
Sprite Mysprites::mboss3;
Sprite Mysprites::mboss4;
Sprite Mysprites::generat;
Sprite Mysprites::mboss3_fire_left;
Sprite Mysprites::mboss3_fire_right;
Sprite Mysprites::missions_pics_dark;
Sprite Mysprites::missions_pics_nor;
Sprite Mysprites::frame;
Sprite Mysprites::persons;
Sprite Mysprites::active_mission;
Sprite Mysprites::cinder_cannon;
Sprite Mysprites::cinder_pulse;
Sprite Mysprites::backscreen;
Sprite Mysprites::titlescreen;
Sprite Mysprites::press;
Sprite Mysprites::copy;
Sprite Mysprites::snail;


//NET
Sprite Mysprites::level_pictures;
Sprite Mysprites::pointer;
Sprite Mysprites::splinter[MAX_SPRSPLINTER];
int    Mysprites::splinter_num;

int Mysprites::back_phase=0;


void Mysprites::load_splinter(void)
{
  splinter_num=0;
  char tmp[1024];
  for(int i=0;i<MAX_SPRSPLINTER;i++)
  {
    sprintf(tmp,"odlamek%d",i);
    File::area(tmp);

    int t=File::specified("lores");
    File::endarea();

    if (t) splinter[i].load(tmp);
    else break;
  }
  splinter_num=i;
  DBG_CHECK(splinter_num);
  DBG_MESSAGE("%d splinter sprites loaded",i);
}

void Mysprites::free_splinter(void)
{
  for(int i=0;i<splinter_num;i++) splinter[i].free();
}

void Mysprites::put_pattern(Screen &screen,int shaded,int phase)
{
  int put_phase=(phase==-1)?back_phase:phase;
  if (shaded)
  {
    screen.put(0,0,patern,put_phase,Spr::with_tool,(void*)&Tools::tool[Tools::BACKSPRITE]);
  }
  else
  {
    screen.put(0,0,patern,put_phase);
  }
}

void Mysprites::change_pattern(void)
{
  back_phase=timeGetTime()%patern.phases;
}

int Mysprites::sprnum(char* name)
{
  for (int i=1;i<Level::max_sprites;i++)
  {
    if (world->get_level().sprite.is_loaded[i])
    {
      if (!strcmpi(world->get_level().sprite[i].name,name))
      {
        return i;
      }
    }
  }
  return 0;
}

void Mysprites::init(void)
{
  patern.load("patern");
  back_phase=timeGetTime()%patern.phases;

  //SHIP
  ship.load("ship");
  ship_tower.load("shiptower");
  ship_tower_point.load("shiptowerpoint");
  ship_exhaust1.load("shipexhaust1");
  ship_exhaust2.load("shipexhaust2");
  ship_border.load("shipborder");
  ship_border_strong.load("shipborderstrong");
  ship_shield.load("shipshield");
  ship_skin.load("shipskin");
  weap1fire.load("weap1fire");
  shotgunfire.load("shotgunfire");

  //DISPINFO
  disp_figuressm.load("dispfigsmall");
  disp_figuresbig.load("dispfigbig");

  //WEAPON
  strzal.load("strzal");
  myrocket.load("myrocket");
  arocket.load("arocket");
  mybomb.load("mybomb");
  myswarmer.load("myswarmer");
  fireball.load("fireball");
  flame.load("flame");
  mine.load("mine");
  asplash.load("asplash");
  bomb.load("bomb");
  kulka.load("gunfire");
  cannonsplash.load("cannonsplash");
  cannonpoint1.load("cannonpoint1");
  cannonpoint2.load("cannonpoint2");
  for(int j=1;j<NoNet::max_users+1;j++)
  {
    char tmp[1024];
    sprintf(tmp,"staticmine%d",j);
    staticmine[j-1].load(tmp);
  }
  indicator.load("indicator");
  indicatore.load("indicatore");
  header.load("header");



  //EXPLODE
  expl_rocket.load("expl_rocket");
  expl_dest_small.load("expl_dest_small");
  expl_mine.load("expl_mine");
  expl_dest.load("expl_destroy");
  expl_air.load("expl_air");
  expl_laser.load("expl_laser");
  expl_swarm.load("expl_swarm");


  mbosssplinter.load("mbosssplinter");

  collect_up.load("coll_up");

  lihgtexpl.load("lightexpl");
  boom.load("boom");
  dymki.load("dymki");
  destsmoke.load("destsmoke");
  farmerdym.load("myswarmersmoke");

  mboss3_fire_left.load("mboss3_fire_left");
  mboss3_fire_right.load("mboss3_fire_right");
  //RADAR
  radar_point.load("radar_point");
  radar_target.load("radar_target");
  radar_scope.load("radar_scope");
  radar_landing.load("radar_landing");

  telep_u.load("teportu");
  telep_d.load("teportd");

  photo_cross.load("photo_cross");

  //OTHER
  spot.load("cursor");
  capsule.load("upgrade");
  brick.load("brick");
  dym.load("dym");

#ifndef SHAREWARE
  fog.load("fog");
  cloud.load("cloud");
#endif

  night.load("night");

  taran.load("taran");
  simple1.load("simple1");
  simple2.load("simple2");
  simple3.load("simple3");
#ifndef SHAREWARE
  mboss1.load("mboss1");
  mboss4.load("mboss4");
#endif
  mboss2.load("mboss2");
  mboss3.load("mboss3");

  generat .load("generat");
  persons.load("persons");
  active_mission.load("active_mission");

  frame.load("frame");
  missions_pics_dark.load("missions_pics_dark");
  missions_pics_nor.load("missions_pics_nor");
  level_pictures.load("level_pictures");
  pointer.load("pointer");
  cinder_cannon.load("cinder_cannon");
  cinder_pulse.load("cinder_pulse");
  snail.load("snail");
#ifdef SHAREWARE
  ordering.load("ordering");
#endif
  load_splinter();
}

void Mysprites::load_title(void)
{
  File::access_on("header",patch_path,Game::get_volume_access_flags());
  Spr::load_prologue();
  Video::load_palette("palette");
  copy.load("copy");
  backscreen.load("screen2");
  titlescreen.load("screen1");
  press.load("press");
  Spr::load_epilogue(0);
  File::access_off();
}

void Mysprites::free_title(void)
{
  backscreen.free();
  titlescreen.free();
  press.free();
  copy.free();
}

void Mysprites::quit(void)
{
  level_pictures.free();
  free_splinter();
  pointer.free();
  ship.free();
  ship_tower.free();
  ship_tower_point.free();
  ship_exhaust1.free();
  ship_exhaust2.free();
  ship_border.free();
  ship_border_strong.free();
  ship_shield.free();
  ship_skin.free();
  weap1fire.free();
  shotgunfire.free();
  disp_figuressm.free();
  disp_figuresbig.free();
  strzal.free();
  myrocket.free();
  arocket.free();
  myswarmer.free();
  mybomb.free();
  fireball.free();
  flame.free();
  mine.free();
  asplash.free();
  bomb.free();
  kulka.free();
  cannonsplash.free();
  for(int j=0;j<NoNet::max_users;j++)
  {
    staticmine[j].free();
  }
  indicator.free();
  indicatore.free();
  telep_u.free();
  telep_d.free();
  photo_cross.free();
  header.free();

  expl_rocket.free();
  expl_dest_small.free();
  expl_mine.free();
  expl_dest.free();
  expl_air.free();
  expl_laser.free();
  expl_swarm.free();

  lihgtexpl.free();
  boom.free();
  dymki.free();
  destsmoke.free();
  farmerdym.free();
  mbosssplinter.free();
  radar_point.free();
  radar_target.free();
  radar_scope.free();
  radar_landing.free();
  patern.free();
  spot.free();
  capsule.free();
  brick.free();
  dym.free();
  generat.free();
  mboss3_fire_left.free();
  mboss3_fire_right.free();
  collect_up.free();
  fog.free();
  night.free();
  cloud.free();
  taran.free();
  simple1.free();
  simple2.free();
  simple3.free();
  mboss1.free();
  mboss2.free();
  mboss3.free();
  mboss4.free();
  pointer.free();
  persons.free();
  missions_pics_nor.free();
  missions_pics_dark.free();
  frame.free();
  cinder_cannon.free();
  cinder_pulse.free();
  snail.free();
#ifdef SHAREWARE
  ordering.free();
#endif
}


//MYSOUND
Sample Mysound::speech_vulcan;
Sample Mysound::speech_swarmers;
Sample Mysound::speech_plasma;
Sample Mysound::speech_missiles;
Sample Mysound::speech_cannon;
Sample Mysound::speech_grenade;
Sample Mysound::speech_emshock;
Sample Mysound::speech_dshield;
Sample Mysound::speech_mine;
Sample Mysound::speech_cloak;
Sample Mysound::speech_shield;
Sample Mysound::speech_chaosupgrade;
Sample Mysound::speech_collected;
Sample Mysound::speech_activated;
Sample Mysound::speech_selected;
Sample Mysound::speech_colours[NoNet::max_users+1];
Sample Mysound::speech_player;
Sample Mysound::speech_chatreqby;
Sample Mysound::speech_chatterm;
Sample Mysound::speech_incomingmsg;
Sample Mysound::speech_youkilled;
Sample Mysound::speech_youbeenkilledby;
Sample Mysound::speech_youkilledyourself;
Sample Mysound::speech_haskilled;
Sample Mysound::speech_haskilledhimself;
Sample Mysound::speech_leftgame;
Sample Mysound::speech_shldlow;
Sample Mysound::speech_shldcritical;
Sample Mysound::speech_shldrecharged;
Sample Mysound::speech_cloakactive;
Sample Mysound::speech_cloak10deactive;
Sample Mysound::speech_cloakdeactive;
Sample Mysound::speech_dshieldactive;
Sample Mysound::speech_dshield10deactive;
Sample Mysound::speech_dshielddeactive;
Sample Mysound::speech_outofammo;
Sample Mysound::speech_alldestroyed;
Sample Mysound::speech_youclearland;
Sample Mysound::speech_clearland;
Sample Mysound::speech_secretplace;
Sample Mysound::speech_misncomplete;
Sample Mysound::speech_misnfailed;
Sample Mysound::speech_timeisup;
Sample Mysound::speech_reachfrag;
Sample Mysound::speech_youreachfrag;
Sample Mysound::speech_you;
Sample Mysound::speech_are;
Sample Mysound::speech_is;
Sample Mysound::speech_thewinner;
Sample Mysound::speech_beamsuccess;
Sample Mysound::speech_beaminterrupt;
Sample Mysound::speech_incomemissile;
Sample Mysound::speech_incomefire;
Sample Mysound::speech_incomeenemy;
Sample Mysound::speech_largeshiprange;
Sample Mysound::speech_largeshipdetect;
Sample Mysound::speech_underattack;
Sample Mysound::speech_basethreat;
Sample Mysound::speech_youarefirst;
Sample Mysound::speech_youaresecond;
Sample Mysound::speech_youarethird;
Sample Mysound::speech_youarefourth;
Sample Mysound::speech_youarelast;
Sample Mysound::speech_fuelleak;
Sample Mysound::speech_hulldamage;
Sample Mysound::speech_cargounstable;
Sample Mysound::speech_explosion;
Sample Mysound::speech_coolingfail;

Sample Mysound::ship_chgun;
Sample Mysound::ship_swarm;
Sample Mysound::ship_plasma;
Sample Mysound::ship_homing;
Sample Mysound::ship_shotgun;
Sample Mysound::ship_cobain;
Sample Mysound::ship_mine;
Sample Mysound::ship_emshock;
Sample Mysound::ship_wallcollis;
Sample Mysound::ship_shipcollis;
Sample Mysound::ship_lifeincrease;
Sample Mysound::ship_lifedecrease;
Sample Mysound::ship_turboon;
Sample Mysound::ship_turbooff;
Sample Mysound::ship_weap_collected;
Sample Mysound::ship_life_collected;
Sample Mysound::ship_invent_collected;
Sample Mysound::ship_brick_collected;
Sample Mysound::ship_start;
Sample Mysound::ship_land;
Sample Mysound::ship_strongmorph_on;
Sample Mysound::ship_strongmorph_off;
Sample Mysound::respawning;
Sample Mysound::beaming;

Sample Mysound::explode[MAXEXPLODE];
Sample Mysound::mine;
Sample Mysound::mineactive;
Sample Mysound::bullet;
Sample Mysound::cannonrocket;
Sample Mysound::asplash;
Sample Mysound::arocket;
Sample Mysound::karabin;
Sample Mysound::storm[MAXTHUNDER];
Sample Mysound::brick;
Sample Mysound::taran;
Sample Mysound::generat;
Sample Mysound::splinter[MAXSPLINTER];
Sample Mysound::spl_ship1;
Sample Mysound::spl_ship2;
Sample Mysound::spl_ship3;
Sample Mysound::mboss1_fire;
Sample Mysound::mboss2_fire;
Sample Mysound::mboss3_fire;
Sample Mysound::mboss4_fire;
Sample Mysound::options_updown;
Sample Mysound::options_switch;
Sample Mysound::options_fx;
Sample Mysound::options_na;
Sample Mysound::options_accept;
Sample Mysound::options_init;
Sample Mysound::options_quit;
Sample Mysound::cheat_on;
Sample Mysound::cheat_off;
Sample Mysound::mission_failed;
Sample Mysound::mission_completed;
Sample Mysound::buffer_overflow;
Sample Mysound::ringing;
Sample Mysound::countdown;
Sample Mysound::titlemove;
Sample Mysound::logo[4];
Sample Mysound::szzzz[MAX_SZZZZ];
Sample Mysound::szzzzend[MAX_SZZZZ];



void Mysound::init(void)
{
  speech_vulcan.load("vulcan");
  speech_swarmers.load("swarmers");
  speech_plasma.load("plasma");
  speech_missiles.load("missiles");
  speech_cannon.load("cannon");
  speech_grenade.load("grenade");
  speech_emshock.load("emshock");
  speech_dshield.load("dshield");
  speech_mine.load("mine");
  speech_cloak.load("cloak");
  speech_shield.load("shield");
  speech_chaosupgrade.load("chaosupgrade");
  speech_collected.load("collected");
  speech_activated.load("activated");
  speech_selected.load("selected");
  speech_player.load("player");
  speech_chatreqby.load("chatreqby");
  speech_chatterm.load("chatterm");
  speech_incomingmsg.load("incomingmsg");
  speech_youkilled.load("youkilled");
  speech_youbeenkilledby.load("youbeenkilledby");
  speech_youkilledyourself.load("youkilledyourself");
  speech_haskilled.load("haskilled");
  speech_haskilledhimself.load("haskilledhimself");
  speech_leftgame.load("leftgame");
  speech_shldlow.load("shldlow");
  speech_shldcritical.load("shldcritical");
  speech_shldrecharged.load("shldrecharged");
  speech_cloakactive.load("cloakactive");
  speech_cloak10deactive.load("cloak10deactive");
  speech_cloakdeactive.load("cloakdeactive");
  speech_dshieldactive.load("dshieldactive");
  speech_dshield10deactive.load("dshield10deactive");
  speech_dshielddeactive.load("dshielddeactive");
  speech_outofammo.load("outofammo");
  speech_alldestroyed.load("alldestroyed");
  speech_youclearland.load("youclearland");
  speech_clearland.load("clearland");
  speech_secretplace.load("secretplace");
  speech_misncomplete.load("misncomplete");
  speech_misnfailed.load("misnfailed");
  speech_timeisup.load("timeisup");
  speech_reachfrag.load("reachfrag");
  speech_youreachfrag.load("youreachfrag");
  speech_you.load("you");
  speech_are.load("are");
  speech_is.load("is");
  speech_thewinner.load("thewinner");
  speech_beamsuccess.load("beamsuccess");
  speech_beaminterrupt.load("beaminterrupt");
  speech_incomemissile.load("incomemissile");
  speech_incomefire.load("incomefire");
  speech_incomeenemy.load("incomeenemy");
  speech_largeshiprange.load("largeshiprange");
  speech_largeshipdetect.load("largeshipdetect");
  speech_underattack.load("underattack");
  speech_basethreat.load("basethreat");
  speech_youarefirst.load("youarefirst");
  speech_youaresecond.load("youaresecond");
  speech_youarethird.load("youarethird");
  speech_youarefourth.load("youarefourth");
  speech_youarelast.load("youarelast");
  speech_colours[1].load("colors1");
  speech_colours[2].load("colors2");
  speech_colours[3].load("colors3");
  speech_colours[4].load("colors4");

#ifndef SHAREWARE
  speech_fuelleak.load("fuelleak");
  speech_hulldamage.load("hulldamage");
  speech_cargounstable.load("cargounstable");
  speech_explosion.load("explosion");
  speech_coolingfail.load("coolingfail");
#endif

  ship_chgun.load("ship_chaingun_shot");
  ship_swarm.load("ship_swarmers_shot");
  ship_plasma.load("ship_plasma_shot");
  ship_homing.load("ship_homing_shot");
  ship_shotgun.load("ship_shotgun_shot");
  ship_cobain.load("ship_bomb_shot");
  ship_mine.load("ship_mine_shot");
  ship_emshock.load("ship_emshock_shot");
  ship_wallcollis.load("ship_wallcollis");
  ship_shipcollis.load("ship_aliencollis");
  ship_lifeincrease.load("ship_lifeincrease");
  ship_lifedecrease.load("ship_lifedecrease");
  ship_turboon.load("ship_turboon");
  ship_turbooff.load("ship_turbooff");
  ship_weap_collected.load("ship_weap_collect");
  ship_life_collected.load("ship_life_collect");
  ship_invent_collected.load("ship_invent_collect");
  ship_brick_collected.load("ship_brick_collect");
  ship_start.load("ship_start");
  ship_land.load("ship_land");
  ship_strongmorph_on.load("ship_strongmorph_on");
  ship_strongmorph_off.load("ship_strongmorph_off");
  respawning.load("respawning");
  beaming.load("beaming");

  generat.load("generator");
  bullet.load("cannon_bullet");
  cannonrocket.load("cannon_rocket");
  asplash.load("simple1_wepon");
  karabin.load("simple2_wepon");
  arocket.load("simple3_wepon");
  taran.load("simple4_wepon");
  mine.load("object_mine_active");
  mineactive.load("object_mine_non_active");

  for(int i=0;i<MAXSPLINTER;i++)
  {
    char tmp[1024];
    sprintf(tmp,"splinter%d",i+1);
    splinter[i].load(tmp);
  }

  for(i=0;i<MAXEXPLODE;i++)
  {
    char tmp[1024];
    sprintf(tmp,"explode%d",i+1);
    explode[i].load(tmp);
  }

  for(i=0;i<MAXTHUNDER;i++)
  {
    char tmp[1024];
    sprintf(tmp,"storm%d",i+1);
    storm[i].load(tmp);
  }

  for(i=0;i<MAX_SZZZZ;i++)
  {
    char tmp[1024];
    sprintf(tmp,"szzzz%d",i+1);
    szzzz[i].load(tmp);
    sprintf(tmp,"szzzzend%d",i+1);
    szzzzend[i].load(tmp);
  }


  for(i=0;i<4;i++)
  {
    char tmp[1024];
    sprintf(tmp,"logo%d",i+1);
    logo[i].load(tmp);
  }

  spl_ship1.load("ship_splinter1");
  spl_ship2.load("ship_splinter2");
  spl_ship3.load("ship_splinter3");
  mboss4_fire.load("mboss4_fire");
  mboss1_fire.load("mboss1_fire");
  mboss2_fire.load("mboss2_fire");
  mboss3_fire.load("mboss3_fire");
  options_updown.load("options_updown");
  options_switch.load("options_switch");
  options_na.load("options_na");
  options_accept.load("options_accept");
  options_fx.load("options_fx");
  options_init.load("options_init");
  options_quit.load("options_quit");
  cheat_on.load("cheat_on");
  cheat_off.load("cheat_off");
  mission_failed.load("mission_failed");
  mission_completed.load("mission_completed");
  buffer_overflow.load("buffer_overflow");
  ringing.load("ringing");
  countdown.load("countdown");
  titlemove.load("titlemove");
}

void Mysound::quit(void)
{
  speech_vulcan.free();
  speech_swarmers.free();
  speech_plasma.free();
  speech_missiles.free();
  speech_cannon.free();
  speech_grenade.free();
  speech_emshock.free();
  speech_dshield.free();
  speech_mine.free();
  speech_cloak.free();
  speech_shield.free();
  speech_chaosupgrade.free();
  speech_collected.free();
  speech_activated.free();
  speech_selected.free();
  speech_player.free();
  speech_chatreqby.free();
  speech_chatterm.free();
  speech_incomingmsg.free();
  speech_youkilled.free();
  speech_youbeenkilledby.free();
  speech_youkilledyourself.free();
  speech_haskilled.free();
  speech_haskilledhimself.free();
  speech_leftgame.free();
  speech_shldlow.free();
  speech_shldcritical.free();
  speech_shldrecharged.free();
  speech_cloakactive.free();
  speech_cloak10deactive.free();
  speech_cloakdeactive.free();
  speech_dshieldactive.free();
  speech_dshield10deactive.free();
  speech_dshielddeactive.free();
  speech_outofammo.free();
  speech_alldestroyed.free();
  speech_youclearland.free();
  speech_clearland.free();
  speech_secretplace.free();
  speech_misncomplete.free();
  speech_misnfailed.free();
  speech_timeisup.free();
  speech_reachfrag.free();
  speech_youreachfrag.free();
  speech_you.free();
  speech_are.free();
  speech_is.free();
  speech_beamsuccess.free();
  speech_beaminterrupt.free();
  speech_incomemissile.free();
  speech_incomefire.free();
  speech_incomeenemy.free();
  speech_largeshiprange.free();
  speech_largeshipdetect.free();
  speech_underattack.free();
  speech_basethreat.free();
  speech_youarefirst.free();
  speech_youaresecond.free();
  speech_youarethird.free();
  speech_youarefourth.free();
  speech_youarelast.free();
  speech_colours[1].free();
  speech_colours[2].free();
  speech_colours[3].free();
  speech_colours[4].free();
  speech_fuelleak.free();
  speech_hulldamage.free();
  speech_cargounstable.free();
  speech_explosion.free();
  speech_coolingfail.free();

  ship_chgun.free();
  ship_swarm.free();
  ship_plasma.free();
  ship_homing.free();
  ship_shotgun.free();
  ship_cobain.free();
  ship_mine.free();
  ship_emshock.free();
  ship_wallcollis.free();
  ship_shipcollis.free();
  ship_lifeincrease.free();
  ship_lifedecrease.free();
  ship_turboon.free();
  ship_turbooff.free();
  ship_weap_collected.free();
  ship_life_collected.free();
  ship_invent_collected.free();
  ship_brick_collected.free();
  ship_start.free();
  ship_land.free();
  ship_strongmorph_on.free();
  ship_strongmorph_off.free();
  respawning.free();
  beaming.free();

  mine.free();
  mineactive.free();
  bullet.free();
  asplash.free();
  arocket.free();
  karabin.free();
  for(int i=0;i<MAXTHUNDER;i++) storm[i].free();
  for(i=0;i<MAXSPLINTER;i++)    splinter[i].free();
  for(i=0;i<MAXEXPLODE;i++)     explode[i].free();
  for(i=0;i<MAX_SZZZZ;i++)      
  {
    szzzz[i].free();
    szzzzend[i].free();
  }


  for(i=0;i<4;i++) logo[i].free();

  brick.free();
  taran.free();
  cannonrocket.free();
  spl_ship1.free();
  spl_ship2.free();
  spl_ship3.free();
  mboss1_fire.free();
  mboss2_fire.free();
  mboss3_fire.free();
  mboss4_fire.free();
  options_updown.free();
  options_switch.free();
  options_fx.free();
  options_init.free();
  options_quit.free();
  cheat_on.free();
  cheat_off.free();
  mission_failed.free();
  mission_completed.free();
  buffer_overflow.free();
  ringing.free();
  countdown.free();
  titlemove.free();
}
