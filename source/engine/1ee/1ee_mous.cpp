#include "1ee_hdrs.h"
#include <SDL.h>

//----------------------------------------------------------------------------
// Mouse buttons
//----------------------------------------------------------------------------
const int mb_Left            = 1;
const int mb_Middle          = 2;
const int mb_Right           = 3;
//----------------------------------------------------------------------------
// Mouse event flags
//----------------------------------------------------------------------------
const int evm_NewX           = 0x00010000; // nowa wspolrzedna X
const int evm_NewY           = 0x00020000; // nowa wspolrzedna Y
const int evm_Down           = 0x00040000; // nacisniecie buttonu
const int evm_Up             = 0x00080000; // zwolnienie buttonu
const int evm_DblClk         = 0x00100000; // double click buttonem
//----------------------------------------------------------------------------
// Mouse event masks
//----------------------------------------------------------------------------
const int mask_Data          = 0x0000FFFF; // maska do otrzymania danych z eventu
//----------------------------------------------------------------------------
// Status flags
//----------------------------------------------------------------------------
const int mouse_Initialized  = 0x00000010; // Mouse zainicjalizowana
//----------------------------------------------------------------------------
// Mouse static variables
//----------------------------------------------------------------------------
Bitflag       Mouse::status(0);
int           Mouse::last_x;
int           Mouse::last_y;
unsigned char Mouse::ref[MOUSE_REF_SIZE];
char         *Mouse::names[MOUSE_NAMES_SIZE];
int           Mouse::screen_sx(0);
int           Mouse::screen_sy(0);
RECT          Mouse::cr;
RECT         *Mouse::clip_rect=NULL;
int           Mouse::pointer_x(0);
int           Mouse::pointer_y(0);
int           Mouse::moved(0);
int           Mouse::relative(0);
//===========================================================================
// Mouse methods
//===========================================================================
// Port: was callback_mouse, the WH_MOUSE hook, which pushed the pointer position with every mouse
// message. Windows coalesced the moves into one message, so the position is now pushed once per
// batch of events (flush) and before each button. The window class had no CS_DBLCLKS, so
// Windows never sent double clicks.
void Mouse::sdl_event (const SDL_Event &e)
{
  switch (e.type)
  {
    case SDL_MOUSEMOTION:
      if (SDL_GetRelativeMouseMode())
      {
        // The hidden pointer stays on the picture (was SetCursorPos in complete).
        pointer_x+=e.motion.xrel;
        pointer_y+=e.motion.yrel;
        if (clip_rect)
        {
          pointer_x=SDL_clamp(pointer_x,(int)clip_rect->left,(int)clip_rect->right-1);
          pointer_y=SDL_clamp(pointer_y,(int)clip_rect->top,(int)clip_rect->bottom-1);
        }
      }
      else
      {
        pointer_x=e.motion.x;
        pointer_y=e.motion.y;
      }
      moved=1;
      break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    {
      int event=ev_Mouse;
      int action=(e.type==SDL_MOUSEBUTTONDOWN) ?evm_Down :evm_Up;
      switch (e.button.button)
      {
        case SDL_BUTTON_LEFT:   event|=(action|mb_Left);   break;
        case SDL_BUTTON_MIDDLE: event|=(action|mb_Middle); break;
        case SDL_BUTTON_RIGHT:  event|=(action|mb_Right);  break;
      }
      if (!SDL_GetRelativeMouseMode())
      {
        pointer_x=e.button.x;
        pointer_y=e.button.y;
      }
      push_position();
      if (event!=ev_Mouse)
        Eem::push_event(event);
      break;
    }
    case SDL_WINDOWEVENT:
      if ((e.window.event==SDL_WINDOWEVENT_FOCUS_GAINED)||
          (e.window.event==SDL_WINDOWEVENT_FOCUS_LOST))
        update_relative();
      break;
  }
}
//---------------------------------------------------------------------------
void Mouse::push_position (void)
{
  int event;
  event=(ev_Mouse|evm_NewX);
  if (pointer_x>0)
    event|=(unsigned short)pointer_x;
  Eem::push_event(event);
  event=(ev_Mouse|evm_NewY);
  if (pointer_y>0)
    event|=(unsigned short)pointer_y;
  Eem::push_event(event);
  moved=0;
}
//---------------------------------------------------------------------------
// Port: Eem calls this before it queues a tick and after each message pump.
void Mouse::flush (void)
{
  if (moved)
    push_position();
}
//---------------------------------------------------------------------------
// Port: relative mode hides the pointer and keeps it in the window, as the full screen original
// did, so mouse steering can't click outside. It is on only while the window has focus.
void Mouse::set_relative (int on)
{
  relative=on;
  update_relative();
}
//---------------------------------------------------------------------------
void Mouse::update_relative (void)
{
  if (Comm::headless||!status.is(mouse_Initialized))
    return;
  SDL_Window *window=SDL_GetKeyboardFocus();
  SDL_bool want=(relative&&window) ?SDL_TRUE :SDL_FALSE;
  if (SDL_GetRelativeMouseMode()==want)
    return;
  SDL_SetRelativeMouseMode(want);
  // Leaving it, put the system pointer where the game's was.
  if (!want&&window)
    SDL_WarpMouseInWindow(window,pointer_x,pointer_y);
}
//---------------------------------------------------------------------------
void Mouse::init (void)
{
  DBG_CHECK(!status.is(mouse_Initialized));
  Comm::reinit_mouse = Mouse::set_ex_param;
  status.set(mouse_Initialized);
  clear();
  init_ref();
  update_relative();
  MESSAGE ("mouse connected.");
}
//---------------------------------------------------------------------------
void Mouse::goto_xy(int x, int y)
{
  if (clip_rect)
  {
    if (screen_sx)
    {
      x=((clip_rect->right-clip_rect->left)*x)/screen_sx;
      x = x+clip_rect->left;
    }
    if (screen_sy)
    {
      y=((clip_rect->bottom-clip_rect->top)*y)/screen_sy;
      y = y+clip_rect->top;
    }
  }
  // Port: was SetCursorPos, with the window's client area when there was no clip rectangle.
  pointer_x = x;
  pointer_y = y;
  if (!Comm::headless&&!SDL_GetRelativeMouseMode()&&SDL_GetMouseFocus())
    SDL_WarpMouseInWindow(SDL_GetMouseFocus(),x,y);
  last_x = x;
  last_y = y;
}
//---------------------------------------------------------------------------
void Mouse::get_xy (int *x, int *y)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_mouse *st = &(Eem::selected()->mouse);
  *x = st->x;
  *y = st->y;
}
//---------------------------------------------------------------------------
void Mouse::set_ex_param (RECT *rc, int sx, int sy)
{
  if (rc)
  {
    // Port: the picture changes place in the window when it is resized. Keep the pointer on the
    // same spot of it (the first time, in its centre).
    if (clip_rect&&(cr.right>cr.left)&&(cr.bottom>cr.top))
    {
      pointer_x=rc->left+(pointer_x-cr.left)*(rc->right-rc->left)/(cr.right-cr.left);
      pointer_y=rc->top+(pointer_y-cr.top)*(rc->bottom-rc->top)/(cr.bottom-cr.top);
    }
    else
    {
      pointer_x=(rc->left+rc->right)/2;
      pointer_y=(rc->top+rc->bottom)/2;
    }
    cr = *rc;
    clip_rect = &cr;
    if (!Comm::production)
      MESSAGE("Mouse params set: left=%d  right=%d  top=%d  bottom=%d  sx=%d  sy=%d",
               cr.left, cr.right, cr.top, cr.bottom, sx, sy);
  }
  else
    clip_rect = NULL;
  screen_sx = sx;
  screen_sy = sy;
  if (rc&&!Comm::headless)
    SDL_ShowCursor(SDL_DISABLE);
}
//---------------------------------------------------------------------------
void Mouse::quit (void)
{
  status = 0;
  if (!Comm::headless)
  {
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_ShowCursor(SDL_ENABLE);
  }
  MESSAGE ("mouse disconnected");
}
//---------------------------------------------------------------------------
int Mouse::is (int code)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_mouse *st = &(Eem::selected()->mouse);
  if (code==anybutton)
  {
    if (st->arr_struck||st->arr_release||st->arr_state)
      return(1);
    return(0);
  }
  else
    return (Bit::is(&st->arr_state, ref[code])||Bit::is(&st->arr_release, ref[code]));
}
//---------------------------------------------------------------------------
int Mouse::dblclk (int code)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_mouse *st = &(Eem::selected()->mouse);
  if (code==anybutton)
  {
    if (st->arr_dblclk)
      return(1);
    return(0);
  }
  else
    return (Bit::is(&st->arr_dblclk, ref[code]));
}
//---------------------------------------------------------------------------
int Mouse::struck (int code)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_mouse *st = &(Eem::selected()->mouse);
  if (code==anybutton)
  {
    if (st->arr_struck)
      return(1);
    return(0);
  }
  else
    return (Bit::is(&st->arr_struck, ref[code]));
}
//---------------------------------------------------------------------------
int Mouse::release (int code)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_mouse *st = &(Eem::selected()->mouse);
  if (code==anybutton)
  {
    if (st->arr_release)
      return(1);
    return(0);
  }
  else
    return (Bit::is(&st->arr_release, ref[code]));
}
//---------------------------------------------------------------------------
int Mouse::press (int code)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_mouse *st = &(Eem::selected()->mouse);
  if (code==anybutton)
  {
    if (st->arr_state&(~(st->arr_struck|st->arr_release)))
      return(1);
    return(0);
  }
  else
    return (Bit::is (&st->arr_state, ref[code])&&
            (!Bit::is(&st->arr_struck, ref[code]))&&
            (!Bit::is(&st->arr_release, ref[code])));
}
//---------------------------------------------------------------------------
int Mouse::is_initialized(void)
//---------------------------------------------------------------------------
{
  return (status.is(mouse_Initialized));
}
//===========================================================================
// Mouse private
//===========================================================================

void Mouse::get_event(int event)
{
  Eem::St_mouse *st = &(Eem::get_new()->mouse);
  if (event&evm_NewX)
    last_x = event&mask_Data;
  else if (event&evm_NewY)
    last_y = event&mask_Data;
  else if (event&evm_DblClk)
    Bit::set(&st->arr_dblclk, event&mask_Data);
  else if (event&evm_Down)
  {
    Bit::set(&st->arr_state, event&mask_Data);
    Bit::set(&st->arr_struck, event&mask_Data);
  }
  else if (event&evm_Up)
  {
    Bit::reset(&st->arr_state, event&mask_Data);
    Bit::set(&st->arr_release, event&mask_Data);
  }
}
//---------------------------------------------------------------------------
void Mouse::init_ref(void)
//---------------------------------------------------------------------------
{
  int i;
  for (i=0; i<MOUSE_REF_SIZE; i++)
  {
    ref[i]=0;
    ref[i]=0;
  }
  for (i=0; i<MOUSE_NAMES_SIZE; i++)
    names[i]=NULL;
  ref[left]          = mb_Left;
  ref[left_button]   = mb_Left;
  ref[leftbutton]    = mb_Left;
  ref[middle]        = mb_Middle;
  ref[middle_button] = mb_Middle;
  ref[middlebutton]  = mb_Middle;
  ref[mid]           = mb_Middle;
  ref[mid_button]    = mb_Middle;
  ref[midbutton]     = mb_Middle;
  ref[right]         = mb_Right;
  ref[right_button]  = mb_Right;
  ref[rightbutton]   = mb_Right;

  names[mb_Left]   = "left button";
  names[mb_Right]  = "right button";
  names[mb_Middle] = "middle button";
}
//---------------------------------------------------------------------------                                                                              
char *Mouse::get_key_name(int code)
{
  return(names[ref[code]]);
}
//---------------------------------------------------------------------------
unsigned char Mouse::get_struck(void)
{
  DBG_CHECK(Eem::last_return_code!=0);   
  DBG_CHECK(Eem::selected()!=NULL);                          
  Eem::St_mouse *st = &(Eem::selected()->mouse);
  for (int i=0; i<MOUSE_NAMES_SIZE; i++)
    if (Bit::is(&st->arr_struck, i))
      return((unsigned char)i);
  return(no_struck);
}
//---------------------------------------------------------------------------
char *Mouse::get_scan_name(unsigned char scan)
{
  return (names[scan]);
}
//---------------------------------------------------------------------------
unsigned char Mouse::get_scan(int code)
{
  return(ref[code]);
}
//---------------------------------------------------------------------------
char *Mouse::get_struck_name(void)
{
  return (names[get_struck()]);
}
//---------------------------------------------------------------------------
void Mouse::clear(void)
{
}
//---------------------------------------------------------------------------
void Mouse::next_read (void)
{
}
//---------------------------------------------------------------------------
void Mouse::complete (void)
{
  Eem::St_mouse *st = &(Eem::get_new()->mouse);
  int mx, my;
  if (clip_rect)
  {
    if (last_x<clip_rect->left)
      mx=clip_rect->left;
    else if (last_x>=clip_rect->right)
      mx=clip_rect->right-1;
    else
      mx=last_x;
    if (last_y<clip_rect->top)
      my=clip_rect->top;
    else if (last_y>=clip_rect->bottom)
      my=clip_rect->bottom-1;
    else
      my=last_y;
    // Port: SetCursorPos dropped here. In relative mode the pointer is already kept on the
    // picture (sdl_event); a visible system pointer in a window is left where it is.
    mx=((mx-clip_rect->left)*screen_sx)/(clip_rect->right-clip_rect->left);
    my=((my-clip_rect->top)*screen_sy)/(clip_rect->bottom-clip_rect->top);
  }
  else
  {
    // Port: no picture yet (was the window's client area).
    mx=0;
    my=0;
  }
  st->x = (short)mx;
  st->y = (short)my;
}
//===========================================================================
