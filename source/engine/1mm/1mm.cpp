#include "1mm_hdrs.h"

#define DPMI 1

#define make_fcc(a,b,c,d) ((unsigned)((unsigned)a+((unsigned)b<<8)+((unsigned)c<<16)+((unsigned)d<<24)))
const unsigned mcb_id=make_fcc('h','e','a','p');
const unsigned guard_value=make_fcc(GUARD,GUARD,GUARD,GUARD);

static char other[] ="other data";

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
size_t Fast_object::objects_table[2]={0,0};
Fast_heap Fast_object::my_heap;
#if HI_DEBUG
  int Fast_object::initialized(0);
#endif
//===========================================================================
// Heap
//===========================================================================
// Port: every block is its own zero-filled malloc allocation behind the original block header
// (owner name, guards, allocation counter); the headers form a list of live blocks for the
// walks, checks and usage reports. The original carved blocks out of one VirtualAlloc'ed arena
// of cwe.ini's memory_required bytes; that size is kept as the budget mem_avail reports against.
// Zero-filling keeps reads of uninitialised memory the same on every platform; the original
// arena started zeroed but handed out reused blocks with their old contents.
static_assert(sizeof(Heap::mcb)%16==0, "block payloads must stay 16-byte aligned");

void Heap::init(unsigned heap_size)
{
  DBG_CHECK(!active);
  size=heap_size;
  memory=NULL;
  first=last=NULL;
  max=0;
  current=0;
  total=size;
  active=1;
  Comm::quit_me (quit, "mmu", "log");
}
//---------------------------------------------------------------------------
void Heap::quit(void)
{
  DBG_CHECK(active);
  if (!critical_error_occurred) 
  {
    MESSAGE("/-- final heap walk begin");
    int left=0;
    int counter = 0;
    for (mcb* walker=first; walker!=NULL; walker=walker->next)
    {
      counter++; 
      check_block(walker, counter, "Heap::quit");
      left=1;
      MESSAGE("| - [%05d] used by %s %dkB", walker->alloc_counter, 
              walker->owner, kilo(walker->size));
    }
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
  active=0;
}
//---------------------------------------------------------------------------
void* Heap::alloc(unsigned block_size,char const* owner)
{
  DBG_CHECK(active);
  if (owner==NULL) 
    owner=other;
  block_size+=((block_size%mcb_align)!=0)?mcb_align-block_size%mcb_align:0;
  mcb* block=(mcb*)calloc(1,sizeof(mcb)+block_size);
  if (block==NULL)
    FAILURE("out of memory for %s [Heap::alloc]", (char*)owner);
  memset(block,GUARD,sizeof(mcb));
  block->id    = mcb_id;
  block->owner = owner;
  block->size  = block_size;
  block->free  = 0;
  block->usage_flag=0;
  block->alloc_counter=alloc_counter;
  alloc_counter++;
  block->prev  = last;
  block->next  = NULL;
  if (last)
    last->next=block;
  else
    first=block;
  last=block;
  current+=block_size;
  if(current>max) max=current;
  return (void*)((char*)block+sizeof(mcb));
}
//---------------------------------------------------------------------------
void Heap::free(void* ptr,char const* where)
{
  if (ptr==NULL) 
    return;
  DBG_CHECK(active);
  mcb* block=(mcb*)((char*)ptr-sizeof(mcb));
#if HI_DEBUG
  check_block(block, 0, "Heap::free",where);
#endif
  if (block->prev) block->prev->next=block->next; else first=block->next;
  if (block->next) block->next->prev=block->prev; else last=block->prev;
  current-=block->size;
  block->id=0;  // a second free of the same block fails check_block
  ::free(block);
  (void)where;
}
//---------------------------------------------------------------------------
void Heap::walk(void)
{
  DBG_CHECK(active);
  int used_blocks = 0;
  int counter=0;
  MESSAGE("/-- heap walk begin");
  for (mcb* walker=first; walker!=NULL; walker=walker->next)
  {
    counter++;
    check_block(walker, counter, "Heap::walk");
    MESSAGE("| - [%05d] used by %s %dkB", (unsigned)walker->alloc_counter,
            walker->owner, kilo(walker->size));
    used_blocks++;
  }
  MESSAGE("\\-- heap walk end");
  MESSAGE("blocks=%d used=%d free=%d",used_blocks, used_blocks,0);
}
//---------------------------------------------------------------------------
void Heap::check(char const* where)
{
  DBG_CHECK(active);
  try 
  {
    int counter = 0;
    for (mcb* walker=first; walker!=NULL; walker=walker->next)
      check_block (walker, ++counter, "Heap::check", where);
    counter = 0;
    for (mcb* walker=last; walker!=NULL; walker=walker->prev)
      check_block(walker, --counter, "Heap::check", where);
  }
  catch (Failure) 
  {
    FAILURE("heap corrupted [Heap::check - %s]", where);
  }
}
//---------------------------------------------------------------------------
unsigned Heap::mem_avail(void)
{
  return (current<size) ? size-current : 0;
}
//---------------------------------------------------------------------------
unsigned Heap::max_avail(void)
{
  return mem_avail();
}
//---------------------------------------------------------------------------
void Heap::check_block(mcb* block, int block_counter, char const* who,char const* where)
{
  if (where==NULL)
    where=who;
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
}
//---------------------------------------------------------------------------
void Heap::usage(char const* filter,char const* open_comment)
{
  if(open_comment==NULL)
    MESSAGE("memory usage:");
  else
    MESSAGE("%s",open_comment);
  int used_blocks = 0;
  int counter     = 0;
  for (mcb* walker=first; walker!=NULL; walker=walker->next)
  {
    counter++;
    check_block(walker, counter, "Heap::usage", "Heap::usage");
    used_blocks++;
    if(!walker->usage_flag) 
    {
      unsigned sum=0;
      unsigned blocks=0;
      for (mcb* second=walker; second!=NULL; second=second->next)
        if((strcmpi((char*)walker->owner,(char*)second->owner)==0)&&(!second->usage_flag)) 
        {
          sum+=second->size;
          blocks++;
          second->usage_flag=1;
        }
      if((sum>0)&&((filter==NULL)||(strstr(walker->owner,filter)!=NULL)))
        MESSAGE("[%05d] %5dkB (%d) used by %s",
                (unsigned)walker->alloc_counter, kilo(sum),blocks,walker->owner);
    }
  }
  for (mcb* walker=first; walker!=NULL; walker=walker->next)
    walker->usage_flag=0;
  MESSAGE("blocks=%d used=%d free=%d",used_blocks,used_blocks,0);
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

// Port: reports the engine heap against its budget (was the Windows page file).
unsigned Diag::swap_usage(int flags)
{
  unsigned ret = 0;
  unsigned total = Heap::mem_avail()+Heap::used();
  unsigned free  = Heap::mem_avail();
  unsigned used  = Heap::used();
  if ((flags&su_Total)!=0)
    ret = total;
  if ((flags&su_Used)!=0)
    ret = used;
  if ((flags&su_Free)!=0)
    ret = free;
  if ((flags&su_Percent)!=0)
    return (total ? (unsigned)(((unsigned long long)ret*100)/total) : 0);
  else if ((flags&su_KB)!=0)
    return (ret/1024);
  else if ((flags&su_MB)!=0)
    return (ret/(1024*1024));
  else
    return(ret);
}
