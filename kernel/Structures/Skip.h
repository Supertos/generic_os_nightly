/* Supertos Industries (2012 - 2026) 
 * Double-linked embedded skip-list implementation.
 *
 * Implementation expects non-packed attrubute unless sizeof(SkipIndex) % sizeof(void*) == 0 (which it is as of 27.01.2026)
 *
 * To use Skip ensure:
 * 1. Your "item" has SkipIndex (each item has the same SkipIndex offset), followed by SkipIndex[N * 2];
 * 2. Your owner has Skip, followed by SkipIndex[SKIPINDEX_LINKS(N)];
 * where N is desired amount of levels.
 * 
 * Since skip-list is designed to be embedded into vast memory blocks (where memory is not concern), it may be reasonable to align SkipIndex and SkipIndex[SKIPINDEX_LINKS(N)] to cacheline.
 * (e.g.)
 * Cacheline 1: Userdata, SkipIndex
 * Cacheline 2: SkipIndex[SKIPINDEX_LINKS(N)]
 * Cacheline 3: Userdata
 * 
 * Such layout will increase chance of prefetching cacheline 1&3 during search.
 * 
 * User code shall not know of SkipIndex[SKIPINDEX_LINKS(N)] contents, nor should it work with SkipIndex. All exported functions work with items.
 */

#pragma once

#define P_100 ((u32)(-1))
#define P_50 (P_100 / 2)
#define P_25 (P_100 / 4)
#define P_0 0

#define SKIPINDEX(lvls) struct { SkipIndex Index; void* Links[2 * lvls] } __attribute__ ((__may_alias__))

#include "Base.h"

typedef struct {
    u16 Levels;
    u16 MaxLevels;
    u32 __Reserved;
} SkipIndex;

typedef struct {
    u32 IndexOffset;
    u16 Levels;
    u32 P;

    int (*Compare)(void* item, void* valueAddr);
    SkipIndex Sentinel;
} Skip;

void* SkipPrev( Skip* list, void* item );

void* SkipNext( Skip* list, void* item );

void ToSkip( Skip* list, void* predecessors[], void* item );

void* FromSkip( Skip* list, void* item );

void SkipPos( Skip* list, void* predecessors[], void* valueAddr );

Skip* SkipNew( void* begin, u32 indexOffset, u16 lvls, void* sentinel );

void SkipInit( Skip* list, u32 p, int (*compare)(void*, void*) );

void* SkipFirst( Skip* list );