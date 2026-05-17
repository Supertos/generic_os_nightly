/* Supertos Industries (2012 - 2026) */
#include "Base.h"

#include "Concurrency/RWLock.h"
#include "Structures/List.h"

#include "SLAB.h"
#include "Cache.h"

typedef struct {
	ListIndex Index;
	u16 WaitsSynchro; // Protects double-free stuff.
	bool Listed;
} CacheMeta;


static inline SLAB* MetaSLAB( CacheMeta* meta ) {
	return (SLAB*)AlignDown(meta, PAGE);
}


static inline bool AnnounceSLABUpdate( CacheMeta* meta ) {
	return FetchAdd16(&meta->WaitsSynchro, 1) == 0;
}


void UpdateSLABState( Cache* self, volatile CacheMeta* meta ) {
	ACQUIRE_WRITER_AUTO(&self->InUse);
	SLAB* slab = MetaSLAB(meta);

	bool exhausted = SLABExhausted(slab), empty = SLABEmpty(slab);
	bool mustList = !exhausted && !empty, mustFree = empty;

	if( mustList != meta->Listed ) {
		mustList ? ToList(&self->SLABs, meta) : FromList(&self->SLABs, meta);
		meta->Listed = mustList;
	}

	meta->WaitsSynchro = 0;
	if( mustFree ) self->Free( self->Allocator, (MemoryBlock){MetaSLAB(meta), MetaSLAB(meta) + PAGE} );
}


CacheMeta* NewCacheSLAB( Cache* self ) {
	void* ptr = self->Allocate(self->Allocator, PAGE, 1).Base;
	if( !ptr ) KernelCrashUndBurn(self->Mark);
	
	SLAB* slab = SLABNew( ptr, ptr, self->BinSize, sizeof(CacheMeta) );
	CacheMeta* meta = SLABMeta(slab);
	*meta = (CacheMeta){0};
	return meta;
}


void* FromCache( Cache* self ) {
	RWLock* lock = &self->InUse;
	void* out = NULL;

	while( !out ) {
		AcquireReader(lock);
		CacheMeta* meta = ListFirst(&self->SLABs);
		ReleaseReader(lock);

		if( !meta ) meta = NewCacheSLAB(self);
		
		AcquireReader(lock);

		SLAB* slab = MetaSLAB(meta);
		bool wasEmpty = SLABEmpty(slab);
		out = FromSLAB(slab);
		bool exhausted = SLABExhausted(slab);

		bool willUpdate = (out && wasEmpty || exhausted) && AnnounceSLABUpdate(meta);

		ReleaseReader(lock);
		if( willUpdate ) UpdateSLABState(self, meta);
	}
	return out;
}


void ToCache( Cache* self, void* addr ) {
	if( !addr ) return;
	AcquireReader(&self->InUse);
	
	SLAB* slab = AlignDown(addr, PAGE);
	CacheMeta* meta = SLABMeta(slab);

	bool wasExhausted = SLABExhausted(slab);
	ToSLAB(slab, addr);
	bool empty = SLABEmpty(slab);

	bool willUpdate = (wasExhausted || empty) && AnnounceSLABUpdate(meta);
	
	ReleaseReader(&self->InUse);
	if( willUpdate ) UpdateSLABState(self, meta);
}


Cache* CacheNew( void* begin, u16 binSize, char* mark ) {
	Cache* cache = begin;
	*cache = (Cache){ .BinSize = binSize, .Mark = mark };
	ListNew( &cache->SLABs, offset_of(CacheMeta, Index) );
	return cache;
}


Cache* CacheSetAllocator( Cache* self, void* allocator, AllocateFunction allocate, FreeFunction free ) {
	self->Allocator = allocator;
	self->Allocate = allocate;
	self->Free = free;
	return self;
}
