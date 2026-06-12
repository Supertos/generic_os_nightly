/* Supertos Industries (2012 - 2026)
 * Bitmap allocator implementation. Shall not be used as standalone allocator.
 * 0 stands for free and 1 for occupied.
 */

#pragma once

#include "Base.h"

typedef usize Bitmap;

#define BITMAP_SIZE(elements) ((elements + bitsof(Bitmap) - 1) / bitsof(Bitmap)) 

static inline bool BitAt( Bitmap* begin, usize elementNo ) {
    usize unit = elementNo / bitsof(Bitmap);
    return !!( begin[unit] & MASK( Bitmap, elementNo % bitsof(Bitmap), 1 ) );
}

usize TrySetBitsAtomic( Bitmap* self, usize unitCount, usize slots );

// Must be atomic for single machine word.
void SetBits( Bitmap* self, usize id, usize slots, bool set );