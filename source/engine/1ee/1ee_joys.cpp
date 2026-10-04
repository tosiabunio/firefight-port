#include "1ee_hdrs.h"
#include <SDL.h>

const int joy_Initialized    = 0x00000010; // Joy zainicjalizowany
const int joy_Active         = 0x00000001; // Joy zainicjalizowany
const int joy_OnLine         = 0x00000002; // Joy zainicjalizowany
const int joy_AfterClear     = 0x00000004; // Joy zainicjalizowany
const int joy_Subsystem      = 0x00000020; // port: SDL game controller subsystem started

const int evj_1              = 0x00010000; // 1/3 czesc danych joysticka
const int evj_2              = 0x00020000; // 2/3 czesc danych joysticka
const int evj_3              = 0x00040000; // 3/3 czesc danych joysticka
//----------------------------------------------------------------------------
// Joy static variables
//----------------------------------------------------------------------------
Bitflag        Joy::status(0);
int            Joy::dead(0);
int            Joy::last_1(0);
int            Joy::last_2(0);
int            Joy::last_3(0);
unsigned       Joy::diag_busy(0);
char          *Joy::names[JOY_NAMES_SIZE];

// Port: the open device. joystick is set for a game controller too.
static SDL_GameController *controller=NULL;
static SDL_Joystick       *joystick=NULL;
static SDL_JoystickID      joystick_id=-1;

static char *name_or_empty (const char *name)
{
  return((char*)(name ?name :""));
}
//===========================================================================
// Joy
//===========================================================================
// Port: the dead zone is a percentage of the axis range on either side of the centre, as with
// winmm's (max-min)*percent/100. SDL axes run from -32768 to 32767, centred on 0.
void Joy::set_dead_zone (int dead_percent)
{
  dead = (65535 * dead_percent)/100;
}
//---------------------------------------------------------------------------
// Port: the first SDL joystick or game controller (was the first winmm joystick). Devices can be
// plugged in later (sdl_event).
void Joy::init (int dead_percent)
{
  status.set(joy_Initialized);
  init_ref();
  set_dead_zone(dead_percent);
  SYSINFO("joystick driver capabilities");
  // Headless runs (tests) ignore real devices. Only an input script's virtual controller, whose
  // subsystem is started by then, is opened (usable).
  if (Comm::headless&&!SDL_WasInit(SDL_INIT_GAMECONTROLLER))
  {
    SYSINFO("  not used when headless");
    return;
  }
  if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)!=0)
  {
    SYSINFO("  SDL game controllers unavailable: %s", (char*)SDL_GetError());
    return;
  }
  status.set(joy_Subsystem);
  int joy_num = SDL_NumJoysticks();
  SYSINFO("  %d joysticks found", joy_num);
  for (int i=0; i<joy_num; i++)
  {
    SYSINFO("  joystick #%d: '%s'%s", i, name_or_empty(SDL_JoystickNameForIndex(i)),
            SDL_IsGameController(i) ?", game controller" :"");
    if (!status.is(joy_Active))
      open_device(i);
  }
}
//---------------------------------------------------------------------------
static int usable (int index)
{
  return (!Comm::headless||SDL_JoystickIsVirtual(index));
}
//---------------------------------------------------------------------------
void Joy::open_device (int index)
{
  if (!usable(index))
    return;
  if (SDL_IsGameController(index))
  {
    controller = SDL_GameControllerOpen(index);
    joystick = controller ?SDL_GameControllerGetJoystick(controller) :NULL;
  }
  else
    joystick = SDL_JoystickOpen(index);
  if (joystick==NULL)
  {
    controller = NULL;
    SYSINFO("  joystick #%d not opened: %s", index, (char*)SDL_GetError());
    return;
  }
  joystick_id = SDL_JoystickInstanceID(joystick);
  if (controller)
    SYSINFO("    game controller '%s'", name_or_empty(SDL_GameControllerName(controller)));
  else
    SYSINFO("    axes %d, buttons %d, hats %d", SDL_JoystickNumAxes(joystick),
            SDL_JoystickNumButtons(joystick), SDL_JoystickNumHats(joystick));
  status.set(joy_Active);
  MESSAGE("joystick connected");
}
//---------------------------------------------------------------------------
void Joy::close_device (void)
{
  if (controller)
    SDL_GameControllerClose(controller);
  else if (joystick)
    SDL_JoystickClose(joystick);
  controller = NULL;
  joystick = NULL;
  joystick_id = -1;
  status.reset(joy_Active);
}
//---------------------------------------------------------------------------
// Port: hot plugging. The original read one joystick found at start and dropped it on an error.
void Joy::sdl_event (const SDL_Event &e)
{
  if (!status.is(joy_Subsystem))
    return;
  if ((e.type==SDL_JOYDEVICEADDED)&&!status.is(joy_Active))  // sent for game controllers too
    open_device(e.jdevice.which);
  else if ((e.type==SDL_JOYDEVICEREMOVED)&&status.is(joy_Active)&&(e.jdevice.which==joystick_id))
  {
    MESSAGE("Joystick disconected");
    // Release whatever it held; the original kept its last state.
    unsigned char joy_state[JOY_ARRAY_SIZE];
    memset(joy_state, 0, JOY_ARRAY_SIZE);
    push_state(joy_state);
    close_device();
    for (int i=0; (i<SDL_NumJoysticks())&&!status.is(joy_Active); i++)
      open_device(i);
  }
}
//---------------------------------------------------------------------------
void Joy::init_ref(void)
{
  for (int i=0; i<JOY_NAMES_SIZE; i++)
    names[i]=NULL;
  names[button1]    = "joy button 1";
  names[button2]    = "joy button 2";
  names[button3]    = "joy button 3";
  names[button4]    = "joy button 4";                  
  names[button5]    = "joy button 5";
  names[button6]    = "joy button 6";
  names[button7]    = "joy button 7";
  names[button8]    = "joy button 8";
  names[button9]    = "joy button 9";
  names[button10]   = "joy button 10";
  names[button11]   = "joy button 11";
  names[button12]   = "joy button 12";
  names[button13]   = "joy button 13";
  names[button14]   = "joy button 14";
  names[button15]   = "joy button 15";
  names[button16]   = "joy button 16";
  names[button17]   = "joy button 17";
  names[button18]   = "joy button 18";
  names[button19]   = "joy button 19";
  names[button20]   = "joy button 20";
  names[button21]   = "joy button 21";
  names[button22]   = "joy button 22";
  names[button23]   = "joy button 23";
  names[button24]   = "joy button 24";
  names[button25]   = "joy button 25";
  names[button26]   = "joy button 26";
  names[button27]   = "joy button 27";
  names[button28]   = "joy button 28";
  names[button29]   = "joy button 29";
  names[button30]   = "joy button 30";
  names[button31]   = "joy button 31";
  names[button32]   = "joy button 32";
  names[left]       = "joy left";
  names[right]      = "joy right";
  names[up]         = "joy up";
  names[down]       = "joy down";
  names[z_minus]    = "joy z-axe minus";
  names[z_plus]     = "joy z-axe plus";
  names[r_minus]    = "joy r-axe minus";
  names[r_plus]     = "joy r-axe plus";
  names[u_minus]    = "joy u-axe minus";
  names[u_plus]     = "joy u-axe plus";
  names[v_minus]    = "joy v-axe minus";
  names[v_plus]     = "joy v-axe plus";
}
//----------------------------------------------------------------------------
void Joy::quit (void)
{
  ENGINFO("joystick status: queue overflown %d", diag_busy);
  close_device();
  if (status.is(joy_Subsystem))
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
  status.reset(joy_Subsystem);
  status.reset(joy_Active);
  status.reset(joy_Initialized);
}
//----------------------------------------------------------------------------
// Port: an axis outside the dead zone sets its minus or plus bit (as the winmm position compared
// with centre-dead and centre+dead).
static void axis_bits (unsigned char *joy_state, int value, int minus, int dead)
{
  if (value < -dead)
    Bit::set(joy_state, minus);
  else if (value > dead)
    Bit::set(joy_state, minus+1);
}
//----------------------------------------------------------------------------
// Port: was joyGetPosEx. A game controller reads as an Xbox pad did through winmm: X/Y the left
// stick, Z the triggers (left is plus), R/U the right stick, and buttons 1-10 A, B, X, Y, the
// shoulders, Back, Start and the stick clicks. Its D-pad, a POV hat the original didn't read,
// also sets the X/Y bits, and so does a plain joystick's first hat.
static void read_device (unsigned char *joy_state, int dead)
{
  if (controller)
  {
    static const SDL_GameControllerButton buttons[] = {
      SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X,
      SDL_CONTROLLER_BUTTON_Y, SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
      SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_BUTTON_BACK,
      SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_LEFTSTICK,
      SDL_CONTROLLER_BUTTON_RIGHTSTICK};
    for (int i=0; i<(int)(sizeof(buttons)/sizeof(buttons[0])); i++)
      if (SDL_GameControllerGetButton(controller, buttons[i]))
        Bit::set(joy_state, Joy::button1+i);
    axis_bits(joy_state, SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX), Joy::x_minus, dead);
    axis_bits(joy_state, SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY), Joy::y_minus, dead);
    axis_bits(joy_state, SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT)-
                         SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT), Joy::z_minus, dead);
    axis_bits(joy_state, SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY), Joy::r_minus, dead);
    axis_bits(joy_state, SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX), Joy::u_minus, dead);
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))  Bit::set(joy_state, Joy::x_minus);
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) Bit::set(joy_state, Joy::x_plus);
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP))    Bit::set(joy_state, Joy::y_minus);
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))  Bit::set(joy_state, Joy::y_plus);
  }
  else
  {
    int buttons = SDL_min(SDL_JoystickNumButtons(joystick), 32);
    for (int i=0; i<buttons; i++)
      if (SDL_JoystickGetButton(joystick, i))
        Bit::set(joy_state, Joy::button1+i);
    int axes = SDL_min(SDL_JoystickNumAxes(joystick), 6);
    for (int i=0; i<axes; i++)
      axis_bits(joy_state, SDL_JoystickGetAxis(joystick, i), Joy::x_minus+2*i, dead);
    if (SDL_JoystickNumHats(joystick)>0)
    {
      Uint8 hat = SDL_JoystickGetHat(joystick, 0);
      if (hat&SDL_HAT_LEFT)  Bit::set(joy_state, Joy::x_minus);
      if (hat&SDL_HAT_RIGHT) Bit::set(joy_state, Joy::x_plus);
      if (hat&SDL_HAT_UP)    Bit::set(joy_state, Joy::y_minus);
      if (hat&SDL_HAT_DOWN)  Bit::set(joy_state, Joy::y_plus);
    }
  }
}
//----------------------------------------------------------------------------
void Joy::callback_joy (void)
{
  if (!status.is(joy_Active))
    return;
  if (status.is(joy_OnLine))
  {
    diag_busy++;
    return;
  }
  status.set(joy_OnLine);
  unsigned char joy_state[JOY_ARRAY_SIZE];
  memset(joy_state, 0, JOY_ARRAY_SIZE);
  read_device(joy_state, dead);
  push_state(joy_state);
  status.reset(joy_OnLine);
}
//----------------------------------------------------------------------------
// Port: split out of callback_joy.
void Joy::push_state (unsigned char *joy_state)
{
  // Port: the halves are (unsigned short), were (short). Sign extension set every event bit when
  // button 16 or 32 was down, and the event then passed for a key or a tick.
  int event;
  event = ev_Joy | evj_1;
  event |= (unsigned short)(*((short*)(&(joy_state[0]))));
  if (event!=last_1)
  {
    last_1 = event;
    Eem::push_event(event);
  }
  event = ev_Joy | evj_2;
  event |= (unsigned short)(*((short*)(&(joy_state[2]))));
  if (event!=last_2)
  {
    last_2 = event;
    Eem::push_event(event);
  }
  event = ev_Joy | evj_3;
  event |= (unsigned short)(*((short*)(&(joy_state[4]))));
  if (event!=last_3)
  {
    last_3 = event;
    Eem::push_event(event);
  }
}
//----------------------------------------------------------------------------
void  Joy::next_read (void)
{

}
//----------------------------------------------------------------------------
void  Joy::get_event (int event)
{
  Eem::St_joy *st = &(Eem::get_new()->joy);
  int pos = 0;
  if ((event&evj_2)!=0)
    pos = 2;
  if ((event&evj_3)!=0)
    pos = 4;
  unsigned short evs = (unsigned short)event;
  (*((short*)(&(st->arr_state[pos])))) = evs;
}
//----------------------------------------------------------------------------
void  Joy::clear (void)
{
  last_1 = 0;
  last_2 = 0;
  last_3 = 0;
  status.set(joy_AfterClear);
}
//----------------------------------------------------------------------------
void  Joy::complete (void)
{
  if (status.is(joy_AfterClear))
  {
    Eem::St_joy *st = &(Eem::get_new()->joy);
    memcpy(st->prev_arr_state, st->arr_state, JOY_ARRAY_SIZE);
    status.reset(joy_AfterClear);
  }
}
//----------------------------------------------------------------------------
int  Joy::is (int code)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_joy *st = &(Eem::selected()->joy);
  if (code==anybutton)
  {
    for (int i=0; i<JOY_ARRAY_SIZE; i++)
      if (st->arr_state[i]!=0)
        return(TRUE);
    return(FALSE);
  }
  else
  {
    return(Bit::is(st->arr_state, code));
  }
}
//----------------------------------------------------------------------------
int  Joy::struck (int code)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_joy *st = &(Eem::selected()->joy);
  if (code==anybutton)
  {
    for (int i=0; i < JOY_ARRAY_SIZE; i++)
      if (st->arr_state[i]!=0)
        for (int k=0; k<8; k++)
        if ((Bit::is(&(st->arr_state[i]), k))&&(!Bit::is(st->prev_arr_state,i*8+k)))
          return(TRUE);
    return(FALSE);
  }
  else
  {
    if ((Bit::is(st->arr_state, code))&&(!Bit::is(st->prev_arr_state, code)))
      return(TRUE);
    else
      return(FALSE);
  }
}
//----------------------------------------------------------------------------
int  Joy::press  (int code)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_joy *st = &(Eem::selected()->joy);
  if (code==anybutton)
  {
    for (int i=0; i < JOY_ARRAY_SIZE; i++)
      if (st->arr_state[i]!=0)
        for (int k=0; k<8; k++)
        if ((Bit::is(&(st->arr_state[i]), k))&&(Bit::is(st->prev_arr_state,i*8+k)))
          return(TRUE);
    return(FALSE);
  }
  else
  {
    if ((Bit::is(st->arr_state, code))&&(Bit::is(st->prev_arr_state, code)))
      return(TRUE);
    else
      return(FALSE);
  }
}
//----------------------------------------------------------------------------
int  Joy::release (int code)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_joy *st = &(Eem::selected()->joy);
  if (code==anybutton)
  {
    for (int i=0; i < JOY_ARRAY_SIZE; i++)
      if (st->prev_arr_state[i]!=0)
        for (int k=0; k<8; k++)
        if ((Bit::is(&(st->prev_arr_state[i]), k))&&(!Bit::is(st->arr_state,i*8+k)))
          return(TRUE);
    return(FALSE);
  }
  else
  {
    if ((!Bit::is(st->arr_state, code))&&(Bit::is(st->prev_arr_state, code)))
      return(TRUE);
    else
      return(FALSE);
  }
}
//----------------------------------------------------------------------------
int  Joy::is_initialized(void)
{
  return (status.is(joy_Initialized));
}
//----------------------------------------------------------------------------
char *Joy::get_key_name(int code)
{
  return (names[code]);
}
//----------------------------------------------------------------------------
char *Joy::get_scan_name(unsigned char scan)
{
  return (names[scan]);
}
//----------------------------------------------------------------------------
unsigned char Joy::get_scan(int code)
{
  return ((unsigned char)code);
}
//----------------------------------------------------------------------------
char *Joy::get_struck_name(void)
{
  return(names[get_struck()]);
}
//----------------------------------------------------------------------------
unsigned char Joy::get_struck(void)
{

  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_joy *st = &(Eem::selected()->joy);
  for (int i=0; i < JOY_ARRAY_SIZE; i++)
    if (st->arr_state[i]!=0)
      for (int k=0; k<8; k++)
      if ((Bit::is(&(st->arr_state[i]), k))&&(!Bit::is(st->prev_arr_state,i*8+k)))
        return((unsigned char)(i*8+k));
  return(no_struck);
}
//----------------------------------------------------------------------------

 

                                  