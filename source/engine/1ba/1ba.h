#ifndef _1BA_H_INCLUDED
#define _1BA_H_INCLUDED

#ifndef EXCLUDE_LIBS
#  ifdef _DEBUG
#    pragma comment(lib,"1bad.lib")
#  else
#    pragma comment(lib,"1ba.lib")
#  endif
#endif

// Do templatu HashArray
#define DEF_PRC_GROW 100   // ile procent nadmiaru ma być w tablicy HashTable
                           // (wielkosc defaultowa - do zmiany przy inicjacji)
                           // Trzeba pamietac ze niektore funkcje hashujace
                           // wymagaja jakiegos minimum zeby sie nie wysypać.
#define DIAG_SIZE    5     // rozmiar tablicy diagnostycznej dla HashTable.
                           // istotne tylko w trybie DEBUG

char *bas_str (int num);

//===========================================================================
// BitFlag
//===========================================================================

class Bitflag
{
public:
  int data;
  Bitflag() : data(0)               {};
  Bitflag(int flag) : data(flag)    {};
  inline int  is(int flags)         {return((data&flags)==flags);};
  inline int  in(int flags)         {return(data&flags);};
  inline void clear(void)           {data=0;};
  inline void set(int flags)        {data|=flags;};
  inline void reset(int flags)      {data&=~flags;};
  inline void toggle(int flags)     {data^=flags;};
  inline void operator=(int flags)  {data=flags;};
};

//===========================================================================
// Bit
//===========================================================================

class Bit
{
  static unsigned char mask[8];
public:
  static inline void set (unsigned char *adr, int bnum) {
    adr[bnum>>3]|=mask[bnum&0x7]; }
  static inline void reset (unsigned char *adr, int bnum) {
    adr[bnum>>3]&=mask[bnum&0x7]^0xff; }
  static inline void toggle (unsigned char *adr, int bnum) {
    adr[bnum>>3]^=mask[bnum&0x7]; }
  static inline int is (unsigned char *adr, int bnum) {
    return(adr[bnum>>3]&mask[bnum&0x7]); }
};

//===========================================================================
// Pointer
//===========================================================================

template<class T>
class Pointer: public Heap_object
{
  T *p;
public:
  Pointer ();
  Pointer (int s, char *name=NULL);
  ~Pointer ();

  void free (char *owner=NULL);
  void alloc (int s, char *name=NULL);
  operator T*();
};

//---------------------------------------------------------------------------
template<class T>
Pointer<T>::Pointer ()
: p(NULL)
{
}
//...........................................................................
template<class T>
Pointer<T>::Pointer (int s, char *name)
: p(NULL)
{
  alloc (s, name);
}
//...........................................................................
template<class T>
Pointer<T>::~Pointer ()
{
  if (p) free ("[bas] Pointer<T> automatic free()");
}
//...........................................................................
template<class T>
void Pointer<T>::free (char *owner)
{
  if (p)
  {
    Heap::free(p, owner);
    p = NULL;
  }
}
//...........................................................................
template<class T>
void Pointer<T>::alloc (int s, char *name)
{
  if (p)
    free ();
  p = (T*)Heap::alloc(sizeof(T)*s, name ?name :"[bas] Pointer<T>");
}
//...........................................................................
template<class T>
inline Pointer<T>::operator T*()
{
  return p;
}
//===========================================================================
// Array
//===========================================================================

template<class T>
class Array: public Heap_object
{
  T *p;
  int size;
  int first;
  char *name;
public:
  Array ();
  Array (int size_, char *name_=NULL, int first_=0);
  Array (int size_, int first_);
  ~Array ();
  void free ();
  void alloc (int size_, char *name_=NULL);
  void remap (int first_);
  void fillbyte (char fill_byte);
  void fill (T &fill_value);
  inline int getsize (void);
  inline int getfirst (void);
  inline char *getname (void);
  inline T &operator[] (int pos);
  inline operator T*();
};

//---------------------------------------------------------------------------
template<class T>
Array<T>::Array ()
: size(0), p(NULL), first(0), name(NULL)
{
}
//...........................................................................
template<class T>
Array<T>::Array (int size_, char *name_, int first_)
: p(NULL), first(first_), name(NULL)
{
  alloc(size_, name_);
}
//...........................................................................
template<class T>
Array<T>::Array (int size_, int first_)
: p(NULL), first(first_), name(NULL)
{
  alloc(size_);
}
//...........................................................................
template<class T>
Array<T>::~Array()
{
  if (p) free ();
}
//...........................................................................
template<class T>
void Array<T>::free()
{
  if (p)
  {
    Heap::free (p, name ?name :"[bas] Array<T>");
    size=0;
    p=NULL;
  }
}
//...........................................................................
template<class T>
void Array<T>::alloc(int size_, char *name_)
{
  try
  {
    if (p) free ();
    p=(T*)Heap::alloc(sizeof(T)*size_, name_ ?name_ :"[bas] Array<T>");
    name=name_;
    size=size_;
  }
  catch (Failure)
  {
    FAILURE (bas_str(0), size_);
  }
}
//...........................................................................
template<class T>
inline void Array<T>::remap (int first_)
{
  first=first_;
}
//...........................................................................
template<class T>
inline T &Array<T>::operator[] (int pos)
{
  DBG_CHECK(pos>=first);
  DBG_CHECK((pos-first)<size);
  return p[(pos-first)];
}
//...........................................................................
template<class T>
inline Array<T>::operator T*()
{
  return p;
}
//...........................................................................
template<class T>
inline int Array<T>::getsize (void)
{
  return(size);
}
//...........................................................................
template<class T>
inline int Array<T>::getfirst (void)
{
  return(first);
}
//...........................................................................
template<class T>
inline char *Array<T>::getname (void)
{
  return(name);
}
//...........................................................................
template<class T>
void Array<T>::fillbyte (char fill_byte)
{
  if (p)
    memset(p,fill_byte,sizeof(T)*size);
}
//...........................................................................
template<class T>
void Array<T>::fill (T &fill_value)
{
  if (p)
    for (int i=0; i<size; i++)
      p[i]=fill_value;
}
//===========================================================================
// Queue
//===========================================================================

template<class T>
class Queue: public Heap_object
{
  int size;
  int quantity;
  int head, tail;
  char *name;
  T *p;
public:
  Queue ();
  Queue (int size_, char *name_=NULL);
  ~Queue ();
  void free ();
  void alloc (int size_, char *name_=NULL);
  void purge (void);
  inline int getsize (void);
  inline int getfree (void);
  inline int getquantity (void);
  inline char *getname (void);
  inline void push (T value);
  inline void unpop (T value);
  inline T *pushempty (void);
  inline void popempty (void);
  inline T pop(void);
  inline T *peek(void);
  inline T &operator[] (int pos); 
};

//---------------------------------------------------------------------------
template<class T>
Queue<T>::Queue ()
: size(0), p(NULL), head(0), tail(0), quantity(0), name(NULL)
{
}
//...........................................................................
template<class T>
Queue<T>::Queue (int size_, char *name_)
: p(NULL), name(NULL)
{
  alloc(size_, name_);
}
//...........................................................................
template<class T>
Queue<T>::~Queue()
{
  if (p) free ();
}
//...........................................................................
template<class T>
void Queue<T>::free()
{
  if (p)
  {
    Heap::free(p, name?name:"[bas] Queue<T>");
    size=0;
    head=0;
    tail=0;
    quantity=0;
    p=NULL;
  }
}
//...........................................................................
template<class T>
void Queue<T>::alloc(int size_, char *name_)
{
  if (p)
    free ();
  try
  {
    p=(T*)Heap::alloc(sizeof(T)*size_, name_?name_:"[bas] Queue<T>");
    name=name_;
    size=size_;
    head=0;
    tail=0;
    quantity = 0;
  }
  catch (Failure)
  {
    FAILURE (bas_str(12),(char*)(name_?name_:"unknown"),size_);
  }
}
//...........................................................................
template<class T>
void Queue<T>::purge()
{
  head = 0;
  tail = 0;
  quantity = 0;
}
//...........................................................................
template<class T>
inline int Queue<T>::getsize (void)
{
  return(size);
}
//...........................................................................
template<class T>
inline char *Queue<T>::getname (void)
{
  return(name);
}
//...........................................................................
template<class T>
inline int Queue<T>::getfree (void)
{
  return(size-quantity);
}

//...........................................................................
template<class T>
inline int Queue<T>::getquantity (void)
{
  return(quantity);
}

//...........................................................................
template<class T>
inline void Queue<T>::push (T value)
{
  DBG_CHECK(quantity<size);
  p[tail]=value;
  tail++;
  if (tail==size)
    tail=0;
  quantity++;
}
//...........................................................................
template<class T>
inline void Queue<T>::unpop (T value)
{
  DBG_CHECK(quantity<size);
  head--;
  if (head==-1)
    head=size-1;
  quantity++;
  p[head]=value;
}
//...........................................................................
template<class T>
inline T *Queue<T>::pushempty (void)
{
  DBG_CHECK(quantity<size);
  T *temp = &p[tail];
  tail++;
  if (tail==size)
    tail=0;
  quantity++;
  return (temp);
}
//...........................................................................
template<class T>
inline void Queue<T>::popempty (void)
{
  DBG_CHECK(quantity>0);
  head++;
  if (head==size)
    head=0;
  quantity--;
}
//...........................................................................
template<class T>
inline T Queue<T>::pop(void)
{
  DBG_CHECK(quantity>0);
  int pos=head;
  head++;
  if (head==size)
    head=0;
  quantity--;
  return (p[pos]);
}
//...........................................................................
template<class T>
inline T *Queue<T>::peek(void)
{
  DBG_CHECK(quantity>0);
  return (&p[head]);
}
//...........................................................................
template<class T>
inline T &Queue<T>::operator[] (int pos)
{
  DBG_CHECK((pos<quantity)&&(pos>=0));
  int real_index = (head+pos)%size;
  return p[real_index];
}
//===========================================================================
// HashArray
//===========================================================================

typedef int (hash_fun_type) (const char*);
typedef int (collision_fun_type) (const int, const int);

template<class T>
class HashArray: public Heap_object
{
  int diag_array[DIAG_SIZE+1];
  int init_size,         // rozmiar podany przy inicjacji (bez narzutu)
      max_counter;       // najwieksza jak dotad liczba elementów dodanych
                         // do tablicy
  Array<char> diag_flag; // tablica flag dla diagnostyki
  char *name;            // unikalna nazwa tablicy
  int counter;           // aktualna liczba elementów dodanych do tablicy
  int size;              // rozmiar tablicy z narzutami (pelny)
  int last_found;        // ostatnie miejsce w ktore strzelila funkcja get_pos
  int search_point;      // miejsce w ktorym zatrzymalo sie ostatnie get_next
  int search_cnt;        // ile znaleziono od ostatniego reset
  int search_eot;        // zmienna logiczna okrellajaca czy ostatnie get_next
                         // wylazlo za tablice
  hash_fun_type       *hash_fun;
  collision_fun_type  *collision_fun;
  T *p;
  Array<char*> labels;

  int get_pos(char *label);
  int first_prime(int num);

public:

  HashArray ();
  HashArray (int size_, char *name_=NULL);
  HashArray (int size_, short prc_grow, char *name_=NULL);
  ~HashArray ();

  void free ();
  void alloc (int size_, char *name_=NULL, short prc_grow=DEF_PRC_GROW);
  inline void alloc (int size_, short prc_grow);

  inline void set_hash_fun (hash_fun_type *fun);
  inline void set_collision_fun (collision_fun_type *fun);

  inline int get_size (void);
  inline char *get_name (void);

  void add(char *label);
  void add(char *label, T value);
  void erase(char *label);

  int  is (char *label);

  inline void reset(void);
  char *get_next(void);
  inline int eot(void);

  T &operator[] (char *label);
};

//---------------------------------------------------------------------------
int def_hash_fun (const char *label);
int def_collision_fun (const int hash, const int shoot);
//...........................................................................
template<class T>
HashArray<T>::HashArray ()
: p(NULL), hash_fun(def_hash_fun), collision_fun(def_collision_fun),
  last_found(-1), search_point(0), search_eot(1), search_cnt(0), counter(0),
  name(NULL)
{
  memset(diag_array, 0, sizeof (diag_array));
  max_counter=0;
}
//...........................................................................
template<class T>
HashArray<T>::HashArray (int size_, char *name_)
: p(NULL), hash_fun(def_hash_fun), collision_fun(def_collision_fun),
  last_found(-1), search_point(0), search_eot(1), search_cnt(0), counter(0),
  name(NULL)
{
  memset(diag_array, 0, sizeof (diag_array));
  max_counter=0;
  alloc(size_, name_, DEF_PRC_GROW);
}
//...........................................................................
template<class T>
HashArray<T>::HashArray (int size_, short prc_grow, char *name_)
: p(NULL), hash_fun(def_hash_fun), collision_fun(def_collision_fun),
  last_found(-1), search_point(0), search_eot(1), search_cnt(0), counter(0),
  name(NULL)
{
  memset(diag_array, 0, sizeof (diag_array));
  max_counter=0;
  alloc(size_, name_, prc_grow);
}
//...........................................................................
template<class T>
HashArray<T>::~HashArray()
{
  if (p||labels) free ();
}
//...........................................................................
template<class T>
void HashArray<T>::free()
{
  int n=0;
  int i;
  for (i=0; i<=DIAG_SIZE; i++)
    n+=diag_array[i];
  if (n)
  {
    char s[256]="";
    char s_temp[80];
    double e=0;
    MESSAGE(bas_str(11), name, init_size, size, max_counter, counter);
    for (i=1; i<=DIAG_SIZE; i++)
    {
      e+=diag_array[i]*i;
      sprintf(&s_temp[0],"%1.2f ",(double)diag_array[i]*100/n);
      strcat(s, s_temp);
    }
    e+=diag_array[0]*DIAG_SIZE;
    e=100/(e/n);
    sprintf(&s_temp[0],"... %1.2f, efficiency %1.2f%%",(double)diag_array[0]*100/n, (double)e);
    strcat(s, s_temp);
    MESSAGE("avg dist: %s", s);
  }
  if (labels)
  {
    labels.free();
  }
  if (p)
  {
    Heap::free(p, name?name:"[bas] HashArray<T>");
    size=0;
    last_found=-1;
    p=NULL;
    counter=0;
    max_counter=0;
    init_size=0;
    diag_flag.free();
  }
}
//...........................................................................
template<class T>
inline void HashArray<T>::alloc(int size_, short prc_grow)
{
  alloc(size_, NULL, prc_grow);
}
//...........................................................................
template<class T>
void HashArray<T>::alloc(int size_, char *name_, short prc_grow)
{
  try
  {
    init_size=size_;
    if (p||labels) free ();
    size=first_prime(size_+((prc_grow*size_)/100));
    p=(T*)Heap::alloc(sizeof(T)*size, name_?name_:"[bas] HashArray<T>");
    name=name_;
    memset(p, 0, sizeof(T)*size);
    labels.alloc(size, name_?name_:"[bas] HashArray<T>");
    labels.fillbyte(0);
    diag_flag.alloc(size, name_?name_:"[bas] HashArray<T>");
    diag_flag.fillbyte(0);
  }
  catch (Failure)
  {
    FAILURE (bas_str(3), name, size_, prc_grow, size);
  }
}
//...........................................................................
template<class T>
int HashArray<T>::first_prime(int num)
{
  if (num%2==0)
    num++;
  while(1)
  {
    for (int i=3; i<num; i+=2)
      if (num%i==0)
      {
        num+=2;
        break;
      }
    if (i>=num)
      return(num);
  }
}
//...........................................................................
template<class T>
int HashArray<T>::get_pos (char *label)
// zwraca pozycje pod która znaleziono etykiete. Jeżeli etykiety nie
// znaleziono w zmiennej "last_found" zwraca puste miejsce, na ktorym przerwano
// wyszukiewanie (potrzebne do funkcji add)
{
  int hash;
  int shoot=1;
  strlwr(label);
  hash=hash_fun(label)%size;
  last_found=hash;
  while ((labels[hash]!=NULL)&&(strcmp(labels[hash], label)!=0)&&(shoot<=size))
  {
    hash=collision_fun(hash, shoot)%size;
    last_found=hash;
    shoot++;
  }
  if ((labels[hash]!=NULL)&&(shoot<=size))
    {
      if (!diag_flag[hash])
      {
        if (shoot<=DIAG_SIZE)
          diag_array[shoot]++;
        else
          diag_array[0]++;
        diag_flag[hash]++;
      }
      return (hash);
    }
  else
    return (-1);
}
//...........................................................................
template<class T>
int HashArray<T>::is (char *label)
{
  return (get_pos(label)!=(-1));
}
//...........................................................................
template<class T>
T &HashArray<T>::operator[] (char *label)
{
  int pos = get_pos(label);
  if (pos==-1)
    FAILURE (bas_str(4), name, label);
  return (p[pos]);
}
//...........................................................................
template<class T>
inline int HashArray<T>::get_size (void)
{
  return(size);
}
//...........................................................................
template<class T>
inline char *HashArray<T>::get_name (void)
{
  return(name);
}
//...........................................................................
template<class T>
inline void HashArray<T>::set_hash_fun (hash_fun_type *fun)
{
  if (counter > 0)
    FAILURE (bas_str(5), name);
  hash_fun=fun;
}
//...........................................................................
template<class T>
inline void HashArray<T>::set_collision_fun (collision_fun_type *fun)
{
  if (counter > 0)
    FAILURE (bas_str(6), name);
  collision_fun=fun;
}
//...........................................................................
template<class T>
void HashArray<T>::add(char *label)
{
  int pos = get_pos(label);
  if (pos!=(-1))
    FAILURE (bas_str(7), name, label);
  else if (labels[last_found]!=NULL)
    FAILURE (bas_str(8), name, label);
  if (counter==init_size)
    Log::warning (bas_str(10), name, label);
  counter++;
  if (max_counter<counter)
    max_counter=counter;
  labels[last_found]=label;
  reset();
}
//...........................................................................
template<class T>
void HashArray<T>::add(char *label, T value)
{
  int pos = get_pos(label);
#if HI_DEBUG
  if (pos!=(-1))
    FAILURE (bas_str(7), name, label);
  else if (labels[last_found]!=NULL)
    FAILURE (bas_str(8), name, label);
  if (counter==init_size)
    WARNING(bas_str(10), name, label);
#endif
  counter++;
  if (max_counter<counter)
    max_counter=counter;
  labels[last_found]=label;
  p[last_found]=value;
  reset();
}
//...........................................................................
template<class T>
void HashArray<T>::erase(char *label)

// Przymazuje zerami wartości w obu tablicach. Istotne dla reszty metod jest
// zerowanie w tablicy labels, ponieważ na tej podstawie jest rozstrzygane
// czy pozycja w tablicy jest wolna czy zajęta.

{
  int pos = get_pos(label);
#if HI_DEBUG
  if (pos==(-1))
    FAILURE (bas_str(9), name, label);
#endif
  counter--;
  memset(&p[pos],0,sizeof(T));
  labels[pos]=NULL;
  reset();
  diag_flag[pos]=0;
}
//...........................................................................
template<class T>
inline void HashArray<T>::reset(void)
{
  search_point=0;
  search_cnt=0;
  search_eot=!counter;
}
//...........................................................................
template<class T>
char *HashArray<T>::get_next(void)
{
  while ((search_point<size)&&(labels[search_point]==NULL))
    search_point++;
  if (search_point==size)
    return(NULL);
  else
    {
      search_point++;
      search_cnt++;
      search_eot=(search_cnt==size);
      return (labels[search_point]);
    }
}
//...........................................................................
template<class T>
inline int HashArray<T>::eot(void)
{
  return (search_eot);
}
//===========================================================================
// DiagQueue
//===========================================================================
#define MAX_DIAG_QUEUE_LINE 1024

class DiagQueue
{
private:
  struct Item
  {
    char log_line[MAX_DIAG_QUEUE_LINE];
  };
  char *name;
  Queue<Item> queue;                               
public:
  DiagQueue(int size = 50, char *name_ = NULL);
  ~DiagQueue(void);
  void log(char *message, ...);
  void dump(void);                                                     
};                                                               

//===========================================================================
// ANOTHER FUNCTIONS
//===========================================================================
char *new_str(char *str);
void del_str(char *str);

unsigned short sum16 (char *data_p, unsigned length);
unsigned short crc16 (char *data_p, unsigned length);

// uwaga - kompresja RLE8 testowana jedynie dla blokow < 255
unsigned rle_8_decompress (unsigned char *dest, unsigned char *source, unsigned source_size);
unsigned rle_8_compress   (unsigned char *dest, unsigned char *source, unsigned source_size);

#endif










