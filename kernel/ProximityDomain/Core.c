/* Supertos Industries (2012 - 2026) */

#include "Core.h"

usize CoreMetaSize() {
	return sizeof(Core);
}


Core* CoreMeta() {
	return ThreadLocalValue();
}


Cache* CacheOfSize( Core* self, usize size ) {
	if( size > CACHELINE || size < CACHE_BASE ) KernelCrashUndBurn("Invalid allocation size.");
	
	usize pwr = LastSetBit(size);
	if( size > (1 << pwr) ) pwr++;
	
	return &self->Caches[pwr - LastSetBit(CACHE_BASE)];
}


Core* CoreNew( void* base, MemoryDomain* owner, u32 smpID, usize physMemory ) {
	Core* self = base;
	
	*self = (Core){ .SMPID = smpID, .Owner = owner, .Self = base };
	 
	for( usize i = 0; i < CACHE_COUNT; i++ )
		CacheNew( &self->Caches[i], CACHE_BASE << i, (Allocator){owner, FromAnyDomain, ToDomains} );
	
	RBTreeNew( &self->SpaceTree, VSpaceIndexOffset(), VSpaceRootOffset(), VSpaceCompare );
	
	VSpace* kernelSpace = FromCache( CacheOfSize(self, VSpaceSize()) );
	VSpaceNew(kernelSpace, 0, self);
	VRegion* dummy = FromCache( CacheOfSize(self, VRegionSize()) );
	
	// First FromVSpace call will create (and propagate it to LocalVMA as shared) in GlobalVMA [0, physMemory] block from bump 
	// Subsequential calls from other kernelSpaces will draw from LocalVMA.
	FromVSpace( VSpaceAllocator(kernelSpace), physMemory, 1 );
	
	VRegionNew(dummy, kernelSpace, Memory(NULL, physMemory), NULL);
	VirtualMap(kernelSpace, NULL, Memory(NULL, physMemory), "", true, NULL);
	
	self->KernelSpace = kernelSpace;
}


Allocator GetFramesAllocator( Core* self ) {
	return (Allocator){ .Self = self->Owner, .To = ToDomains, .From = FromAnyDomain };
}


void RegisterCoreVSpace( Core* self, void* item ) {
	ToRBTree( &self->SpaceTree, item );
}

void UnregisterCoreVSpace( Core* self, void* item ) {
	FromRBTree( &self->SpaceTree, item );
}

LocalVMA* CoreVMA( Core* self ) {
	return DomainVMA(self->Owner);
}