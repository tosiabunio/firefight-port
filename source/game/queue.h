#ifndef __QUEUE__
#define __QUEUE__

#if HI_DEBUG
void NoQueue_statistics(void);
void update_NoQueue_statistics(int usage,char* user_name);
#endif

template<class T> class NoQueue : public Heap_object
{
  private:
   T *buffer;
   int ignore_usage;
   int size;
   int first;
   int last;
   int used;
   int look;
   int looked;
   char *user_name;
  public:
   NoQueue(int _size,char *t,int _ignore_usage=0);
   ~NoQueue(void);
   void add(T *elem);
   void flush(void);
   int empty(void) {return !used;}
   int get_used(void) {return used;}
   T *head(void);
   T *see_first(void);
   T *see_next(void);
};

extern char *queue_mbn;

template<class T> inline NoQueue<T>::NoQueue(int _size,char *t,int _ignore_usage)
{
  DBG_CHECK(_size>0);
  ignore_usage=_ignore_usage;
  user_name=t;
  size=_size;
  first=0;
  last=0;
  used=0;
  look=-1;
  looked=0;
  buffer=(T*)Heap::alloc(sizeof(T)*size,queue_mbn);
}

template<class T> inline NoQueue<T>::~NoQueue(void)
{
  if (buffer)
  {
    Heap::free((void*)buffer);
    buffer=NULL;
  }
}

template<class T> inline void NoQueue<T>::add(T* elem)
{
  DBG_CHECK(used<size);
  if (used>=size)
  {
    return;
  }
  buffer[last]=(*elem);
  last++;
  if (last>=size)
  {
    last-=size;
  }
  used++;
#if HI_DEBUG
  if (!ignore_usage)
  {
    update_NoQueue_statistics((used*100)/size,user_name);
  }
#endif
}

template<class T> inline void NoQueue<T>::flush(void)
{
  first=0;
  last=0;
  used=0;
}

template<class T> inline T* NoQueue<T>::head(void)
{
  if (!used)
  {
    return NULL;
  }
  else
  {
    int to_return=first;
    first++;
    if (first>=size)
    {
      first-=size;
    }
    used--;
    return &buffer[to_return];
  }
}

template<class T> inline T* NoQueue<T>::see_first(void)
{
  if (!used)
  {
    return NULL;
  }
  else
  {
    look=first;
    looked=1;
    return &buffer[first];
  }
}

template<class T> inline T* NoQueue<T>::see_next(void)
{
  DBG_CHECK(look!=-1);
  if (looked>used)
  {
    look=-1;
    looked=0;
    return NULL;
  }
  else
  {
    int to_return=look;
    look++;
    if (look>=size)
    {
      look-=size;
    }
    looked++;
    return &buffer[to_return];
  }
}

#endif
