#include "1ee_hdrs.h"

const int joy_Initialized    = 0x00000010; // Joy zainicjalizowany
const int joy_Active         = 0x00000001; // Joy zainicjalizowany
const int joy_OnLine         = 0x00000002; // Joy zainicjalizowany
const int joy_AfterClear     = 0x00000004; // Joy zainicjalizowany

const int evj_1              = 0x00010000; // 1/3 czesc danych joysticka
const int evj_2              = 0x00020000; // 2/3 czesc danych joysticka
const int evj_3              = 0x00040000; // 3/3 czesc danych joysticka
//----------------------------------------------------------------------------
// Joy static variables
//----------------------------------------------------------------------------
Bitflag        Joy::status(0);
JOYCAPS        Joy::caps; 
unsigned       Joy::dev_number(0xFFFFFFFF); 
DWORD          Joy::read_flags = JOY_RETURNBUTTONS | JOY_RETURNCENTERED | JOY_USEDEADZONE;
DWORD          Joy::x_center;
DWORD          Joy::y_center;
DWORD          Joy::z_center;
DWORD          Joy::r_center;
DWORD          Joy::u_center;
DWORD          Joy::v_center;

DWORD          Joy::x_dead(0);
DWORD          Joy::y_dead(0);
DWORD          Joy::z_dead(0);
DWORD          Joy::r_dead(0);
DWORD          Joy::u_dead(0);
DWORD          Joy::v_dead(0);
int            Joy::last_1(0);
int            Joy::last_2(0);
int            Joy::last_3(0);
unsigned       Joy::diag_busy(0);
char          *Joy::names[JOY_NAMES_SIZE];
//===========================================================================
// Joy
//===========================================================================
void Joy::set_dead_zone (int dead_percent)
{
  if (caps.wNumAxes > 0)
  {
    read_flags |= JOY_RETURNX; 
    x_center = (caps.wXmax + caps.wXmin + 1)/2;
    x_dead = ((caps.wXmax - caps.wXmin) * dead_percent)/100;
  }
  if (caps.wNumAxes > 1)
  {
    read_flags |= JOY_RETURNY; 
    y_center = (caps.wYmax + caps.wYmin + 1)/2;
    y_dead = ((caps.wYmax - caps.wYmin) * dead_percent)/100;
  }
  if (caps.wNumAxes > 2)
  {
    read_flags |= JOY_RETURNZ; 
    z_center = (caps.wZmax + caps.wZmin + 1)/2;
    z_dead = ((caps.wZmax - caps.wZmin) * dead_percent)/100;
  }
  if (caps.wNumAxes > 3)
  {
    read_flags |= JOY_RETURNR; 
    r_center = (caps.wRmax + caps.wRmin + 1)/2;
    r_dead = ((caps.wRmax - caps.wRmin) * dead_percent)/100;
  }
  if (caps.wNumAxes > 4)
  {
    read_flags |= JOY_RETURNU; 
    u_center = (caps.wUmax + caps.wUmin + 1)/2;
    u_dead = ((caps.wUmax - caps.wUmin) * dead_percent)/100;
  }
  if (caps.wNumAxes > 5)
  {
    read_flags |= JOY_RETURNV; 
    v_center = (caps.wVmax + caps.wVmin + 1)/2;
    v_dead = ((caps.wVmax - caps.wVmin) * dead_percent)/100;
  }
}
//---------------------------------------------------------------------------
void Joy::init (int dead_percent)
{
  status.set(joy_Initialized);
  init_ref();
  UINT joy_num = joyGetNumDevs();
  JOYCAPS jc;
  JOYINFO ji;
  MMRESULT res_jc, res_ji;
  BOOL found = FALSE;
  SYSINFO("joystick driver capabilities");
  SYSINFO("  %d joysticks supported", joy_num);
  for (unsigned i=0; i<joy_num; i++)
  {
    res_ji = joyGetPos(i, &ji);
    res_jc = joyGetDevCaps(i, &jc, sizeof(JOYCAPS));
    if ((res_ji==JOYERR_NOERROR)&&(res_ji==JOYERR_NOERROR))
    {
      SYSINFO("  joystick #%d found",i);
      SYSINFO("    product name :'%s'", jc.szPname);
      SYSINFO("    registry key : '%s'", jc.szRegKey);
      SYSINFO("    driver OEM : '%s'", jc.szOEMVxD);
      SYSINFO("    axes max/now : %d/%d", jc.wMaxAxes, jc.wNumAxes);
      if (jc.wNumAxes > 0)
        SYSINFO("    %c min/max : %d/%d", 'X', jc.wXmin, jc.wXmax);
      if (jc.wNumAxes > 1)
        SYSINFO("    %c min/max : %d/%d", 'Y', jc.wYmin, jc.wYmax);
      if (jc.wNumAxes > 2)
        SYSINFO("    %c min/max : %d/%d", 'Z', jc.wZmin, jc.wZmax);
      if (jc.wNumAxes > 3)
        SYSINFO("    %c min/max : %d/%d", 'R', jc.wRmin, jc.wRmax);
      if (jc.wNumAxes > 4)
        SYSINFO("    %c min/max : %d/%d", 'U', jc.wUmin, jc.wUmax);
      if (jc.wNumAxes > 5)
        SYSINFO("    %c min/max : %d/%d", 'V', jc.wVmin, jc.wVmax);
      SYSINFO("    buttons num : %d", jc.wNumButtons);
      SYSINFO("    polling min/max : %d/%d ms", jc.wPeriodMin, jc.wPeriodMax);
      if (dev_number==0xFFFFFFFF)  // Will use first available joystick
      {
        dev_number = i;
        caps = jc;
        status.set(joy_Active);
        set_dead_zone(dead_percent);
      }
      found = TRUE;
    }
  }
  if (!found)
    SYSINFO("  no joysticks found");
  else
    MESSAGE("joystick connected");
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
  status.reset(joy_Active);
  status.reset(joy_Initialized);
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
  JOYINFOEX ji;
  memset(&ji, 0, sizeof(ji));
  ji.dwSize = sizeof(ji);
  ji.dwFlags = read_flags;
  MMRESULT res = joyGetPosEx(JOYSTICKID1, &ji);
  if (res!=MMSYSERR_NOERROR)
  {
    MESSAGE("Joystick disconected");
    status.reset(joy_Active);
  }
  else
  {
    memcpy(joy_state, &ji.dwButtons, 4);
    if (caps.wNumAxes>0)
    {
      if (ji.dwXpos < x_center - x_dead)
        Bit::set(joy_state, x_minus);
      else if (ji.dwXpos > x_center + x_dead)
        Bit::set(joy_state, x_plus);
      if (caps.wNumAxes>1)
      {
        if (ji.dwYpos < y_center - y_dead)
          Bit::set(joy_state, y_minus);
        else if (ji.dwYpos > y_center + y_dead)
          Bit::set(joy_state, y_plus);
        if (caps.wNumAxes>2)
        {
          if (ji.dwZpos < z_center - z_dead)
            Bit::set(joy_state, z_minus);
          else if (ji.dwZpos > z_center + z_dead)
            Bit::set(joy_state, z_plus);
          if (caps.wNumAxes>3)
          {
            if (ji.dwRpos < r_center - r_dead)
              Bit::set(joy_state, r_minus);
            else if (ji.dwRpos > r_center + r_dead)
              Bit::set(joy_state, r_plus);
            if (caps.wNumAxes>4)
            {
              if (ji.dwUpos < u_center - u_dead)
                Bit::set(joy_state, u_minus);
              else if (ji.dwUpos > u_center + u_dead)
                Bit::set(joy_state, u_plus);
              if (caps.wNumAxes>5)
              {
                if (ji.dwVpos < v_center - v_dead)
                  Bit::set(joy_state, v_minus);
                else if (ji.dwVpos > v_center + v_dead)
                  Bit::set(joy_state, v_plus);
              }
            }
          }
        }
      }
    }
    int event;
    event = ev_Joy | evj_1;
    event |= (*((short*)(&(joy_state[0]))));
    if (event!=last_1)
    {
      last_1 = event;
      Eem::push_event(event);
    }
    event = ev_Joy | evj_2;
    event |= (*((short*)(&(joy_state[2]))));
    if (event!=last_2)
    {
      last_2 = event;
      Eem::push_event(event);
    }
    event = ev_Joy | evj_3;
    event |= (*((short*)(&(joy_state[4]))));
    if (event!=last_3)
    {
      last_3 = event;
      Eem::push_event(event);
    }
  }
  status.reset(joy_OnLine);
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

 

                                  