// Port: the words the original's object slots held at ACannon::global_time's offset (stale.h).

#include <unordered_map>
#include <unordered_set>

#include "headers.h"

namespace
{
  std::unordered_map<void*,unsigned> words;  // by slot; a slot not in it holds 0
  std::unordered_set<void*> deleting;         // slots object_deleted saw and not yet freed
}

void Stale_memory::reset(void)
{
  words.clear();
  deleting.clear();
}

void Stale_memory::object_deleted(Object *o)
{
  void *slot=dynamic_cast<void*>(o);
  unsigned word=Stale_memory::word(slot);
  copy_word(o,(unsigned char*)&word);
  words[slot]=word;
  deleting.insert(slot);  // a destructor may delete other objects before this slot is freed
}

void Stale_memory::slot_freed(void *slot, size_t size)
{
  if (!deleting.erase(slot))
  {
    WARNING("Stale_memory: a game object of %d bytes was deleted without object_deleted",(int)size);
  }
}

int Stale_memory::word(void *slot)
{
  std::unordered_map<void*,unsigned>::const_iterator it=words.find(slot);
  return (it==words.end()) ?0 :(int)it->second;
}

// Bytes from..from+n-1 of a member go to bytes dest..dest+n-1 of the word. Integers are little
// endian in the original and in the port alike.
void Stale_memory::copy(unsigned char *w, int dest, const void *field, int from, int n)
{
  memcpy(w+dest,(const unsigned char*)field+from,n);
}

// A pointer: the original's address isn't known, but any 32-bit address it had was a large
// positive number (the image base, 0x00400000, stands for it) unless it was NULL.
void Stale_memory::address(unsigned char *w, int dest, int not_null, int from, int n)
{
  unsigned value=not_null ?0x00400000u :0u;
  memcpy(w+dest,(const unsigned char*)&value+from,n);
}
