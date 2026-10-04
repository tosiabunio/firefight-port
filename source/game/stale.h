#ifndef _STALE_H_INCLUDED
#define _STALE_H_INCLUDED

// Port: the original read one member before it was ever set: ACannon's constructor has
// "global_time==Mp::BEYONDTIME" where it meant "=". Game objects live in FastAlloc slots that are
// reused, last freed first, so a new cannon got whatever the slot's earlier occupants had left at
// that offset. In the original's 32-bit MSVC layout that is offset 232. The original demos depend
// on it: a cannon built away from the screen kept or lost its timer by it.
//
// Stale_memory keeps that word for each slot as the original's memory held it. When a game object
// is deleted, the bytes its own 32-bit layout has at offsets 232-235 replace the slot's word;
// objects that end before offset 232 leave it alone. A slot never used holds 0. The table of what
// each class has there (stale_gen.cpp) is generated from clang's MSVC record layouts by
// tools/layout/stale_memory.py.
//
// Every delete of a game object must call object_deleted first. FastAlloc's operator delete checks
// that it did (slot_freed).

class Object;

class Stale_memory
{
  public:
   static const int offset;                // of ACannon::global_time in the original's layout
   static void reset(void);                // every slot never used
   static void object_deleted(Object *o);  // before the object is deleted
   static void slot_freed(void *slot, size_t size);  // FastAlloc's operator delete
   static int  word(void *slot);           // what the original's slot held at the offset
  private:
   static void copy_word(Object *o, unsigned char *w);
   static void copy(unsigned char *w, int dest, const void *field, int from, int n);
   static void address(unsigned char *w, int dest, int not_null, int from, int n);
};

#endif
