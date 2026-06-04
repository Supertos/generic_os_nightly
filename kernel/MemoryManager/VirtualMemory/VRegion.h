/* Supertos Industries (2012 - 2026) 
 * VirtualSpace is modified only by owner CPU.
 * 
 * VirtualRegions system utilises epoch-based reclamation required by kernel structures mapped into userspace.
 * VirtualFamily describes region in VirtualSpace, that shall be blit onto other VirtualSpaces.
 * VirtualRegion describes single version, and can only be modified/blit onto when is the newest.
 * Outdated VirtualRegions (those that have predecessor (front insertion)) are alive as long as single VirtualSpace has it included.
 * Upon last reference lost (and virtual mappings purged + physics freed if THEIR reference count is zero), VirtualRegion is destroyed.
 * 
 * VirtualSpace can include only newest VirtualRegion.
 * Newest VirtualRegion has at least one reference (that is, owner VirtualSpace), and will never be removed unless VSpaceDispose is called.
 */

#pragma once

#include <assert.h>

#include "Concurrency/RWLock.h"
#include "Concurrency/Spinlock.h"
#include "Base.h"
#include "Paging.h"
#include "VAllocator.h"
#include "VSpace.h"
#include "MemoryManager/Service.h"

#define BASE_BITS (VIRT_BITS - __PAGE_EXP)
#define SIZE_LOW_BITS (bitsof(u64) - BASE_BITS)
#define SIZE_HIGH_BITS (VIRT_BITS - __PAGE_EXP - SIZE_LOW_BITS)

typedef struct {
    VirtualSpaceVMA* Allocator;
    VirtualSpace* Owner;
    MapInterface* Interface;
    MemoryBlock Memory;
    RWLock ReadLock;
    u16 Version;
    u32 ExternalReferences;
} VirtualRegion;

typedef struct {
    RBIndex Index;
    MemoryBlock Memory;
    u64 Version;
    VirtualRegion* Region;
} VirtualInclude;

static_assert( sizeof(VirtualRegion) <= 64, "VirtualRegion is too large for internal meta SLAB!" );
static_assert( sizeof(VirtualInclude) <= 64, "VirtualInclude is too large for internal meta SLAB!" );

void* MapMemory( VirtualRegion* region, usize pages, char* mode );

void UnmapMemory( VirtualRegion* region, uptr vBegin, usize pages );

static inline void VRegionLock( VirtualRegion* self ) { AcquireWriter(&self->ReadLock); }

static inline void VRegionRelease( VirtualRegion* self ) { ReleaseWriter(&self->ReadLock); }

VirtualInclude* VRegionInclude( VirtualSpace* self, VirtualRegion* region, MemoryBlock base, char* mode );

void VRegionDeclude( VirtualSpace* self, VirtualInclude* include );

VirtualRegion* VRegionNew( VirtualSpace* space, usize size, usize align, MapInterface* interface );

void VRegionDispose( VirtualRegion* region );

int VSpaceIncludeCompare( VirtualInclude* entry, uptr* addr );

VirtualSpace* VSpaceNew( CPUMemoryService* self, u16 pcid, u64 ownerCore );

void VSpaceDispose( VirtualSpace* space );

VirtualInclude* TryUpgradeInclude( VirtualInclude* include, char* mode );

VirtualRegion* SpaceAddressToRegion( VirtualSpace* self, uptr addr );