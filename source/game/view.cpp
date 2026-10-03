#include "headers.h"

//KLASA VIEW
int View::max_used=0;
int View::initialized=0;
View::Cell View::space[max_objects];
int View::begin[planes_numb][View::AFTERSHD+1];
int View::first_free;

void View::init(void)
{
  DBG_CHECK(!initialized);
  int i,j;
  for (i=0;i<planes_numb;i++)
    for (j=0;j<View::AFTERSHD+1;j++)
    {
      begin[i][j]=NIL;
    }
  first_free=0;
  initialized=1;
}

void View::quit(void)
{
  initialized=0;
}

void View::add(Visible* ptr,int plane,View::Vprty prty)
{
  DBG_CHECK(initialized);
  DBG_CHECK(!(first_free>=max_objects));
  if (first_free>=max_objects)
  {
    return;
  }
  space[first_free].ptr=ptr;
  space[first_free].next=begin[plane][(int)prty];
  begin[plane][(int)prty]=first_free;
  first_free++;
#if HI_DEBUG
  if (first_free>max_used)
  {
    max_used=first_free;
  }
#endif
}

void View::run(Screen& screen,int plane)
{
  DBG_CHECK(initialized);
  screen.origin(-world->get_x(),-world->get_y());
  int i,j;
  j=begin[plane][View::BEFORESHD];
  while (j!=NIL)
  {
    ((Shadow*)space[j].ptr)->draw_shadow(screen,plane);
    j=space[j].next;
  }
  for (i=1;i<View::AFTERSHD;i++)
  {
    j=begin[plane][i];
    while (j!=NIL)
    {
      space[j].ptr->draw_object(screen,plane);
      j=space[j].next;
    }
  }
  j=begin[plane][View::AFTERSHD];
  while (j!=NIL)
  {
    ((Shadow*)space[j].ptr)->draw_shadow(screen,plane);
    j=space[j].next;
  }
}

void View::flush(void)
{
  DBG_CHECK(initialized);
  for (int plane=0;plane<planes_numb;plane++)
  {
    for (int i=0;i<=View::AFTERSHD;i++)
    {
      begin[plane][i]=NIL;
    }
  }
  first_free=0;
}

#if HI_DEBUG
void View::statistics(void)
{
  if (max_used>0)
  {
    if (max_used>max_objects/2)
    {
      DBG_WARNING("[viw] occupancy exceeded 50%");
    }
//    MESSAGE("maximum occupancy: %d%%",(max_used*100)/max_objects);
//    MESSAGE("max number sprites displayed: %d",max_used);
  }
}
#endif
//KLASA DISPLAY
Screen& Display::screen(void)
{
  return *Video::get_screen();
}

void Display::copy2vga(void)
{
  Video::release_screen();
  Video::show_screen();
}

void Display::clear_vga(int palette)
{
  Video::clear_screen();
  Video::select_vpalette(palette);
  Video::clear_screen();
}

void Display::release_screen(void)
{
  Video::release_screen();
}





