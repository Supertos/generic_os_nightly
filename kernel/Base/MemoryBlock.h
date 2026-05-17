/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Types.h"
#include "Utilities.h"

#define NO_MEMBLOCK (MemoryBlock){0}

typedef struct {
	uptr Base;
	uptr End;
} MemoryBlock;

typedef MemoryBlock (*AllocateFunction)( void* self, usize size, usize align );
typedef void (*FreeFunction)( void* self, usize size, usize align );

typedef struct {
	void* Self;
	AllocateFunction From;
	FreeFunction To;
} Allocator;

static inline usize MemorySize( MemoryBlock self ) {
    return self.End - self.Base;
}

static inline bool ValidMemory( MemoryBlock self ) {
	return !!MemorySize(self);
}

static inline bool MemorySuits( MemoryBlock self, usize size, usize align ) {
	return self.End - size >= AlignUp(self.Base, align);
}

static inline MemoryBlock MemoryOfSize( uptr base, usize size ) {
    return (MemoryBlock){ base, base + size };
}

static inline MemoryBlock ReplenishAllocator( Allocator* from, Allocator* to, usize size, usize align ) {
	MemoryBlock out = {0};
	
    while( !MemorySuits(out, size, align) ) {
		out = from->From( from->Self, size, align );
        if( !ValidMemory(out) ) break;
        to->To(to->Self, out);
    }

    return out;
}