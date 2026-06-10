/* Supertos Industries (2012 - 2026) */
#include "RBAllocator.h"


typedef struct {
    uptr Base;
    uptr End;
    RBIndex Index;
} Entry;


static inline void Coalesce( RBAllocator* self, Entry* center ) {
    Entry *cur = RBTreeNext(&self->Free, center) ?: center, *prev;
    
	for( usize i = 0; i < 3; i++ ) {
		if( !(prev = RBTreePrev(&self->Free, cur)) ) break;
		if( prev->End == cur->Base )
			prev->End = cur->End;
			
		if( prev->End == cur->End )
			ToCache( FromRBTree(&self->Free, cur) );
		cur = prev;
	}
}


void __ToRBAllocator( RBAllocator* self, MemoryBlock block ) {
    Entry* new = FromCache(self->Cache);
    *new = (Entry){ .Base = block.Base, .End = block.End };
    ToRBTree(&self->Free, new);
    Coalesce(self, new);
}


void ToRBAllocator( RBAllocator* self, MemoryBlock block ) {
    ACQUIRE_AUTO(&self->Lock);
	__ToRBAllocator(self, block);
}


MemoryBlock FromRBAllocator( RBAllocator* self, usize size, usize align, bool bestFit ) {
    ACQUIRE_AUTO(&self->Lock);
    RBTree* free = &self->Free;
    Entry *best = NULL, *cur = NULL;
    usize score = ~0ULL;
	
	while( cur = RBTreeNext(free, cur) ) {
        uptr aligned = AlignUp(cur->Base, align);
		usize length = cur->End >= aligned ? cur->End - aligned : 0;
        
        if( length < size || length - size > score ) continue;

        best = cur;
        score = length - size;
        if( score == 0 || !bestFit ) break;
    }
    
    if( !best ) return NO_MEMBLOCK;
	
	uptr base = AlignUp(best->Base, align);
	if( Aligned(best->Base, align) ) {
		best->Base += size;
	}else{
		uptr end = best->End;
		best->End = base;
		if( base + size != end ) __ToRBAllocator( self, (MemoryBlock){base + size, end} );
	}

    if( best->End == best->Base ) ToCache( FromRBTree(free, best) );
    return MemoryOfSize(base, size);
}


int Compare( Entry* a, uptr* b ) {
    return (a->End > *b) - (*b > a->End);
}


RBAllocator* RBAllocatorNew( void* begin, Cache* cache ) {
    RBAllocator* self = begin;
    RBTreeNew( &self->Free, offsetof(Entry, Index), offsetof(Entry, End), Compare );
    self->Cache = cache;
    return self;
}


void RBAllocatorSetCache( RBAllocator* self, Cache* newCache ) {
	self->Cache = newCache;
}