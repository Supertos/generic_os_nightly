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

#include "Base.h"

#include "Concurrency/RWLock.h"
#include "Concurrency/Spinlock.h"

#include "MemoryManager/Primitives/RBAllocator.h"
#include "VSpace.h"

typedef u16 RegionVersion;

typedef struct {
    RBAllocator* Allocator;
    VSpace* Space;
    MapInterface* Interface;
    MemoryBlock Memory;
    RWLock ReadLock;
    RegionVersion Version;
    u32 ExternalReferences;
} VRegion;


static_assert( sizeof(VRegion) <= 64, "VRegion is too large for internal meta SLAB!" );

void VRegionMap( VRegion* self, VSpace* trg, MemoryBlock to, RegionVersion* version, char* mode );

void VRegionUnmap( VRegion* self, VSpace* trg, MemoryBlock where );

VRegion* VRegionNew( void* base, VSpace* space, MemoryBlock where, MapInterface* interface );

bool VRegionDispose( VRegion* self );

bool OutdatedRegionVersion( VRegion* self, RegionVersion compare );

MemoryBlock MapAtVRegion( VRegion* self, MemoryBlock physics, usize align, char* mode, bool allowHuge );

void UnmapAtVRegion( VRegion* self, MemoryBlock virtual );

void LockVRegion( VRegion* self );

void ReleaseVRegion( VRegion* self );

void VRegionTransfer( VRegion* self, Core* core );

VSpace* VRegionSpace( VRegion* self );