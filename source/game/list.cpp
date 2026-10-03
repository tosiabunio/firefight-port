#include "headers.h"

#if HI_DEBUG

static int global_max_usage(0);
static char* max_usage_user_name=NULL;

void List_statistics(void)
{
  if (max_usage_user_name)
  {
    DBG_MESSAGE("greatest number of stored objects: %d",global_max_usage);
    DBG_MESSAGE("owner: \"%s\"",max_usage_user_name);
  }
}

void update_list_statistics(int usage,char* user_name)
{
  if (usage>global_max_usage)
  {
    global_max_usage=usage;
    max_usage_user_name=user_name;
  }
}

#endif

