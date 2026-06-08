/* Supertos Industries (2012 - 2026) */
#include "RBAllocator.h"


typedef struct {
    uptr Base;
    uptr End;
    RBIndex Index;
} Entry;


static inline void Coalesce( RBAllocator* self, Entry* center ) {
    Entry *cur = RBTreeNext(&self->Free, center) ?: center, *prev;
    usize i = 2;

    while( (i--) > 0 && (prev = RBTreePrev(&self->Free, cur)) ) {
        if( prev->End == cur->Base ) {
            cur->Base = prev->Base;
            ToCache( self->Cache, FromRBTree(&self->Free, prev) );
        }else{
            cur = prev;
        }
    }
}


void ToRBAllocator( RBAllocator* self, MemoryBlock block ) {
    ACQUIRE_AUTO(&self->Lock);
    Entry* newEntry = FromCache(self->Cache);
    *newEntry = (Entry){ .Base = block.Base, .End = block.End };
    ToRBTree(&self->Free, newEntry);
    Coalesce(self, newEntry);
}


MemoryBlock FromRBAllocator( RBAllocator* self, usize size, usize align, bool bestFit ) {
    ACQUIRE_AUTO(&self->Lock);
    RBTree* free = &self->Free;
    Entry *best = NULL, *cur;
    usize bestScore = ~0ULL;
    if( !cur ) return NO_MEMBLOCK;
    
    while( (!best || bestFit) && (cur = RBTreeNext(free, cur)) ) {
        uptr aligned = AlignUp(cur->Base, align);
        if( aligned > cur->End ) continue;
        
        usize length = cur->End - aligned, score = length - size;
        if( length < size || score > bestScore ) continue;

        best = cur;
        bestScore = score;
        if( score == 0 ) break;
    }
    
    if( !best ) return NO_MEMBLOCK;

    uptr base = best->Base, alignedBase = AlignUp(base, align);
    best->Base = alignedBase + size;

    if( base != alignedBase ) ToRBAllocator( self, (MemoryBlock){base, alignedBase - base} );

    if( best->End == best->Base ) ToCache( self->Cache, FromRBTree(free, best) );
    return (MemoryBlock){ .Base = alignedBase, .End = alignedBase + size };
}


int Compare( Entry* a, uptr* b ) {
    return (a->End > *b) - (*b > a->End);
}


RBAllocator* RBAllocatorNew( void* begin, Cache* cache ) {
    RBAllocator* self = begin;
    RBTreeNew( &self->Free, offset_of(Entry, Index), offset_of(Entry, End), Compare );
    self->Cache = cache;
    return self;
}