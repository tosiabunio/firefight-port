#include "1ee_hdrs.h"
#include <algorithm>

//----------------------------------------------------------------------------
// Status flags
//----------------------------------------------------------------------------
const int timer_Initialized  = 0x00000010; // Timer zainicjalizowany
//----------------------------------------------------------------------------
// Timer static variables
//----------------------------------------------------------------------------
Bitflag            Timer::status(0);
int                Timer::accuracy(0);
Timer::Timer_item  Timer::timers[TIMER_ARRAY_SIZE];
//===========================================================================
// Timer
//===========================================================================
void Timer::init(int desired_accuracy, int critical_accuracy)
{
  try
  {
    DBG_CHECK(!status.is(timer_Initialized));
    memset(timers, 0, sizeof(timers));
    TIMECAPS tc;
    if(timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR)
      FAILURE2 (Eem_error::general, "cannot initialize timer");
    accuracy = std::min(std::max(tc.wPeriodMin, (UINT)desired_accuracy), tc.wPeriodMax);
    if (accuracy > desired_accuracy)
      WARNING("system timer slow. Accuracy: %d", accuracy);
    if (accuracy > critical_accuracy)
      FAILURE2(Eem_error::general, "system timer to slow");
    timeBeginPeriod(accuracy);
    ENGINFO ("timer status: accuracy desired = %d, critical = %d, current %d", 
             desired_accuracy, critical_accuracy, accuracy);
    status.set(timer_Initialized);
    Comm::quit_me(quit, "*timer", "log");
  }
  catch (Failure)
  {
    FAILURE ("timer initialization error");
  }
}
//---------------------------------------------------------------------------
void Timer::quit(void)
{
  for (int i=0; i<TIMER_ARRAY_SIZE; i++)
    if (timers[i].id!=0)
      timeKillEvent(timers[i].id);
  timeEndPeriod (accuracy);
  status=0;
  MESSAGE("timer destructed");
}
//---------------------------------------------------------------------------
int Timer::add(int interval, Timer_function function, Timer_object *object, 
               unsigned int *counter, char *name)
{
  if (interval<accuracy)
    FAILURE("unable to set timer event '%s' (%d ms interval). Accuracy to low (%d) [Timer::add]",
            (char*)name?name:"unknown", interval, accuracy);
  if (name!=NULL)
    for (int i=0; i<TIMER_ARRAY_SIZE; i++)
      if ((timers[i].name!=NULL)&&(strcmpi(name, timers[i].name)==0))
      FAILURE("timer event <%s> already exist [Timer::add]", name);
  int found=-1;
  for (int i=0; i<TIMER_ARRAY_SIZE; i++)
    if (timers[i].id==0)
    {
      found = i;
      break;
    }
  if (found==-1)
    FAILURE("sorry no room for timer event '%s'  [Timer::add]", (char*)name?name:"unknown");
  int id = timeSetEvent(interval, accuracy, callback_timer, (DWORD_PTR)found, TIME_PERIODIC);
  if (id==0)
    FAILURE ("cannot set timer event '%s' [Timer::add]", (char*)name?name:"unknown"); 
  timers[found].id = id;
  timers[found].name = name;
  timers[found].function = function;
  timers[found].object = object;
  timers[found].counter = counter;
  MESSAGE("timer event '%s' initialized. interval = %d ms", (char*)name?name:"unknown", interval);
  return (id);
}
//---------------------------------------------------------------------------
int  Timer::add(int interval, Timer_function function, char *name)
{
  return (add(interval, function, NULL, NULL, name));
}
//---------------------------------------------------------------------------
int  Timer::add(int interval, Timer_object *object, char *name)
{
  return (add(interval, NULL, object, NULL, name));
}
//---------------------------------------------------------------------------
int  Timer::add(int interval, unsigned int *counter, char *name)
{
  return (add(interval, NULL, NULL, counter, name));
}
//---------------------------------------------------------------------------
void Timer::kill(int timer_id)
{
  if (!timer_id)
    FAILURE("incorrect timer id [Timer::kill]"); 
  int found=-1;
  for (int i=0; i<TIMER_ARRAY_SIZE; i++)
    if (timers[i].id==timer_id)
    {
      found = i;
      break;
    }
  if (found==-1)
    FAILURE("timer not present [Timer::kill]"); 
  timeKillEvent(timers[found].id);
  MESSAGE("timer event '%s' destructed", (char*)(timers[found].name?timers[found].name:"unknown"));
  memset(&timers[found],0,sizeof(timers[found]));
}
//---------------------------------------------------------------------------
void Timer::kill(char *name)
{
  DBG_CHECK(name!=NULL);
  int found=-1;
  for (int i=0; i<TIMER_ARRAY_SIZE; i++)
    if ((timers[i].name!=NULL)&&(strcmpi(name, timers[i].name)==0))
    {
      found = i;
      break;
    }
  if (found==-1)
    FAILURE("timer '%s' not present [Timer::kill]", name); 
  timeKillEvent(timers[found].id);
  memset(&timers[found],0,sizeof(timers[found]));
  MESSAGE("timer event '%s' destructed", (char*)timers[found].name?timers[found].name:"unknown");
}
//---------------------------------------------------------------------------
void CALLBACK Timer::callback_timer (UINT  IDEvent, UINT  uReserved, DWORD_PTR dwUser,	
                              DWORD_PTR dwReserved1,	DWORD_PTR dwReserved2)
{
  if (timers[dwUser].counter!=NULL)
    timers[dwUser].counter++;
  if (timers[dwUser].function!=NULL)
    timers[dwUser].function();
  if (timers[dwUser].object!=NULL)
    timers[dwUser].object->tick();
  (void)uReserved;
  (void)dwReserved1;
  (void)dwReserved2;
  (void)IDEvent;
}
//---------------------------------------------------------------------------
int Timer::is_initialized(void)
{
  return (status.is(timer_Initialized));
}
//===========================================================================
