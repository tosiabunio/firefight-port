#include "headers.h"

//KLASA ITEM
Item::Item(int _select_status)
{
  select_status=_select_status;
}

int Item::selectable(int status)
{
  if (status==-1)
  {
    return select_status;
  }
  else if (status==0)
  {
    select_status=0;
  }
  else
  {
    select_status=1;
  }
  return select_status;
}

int Item::set_status(int _status)
{
  return (status=_status);
}
//KLASA TEXTITEM
TextItem::TextItem(char* description,Text& descript_text)
:Item(0)
{
  text=description;
  (void)descript_text;
}

void TextItem::draw(Screen& screen,int x,int y,int active)
{
  (void)active;
  Print::print(screen,Print::TSPRITE,Print::FSTATIC,x,y,text);
}

int TextItem::run(Menu &menu)
{
  (void)menu;
#if HI_DEBUG
  FAILURE("TextItem::run(Screen& screen,int x,int y) - this function shouldn't have been called.");
#endif
  return 0;
}

int TextItem::get_dx(void)
{
  return Print::get_dx(Print::TSPRITE,text);
}

int TextItem::get_dy(void)
{
  return Print::get_dy(Print::TSPRITE,text);
}
//KLASA SUBITEM
SubItem::SubItem(char* description,Text& descript_text)
:Item(1)
{
  text=description;
  (void)descript_text;
}

void SubItem::draw(Screen& screen,int x,int y,int active)
{
  if (select_status)
  {
    if (active)
    {
      Print::print(screen,Print::TSPRITE,Print::FHIGHLIGHTED,x,y,text);
    }
    else
    {
      Print::print(screen,Print::TSPRITE,Print::FNORMAL,x,y,text);
    }
  }
  else
  {
    Print::print(screen,Print::TSPRITE,Print::FDISABLED,x,y,text);
  }
}

int SubItem::run(Menu &menu)
{
  (void)menu;
  return KbdStat::menu_accept();
}

int SubItem::get_dx(void)
{
  return Print::get_dx(Print::TSPRITE,text);
}

int SubItem::get_dy(void)
{
  return Print::get_dy(Print::TSPRITE,text);
}
//KLASA BUTTITEM
ButtItem::ButtItem(char* description,Text& descript_text)
:Item(1)
{
  status=0;
  text=(char*)Heap::alloc(sizeof(char)*1024,item_mbn);
  strcpy(text,"  ");
  strcat(text,description);
  (void)descript_text;
}

ButtItem::~ButtItem(void)
{
  if (text!=NULL)
  {
    Heap::free(text);
    text=NULL;
  }
}

void ButtItem::draw(Screen& screen,int x,int y,int active)
{
  if (!status)
  {
    *text='\xcb';
  }
  else
  {
    *text='\xca';
  }
  char temp[3];
  for (int i=0;i<2;i++)
  {
    temp[i]=text[i];
  }
  temp[2]='\0';
  x-=Print::get_dx(Print::TSPRITE,temp);
  if (select_status)
  {
    if (active)
    {
      Print::print(screen,Print::TSPRITE,Print::FHIGHLIGHTED,x,y,text);
    }
    else
    {
      Print::print(screen,Print::TSPRITE,Print::FNORMAL,x,y,text);
    }
  }
  else
  {
    Print::print(screen,Print::TSPRITE,Print::FDISABLED,x,y,text);
  }
}

int ButtItem::run(Menu &menu)
{
  (void)menu;
  if (KbdStat::menu_set_option())
  {
    status=!status;
    Sound::st_play(&Mysound::options_switch);
  }
  return status;
}

int ButtItem::get_dx(void)
{
  return Print::get_dx(Print::TSPRITE,text);
}

int ButtItem::get_dy(void)
{
  return Print::get_dy(Print::TSPRITE,text);
}
//KLASA RBUTTITEM
RButtItem::RButtItem(char* description,Text& descript_text)
:Item(1)
{
  status=0;
  item_left=1;
  potential=status;
#if HI_DEBUG
  if (!descript_text.exist(description))
  {
    FAILURE("RButtItem::RButtItem(char* description,Text& descript_text) - there is no label %s in descript_text",description);
  }
#endif
  size=descript_text.size(description);
#if HI_DEBUG
  if (size<=0)
  {
    FAILURE("RButtItem::RButtItem(char* description,Text& descript_text) - illegal size (size<=0).");
  }
#endif
  text=(char**)Heap::alloc(sizeof(char*)*size,item_mbn);
  int i;
  for (i=0;i<size;i++)
  {
    text[i]=(char*)Heap::alloc(sizeof(char)*1024,item_mbn);
  }
  for (i=0;i<size;i++)
  {
    strcpy(text[i],"  ");
    strcat(text[i],descript_text.string(description,i));
  }
  space_y=descript_text.value("/globals/radio_button_between_y");
}

RButtItem::~RButtItem(void)
{
  if (text)
  {
    for (int i=0;i<size;i++)
    {
      Heap::free((void*)text[i]);
    }
    Heap::free((void*)text);
    text=NULL;
  }
}

int RButtItem::set_status(int _status)
{
  status=_status;
  potential=status;
  return status;
}

void RButtItem::draw(Screen& screen,int x,int y,int active)
{
  for (int i=0;i<size;i++)
  {
    if (i!=status)
    {
      *text[i]='\xc9';
    }
    else
    {
      *text[i]='\xc8';
    }
    char temp[3];
    for (int j=0;j<2;j++)
    {
      temp[j]=text[i][j];
    }
    temp[2]='\0';
    int posx=x-Print::get_dx(Print::TSPRITE,temp);
    Print::Filters filter;
    if (select_status)
    {
      if (active&&i==potential)
      {
        filter=Print::FHIGHLIGHTED;
      }
      else
      {
        filter=Print::FNORMAL;
      }
    }
    else
    {
      filter=Print::FDISABLED;
    }
    Print::print(screen,Print::TSPRITE,filter,posx,y,text[i]);
    y+=Print::get_dy(Print::TSPRITE,text[i])+space_y;
  }
}

int RButtItem::run(Menu &menu)
{
  int ignore_keyboard=0;
  if (item_left)
  {
    potential=status;
    item_left=0;
    ignore_keyboard=1;
    menu.freeze_keys();
  }
  if (KbdStat::menu_up()&&!ignore_keyboard)
  {
    if (--potential<0)
    {
      potential=0;
      item_left=1;
      menu.set_active(menu.find_first_selectable(-1));
      menu.unfreeze_keys();
    }
    Sound::st_play(&Mysound::options_updown);
 }
  if (KbdStat::menu_down()&&!ignore_keyboard)
  {
    if (++potential>=size)
    {
      potential=size-1;
      item_left=1;
      menu.set_active(menu.find_first_selectable(1));
      menu.unfreeze_keys();
    }
    Sound::st_play(&Mysound::options_updown);
  }
  if (KbdStat::menu_page_down()&&!ignore_keyboard)
  {
    for (int i=menu.size-1;i>=0;i--)
    {
      if (menu.items[i]->selectable())
      {
        item_left=1;
        menu.unfreeze_keys();
        menu.set_active(i);
        break;
      }
    }
    Sound::st_play(&Mysound::options_updown);
  }
  if (KbdStat::menu_page_up()&&!ignore_keyboard)
  {
    for (int i=0;i<menu.size;i++)
    {
      if (menu.items[i]->selectable())
      {
        item_left=1;
        menu.unfreeze_keys();
        menu.set_active(i);
        break;
      }
    }
    Sound::st_play(&Mysound::options_updown);
  }
  if (KbdStat::menu_set_option()&&!ignore_keyboard)
  {
    status=potential;
    Sound::st_play(&Mysound::options_switch);
  }
  if (KbdStat::menu_quit()&&!ignore_keyboard)
  {
    Menu::pop_menu();
  }
  return status;
}

int RButtItem::get_dx(void)
{
  int x_size(0);
  int temp(0);
  for (int i=0;i<size;i++)
  {
    if ((temp=Print::get_dx(Print::TSPRITE,text[i]))>x_size)
    {
      x_size=temp;
    }
  }
  return x_size;
}

int RButtItem::get_dy(void)
{
  int y_size(0);
  for (int i=0;i<size;i++)
  {
    y_size+=Print::get_dy(Print::TSPRITE,text[i])+space_y;
  }
  return y_size;
}
//KLASA ZIPPITEM
ZippItem::ZippItem(char* description,Text& descript_text)
:Item(1)
{
  text=(char*)Heap::alloc(sizeof(char)*1024,item_mbn);
  char buffer[1024];
  char *delims=",";
  strcpy(buffer,description);
  char *down=strtok(buffer,delims);
  char *up=strtok(NULL,delims);
  strcpy(text,strtok(NULL,delims));
  sscanf(down,"%d",&down_range);
  sscanf(up,"%d",&up_range);
  DBG_CHECK(down_range<up_range);
  zipper_length=descript_text.value("/globals/zipper_length");
  zipper_states=descript_text.value("/globals/zipper_states");
  zipper_item_width=descript_text.value("/globals/zipper_item_width");
  first_time_status=1;
  set_status((down_range+up_range)/2);
}

ZippItem::~ZippItem(void)
{
  if (text!=NULL)
  {
    Heap::free((void*)text);
    text=NULL;
  }
}

void ZippItem::draw(Screen& screen,int x,int y,int active)
{
  DBG_CHECK(status>=down_range&&status<=up_range);
  Print::Filters filter;
  if (select_status)
  {
    if (active)
    {
      filter=Print::FHIGHLIGHTED;
    }
    else
    {
      filter=Print::FNORMAL;
    }
  }
  else
  {
    filter=Print::FDISABLED;
  }
  Print::print(screen,Print::TSPRITE,filter,x,y,text);
  int zipper_width(0);
  int i;
  for (i=0;i<zipper_length;i++)
  {
    zipper_width+=Print::get_dx(Print::TSPRITE,"\xcc");
  }
  x+=zipper_item_width-zipper_width;
  int left_x=x;
  for (i=0;i<zipper_length;i++)
  {
    Print::print(screen,Print::TSPRITE,filter,x,y,"\xcc");
    x+=Print::get_dx(Print::TSPRITE,"\xcc");
  }
  int len=x-left_x-Print::get_dx(Print::TSPRITE,"\xcd");
  int posx=left_x+(state*len/(zipper_states-1));
  Print::print(screen,Print::TSPRITE,filter,posx,y,"\xcd");
}

int ZippItem::run(Menu &menu)
{
  (void)menu;
  DBG_CHECK(status>=down_range&&status<=up_range);
  if (KbdStat::menu_right())
  {
    if (state<zipper_states-1)
    {
      state++;
      int interval=up_range-down_range;
      status=down_range+(state*interval)/(zipper_states-1);
    }
  }
  if (KbdStat::menu_left())
  {
    if (state>0)
    {
      state--;
      int interval=up_range-down_range;
      status=down_range+(state*interval)/(zipper_states-1);
    }
  }
  DBG_CHECK(status>=down_range&&status<=up_range);
  return status;
}

int ZippItem::set_status(int _status)
{
  DBG_CHECK(_status>=down_range&&_status<=up_range);
  if (_status!=status||first_time_status)
  {
    status=_status;
    int interval=up_range-down_range;
    int temp_status=status-down_range;
    state=(temp_status*zipper_states)/(interval+1);
    DBG_CHECK(state>=0&&state<zipper_states);
    first_time_status=0;
  }
  DBG_CHECK(status>=down_range&&status<=up_range);
  return status;
}

int ZippItem::get_dx(void)
{
  DBG_CHECK(status>=down_range&&status<=up_range);
  int x_size=Print::get_dx(Print::TSPRITE,text)+Print::get_dx(Print::TSPRITE," ");
  for (int i=0;i<zipper_length;i++)
  {
    x_size+=Print::get_dx(Print::TSPRITE,"\xcc");
  }
  return x_size;
}

int ZippItem::get_dy(void)
{
  DBG_CHECK(status>=down_range&&status<=up_range);
  return Print::get_dy(Print::TSPRITE,text);
}



























































