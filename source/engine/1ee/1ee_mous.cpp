#include "1ee_hdrs.h"

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
HHOOK         Mouse::hook_handler(NULL);
int           Mouse::last_x;
int           Mouse::last_y;
HWND          Mouse::hwnd=NULL;
unsigned char Mouse::ref[MOUSE_REF_SIZE];
char         *Mouse::names[MOUSE_NAMES_SIZE];
int           Mouse::screen_sx(0);
int           Mouse::screen_sy(0);
RECT          Mouse::cr;
RECT         *Mouse::clip_rect=NULL;
//===========================================================================
// Mouse methods
//===========================================================================
LRESULT CALLBACK Mouse::callback_mouse (int nCode, WPARAM wParam, LPARAM lParam)
{
  MOUSEHOOKSTRUCT *mh = (MOUSEHOOKSTRUCT*)lParam;
  int event;
  event=(ev_Mouse|evm_NewX);
  if (mh->pt.x>0)
    event|=(unsigned short)mh->pt.x;
  Eem::push_event(event);
  event=(ev_Mouse|evm_NewY);
  if (mh->pt.y>0)
    event|=(unsigned short)mh->pt.y;
  Eem::push_event(event);
  event=ev_Mouse;
  switch (wParam)
  {
    case WM_LBUTTONDBLCLK: event|=(evm_DblClk|mb_Left);   break;
    case WM_LBUTTONDOWN:   event|=(evm_Down  |mb_Left);   break;
    case WM_LBUTTONUP:     event|=(evm_Up    |mb_Left);   break;
    case WM_MBUTTONDBLCLK: event|=(evm_DblClk|mb_Middle); break;
    case WM_MBUTTONDOWN:   event|=(evm_Down  |mb_Middle); break;
    case WM_MBUTTONUP:     event|=(evm_Up    |mb_Middle); break;
    case WM_RBUTTONDBLCLK: event|=(evm_DblClk|mb_Right);  break;
    case WM_RBUTTONDOWN:   event|=(evm_Down  |mb_Right);  break;
    case WM_RBUTTONUP:     event|=(evm_Up    |mb_Right);  break;
  }
  if (event!=ev_Mouse)
    Eem::push_event(event);
  if (nCode<0)
    return(CallNextHookEx(hook_handler, nCode, wParam, lParam));
  else
    return(0);
}
//---------------------------------------------------------------------------
void Mouse::init (void)
{
  DBG_CHECK(!status.is(mouse_Initialized));
  hwnd = Comm::hwnd;
  Comm::reinit_mouse = Mouse::set_ex_param;
  status.set(mouse_Initialized);
  clear();
  init_ref();
  hook_handler = SetWindowsHookEx(WH_MOUSE, (HOOKPROC)Mouse::callback_mouse,
                                  NULL, Eem::get_thread());
  if (hook_handler==NULL)
    FAILURE2(Eem_error::general, "unable to hook mouse [Mouse::init]");
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
  else
  {
    RECT r;
    POINT pt;
    GetClientRect(hwnd, &r);
    pt.x=0; pt.y=0;
    ClientToScreen(hwnd, &pt); 
    r.left   +=pt.x; 
    r.right  +=pt.x;
    r.top    +=pt.y;
    r.bottom +=pt.y;
    if (screen_sx)
    {
      x = ((r.right-r.left)*x)/screen_sx;
      x = x+r.left;
    }
    if (screen_sy)
    {
      y = ((r.bottom-r.top)*y)/screen_sy;
      y = y+r.top;
    }
  }
  SetCursorPos(x,y);
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
  if (rc)
    ShowCursor(FALSE);
}
//---------------------------------------------------------------------------
void Mouse::quit (void)
{
  UnhookWindowsHookEx(hook_handler);
  status = 0;
  hook_handler = NULL;
  ShowCursor(TRUE); 
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
    if ((mx!=last_x)||(my!=last_y))
      SetCursorPos(mx, my);
    mx=((mx-clip_rect->left)*screen_sx)/(clip_rect->right-clip_rect->left);
    my=((my-clip_rect->top)*screen_sy)/(clip_rect->bottom-clip_rect->top);
  }
  else
  {
    RECT r;
    POINT pt;
    GetClientRect(hwnd, &r);
    pt.x=0; pt.y=0;
    ClientToScreen(hwnd, &pt); 
    r.left   +=pt.x; 
    r.right  +=pt.x;
    r.top    +=pt.y;
    r.bottom +=pt.y;
    if (last_x<r.left)
      mx=r.left;
    else if (last_x>=r.right)
      mx=r.right-1;
    else
      mx=last_x;
    if (last_y<r.top)
      my=r.top;
    else if (last_y>=r.bottom)
      my=r.bottom-1;
    else
      my=last_y;
    mx=((mx-r.left)*screen_sx)/(r.right-r.left);
    my=((my-r.top)*screen_sy)/(r.bottom-r.top);
  }
  st->x = (short)mx;
  st->y = (short)my;
}
//===========================================================================
