#include "headers.h"

#if HI_DEBUG

static int global_max_usage(0);
static char* max_usage_user_name=NULL;

void NoQueue_statistics(void)
{
  if (max_usage_user_name)
  {
    if (global_max_usage>50)
    {
      DBG_WARNING("[que] usage exceeded 50%");
    }
//    MESSAGE("maximum occupancy: %d%%",global_max_usage);
//    MESSAGE("owner: \"%s\"",max_usage_user_name);
  }
}

void update_NoQueue_statistics(int usage,char* user_name)
{
  if (usage>global_max_usage)
  {
    global_max_usage=usage;
    max_usage_user_name=user_name;
  }
}

#endif