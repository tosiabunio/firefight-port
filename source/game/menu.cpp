#include "headers.h"

//KLASA MENU
Text Menu::descript_text;
Sprite Menu::titles_sprite_in;
Sprite Menu::titles_sprite_out;
int Menu::initialized=0;
int Menu::flushing=0;
int Menu::ignore_quit=0;
int Menu::between_y=0;
int Menu::x=0;
int Menu::y=0;
int Menu::glass_x=0;
int Menu::glass_y=0;
int Menu::glass_sx=0;
int Menu::glass_sy=0;
int Menu::sprite_x=0;
int Menu::sprite_y=0;
Tool Menu::box_tool;
Tool Menu::letter_in_tool;
Tool Menu::letter_out_tool;
Menu *Menu::recurse_buf[Menu::recurse_depth];
int Menu::recurse_cursor=-1;
int Menu::prev_fx=0;
int Menu::delay_time=0;
int Menu::time_fx=0;
int Menu::register_phase=0;
int Menu::vpalette_beforeregister=0;
int Menu::enable_game_menus=0;

// Port: the launcher is gone, so "EXIT TO LOADER" (title.tdf, which stays as shipped) now says
// what the item does: QUIT GAME, and CONFIRM QUIT on the confirmation screen (game_quit). On
// macOS the help names Command-Q, which quits there (SDL's application menu sends the quit event
// that closing the window sends); Option-X still opens the confirmation, as Alt-X does elsewhere.
static char *port_label(const char *area,char *label)
{
#ifdef __APPLE__
  if (strcmp(label,"ALT+X")==0) return (char*)"CMD+Q";
#endif
  if (strcmp(label,"EXIT TO LOADER")!=0) return label;
  return (char*)(strcmpi(area,"game_quit")==0 ?"CONFIRM QUIT" :"QUIT GAME");
}

void Menu::init(char *_descript,char *_titles_in,char *_titles_out)
{
  DBG_CHECK(!initialized);
  recurse_cursor=-1;
  flushing=0;
  ignore_quit=0;
  prev_fx=0;
  delay_time=0;
  time_fx=0;
  register_phase=0;
  vpalette_beforeregister=0;
  enable_game_menus=0;
  titles_sprite_in.load(_titles_in);
  titles_sprite_out.load(_titles_out);
  descript_text.load(_descript);
  descript_text.area("globals");
  between_y=descript_text.value("between_y");
  x=descript_text.value("x");
  y=descript_text.value("y");
  glass_x=descript_text.value("glass_x");
  glass_y=descript_text.value("glass_y");
  glass_sx=descript_text.value("glass_sx");
  glass_sy=descript_text.value("glass_sy");
  sprite_x=descript_text.value("sprite_x");
  sprite_y=descript_text.value("sprite_y");
  int r1(0),g1(0),b1(0),r2(0),g2(0),b2(0);
  if (sscanf(descript_text.string("box_tool"),"%d,%d,%d,%d,%d,%d",&r1,&g1,&b1,&r2,&g2,&b2)>4)
  {
    box_tool.glass(Color(r1,g1,b1),Color(r2,g2,b2));
  }
  else
  {
    box_tool.glass(Color(r1,g1,b1),r2);
  }
  if (sscanf(descript_text.string("letter_in_tool"),"%d,%d,%d,%d,%d,%d",&r1,&g1,&b1,&r2,&g2,&b2)>4)
  {
    letter_in_tool.glass(Color(r1,g1,b1),Color(r2,g2,b2));
  }
  else
  {
    letter_in_tool.glass(Color(r1,g1,b1),r2);
  }
  if (sscanf(descript_text.string("letter_out_tool"),"%d,%d,%d,%d,%d,%d",&r1,&g1,&b1,&r2,&g2,&b2)>4)
  {
    letter_out_tool.glass(Color(r1,g1,b1),Color(r2,g2,b2));
  }
  else
  {
    letter_out_tool.glass(Color(r1,g1,b1),r2);
  }
  descript_text.endarea();
  descript_text.area("help");
  for(int i=0;i<descript_text.size("labels");i++)
  {
    DBG_CHECK(i<2*MAX_KEYS);
    DBG_CHECK(strlen(descript_text.string("labels",i))<MAX_KEYS_LEN);
    strcpy(key_table[i],port_label("help",descript_text.string("labels",i)));
  }
  descript_text.endarea();
  initialized=1;
}

void Menu::done(void)
{
  flush();
  descript_text.free();
  titles_sprite_in.free();
  titles_sprite_out.free();
  initialized=0;
}

void Menu::flush(void)
{
  if (initialized)
  {
    flushing=1;
    while (in_use())
    {
      pop_menu();
    }
    flushing=0;
    ignore_quit=0;
  }
}

int Menu::in_use(void)
{
  DBG_CHECK(initialized);
  return (recurse_cursor>=0);
}

void Menu::push_menu(Menu *menu)
{
  DBG_CHECK(initialized);
  DBG_CHECK(recurse_cursor>=-1&&recurse_cursor<recurse_depth-1);
  recurse_buf[(++recurse_cursor)]=menu;
}

void Menu::pop_menu(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(recurse_cursor>=0&&recurse_cursor<recurse_depth);
  if (!flushing)
  {
    Sound::st_play(&Mysound::options_quit);
  }
  if (recurse_buf[recurse_cursor]->type()==QUIT)
  {
    ignore_quit=0;
  }
  else if (recurse_buf[recurse_cursor]->type()==REGISTER)
  {
    Display::clear_vga(vpalette_beforeregister);
  }
  delete recurse_buf[recurse_cursor--];
}

Menu *Menu::top_menu(void)
{
  DBG_CHECK(initialized);
  DBG_CHECK(recurse_cursor>=0&&recurse_cursor<recurse_depth);
  return recurse_buf[recurse_cursor];
}

Menu::Menu(char *_label_name,MenuType _menutype)
{
  DBG_CHECK(initialized);
  menutype=_menutype;
  label_name=_label_name;
  descript_text.area(label_name);
  size=descript_text.size("definition")/2;
  title_phase=descript_text.value("title_phase");
  first_time=1;
  freezed_keys=0;
#if HI_DEBUG
  if (size<=0||size>=Menu::max_items)
  {
    FAILURE("Invalid size of \"%s\" in descript_text.",label_name);
  }
#endif
  char type=0;
  int global_y=between_y;
  int i;
  for (i=0;i<size;i++)
  {
    sscanf(descript_text.string("definition",2*i),"%c,%c,%c,%d,%d",&type,&menuinfo[i].this_letter,&menuinfo[i].next_letter,&menuinfo[i].x,&menuinfo[i].y);
    char *label=port_label(label_name,descript_text.string("definition",2*i+1));  // port
    switch (type)
    {
      case 't': items[i]=NEW(TextItem(label,descript_text),item_mbn);
                break;
      case 's': items[i]=NEW(SubItem(label,descript_text),item_mbn);
                break;
      case 'b': items[i]=NEW(ButtItem(label,descript_text),item_mbn);
                break;
      case 'r': items[i]=NEW(RButtItem(label,descript_text),item_mbn);
                break;
      case 'z': items[i]=NEW(ZippItem(label,descript_text),item_mbn);
                break;
#if HI_DEBUG
       default: FAILURE("Type of %d. item in \"%s\" area in descript_text is invalid.",i,label_name);
#endif
    }
    if (menuinfo[i].x==-1)
    {
      menuinfo[i].x=(Mp::SX-items[i]->get_dx())/2;
    }
    if (menuinfo[i].y==-1)
    {
      menuinfo[i].y=global_y;
    }
    global_y=menuinfo[i].y+items[i]->get_dy()+between_y;
  }
#if HI_DEBUG
  for (i=0;i<size;i++)
  {
    for (int j=i+1;j<size;j++)
    {
      if (menuinfo[i].this_letter==menuinfo[j].this_letter)
      {
        FAILURE("Items %d and %d in \"%s\" area in descript_text have the same descripting letter.",i,j,label_name);
      }
    }
  }
  for (i=0;i<size;i++)
  {
    char temp=menuinfo[i].next_letter;
    int exist(0);
    for (int j=0;j<size&&!exist;j++)
    {
      if (temp==menuinfo[j].this_letter)
      {
        exist=1;
      }
    }
    if (!exist)
    {
      FAILURE("Item %d in \"%s\" area in descript_text has invalid return letter.",i,label_name);
    }
  }
  char temp=*(descript_text.string("default"));
  int exist(0);
  for (int j=0;j<size&&!exist;j++)
  {
    if (temp==menuinfo[j].this_letter)
    {
      exist=1;
    }
  }
  if (exist)
  {
    if (!(get_item(temp)->selectable()))
    {
      FAILURE("Default letter in \"%s\" area in descript_text is invalid (this item cannot be activated).",label_name);
    }
    else
    {
      set_active(temp);
    }
  }
  else
  {
    FAILURE("Default letter in \"%s\" area in descript_text is invalid.",label_name);
  }
#else
  set_active(*((char*)descript_text.string("default")));
#endif
  descript_text.endarea();
  Sound::st_play(&Mysound::options_init);
}

Menu::~Menu(void)
{
  for (int i=0;i<size;i++)
  {
    delete get_item(i);
  }
}

int Menu::run_item(char letter)
{
  return (get_item(letter)->run(*this));
}

int Menu::run_item(int number)
{
  DBG_CHECK(number>=0&&number<size);
  return (get_item(number)->run(*this));
}

int Menu::run_item(void)
{
  DBG_CHECK(active>=0&&active<size);
  return (get_item()->run(*this));
}

char Menu::get_active_letter(void)
{
  DBG_CHECK(active>=0&&active<size);
  return menuinfo[active].this_letter;
}

int Menu::get_active_number(void)
{
  DBG_CHECK(active>=0&&active<size);
  return active;
}

Item *Menu::get_item(char letter)
{
  return items[get_number(letter)];
}

Item *Menu::get_item(int number)
{
  DBG_CHECK(number>=0&&number<size);
  return items[number];
}

Item *Menu::get_item(void)
{
  DBG_CHECK(active>=0&&active<size);
  return items[active];
}

int Menu::find_first_selectable(int dir)
{
  DBG_CHECK(active>=0&&active<size);
  int temp=0;
  for (int i=1;i<size;i++)
  {
    if (dir>=0)
    {
      temp=(active+i)%size;
    }
    else
    {
      temp=(active-i)<0?active-i+size:active-i;
    }
    if (items[temp]->selectable())
    {
       return temp;
    }
  }
  return active;
}

void Menu::draw(Screen& screen,int display_active)
{
  int ox=screen.ox;
  int oy=screen.ox;
  screen.origin(0,0);
  screen.rectangle(box_tool,glass_x,glass_y,glass_sx,glass_sy);
  screen.put(sprite_x,sprite_y,titles_sprite_out,title_phase,Spr::with_tool,(void*)&letter_out_tool);
  screen.shape(sprite_x,sprite_y,titles_sprite_in,title_phase,Spr::with_tool,(void*)&letter_in_tool);
  screen.origin(x,y);
  for (int i=0;i<size;i++)
  {
    if (i!=active)
    {
      items[i]->draw(screen,menuinfo[i].x,menuinfo[i].y,0);
    }
    else
    {
      if (display_active)
      {
        items[i]->draw(screen,menuinfo[i].x,menuinfo[i].y,1);
      }
    }
  }
  screen.origin(ox,oy);
}

char Menu::run(void)
{
  if (!first_time)
  {
    if (!freezed_keys)
    {
      if (KbdStat::menu_quit())
      {
        return 0;
      }
      if (KbdStat::menu_up())
      {
        active=find_first_selectable(-1);
        Sound::st_play(&Mysound::options_updown);
      }
      if (KbdStat::menu_down())
      {
        active=find_first_selectable(1);
        Sound::st_play(&Mysound::options_updown);
      }
      if (KbdStat::menu_page_down())
      {
        for (int i=size-1;i>=0;i--)
        {
          if (items[i]->selectable())
          {
             set_active(i);
             Sound::st_play(&Mysound::options_updown);
             break;
          }
        }
      }
      if (KbdStat::menu_page_up())
      {
        for (int i=0;i<size;i++)
        {
          if (items[i]->selectable())
          {
             set_active(i);
             Sound::st_play(&Mysound::options_updown);
             break;
          }
        }
      }
    }
  }
  return get_active_letter();
}

void Menu::set_active(int number)
{
  DBG_CHECK(number>=0&&number<size);
  DBG_CHECK(get_item(number)->selectable());
  active=number;
}

void Menu::set_active(char letter)
{
  set_active(get_number(letter));
}

void Menu::set_return_active(int number)
{
  DBG_CHECK(number>=0&&number<size);
  set_active(menuinfo[number].next_letter);
}

void Menu::set_return_active(void)
{
  set_return_active(active);
}

void Menu::set_return_active(char letter)
{
  set_return_active(get_number(letter));
}

char Menu::get_letter(int number)
{
  DBG_CHECK(number>=0&&number<size);
  return menuinfo[number].this_letter;
}

int Menu::get_number(char letter)
{
  for (int i=0;i<size;i++)
  {
    if (letter==menuinfo[i].this_letter)
    {
      return i;
    }
  }
#if HI_DEBUG
  FAILURE("There is no item descripted by %c letter in %s area in descript_text.",letter,label_name);
#endif
  return -1;
}

Menu::MenuType Menu::type(void)
{
  return menutype;
}

void Menu::serve(void)
{
  DBG_CHECK(initialized);
  if (in_use())
  {
    Menu *menu=top_menu();
    char result=menu->run();
    if (result)
    {
      switch (menu->type())
      {
        case OPTIONS: serve_options(*menu,result);
                      break;
        case DISPLAY: serve_display(*menu,result);
                      break;
          case SOUND: serve_sound(*menu,result);
                      break;
       case CONTROLS: serve_controls(*menu,result);
                      break;
           case QUIT: serve_quit(*menu,result);
                      break;
          case CHEAT: serve_cheat(*menu,result);
                      break;
   case ABORTMISSION: serve_abortmission(*menu,result);
                      break;
 case RESTARTMISSION: serve_restartmission(*menu,result);
                      break;
           case HELP: serve_help(*menu,result);
                      break;
       case REGISTER: serve_register(*menu,result);
                      break;
           case GAME: serve_game(*menu,result);
                      break;
             default: DBG_CHECK(0);
                      break;
      }
      menu->first_time=0;
    }
    else
    {
      pop_menu();
    }
  }
}

void Menu::display(Screen &screen)
{
  DBG_CHECK(initialized);
  if (in_use())
  {
    if (top_menu()->type()==HELP)
    {
      top_menu()->draw(screen,0);
      draw_help(screen);
    }
    else if (top_menu()->type()==REGISTER)
    {
      draw_register(screen);
    }
    else
    {
      top_menu()->draw(screen,1);
    }
  }
}

void Menu::serve_options(Menu &menu,char letter)
{
  DBG_CHECK(initialized);
  switch (letter)
  {
    case 'q': if (menu.run_item())
              {
                pop_menu();
              }
              break;
    case 'd': if (menu.run_item())
              {
                Menu::display();
              }
              break;
    case 's': if (menu.run_item())
              {
                Menu::sound();
              }
              break;
    case 'c': if (menu.run_item())
              {
                Menu::controls();
              }
              break;
    case 'm': if (menu.run_item())
              {
                Menu::abortmission();
              }
              break;
    case 'g': if (menu.run_item())
              {
                Menu::game();
              }
              break;
    case 'e': if (menu.run_item())
              {
                Menu::quit();
              }
              break;
    case 'r': if (menu.run_item())
              {
                Menu::register_menu();
              }
              break;
     default: DBG_CHECK(0);
              break;
  }
}

void Menu::serve_display(Menu &menu,char letter)
{
  DBG_CHECK(initialized);
  switch (letter)
  {
    case 'q': if (menu.run_item())
              {
                pop_menu();
              }
              break;
    case 'r': Menu::resolution_adjust(menu);
              break;
    case 'b': Menu::gamma_adjust(menu);
              break;
    case 'd': Menu::detail_level_adjust(menu);
              break;
     default: DBG_CHECK(0);
              break;
  }
}

void Menu::serve_sound(Menu &menu,char letter)
{
  DBG_CHECK(initialized);
  menu.get_item('m')->set_status(SysSet::get(SysSet::MUSIC_VOL));
  menu.get_item('s')->set_status(SysSet::get(SysSet::FX_VOL));
  switch (letter)
  {
    case 'q': if (menu.run_item())
              {
                pop_menu();
              }
              break;
    case 'm': Menu::music_adjust(menu);
              break;
    case 's': Menu::fx_adjust(menu);
              break;
     default: DBG_CHECK(0);
              break;
  }
  if (prev_fx!=SysSet::get(SysSet::FX_VOL))
  {
    prev_fx=SysSet::get(SysSet::FX_VOL);
    time_fx=delay_time;
  }
  if (time_fx>0)
  {
    if ((time_fx-=KbdStat::time())<=0)
    {
      Sound::st_play(&Mysound::options_fx);
      time_fx=0;
    }
  }
}

void Menu::serve_controls(Menu &menu,char letter)
{
  DBG_CHECK(initialized);
  switch (letter)
  {
    case 'q': if (menu.run_item())
              {
                pop_menu();
              }
              break;
    case 't': Menu::input_device_adjust(menu);
              break;
     default: DBG_CHECK(0);
              break;
  }
}

void Menu::serve_quit(Menu &menu,char letter)
{
  DBG_CHECK(initialized);
  switch (letter)
  {
    case 'y': if (menu.run_item())
              {
                Display::clear_vga(9);
                throw TerminateGame();
              }
    case 'n': if (menu.run_item())
              {
                pop_menu();
              }
              break;
     default: DBG_CHECK(0);
              break;
  }
}

void Menu::serve_cheat(Menu &menu,char letter)
{
  DBG_CHECK(initialized);
  DBG_CHECK(!NoNet::is_network_mode());
  switch (letter)
  {
    case 'q': if (menu.run_item())
              {
                pop_menu();
              }
              break;
    case 'i': Menu::immortal_adjust(menu);
              break;
    case 'l': if (menu.run_item())
              {
                NoNet::get_ship()->get_user()->set_full_life();
              }
              break;
    case 'e': if (menu.run_item())
              {
                NoNet::get_ship()->get_user()->set_full_weap();
                NoNet::get_ship()->get_user()->set_full_invent();
              }
              break;
     default: DBG_CHECK(0);
              break;
  }
}

void Menu::serve_abortmission(Menu &menu,char letter)
{
  DBG_CHECK(initialized);
  switch (letter)
  {
    case 'y': if (menu.run_item())
              {
                Display::clear_vga(9);
                throw TerminateMission();
              }
    case 'n': if (menu.run_item())
              {
                pop_menu();
              }
              break;
     default: DBG_CHECK(0);
              break;
  }
}

void Menu::serve_restartmission(Menu &menu,char letter)
{
  DBG_CHECK(initialized);
  switch (letter)
  {
    case 'y': if (menu.run_item())
              {
                Game::stop_pause();
                gamemanager->set_restart(GameManager::RESTART);
                flush();
              }
              break;
    case 'n': if (menu.run_item())
              {
                pop_menu();
              }
              break;
     default: DBG_CHECK(0);
              break;
  }
}

void Menu::serve_help(Menu &menu,char letter)
{
  DBG_CHECK(initialized);
  if (KbdStat::menu_anykey()&&!menu.first_time)
  {
    pop_menu();
  }
}

void Menu::serve_register(Menu &menu,char letter)
{
  DBG_CHECK(initialized);
#ifdef SHAREWARE
  if (!menu.first_time)
  {
    if (KbdStat::menu_left_struck()||KbdStat::menu_up())
    {
      register_phase--;
    }
    else if (KbdStat::menu_right_struck()||KbdStat::menu_down())
    {
      register_phase++;
    }

    if (register_phase<0)
    {
      register_phase=Mysprites::ordering.phases-1;
    }
    else if (register_phase>Mysprites::ordering.phases-1)
    {
      register_phase=0;
    }

    if (KbdStat::menu_left_struck()||KbdStat::menu_right_struck())
    {
      Sound::st_play(&Mysound::options_updown);
    }
  }
#endif
}

void Menu::serve_game(Menu &menu,char letter)
{
  DBG_CHECK(initialized);
  switch (letter)
  {
    case 'q': if (menu.run_item())
              {
                pop_menu();
              }
              break;
    case 'a': if (menu.run_item())
              {
                DBG_CHECK(!NoNet::is_network_mode());
                DBG_CHECK(enable_game_menus);
                Display::clear_vga(9);
                throw TerminateMission();
              }
              break;
    case 'r': if (menu.run_item())
              {
                DBG_CHECK(!NoNet::is_network_mode());
                DBG_CHECK(enable_game_menus);
                Game::stop_pause();
                gamemanager->set_restart(GameManager::RESTART);
                flush();
              }
              break;
    case 'e': if (menu.run_item())
              {
                Menu::quit();
              }
              break;
     default: DBG_CHECK(0);
              break;
  }
}

void Menu::options(int enable)
{
  enable_game_menus=enable;
  char *label=NULL;
  if (enable_game_menus&&!NoNet::is_network_mode())
  {
    label="options";
  }
  else
  {
    label="options_short";
  }
  Menu &menu=*NEW(Menu(label,Menu::OPTIONS),menu_mbn);
  push_menu(&menu);
}

void Menu::display(void)
{
  Menu &menu=*NEW(Menu("display",Menu::DISPLAY),menu_mbn);
  push_menu(&menu);
  menu.get_item('b')->set_status(SysSet::get(SysSet::GAMMA));
  menu.get_item('d')->set_status(SysSet::get(SysSet::DETAIL_LEVEL));
  if (!Mp::HIRESAVAIL)
  {
    menu.get_item('r')->selectable(0);
    menu.set_active('b');
  }
  else
  {
    menu.get_item('r')->set_status(SysSet::get(SysSet::RESOLUTION));
  }
}

void Menu::sound(void)
{
  Menu &menu=*NEW(Menu("sounds",Menu::SOUND),menu_mbn);
  push_menu(&menu);
  menu.get_item('m')->set_status(SysSet::get(SysSet::MUSIC_VOL));
  menu.get_item('s')->set_status(SysSet::get(SysSet::FX_VOL));
  prev_fx=SysSet::get(SysSet::FX_VOL);
  delay_time=Mp::METRONQUALITY/3;
  time_fx=0;
}

void Menu::controls(void)
{
  Menu &menu=*NEW(Menu("controls",Menu::CONTROLS),menu_mbn);
  push_menu(&menu);
  menu.get_item('t')->set_status(SysSet::get(SysSet::INPUT_DEVICE));
}

void Menu::quit(void)
{
  if (!ignore_quit)
  {
    Menu &menu=*NEW(Menu("game_quit",Menu::QUIT),menu_mbn);
    push_menu(&menu);
    ignore_quit=1;
  }
}

void Menu::cheat(void)
{
  Menu &menu=*NEW(Menu("in_game_secret",Menu::CHEAT),menu_mbn);
  push_menu(&menu);
  menu.get_item('i')->set_status(NoNet::get_ship()->is_immortal());
}

void Menu::abortmission(void)
{
  DBG_CHECK(!NoNet::is_network_mode());
  Menu &menu=*NEW(Menu("in_game_abort",Menu::ABORTMISSION),menu_mbn);
  push_menu(&menu);
}

void Menu::restartmission(void)
{
  DBG_CHECK(!NoNet::is_network_mode());
  Menu &menu=*NEW(Menu("in_game_restart",Menu::RESTARTMISSION),menu_mbn);
  push_menu(&menu);
}

void Menu::help(void)
{
  Menu &menu=*NEW(Menu("in_game_help",Menu::HELP),menu_mbn);
  push_menu(&menu);
}

void Menu::register_menu(void)
{
#ifdef SHAREWARE
  register_phase=0;
  vpalette_beforeregister=Video::vpalette_num;
  Display::clear_vga(8);
  Menu &menu=*NEW(Menu("register",Menu::REGISTER),menu_mbn);
  push_menu(&menu);
#endif
}

void Menu::game(void)
{
  char *label=NULL;

  //dupa!!!!
  
  if (enable_game_menus&&!NoNet::is_network_mode())
  {
    label="in_game_game";
  }
  else
  {
    label="game";
  }

  
  Menu &menu=*NEW(Menu(label,Menu::GAME),menu_mbn);
  push_menu(&menu);
}

void Menu::input_device_adjust(Menu& menu)
{
  int new_input_device=menu.run_item();
  if (new_input_device!=SysSet::get(SysSet::INPUT_DEVICE))
  {
    SysSet::set(SysSet::INPUT_DEVICE,new_input_device);
  }
}

void Menu::gamma_adjust(Menu& menu)
{
  int new_gamma=menu.run_item();
  if (new_gamma!=SysSet::get(SysSet::GAMMA))
  {
    SysSet::set(SysSet::GAMMA,new_gamma);
  }
}

void Menu::fx_adjust(Menu& menu)
{
  int new_fx=menu.run_item();
  if (new_fx!=SysSet::get(SysSet::FX_VOL))
  {
    SysSet::set(SysSet::FX_VOL,new_fx);
  }
}

void Menu::music_adjust(Menu& menu)
{
  int new_music=menu.run_item();
  if (new_music!=SysSet::get(SysSet::MUSIC_VOL))
  {
    SysSet::set(SysSet::MUSIC_VOL,new_music);
  }
}

void Menu::resolution_adjust(Menu& menu)
{
  int new_resolution=menu.run_item();
  if (new_resolution!=SysSet::get(SysSet::RESOLUTION))
  {
    SysSet::set(SysSet::RESOLUTION,new_resolution);
  }
}

void Menu::detail_level_adjust(Menu& menu)
{
  int new_detail_level=menu.run_item();
  if (new_detail_level!=SysSet::get(SysSet::DETAIL_LEVEL))
  {
    SysSet::set(SysSet::DETAIL_LEVEL,new_detail_level);
  }
}

void Menu::reverse_stereo_adjust(Menu& menu)
{
  int new_stereo=menu.run_item();
  if (new_stereo!=SysSet::get(SysSet::REVERSE_STEREO))
  {
    SysSet::set(SysSet::REVERSE_STEREO,new_stereo);
  }
}

void Menu::immortal_adjust(Menu& menu)
{
  DBG_CHECK(!NoNet::is_network_mode());
  int new_immortal=menu.run_item();
  if (new_immortal&&!NoNet::get_ship()->is_immortal())
  {
    NoNet::get_ship()->enable_immortal();
  }
  else if (!new_immortal&&NoNet::get_ship()->is_immortal())
  {
    NoNet::get_ship()->disable_immortal();
  }
}


void Menu::display_string(Screen& screen,int x,int y,char *text,int col)
{
  DBG_CHECK(strlen(text)<256);
  char buf[256];
  strncpy(buf,text,256);
  buf[256-1]='\0';
  _strupr(buf);
  screen.ink=Color::black;
  screen.print(x+1,y,buf);
  switch (col)
  {
    case 0: screen.ink=(Color::white);break;
    case 1: screen.ink=(Tools::colors[Tools::NET_FIRST]);break;
    case 2: screen.ink=(Tools::colors[Tools::NET_FIRST+1]);break;
    case 3: screen.ink=(Tools::colors[Tools::NET_FIRST+2]);break;
    case 4: screen.ink=(Tools::colors[Tools::NET_FIRST+3]);break;
    default: DBG_CHECK(0);break;
  }
  screen.print(x,y,buf);
}


char Menu::key_table[2*MAX_KEYS][MAX_KEYS_LEN];

void Menu::draw_help(Screen &screen)
{
  int ox=screen.ox;
  int oy=screen.oy;
  int handle[100];

  handle[0]=RegData::Controls.VKeys[RegData::ctl_MoveRight];
  handle[1]=RegData::Controls.VKeys[RegData::ctl_MoveLeft];
  handle[2]=RegData::Controls.VKeys[RegData::ctl_MoveForward];
  handle[3]=RegData::Controls.VKeys[RegData::ctl_MoveBack];
  handle[4]=RegData::Controls.VKeys[RegData::ctl_Fire1];
  handle[5]=RegData::Controls.VKeys[RegData::ctl_Fire2];
  handle[6]=RegData::Controls.VKeys[RegData::ctl_StrafeLeft];
  handle[7]=RegData::Controls.VKeys[RegData::ctl_StrafeRight];
  handle[8]=RegData::Controls.VKeys[RegData::ctl_StrafeShift];
  handle[9]=RegData::Controls.VKeys[RegData::ctl_PrevInvent];
  handle[10]=RegData::Controls.VKeys[RegData::ctl_NextInvent];
  handle[11]=RegData::Controls.VKeys[RegData::ctl_UseInvent];
  handle[12]=RegData::Controls.VKeys[RegData::ctl_TurboShift];
  handle[13]=RegData::Controls.VKeys[RegData::ctl_TurboLock];
  handle[14]=RegData::Controls.VKeys[RegData::ctl_Weapon1];
  handle[15]=RegData::Controls.VKeys[RegData::ctl_Weapon2];
  handle[16]=RegData::Controls.VKeys[RegData::ctl_Weapon3];
  handle[17]=RegData::Controls.VKeys[RegData::ctl_Weapon4];
  handle[18]=RegData::Controls.VKeys[RegData::ctl_Weapon5];
  handle[19]=RegData::Controls.VKeys[RegData::ctl_Weapon6];
  handle[20]=RegData::Controls.VKeys[RegData::ctl_NextWeapon];
  handle[21]=RegData::Controls.VKeys[RegData::ctl_PrevWeapon];
  handle[22]=RegData::Controls.VKeys[RegData::ctl_GotoMouse];

  screen.origin(0,0);

  int stx=180;
  int sty=25;
  int dy=5;
  char tmp[1024];
  int x=stx;
  int y=sty;
  //screen.rectangle(Tools::tool[Tools::LSHADOW],10,5,300,140);
  //screen.rectangle(Tools::tool[Tools::HELP],Layout::helppanel.x,Layout::helppanel.y,Layout::helppanel.w,Layout::helppanel.h);
  Print::print(screen,Print::TSPRITE,Print::FHIGHLIGHTED,x,10,GAMETXT("syskeys"));
  int i;
  for(i=0;i<2*MAX_KEYS;i+=2)
  {
     if (key_table[i][0]=='\0') break;
     sprintf(tmp,"%-22s %s",key_table[i],key_table[i+1]);
     display_string(screen,x,y,tmp,0);
     y+=dy;
  }

  x=20;
  Print::print(screen,Print::TSPRITE,Print::FHIGHLIGHTED,x,10,GAMETXT("gamekeys"));
  y=sty;
  for(i=0;i<23;i+=1)
  {
     sprintf(tmp,"%-26s %s",Vkey::get_key_name(handle[i]),Vkey::get_scan_name(handle[i]));
     display_string(screen,x,y,tmp,0);
     y+=dy;
  }
  screen.origin(ox,oy);
}

void Menu::draw_register(Screen &screen)
{
#ifdef SHAREWARE
  int ox=screen.ox;
  int oy=screen.oy;
  int sprsx=Mysprites::ordering[register_phase].r-Mysprites::ordering[register_phase].l;
  int sprsy=Mysprites::ordering[register_phase].d-Mysprites::ordering[register_phase].u;
  int posx=(Mp::SX-sprsx)/2;
  int posy=(Mp::SY-sprsy)/2;
  screen.origin(0,0);
  screen.put(posx,posy,Mysprites::ordering,register_phase);
  screen.origin(ox,oy);
#endif
}
