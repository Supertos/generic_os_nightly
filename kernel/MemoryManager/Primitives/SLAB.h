/* Supertos Industries (2012 - 2026) 
 * Self-contained allocator in evenly split PAGE.
 * Returns objects of single size.
 * Lockless.
 */
#pragma once

#include "Base.h"

typedef struct {
    u16 Elements;
    u16 MaxElements;
    u16 Size;
    u16 ReservedSpaceOffset;
    uptr Base;
} SLAB;

SLAB* SLABNew( uptr begin, uptr metaBegin, usize elementSize, usize metaOffset );

void* FromSLAB( SLAB* slab );

void ToSLAB( SLAB* slab, uptr addr );

static inline bool SLABExhausted( SLAB* slab ) { return slab->Elements == 0; }
static inline bool SLABEmpty( SLAB* slab ) { return slab->Elements == slab->MaxElements; }

static inline void* SLABMeta( SLAB* slab ) {
	return (uptr)(slab + 1) + sizeof(Bitmap) * BITMAP_SIZE(PAGE / slab->Size);
}