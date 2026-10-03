#include "1ba_hdrs.h"

unsigned char Bit::mask[8]={ 0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80 };

//===========================================================================

// teksty wypisywane w templatach.

static char* bas_msg[] = {
            /*  0 */        "Array<T> allocation error %d",
            /*  1 */        "Array<T> range check error. Index to low",
            /*  2 */        "Array<T> range check error. Index to high",
            /*  3 */        "HashArray<T>'%s' allocation error %d + %d %% = prime %d",
            /*  4 */        "HashArray<T>'%s'::operator[] - label not found %s",
            /*  5 */        "HashArray<T>'%s' redefinition of hash function not allowed",
            /*  6 */        "HashArray<T>'%s' redefinition of collision function not allowed",
            /*  7 */        "HashArray<T>'%s'::add - duplicate label %s",
            /*  8 */        "HashArray<T>'%s'::add - overflow on label %s",
            /*  9 */        "HashArray<T>'%s'::erase - label not found %s",
            /* 10 */        "HashArray<T>'%s' - declared size exceded on label: %s",
            /* 11 */        "HashArray<T>'%s' - declared %d, real %d, used %d, on quit %d",
            /* 12 */        "Queue<T>'%s' - allocation error (%d)",
            /* 13 */        "Queue<T>'%s' - overflow",
            /* 14 */        "Queue<T>'%s' - underflow",
            /* 15 */        "Queue<T>'%s'::operator[] out of range"
                          };

char *bas_str (int num)
{
  return (bas_msg[num]);
}

//===========================================================================
// HashTable
//===========================================================================

int def_hash_fun (const char *label)
{
  int res=0;
  int cnt=0;
  while (label[cnt])
  {
    res=(res<<1)+tolower(label[cnt]);
    cnt++;
  }
  res^=cnt;
  return (abs(res));
}

//...........................................................................

int def_collision_fun (const int hash, const int shoot)
{
  int res=3;
  for (int i=0; i<(shoot-1); i++)
    res+=res;
  return (hash+res);
}

//===========================================================================
// DiagQueue
//===========================================================================
DiagQueue::DiagQueue(int size, char *name_)
{
  queue.alloc(size, "DiagQueue::queue");
  name = name_;
}
//---------------------------------------------------------------------------
DiagQueue::~DiagQueue(void)
{
  queue.free();
}
//---------------------------------------------------------------------------
void DiagQueue::log(char *message, ...)
{
  if (queue.getfree()==0)
    queue.popempty();
  Item *item = queue.pushempty();
  va_list argptr; \
  va_start(argptr, message); \
  _vsnprintf(item->log_line, MAX_DIAG_QUEUE_LINE-1, message, argptr); \
  va_end(argptr); \
}
//---------------------------------------------------------------------------
void DiagQueue::dump(void)
{
  if (name)
    MESSAGE(name);
  for (int i=0; i<queue.getquantity(); i++)
    MESSAGE(queue[i].log_line);
}
//===========================================================================
// ANOTHER FUNCTIONS
//===========================================================================
char *new_str(char *str)
{
  if (str==NULL)
    return (str);
  int size   = strlen(str)+1;
  char *temp = (char*)Heap::alloc(sizeof(char)*size, "[bas] new_str");
  memcpy(temp,str,size);
  return (temp);
}
//---------------------------------------------------------------------------
void del_str(char *str)
{
  if (str==NULL)
    return;
  Heap::free(str,"[bas] del_str");
}
//---------------------------------------------------------------------------
unsigned short sum16 (char *data_p, unsigned length)
{
  short result;
  char *csum1=&((char*)&result)[0];
  char *csum2=&((char*)&result)[1];
  (*csum1)=(char)-1;
  (*csum2)=(char)length;
  for (unsigned i=0; i<length; i++)
  {
    (*csum1)=(char)((*csum1)-data_p[i]);
    (*csum2)=(char)((*csum2)^data_p[i]);
  }
  return(result);
}
//---------------------------------------------------------------------------
#define POLY 0x8408

unsigned short crc16(char *data_p, unsigned length)
{
  unsigned char i;
  unsigned int data;
  unsigned int crc = 0xffff;
  if (length == 0)
    return ((unsigned short)~crc);
  do
  {
    for (i=0, data=(unsigned int)0xff & *data_p++; i < 8; i++, data >>= 1)
    {
      if ((crc & 0x0001) ^ (data & 0x0001))
        crc = (crc >> 1) ^ POLY;
      else  crc >>= 1;
    }
  } while (--length);
  crc = ~crc;
  data = crc;
  crc = (crc << 8) | (data >> 8 & 0xff);
  return ((unsigned short)crc);
}
//-----------------------------------------------------------------------------
unsigned rle_8_compress   (unsigned char *dest, unsigned char *source, unsigned source_size)
{
  unsigned char *start = source;
  unsigned char *s     = source;
  unsigned char *d     = dest;
  unsigned char *s_end = s + source_size;
  BOOL data = TRUE;
  while (start<s_end)
  {
    if (data) 
    {
      while (*s) s++;
      if (s>s_end)
        s = s_end;
      *d = (unsigned char)(s-start);                 
      memcpy(d+1, start, *d);
      start+=*d;
      d+=*d+1;
    }
    else
    {
      while (!*s) s++;
      if (s>s_end)
        s = s_end;
      *d = (unsigned char)(s-start);
      start+=*d++;
    }
    data = !data;
  }
  return (d-dest);
}
//--------------------------------------------------------------------------------------------
unsigned rle_8_decompress (unsigned char *dest, unsigned char *source, unsigned source_size)
{
  unsigned char *s     = source;
  unsigned char *d     = dest;
  unsigned char *s_end = s + source_size;
  BOOL data = TRUE;
  while (s<s_end)
  {
    if (data)
    {
      memcpy(d, s+1, *s);
      d+=*s;
      s+=*s+1;
    }
    else
    {
      memset(d, 0, *s);
      d+=*s++;
    }
    data = !data;
  }
  return(d-dest);
}
//--------------------------------------------------------------------------------------------
