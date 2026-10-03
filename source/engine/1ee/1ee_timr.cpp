#include "1ee_hdrs.h"
#include <SDL.h>

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
// Port: rewritten for the main thread. The original ran a multimedia timer (timeSetEvent)
// whose thread called the timer functions; now Comm::process_messages calls service(), which
// fires every timer that is due according to SDL_GetPerformanceCounter. That also removes the
// unsynchronised access from the timer thread to the event queue.
int Timer::last_id(0);

// A timer that falls further behind than this (a breakpoint, a suspended machine) skips the
// rest instead of firing a burst; the Eem queue would drop those ticks anyway.
static const unsigned max_catch_up_ms = 1000;

void Timer::init(int desired_accuracy, int critical_accuracy)
{
  try
  {
    DBG_CHECK(!status.is(timer_Initialized));
    memset(timers, 0, sizeof(timers));
    accuracy = 1;
    ENGINFO ("timer status: accuracy desired = %d, critical = %d, current %d (main thread)", 
             desired_accuracy, critical_accuracy, accuracy);
    status.set(timer_Initialized);
    Comm::tick_service = service;
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
  Comm::tick_service = NULL;
  memset(timers, 0, sizeof(timers));
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
  unsigned long long frequency = SDL_GetPerformanceFrequency();
  timers[found].id = ++last_id;
  timers[found].name = name;
  timers[found].function = function;
  timers[found].object = object;
  timers[found].counter = counter;
  timers[found].period = frequency*(unsigned)interval/1000;
  timers[found].next_due = SDL_GetPerformanceCounter()+timers[found].period;
  MESSAGE("timer event '%s' initialized. interval = %d ms", (char*)name?name:"unknown", interval);
  return (timers[found].id);
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
  MESSAGE("timer event '%s' destructed", name);
  memset(&timers[found],0,sizeof(timers[found]));
}
//---------------------------------------------------------------------------
void Timer::fire(Timer_item &timer)
{
  if (timer.counter!=NULL)
    (*timer.counter)++;  // the original incremented the pointer; no caller uses counters
  if (timer.function!=NULL)
    timer.function();
  if (timer.object!=NULL)
    timer.object->tick();
}
//---------------------------------------------------------------------------
void Timer::service(void)
{
  if (!status.is(timer_Initialized))
    return;
  unsigned long long now = SDL_GetPerformanceCounter();
  unsigned long long max_lag = SDL_GetPerformanceFrequency()*max_catch_up_ms/1000;
  for (int i=0; i<TIMER_ARRAY_SIZE; i++)
  {
    Timer_item &timer = timers[i];
    if ((timer.id==0)||(now<timer.next_due))
      continue;
    if (now-timer.next_due>max_lag)
      timer.next_due = now-max_lag;
    int id = timer.id;
    while ((timer.id==id)&&(now>=timer.next_due))
    {
      timer.next_due += timer.period;
      fire(timer);
    }
  }
}
//---------------------------------------------------------------------------
unsigned Timer::ms_to_next(void)
{
  unsigned long long now = SDL_GetPerformanceCounter();
  unsigned long long frequency = SDL_GetPerformanceFrequency();
  unsigned result = 100;
  for (int i=0; i<TIMER_ARRAY_SIZE; i++)
    if (timers[i].id!=0)
    {
      if (timers[i].next_due<=now)
        return(0);
      unsigned long long ms = ((timers[i].next_due-now)*1000+frequency-1)/frequency;
      if (ms<result)
        result = (unsigned)ms;
    }
  return(result);
}
//---------------------------------------------------------------------------
int Timer::is_initialized(void)
{
  return (status.is(timer_Initialized));
}
