/* Supertos Industries (2012 - 2026) 
 * RBTree allocator with auto coalescing.
 */
#pragma once

#define BEST_FIT true
#define FIRST_FIT false

#include "Base.h"

#include "Structures/RBTree.h"
#include "Cache.h"

#include "Concurrency/Spinlock.h"

#include "RBAllocator.h"

typedef struct {
    RBTree Free;
    Cache* Cache;
    Spinlock Lock;
} RBAllocator; 

MemoryBlock FromRBAllocator( RBAllocator* self, usize size, usize align, bool bestFit );

void RBAllocatorSetCache( RBAllocator* self, Cache* newCache );

static inline MemoryBlock FromRBAllocatorFirst( RBAllocator* self, usize size, usize align ) { return FromRBAllocator(self, size, align, FIRST_FIT); }

static inline MemoryBlock FromRBAllocatorBest( RBAllocator* self, usize size, usize align ) { return FromRBAllocator(self, size, align, BEST_FIT); }

void ToRBAllocator( RBAllocator* self, MemoryBlock block );

RBAllocator* RBAllocatorNew( void* begin, Cache* cache );
 
static inline usize RBAllocatorCacheSize() { return 40; }