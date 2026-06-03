/* Supertos Industries (2012 - 2026) */

#pragma once

#include "Base.h"

#include "Arena.h"
#include "Primitives/Cache.h"
#include "VirtualMemory/VAllocator.h"
#include "VirtualMemory/VSpace.h"
#define CACHES 3 // 64, 32, 16
#define MIN_CACHE 16

static_assert( CACHELINE == 64, "Cache count may be subotpimal." );

typedef struct {
    Cache Caches[CACHES];

    RBTree VirtualSpaces;
    LocalVMA* VMA;
    VirtualSpace* LocalKernelSpace;
    RWLock* SpaceTreeLock;
 
    AllocateFunction Allocate;
    FreeFunction Free;
    void* Allocator;
} CPUMemoryService;


static inline Cache* CacheOfSize( CPUMemoryService* self, usize size ) {
    if( size == sizeof(void*) ) KernelCrashUndBurn("Pointer size allocations are forbidden. Check for sizeof(type*).");
    if( size > CACHELINE ) KernelCrashUndBurn("Size > CACHELINE(64) is not supported.");
    usize pwr = LastSetBit(size / MIN_CACHE);
    while( size > (1 << pwr) * MIN_CACHE ) pwr += 1;
    return &self->Caches[pwr];
}

static inline void* New( CPUMemoryService* self, usize size ) {
    return FromCache(CacheOfSize(self, size));
}

static inline void Dispose( CPUMemoryService* self, void* addr, usize size ) {
    ToCache( CacheOfSize(self, size), addr );
}

static inline CPUMemoryService* CPUMemoryServiceNew( void* base, Arena* arena ) {
    CPUMemoryService* self = base;
    *self = (CPUMemoryService){ .Allocate = FromArenasKernel, .Free = ToArenas, .Allocator = arena };

    for( usize i = 1; i < CACHES; i++ ) {
        CacheNew( &self->Caches[i], 16 << i, "ARENA CACHE" );
        CacheSetAllocator( &self->Caches[i], arena, FromArenasKernel, ToArenas );
    }

    RBTreeNew( &self->VirtualSpaces, offset_of(VirtualSpace, Index), offset_of(VirtualSpace, CR3), VirtualSpaceComparator );   
}