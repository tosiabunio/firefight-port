// MSVC 4 C runtime behaviour that the simulation depends on, cloned so every platform and
// compiler gets what the original 1.1 FIREFGHT.EXE got (docs/porting-plan.md, phase 5).
//
// - rand/srand: Rand::init fills its table from srand(0) and 1024 rand() calls, and every
//   random decision in the game comes from that table. Other C runtimes have other generators.
// - qsort: the collision results and the visible level objects are sorted with many equal keys.
//   The order of equal elements is whatever the algorithm leaves, so it has to be MSVC 4's: the
//   classic pre-2005 quicksort with a median pivot and selection sort below 9 elements.
//
// Both were checked against the code in the original exe, run in an emulator; the crt_vectors
// test compares the clones with its output (tests/golden/crt_rand.txt, crt_qsort.txt).

#ifndef FF_COMPAT_MSVC4_H
#define FF_COMPAT_MSVC4_H

#include <stddef.h>

void msvc4_srand(unsigned seed);
int  msvc4_rand(void);
void msvc4_qsort(void *base, size_t num, size_t width, int (*compare)(const void *, const void *));

#endif
