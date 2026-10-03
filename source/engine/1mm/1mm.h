#ifndef _1MM_H_INCLUDED
#define _1MM_H_INCLUDED

#ifndef EXCLUDE_LIBS
#ifdef _DEBUG
#pragma comment(lib,"1mmd.lib")
#else
#pragma comment(lib,"1mm.lib")
#endif
#endif

#define kB *(1<<10)
#define MB *(1<<20)

#define kilo(bytes) (((unsigned)bytes%1024)==0)?((unsigned)bytes/1024):(((unsigned)bytes/1024)+1)
#define GUARD 0xCD
#define NEW(object,name) ((Heap_object::new_blockname=name), new object)


struct Mmu_error
{
  enum
  {
    not_enough_memory = 125
  };
};

class Diag
{
public:
  enum
  {
    su_Total   = 0x00000001,
    su_Used    = 0x00000002,
    su_Free    = 0x00000004,
    su_Percent = 0x00000008,
    su_KB      = 0x00000010,
    su_MB      = 0x00000020,
  };
  static unsigned swap_usage (int flags=(su_Used|su_KB));
};



//===========================================================================
// Heap
//===========================================================================

class Heap 
{
public:
  Heap() {active = 0;};
  ~Heap() {};                    
  static void     init   (unsigned heap_size);
  static void*    alloc  (unsigned block_size, char const* owner=0);
  static void     free   (void* ptr,char const* where="uknown file_line");
  static void     usage  (char const* filter,char const* open_comment=0);
  static unsigned mem_avail(void);
  static unsigned max_avail(void);
  static void  walk(void);
  static void  check(char const* where);
private:
  static void quit(void);
  // Block header. Was packed to 32 bytes for 32-bit pointers; on 64-bit it is naturally
  // aligned and padded so that every payload (header + block) stays 16-byte aligned.
  enum { mcb_align = 16 };
  struct alignas(16) mcb {
    unsigned       guard_1;     //4      
    unsigned       id;          //4
    char const*    owner;       //4
    unsigned       size;        //4
    mcb*           prev;        //4
    mcb*           next;        //4
    char           free;        //1
    char           usage_flag;  //1
    unsigned short alloc_counter; //2
    unsigned       guard_2;     //4=32
  };
  static unsigned short alloc_counter;
  static int      active;
  static void*    memory;
  static unsigned size;
  static mcb*     first;
  static mcb*     last;
  static unsigned max;
  static unsigned current;
  static unsigned total;
  static mcb*  find_best(unsigned req_size);
  static void  compact(mcb* block);
  static void  check_block (mcb* block, int block_number, 
                            char const* who,char const* where=NULL);

};

extern Heap heap;

//===========================================================================
// Fast_heap
//===========================================================================

class Fast_heap
{
  int initialized;
  void *objects;
  void **free_stack;
  char *name;
  size_t objects_num;
  size_t max_size;
  size_t stack_ptr;
#if HI_DEBUG
  size_t min_free;
  size_t allocations;
  char *diag_flags;
#endif
public:
  Fast_heap()
    : initialized(0) {};
  Fast_heap(size_t *size_table, size_t objects_num_, char *name_=NULL)
    : initialized(0) { init(size_table, objects_num_, name_); }
  Fast_heap(size_t max_size_, size_t objects_num_, char *name_=NULL)
    : initialized(0) { init(max_size_, objects_num_, name_); }
  void init(size_t *size_table, size_t objects_num_, char *name_=NULL);
  void init(size_t max_size_, size_t objects_num_, char *name_=NULL);
  void *alloc(void);
  void free(void *ptr);
  void quit();
  ~Fast_heap()
    { if (initialized) quit(); }
};
//===========================================================================
// Heap_object
//===========================================================================

class Heap_object
{
public:
  static char *new_blockname;
  void* operator new(size_t objectsize_);
  void operator delete(void *ptr);
};

//===========================================================================
// Fast_object       : obiekt alokujacy sie przez new w fast alokatorze
//============================================================================
class Fast_object
{
  private:
#if HI_DEBUG
   static int initialized;
#endif
   static Fast_heap my_heap;
   static size_t objects_table[2];
  public:
   static void init(int size);
   static void register_type(int type_size);
   static void quit(void);
   Fast_object(void) {}
   virtual ~Fast_object(void) {}
   void *operator new(size_t obj_size);
   void operator delete(void *ptr,size_t obj_size);
};
//============================================================================

#endif
