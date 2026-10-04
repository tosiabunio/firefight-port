// Clones of MSVC 4 C runtime functions (see msvc4.h).

#include <compat/msvc4.h>

#include <stdint.h>

// --- rand ---------------------------------------------------------------------------------------

namespace {
uint32_t holdrand = 1;  // the CRT's state; its value before any srand
} // namespace

void msvc4_srand(unsigned seed)
{
  holdrand = (uint32_t)seed;
}

// holdrand*214013+2531011, wrapping at 32 bits; bits 16-30 are the result (0..32767).
int msvc4_rand(void)
{
  holdrand = holdrand * 214013u + 2531011u;
  return (int)((holdrand >> 16) & 0x7fff);
}

// --- qsort --------------------------------------------------------------------------------------
// The CRT's qsort.c as MSVC shipped it until Visual C++ 2005, which is what the 1.1 exe holds
// (0x48a340; docs/original-archive.md). Pointer differences are signed, as in the original.

namespace {

typedef int (*Compare)(const void *, const void *);

const size_t cutoff = 8;  // partitions of this many elements or fewer go to shortsort

void swap(char *a, char *b, size_t width)
{
  if (a != b)
    while (width--)
    {
      const char tmp = *a;
      *a++ = *b;
      *b++ = tmp;
    }
}

// Selection sort: moves the largest element to the end, repeatedly. Of equal largest elements
// the first one found is the one moved.
void shortsort(char *lo, char *hi, size_t width, Compare compare)
{
  while (hi > lo)
  {
    char *max = lo;
    for (char *p = lo + width; p <= hi; p += width)
      if (compare(p, max) > 0)
        max = p;
    swap(max, hi, width);
    hi -= width;
  }
}

} // namespace

void msvc4_qsort(void *base, size_t num, size_t width, int (*compare)(const void *, const void *))
{
  char *lostk[30], *histk[30];  // pending partitions; the smaller one is always done first
  int stkptr = 0;

  if (num < 2 || width == 0)
    return;

  char *lo = (char *)base;
  char *hi = (char *)base + width * (num - 1);

  for (;;)
  {
    const size_t size = (size_t)(hi - lo) / width + 1;
    if (size <= cutoff)
      shortsort(lo, hi, width, compare);
    else
    {
      // The middle element becomes the pivot, at lo.
      swap(lo + (size / 2) * width, lo, width);
      char *loguy = lo;
      char *higuy = hi + width;
      for (;;)
      {
        do
          loguy += width;
        while (loguy <= hi && compare(loguy, lo) <= 0);
        do
          higuy -= width;
        while (higuy > lo && compare(higuy, lo) >= 0);
        if (higuy < loguy)
          break;
        swap(loguy, higuy, width);
      }
      swap(lo, higuy, width);

      // Recurse into the smaller part now and stack the larger one.
      if (higuy - 1 - lo >= hi - loguy)
      {
        if (lo + width < higuy)
        {
          lostk[stkptr] = lo;
          histk[stkptr] = higuy - width;
          ++stkptr;
        }
        if (loguy < hi)
        {
          lo = loguy;
          continue;
        }
      }
      else
      {
        if (loguy < hi)
        {
          lostk[stkptr] = loguy;
          histk[stkptr] = hi;
          ++stkptr;
        }
        if (lo + width < higuy)
        {
          hi = higuy - width;
          continue;
        }
      }
    }
    if (--stkptr < 0)
      return;
    lo = lostk[stkptr];
    hi = histk[stkptr];
  }
}
