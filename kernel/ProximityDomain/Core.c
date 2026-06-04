/* Supertos Industries (2012 - 2026) */

#include "Core.h"

#define NEW(type, ...) ({})

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
}


// static inline void* New( CPUMemoryService* self, usize size ) {
    // return FromCache(CacheOfSize(self, size));
// }

// static inline void Dispose( CPUMemoryService* self, void* addr, usize size ) {
    // ToCache( CacheOfSize(self, size), addr );
// }
