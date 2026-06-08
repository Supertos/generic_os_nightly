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


Core* CoreNew( void* base, ProximityDomain* owner, u32 smpID ) {
	Core* self = base;
	
	*self = (CPUMemoryService){ .SMPID = smpID, .Owner = owner };
	
	for( usize i = 0; i < CACHE_COUNT; i++ ) {
		CacheNew( &self->Caches[i], CACHE_BASE << i, "CORE CACHE" );
		CacheSetAllocator( &self->Caches[i], owner, FromAnyDomain, ToDomain );
	}
	
	RBTreeNew( &self->SpaceTree, VSpaceIndexOffset(), VSpaceRootOffset(), VSpaceCompare );
}


Allocator GetFramesAllocator( Core* self ) {
	return (Allocator){ .Self = self->Owner, .To = ToDomain, .From = FromAnyDomain };
}


void RegisterCoreVSpace( Core* self, void* item ) {
	ToRBTree( &self->SpaceTree, item );
}

void UnregisterCoreVSpace( Core* self, void* item ) {
	FromRBTree( &self->SpaceTree, item );
}

LocalVMA* CoreVMA( Core* self ) {
	return ProximityDomainVMA(self->Owner);
}