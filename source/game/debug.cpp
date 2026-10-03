#include "headers.h"

int Debuginfo::status(0);
int Debuginfo::options(2);

void Debuginfo::run(void)
{
  if (KbdStat::colisworld())
  {
    status=(++status)%options;
  }
}

void Debuginfo::draw(Screen& screen)
{
  screen.origin(0,0);
  screen.ink=Color::white;
  switch (status)
  {
    case  0: break;
    case  1: LowColis::show_cworld(world->get_current_displayed(),screen,0,0);
             break;
    default: DBG_CHECK(0);
             break;
  }
}

