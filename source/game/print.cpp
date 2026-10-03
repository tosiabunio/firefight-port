#include "headers.h"

Sprite Print::big;
Sprite Print::sml;
Sprite Print::big_italic;
Sprite Print::sml_italic;
int Print::colors[Print::_FLAST+1];
Tool Print::tools[Print::_FLAST+1];
int Print::space_x;
int Print::space_y;
int Print::sh_offset_x;
int Print::sh_offset_y;
char Print::case_term;
char Print::italic_term;
unsigned char Print::conv[256];
#if HI_DEBUG
int Print::initialized(0);
#endif

void Print::init(char *small_name,char *small_italic_name,char *big_name,char *big_italic_name,char *descr_text)
{
  Text text;
  text.load(descr_text);
  sml.load(small_name);
  sml_italic.load(small_italic_name);
  big.load(big_name);
  big_italic.load(big_italic_name);
  space_x=text.value("space_x");
  space_y=text.value("space_y");
  sh_offset_x=text.value("sh_offset_x");
  sh_offset_y=text.value("sh_offset_y");
  DBG_CHECK(sh_offset_x>=0);
  DBG_CHECK(sh_offset_y>=0);
  case_term=*text.string("case_term");
  italic_term=*text.string("italic_term");
  int colors_numb=text.size("colors_definition");
  int tools_numb=text.size("tools_definition");
  DBG_CHECK(colors_numb==(int)_FLAST);
  DBG_CHECK(tools_numb==(int)_FLAST);
  unsigned char *ptr=(unsigned char*)text.string("conv_table");
  int conv_size=strlen((char*)ptr);
  for (int i=0;i<256;i++)
  {
    conv[i]=0;
  }
  for (i=0;i<conv_size;i++)
  {
    conv[ptr[i]]=i;
  }
  int r1(0),g1(0),b1(0),r2(0),g2(0),b2(0);
  for (i=0;i<_FLAST;i++)
  {
    sscanf(text.string("colors_definition",i),"%d,%d,%d",&r1,&g1,&b1);
    colors[i]=Video::closest(Color(r1,g1,b1));
    sscanf(text.string("tools_definition",i),"%d,%d,%d,%d,%d,%d",&r1,&g1,&b1,&r2,&g2,&b2);
    tools[i].glass(Color(r1,g1,b1),Color(r2,g2,b2));
  }
  tools[_FLAST].brightness(text.value("shadow_power"));
  colors[_FLAST]=Video::closest(Color::cblack);
#if HI_DEBUG
  initialized=1;
#endif
}

void Print::quit(void)
{
  sml.free();
  sml_italic.free();
  big.free();
  big_italic.free();
#if HI_DEBUG
  initialized=0;
#endif
}

void Print::print(Screen& screen,Type type,Filters filter,int x,int y,char *text,...)
{
  DBG_CHECK(initialized);
  DBG_CHECK((int)filter>=0&&(int)filter<=_FLAST);
  DBG_CHECK((int)type>=0&&(int)type<_TLAST);
  char buffer[1024];
  va_list argptr;
  va_start(argptr,text);
  vsprintf(buffer,text,argptr);
  va_end(argptr);
  switch (type)
  {
    case TSPRITE: {
                      int use_big=1;
                      int use_italic=0;
                      int posx=x;
                      int posy=y;
                      int str_ptr=0;
                      if (toupper(buffer[0])==case_term)
                      {
                        use_big=0;
                        str_ptr=1;
                      }
                      unsigned char character=0;
                      while ((character=toupper(buffer[str_ptr]))!='\0')
                      {
                        if (character==italic_term)
                        {
                          use_italic=!use_italic;
                        }
                        else if (character=='\n')
                        {
                          posx=x;
                          posy+=get_dy(type,use_big,use_italic)+space_y;
                        }
                        else
                        {
                          Sprite *spr=NULL;
                          if (use_big)
                          {
                            if (use_italic)
                            {
                              spr=&big_italic;
                            }
                            else
                            {
                              spr=&big;
                            }
                          }
                          else
                          {
                            if (use_italic)
                            {
                              spr=&sml_italic;
                            }
                            else
                            {
                              spr=&sml;
                            }
                          }
                          posx+=get_dx(type,character,use_big,use_italic);
                          screen.shape(posx+sh_offset_x,posy+sh_offset_y,*spr,conv[character],Spr::with_tool,(void*)&tools[_FLAST]);
                          screen.put(posx,posy,*spr,conv[character],Spr::with_tool,(void*)&tools[filter]);
                          posx+=space_x;
                        }
                        str_ptr++;
                      }
                    }
                    break;
    case TFONT:     screen.ink=colors[filter];
                    screen.print(x,y,buffer);
                    break;
  }
}

int Print::get_dx(Type type,char *text,...)
{
  DBG_CHECK(initialized);
  DBG_CHECK((int)type>=0&&(int)type<_TLAST);
  char buffer[1024];
  va_list argptr;
  va_start(argptr,text);
  vsprintf(buffer,text,argptr);
  va_end(argptr);
  int use_big=1;
  int use_italic=0;
  int size_x=0;
  int temp_size=0;
  int str_ptr=0;
  if (toupper(buffer[0])==case_term)
  {
    use_big=0;
    str_ptr=1;
  }
  unsigned char character=0;
  while ((character=toupper(buffer[str_ptr]))!='\0')
  {
    if (character==italic_term)
    {
      use_italic=!use_italic;
    }
    else if (character=='\n')
    {
      if (temp_size>size_x)
      {
        size_x=temp_size;
        temp_size=0;
      }
    }
    else
    {
      switch (type)
      {
        case TSPRITE:   temp_size+=space_x;
        case TFONT:     temp_size+=get_dx(type,character,use_big,use_italic);
                        break;
      }
    }
    str_ptr++;
  }
  if (temp_size>size_x)
  {
    size_x=temp_size;
  }
  return size_x;
}

int Print::get_dy(Type type,char *text,...)
{
  DBG_CHECK(initialized);
  DBG_CHECK((int)type>=0&&(int)type<_TLAST);
  char buffer[1024];
  va_list argptr;
  va_start(argptr,text);
  vsprintf(buffer,text,argptr);
  va_end(argptr);
  int use_big=1;
  int use_italic=0;
  int size_y=0;
  int str_ptr=0;
  if (toupper(buffer[0])==case_term)
  {
    use_big=0;
    str_ptr=1;
  }
  unsigned char character=0;
  while ((character=buffer[str_ptr])!='\0')
  {
    if (character==italic_term)
    {
      use_italic=!use_italic;
    }
    else if (character=='\n')
    {
      switch (type)
      {
        case TSPRITE:   size_y+=get_dy(type,use_big,use_italic)+space_y;
                        break;
        case TFONT:     size_y+=(int)Spr::font_sy;
                        break;
      }
    }
    str_ptr++;
  }
  switch (type)
  {
    case TSPRITE:   size_y+=get_dy(type,use_big,use_italic)+space_y;
                    break;
    case TFONT:     size_y+=(int)Spr::font_sy;
                    break;
  }
  return size_y;
}

int Print::get_dx(Type type,unsigned char character,int use_big,int use_italic)
{
  DBG_CHECK(initialized);
  DBG_CHECK((int)type>=0&&(int)type<_TLAST);
  switch (type)
  {
    case TSPRITE: {  
                    Sprite *spr=NULL;
                    if (use_big)
                    {
                      if (use_italic)
                      {
                        spr=&big_italic;
                      }
                      else
                      {
                        spr=&big;
                      }
                    }
                    else
                    {
                      if (use_italic)
                      {
                        spr=&sml_italic;
                      }
                      else
                      {
                        spr=&sml;
                      }
                    }
                    return -(*spr)[conv[character]].l;
                  }
    case TFONT:   return Spr::font_sx;
  }
  return 0;
}

int Print::get_dy(Type type,int use_big,int use_italic)
{
  DBG_CHECK(initialized);
  DBG_CHECK((int)type>=0&&(int)type<_TLAST);
  switch (type)
  {
    case TSPRITE: {  
                    Sprite *spr=NULL;
                    if (use_big)
                    {
                      if (use_italic)
                      {
                        spr=&big_italic;
                      }
                      else
                      {
                        spr=&big;
                      }
                    }
                    else
                    {
                      if (use_italic)
                      {
                        spr=&sml_italic;
                      }
                      else
                      {
                        spr=&sml;
                      }
                    }
                    return (*spr)[conv['A']].d;
                  }
      case TFONT: return Spr::font_sy;
  }
  return 0;
}

Tool& Print::get_tool(Filters filter)
{
  DBG_CHECK(initialized);
  DBG_CHECK((int)filter>=0&&(int)filter<=_FLAST);
  return tools[filter];
}

int Print::get_color(Filters filter)
{
  DBG_CHECK(initialized);
  DBG_CHECK((int)filter>=0&&(int)filter<=_FLAST);
  return colors[filter];
}
