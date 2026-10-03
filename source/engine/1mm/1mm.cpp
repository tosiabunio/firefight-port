#include "1mm_hdrs.h"

#define DPMI 1

#define make_fcc(a,b,c,d) ((unsigned)((unsigned)a+((unsigned)b<<8)+((unsigned)c<<16)+((unsigned)d<<24)))
const unsigned mcb_id=make_fcc('h','e','a','p');
const unsigned guard_value=make_fcc(GUARD,GUARD,GUARD,GUARD);

static char other[] ="other data";
static char free_owner[] = "heap";

//---------------------------------------------------------------------------
// Heap static variables 
//---------------------------------------------------------------------------
Heap heap;
int Heap::active=0;
unsigned Heap::max=0;
unsigned Heap::current=0;
unsigned Heap::total=0;
Heap::mcb* Heap::first;
Heap::mcb* Heap::last;
void* Heap::memory;
unsigned Heap::size;
unsigned short Heap::alloc_counter=0;
//---------------------------------------------------------------------------
// Heap_object static variables
//---------------------------------------------------------------------------
char *Heap_object::new_blockname=NULL;
//---------------------------------------------------------------------------
// Fast_object static variables
//---------------------------------------------------------------------------
unsigned int Fast_object::objects_table[2]={0,0};
Fast_heap Fast_object::my_heap;
#if HI_DEBUG
  int Fast_object::initialized(0);
#endif
//===========================================================================
// Heap
//===========================================================================
void Heap::init(unsigned heap_size)
{
  DBG_CHECK(!active);
#ifndef MMU_USE_WINDOWS_HEAP
        size=heap_size;
        if ((size%65536)!=0) 
          size = (size/65536+1)*65536;
        memory = VirtualAlloc (NULL,size,MEM_RESERVE,PAGE_READWRITE);
        memory = VirtualAlloc (memory,size,MEM_COMMIT,PAGE_READWRITE);
        if (memory==NULL) 
          FAILURE2(Mmu_error::not_enough_memory, "unable to alloc memory (error code=%0X) [Heap::init]", 
                   GetLastError());
        first=(mcb*)memory;
        memset(first,GUARD,sizeof(mcb));
        first->id    = mcb_id;
        first->free  = 1;
        first->owner = free_owner;
        first->size  = size-sizeof(mcb);
        first->prev  = NULL;
        first->next  = NULL;
        last=first;
        max=0;
        current=0;
        total=first->size;
#else
  (void)heap_size;
#endif
  active=1;
  Comm::quit_me (quit, "mmu", "log");
}
//---------------------------------------------------------------------------
void Heap::quit(void)
{
  DBG_CHECK(active);
#ifndef MMU_USE_WINDOWS_HEAP
        if (!critical_error_occurred) 
        {
          MESSAGE("/-- final heap walk begin");
          int left=0;
          mcb* walker=first;
          int counter = 0;
          do 
          {
            counter++; 
            check_block(first, counter, "Heap::quit");
            if (!walker->free) 
            {
              left=1;
              MESSAGE("| - [%05d] used by %s %dkB", walker->alloc_counter, 
                      walker->owner, kilo(walker->size));
            }
            walker=walker->next;
          } while(walker!=NULL);
          if (left) 
          {
              MESSAGE("\\-- final heap walk end - allocated blocks left on heap");
              WARNING("allocated blocks at heap quit");
          }
          else 
            MESSAGE("\\-- final heap walk end - heap ok");
        }
        unsigned t=kilo(total);
        unsigned u=kilo(max);
        unsigned f=t-u;
        ENGINFO ("Heap usage - total=%dkB used=%dkB free=%dkB", t,u,f);
        VirtualFree (memory,0,MEM_RELEASE);
#endif
  active=0;
}
//---------------------------------------------------------------------------
Heap::mcb* Heap::find_best(unsigned req_size)
{
#ifndef MMU_USE_WINDOWS_HEAP
        mcb* walker=first;
        mcb* best=NULL;
        unsigned best_size=0xFFFFFFFF;
      #if HI_DEBUG
        int counter = 0;
      #endif
        do 
        {
      #if HI_DEBUG
          counter++;
          check_block(walker, counter, "Heap::alloc","Heap::find_best");
      #endif
          if((walker->free)&&(walker->size>=req_size)) 
          {
            if((walker->size-req_size)<best_size) 
            {
              best=walker;
              best_size=walker->size-req_size;
            }
          }
          walker=walker->next;
        } while(walker!=NULL);
        if(best==NULL) 
          return first;
        else 
          return best;
#else
        (void)req_size;
        FAILURE("Illegal call in Windows Heap mode [Heap::find_best]");
        return(NULL);
#endif
}
//---------------------------------------------------------------------------
void* Heap::alloc(unsigned block_size,char const* owner)
{
  DBG_CHECK(active);
#ifndef MMU_USE_WINDOWS_HEAP
        DBG_ONLY check_block (first, 1, "Heap::alloc");
        if (owner==NULL) 
          owner=other;
        block_size+=((block_size%4)!=0)?4-block_size%4:0;
        mcb* best_free=find_best(block_size);
        if ((best_free->size<block_size)||(!best_free->free))    // wywalic do find_best
          FAILURE("out of memory [Heap::alloc]");
        if ((best_free->size-block_size)<(sizeof(mcb)+4)) 
        {
          best_free->id    = mcb_id;
          best_free->owner = owner;
          best_free->free  = 0;
          best_free->usage_flag=0;
        } 
        else 
        {
          mcb* free_piece=(mcb*)((char*)best_free+sizeof(mcb)+block_size);
          *free_piece = *best_free;
          free_piece->size = best_free->size-block_size-sizeof(mcb);
  	      free_piece->prev = best_free;
		      if (best_free->next) 
		        best_free->next->prev=free_piece;
		      best_free->next  = free_piece;
          best_free->id    = mcb_id;
          best_free->owner = owner;
          best_free->size  = block_size;
          best_free->free  = 0;
          best_free->usage_flag=0;
          if (best_free==last) 
            last=free_piece;
        }
	      best_free->alloc_counter=alloc_counter;
	      alloc_counter++;
        current+=best_free->size;
        if(current>max) max=current;
        DBG_ONLY check (FILE_LINE);
        return (void*)((char*)best_free+sizeof(mcb));
#else
        void *temp = VirtualAlloc(NULL, block_size, MEM_COMMIT, PAGE_READWRITE);
        if (!temp)
          FAILURE("out of memory for %s [Heap::alloc]", owner?owner:"<unknown object>");
        return(temp);
#endif
}
//---------------------------------------------------------------------------
void Heap::free(void* ptr,char const* where)
{
  if (ptr==NULL) 
    return;
  DBG_CHECK(active);
#ifndef MMU_USE_WINDOWS_HEAP
        mcb* block=(mcb*)((char*)ptr-sizeof(mcb));
      #if HI_DEBUG
        check_block(block, 0, "Heap::free",where);
        if (block->free) 
          FAILURE("attempt to free already free block [Heap::free - %s]",where);
      #endif
        block->id=mcb_id;
        block->owner="heap";
        block->free=1;
        current-=block->size;
        compact (block);
        DBG_ONLY check(FILE_LINE);
        (void)where;
#else
      (void)where;
      VirtualFree(ptr, 0, MEM_RELEASE);
#endif
}
//---------------------------------------------------------------------------
void Heap::compact(mcb* block)
{																									
#ifndef MMU_USE_WINDOWS_HEAP
      if ((block==NULL)||(!block->free)) 
        return;
      DBG_ONLY check_block (block, 0, "Heap::compact");
      if (block->next!=NULL) 
      {
        if (block->next->free) 
        {
          block->size+=block->next->size+sizeof(mcb);
          if (block->next->next==NULL) 
          {
            block->next=NULL;
            last=block;
          } 
          else 
          {
            block->next->next->prev=block;
            block->next=block->next->next;
          }
        }
      }
      compact(block->prev);
#else
      (void)block;
#endif
}
//---------------------------------------------------------------------------
void Heap::walk(void)
{
DBG_CHECK(active);
#ifndef MMU_USE_WINDOWS_HEAP
      int used_blocks = 0;
      int free_blocks = 0;
      mcb* walker     = first;
      int counter=0;
      MESSAGE("/-- heap walk begin");
      do 
      {
        counter++;
        check_block(walker, counter, "Heap::walk");
        if(walker->free) 
        {
          MESSAGE("| + free %dkB", kilo(walker->size));
          free_blocks++;
        } 
        else 
        {
          MESSAGE("| - [%05d] used by %s %dkB", (unsigned)walker->alloc_counter,
                  walker->owner, kilo(walker->size));
          used_blocks++;
        }
        walker=walker->next;
      } while(walker!=NULL);
      MESSAGE("\\-- heap walk end");
      MESSAGE("blocks=%d used=%d free=%d",used_blocks+free_blocks, used_blocks,free_blocks);
#endif
}
//---------------------------------------------------------------------------
void Heap::check(char const* where)
{
#ifndef MMU_USE_WINDOWS_HEAP
      DBG_CHECK(active);
      try 
      {
        mcb* walker=first;
        int counter = 0;
        do 
        {
          counter++;
          check_block (walker, counter, "Heap::check", where);
          walker=walker->next;
        } while (walker!=NULL);
        walker=last;
        counter = 0;
        do 
        {
          counter--; 
          check_block(walker, counter, "Heap::check", where);
          walker=walker->prev;
        } while (walker!=NULL);
      }
      catch (Failure) 
      {
        FAILURE("heap corrupted [Heap::check - %s]", where);
      }
#else
      (void) where;
#endif
}
//---------------------------------------------------------------------------
unsigned Heap::mem_avail(void)
{
#ifndef MMU_USE_WINDOWS_HEAP
        unsigned avail=0;
        mcb* walker=first;
      #if HI_DEBUG
        int counter = 0;
      #endif
        do 
        {
      #if HI_DEBUG
          counter++;
          check_block(walker, counter, "Heap::mem_avail");
      #endif
          if (walker->free) 
            avail+=walker->size;
          walker=walker->next;
        } while (walker!=NULL);
        return avail;
#else
        return(Diag::swap_usage(Diag::su_Free));
#endif
}
//---------------------------------------------------------------------------
unsigned Heap::max_avail(void)
{
#ifndef MMU_USE_WINDOWS_HEAP
        unsigned max = 0;                               
        mcb* walker  = first;
      #if HI_DEBUG
        int counter  = 0;
      #endif
        do 
        {
      #if HI_DEBUG
          counter++;
          check_block (walker, counter, "Heap::mem_avail");
      #endif
          if ((walker->free)&&(walker->size>max)) 
            max=walker->size;
          walker=walker->next;
        } while( walker!=NULL);
        return max;
#else
        return(Diag::swap_usage(Diag::su_Free));
#endif
}
//---------------------------------------------------------------------------
void Heap::check_block(mcb* block, int block_counter, char const* who,char const* where)
{
#ifndef MMU_USE_WINDOWS_HEAP
      if (where==NULL)
        where=who;
      if ((unsigned)block<(unsigned)memory) 
        TERMINATE("[%s] - attempt to access memory below heap, block %d [%s]", who, block_counter,where);
      if ((unsigned)block>=((unsigned)memory+(unsigned)size)) 
        TERMINATE("[%s] - attempt to access memory above heap, block %d [%s]", who, block_counter,where);
      if (block->id!=mcb_id)
        TERMINATE("[%s] - attempt to access corrupted block, block %d [%s]", who, block_counter,where);
      if (block->guard_1!=guard_value)
        TERMINATE("[%s] - attempt to access block with corrupted guard %d, block %d [%s]", who, 1,block_counter,where);
      if (block->guard_2!=guard_value)
        TERMINATE("[%s] - attempt to access block with corrupted guard %d, block %d [%s]", who, 2,block_counter,where);
      if (block->next==block)
        TERMINATE("[%s] - block->next points to the same block, block %d [%s]", who, block_counter, where);
      if (block->prev==block)
        TERMINATE("[%s] - block->prev points to the same block, block %d [%s]", who, block_counter, where);
#else
      (void)block;
      (void)block_counter;
      (void)who;
      (void)where;
#endif
}
//---------------------------------------------------------------------------
void Heap::usage(char const* filter,char const* open_comment)
{
#ifndef MMU_USE_WINDOWS_HEAP
      if(open_comment==NULL)
        MESSAGE("memory usage:");
      else
        MESSAGE("%s",open_comment);
      int used_blocks = 0;
      int free_blocks = 0;
      int counter     = 0;
      mcb* walker=first;
      do 
      {
        counter++;
        check_block(walker, counter, "Heap::usage", "Heap::usage");
        if(walker->free) 
          free_blocks++;
        else 
          used_blocks++;
        if((!walker->free)&&(!walker->usage_flag)) 
        {
          unsigned sum=0;
          unsigned blocks=0;
          mcb* second=walker;
          do 
          {
            if((strcmpi(walker->owner,second->owner)==0)&&(!second->usage_flag)) 
            {
              sum+=second->size;
              blocks++;
              second->usage_flag=1;
            }
            second=second->next;
          } while(second!=NULL);
          if(sum>0) 
          {
            if (filter==NULL)
              MESSAGE("[%05d] %5dkB (%d) used by %s",
                      (unsigned)walker->alloc_counter, kilo(sum),blocks,walker->owner);
            else if(strstr(walker->owner,filter)!=NULL) 
              MESSAGE("[%05d] %5dkB (%d) used by %s",
                      (unsigned)walker->alloc_counter, kilo(sum),blocks,walker->owner);
          }
        }
        walker->usage_flag=0;
        walker=walker->next;
      } while(walker!=NULL);
      MESSAGE("blocks=%d used=%d free=%d",used_blocks+free_blocks,
                                                 used_blocks,free_blocks);
#else
      (void)filter;
      (void)open_comment;
      MESSAGE("windows heap usage - %d (%d %%)", 
        Diag::swap_usage(Diag::su_Used|Diag::su_KB), 
        Diag::swap_usage(Diag::su_Used|Diag::su_KB));
#endif
}
//===========================================================================
// Heap_object
//===========================================================================
void* Heap_object::operator new(size_t object_size)
{
  if (new_blockname==NULL)
    return (Heap::alloc(object_size));
  else
  {
    void *result=(Heap::alloc(object_size,new_blockname));
    new_blockname=NULL;
    return (result);
  }
}
//---------------------------------------------------------------------------
void Heap_object::operator delete(void *ptr)
{
  Heap::free(ptr);
}
//===========================================================================
// Fast_heap
//===========================================================================
static char unknown[]   = "unknown";
static char fast_heap[] = "[mmu] Fast_heap";

#define THIS_NAME (char*)(name?name:unknown)
#define GLOB_NAME (char*)(name?name:fast_heap)

void Fast_heap::init(size_t *size_table, size_t objects_num_, char *name_)
{
  size_t max=0;
  while (*size_table)
  {
    if (*size_table>max)
      max=*size_table;
    size_table++;
  }
  init(max, objects_num_, name_);
}
//---------------------------------------------------------------------------
void Fast_heap::init(size_t max_size_, size_t objects_num_, char *name_)
{

  try
  {
    objects_num=objects_num_;
    max_size=max_size_;
    name=name_;
    DBG_CHECK(max_size>0);
    DBG_CHECK(objects_num>0);
    objects    = (void*)Heap::alloc(objects_num*max_size,GLOB_NAME);
    free_stack = (void**)Heap::alloc(objects_num*sizeof(void*),GLOB_NAME);
    stack_ptr=0;
    unsigned int i;
#if HI_DEBUG
    allocations=0;
    diag_flags = (char*)Heap::alloc((size_t)objects_num,GLOB_NAME);
    for (i=0; i<(int)(objects_num); i++)
      diag_flags[i]=0;
#endif
    for (i=0; i<objects_num; i++)
    {
      free_stack[stack_ptr]=(void*)((size_t)objects+(i*max_size));
      stack_ptr++;
    }
    ENGINFO("Fast_heap '%s' - %d objects, object size %d",
            THIS_NAME, objects_num, max_size);
#if HI_DEBUG
    min_free=stack_ptr;
#endif
    initialized=1;
  }
  catch (Failure)
  {
    FAILURE("unable to initialize Fast_heap [Fast_heap'%s'::init]", THIS_NAME);
  }
}
//---------------------------------------------------------------------------
void *Fast_heap::alloc(void)
{
#if HI_DEBUG
  if (!stack_ptr)
    FAILURE("unable to alloc - too many objects [Fast_heap<%s>::alloc]", THIS_NAME);
  stack_ptr--;
  diag_flags[((size_t)free_stack[stack_ptr]-(size_t)objects)/max_size]=1;
  if (stack_ptr<min_free)
    min_free--;
  allocations++;
  return(free_stack[stack_ptr]);
#else
  return(free_stack[--stack_ptr]);
#endif
}
//---------------------------------------------------------------------------
void Fast_heap::free(void *ptr)
{
#if HI_DEBUG
  if (((size_t)ptr<(size_t)objects)||
      ((size_t)ptr>=((size_t)objects+max_size*objects_num)))
    FAILURE("pointer out of Fast_heap [Fast_heap'%s'::free]", THIS_NAME);
  if (((size_t)ptr-(size_t)objects)%max_size)
    FAILURE("pointer corrupted [Fast_heap'%s'::free]", THIS_NAME);
  if (diag_flags[((size_t)ptr-(size_t)objects)/max_size]==0)
    FAILURE("block not allocated [Fast_heap'%s'::free]", THIS_NAME);
  diag_flags[((size_t)ptr-(size_t)objects)/max_size]=0;
#endif
  free_stack[stack_ptr]=ptr;
  stack_ptr++;
}
//---------------------------------------------------------------------------
void Fast_heap::quit()
{
  Heap::free(objects, GLOB_NAME);
  Heap::free(free_stack, GLOB_NAME);
  if (stack_ptr!=objects_num)
    WARNING("%d allocated blocks left on Fast_heap<%s>", 
             objects_num-stack_ptr, THIS_NAME);
#if HI_DEBUG
  Heap::free(diag_flags, GLOB_NAME);
  DBG_MESSAGE("Fast_heap'%s' usage %d, allocations %d, left %d",
              THIS_NAME, objects_num-min_free, allocations, objects_num-stack_ptr);
#else
  MESSAGE("Fast_heap'%s' destructed", THIS_NAME);
#endif
  initialized=0;
}
//===========================================================================
// Fast_object
//===========================================================================
//-----------------------------------------------------------------------------
void Fast_object::init(int size)
{
#if HI_DEBUG
  if (initialized)
    FAILURE("Fast_object already initialized [Fast_object::init]");
  if (size<=0)
    FAILURE("improper allocator size %d [Fast_object::init]", size);
  if (objects_table[0]<=0)
    FAILURE("no type registered [Fast_object::init]");
  initialized=1;
#endif
  my_heap.init(objects_table, size, "Fast_object");
}
//-----------------------------------------------------------------------------
void Fast_object::quit(void)
{
#if HI_DEBUG
  if (initialized)
    my_heap.quit();
  initialized=0;
#else
  my_heap.quit();
#endif
}
//-----------------------------------------------------------------------------
void Fast_object::register_type(int type_size)
{
#if HI_DEBUG
  if (initialized)
    FAILURE("Fast_object::register_type is not allowed after init");
#endif
  if ((size_t)type_size>objects_table[0])
    objects_table[0]=type_size;
}
//-----------------------------------------------------------------------------
void *Fast_object::operator new(size_t obj_size)
{
#if HI_DEBUG
  if (!initialized)
    FAILURE("Fast_object not initialized [Fast_object::new]");
  if (obj_size>objects_table[0])
    FAILURE("unable to alloc - object is too large [Fast_object::new]");
#else
  (void)obj_size;
#endif
  void *ptr=my_heap.alloc();
  return ptr;
}
//-----------------------------------------------------------------------------
void Fast_object::operator delete(void *ptr,size_t obj_size)
{
  if (ptr==NULL)
    return;
#if HI_DEBUG
  if (!initialized)
    FAILURE("Fast_object not initialized [Fast_object::delete]");
  if (obj_size>objects_table[0])
    FAILURE("object is too large [Fast_object::delete]");
#else
  (void)obj_size;
#endif 
  my_heap.free(ptr);
}
//===========================================================================

unsigned Diag::swap_usage(int flags)
{
  unsigned ret = 0;
  MEMORYSTATUS status;
  status.dwLength=sizeof(status);
  GlobalMemoryStatus(&status);
  unsigned total = status.dwTotalPageFile;
  unsigned free  = status.dwAvailPageFile;
  unsigned used  = status.dwTotalPageFile-status.dwAvailPageFile;
  if ((flags&su_Total)!=0)
    ret = total;
  if ((flags&su_Used)!=0)
    ret = used;
  if ((flags&su_Free)!=0)
    ret = free;
  if ((flags&su_Percent)!=0)
    return ((unsigned)(ret*100)/total);
  else if ((flags&su_KB)!=0)
    return (ret/1024);
  else if ((flags&su_MB)!=0)
    return (ret/(1024*1024));
  else
    return(ret);
}

