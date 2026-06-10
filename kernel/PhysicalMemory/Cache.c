/* Supertos Industries (2012 - 2026) */
#include "Cache.h"

#include "Structures/Bitmap.h"

#define FREE false
#define USED true
#define USED_WHOLE ~0ULL

typedef struct {
    u16 BinCount;
	u16 BinSize;
	u16 Synchro;
	u16 Present;
    void* Owner;
	ListIndex Index;
	Bitmap Slots[]
} SLAB;


static inline usize MaxSlots( usize binSize ) {
	return (PAGE - AlignUp(sizeof(SLAB), CACHELINE)) / binSize;
}


static inline usize Slots( usize binSize ) {
	return PAGE / binSize;
}


SLAB* SLABNew( uptr begin, usize binSize, void* owner ) {
	usize slots = PAGE / binSize, maxSlots = MaxSlots(binSize);
    
	SLAB* self = begin;
    *self = (SLAB){ .BinCount = maxSlots, .BinSize = binSize, .Owner = owner };

    FillMemory( self->Slots, BITMAP_SIZE(slots), USED_WHOLE );
	SetBits( self->Slots, slots - maxSlots, maxSlots, FREE );
    return self;
}


void* FromSLAB( SLAB* self ) {
    if( self->BinCount == 0 ) return NULL;
	
	usize slot = TrySetBitmapAtomic( self->Slots, BITMAP_SIZE(Slots(self->BinSize)), 1 );
	if( slot == NOID ) return NULL;
	FetchSub16(&self->BinCount, 1);
	
	return (uptr)self + slot * self->BinSize;
}


void ToSLAB( uptr addr ) {
    if( addr == NULL ) return;
	
	SLAB* self = AlignDown(addr, PAGE);
	usize slot = (addr % PAGE) / self->BinSize;
	
	SetBits( self->Slots, slot, 1, FREE );
    FetchAdd16(&self->BinCount, 1);
}


void UpdateSLAB( Cache* self, SLAB* slab ) {
	if( FetchAdd16( &slab->Synchro, 1 ) > 0 ) return;
	ACQUIRE_WRITER_AUTO(&self->InUse);
	
	bool empty = (slab->BinCount == 0), full = (slab->BinCount == MaxSlots(slab->BinSize));
	bool mustList = !empty && !full, mustFree = empty;
	
	if( mustList != slab->Present ) {
		mustList ? ToList(&self->Fragments, slab) : FromList(&self->Fragments, slab);
		slab->Present = mustList;
	}
	
	slab->Synchro = 0;
	architecture_barrier;
	compiler_barrier;
	
	if( mustFree ) To( self->Source, MemoryOfSize(slab, PAGE) );
}


SLAB* NewCacheSLAB( Cache* self ) {
	MemoryBlock mem = From( self->Source, PAGE, PAGE );
	if( !ValidMemory(mem) ) return NULL;
	return SLABNew(mem.Base, self->BinSize, self);
}


void* FromCache( Cache* self ) {
	RWLock* lock = &self->InUse;
	void* out = NULL;
	
	while( !out ) {
		AcquireReader(lock);
		SLAB* cur = ListFirst(&self->SLABs);
		
		if( !cur ) {
			ReleaseReader(lock);
			// There is indeed no protection from multiple cores creating new SLABs.
			// It's intended, think of this as high-load caching.
			cur = NewCacheSLAB(self);
			if( !cur ) return NULL;
			AcquireReader(lock);
		}
		
		bool wasFull = (cur->BinCount == MaxSlots(cur->BinSize));
		out = FromSLAB(cur);
		bool empty = (cur->BinCount == 0);
		
		bool willUpdate = (wasFull || empty);
		ReleaseReader(lock);
		if( willUpdate ) UpdateSLAB(self, cur);
	}
	return out;
}


void ToCache( uptr addr ) {
	SLAB* slab = AlignDown(addr, PAGE);
	Cache* self = slab->Owner;
	AcquireReader(&self->InUse);
	
	bool wasEmpty = (slab->BinCount == 0);
	ToSLAB(addr);
	bool full = (slab->BinCount == MaxSlots(slab->BinSize));
	
	bool willUpdate = (wasEmpty || full);
	ReleaseReader(&self->InUse);
	if( willUpdate ) UpdateSLAB(self, slab);
}


Cache* CacheNew( void* base, u16 binSize, Allocator source ) {
	Cache* self = base;
	*self = (Cache){ .BinSize = binSize, .Source = source };
	ListNew( &self->Fragments, offsetof(SLAB, Index) );
	return self;
}