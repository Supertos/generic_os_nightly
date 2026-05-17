/* Supertos Industries (2012 - 2026) 
 * VirtualSpace - Abstraction layer over MMU mechanism commonly found on x86 and ARM architectures.
 * The main difference from typical VMM subsystems is responsibility pyramid:
 * User -> VMM (Locate region and its MapInterface) -> MapInterface (Execute policy over VMM) -> VMM (Mapping, unmapping, vm allocation)
 */
#pragma once
typedef struct VirtualSpace VirtualSpace;
#include "Base.h"
#include "Paging.h"
#include "MemoryManager/Service.h"
#include "VAllocator.h"
#include "PhysicsInterface.h"

#define OWNER_CORE_BITS (bitsof(u64) - PCID_BITS)

typedef struct {
	RWLock Lock;
	RBTree Tree;
} VirtualIncludeList;

typedef struct {
	u64 CR3;
	u32 OwnerCore;
	u32 PCID;
	CPUMemoryService* Service;
    VirtualSpaceVMA* Allocator;
	VirtualIncludeList* Includes;
	RBIndex Index;
} VirtualSpace;

static_assert( sizeof(VirtualSpace) <= 64, "VirtualSpace is too big for internal SLAB." );
static_assert( sizeof(VirtualIncludeList) <= 64, "VirtualIncludeList is too big for internal SLAB." );
static_assert( sizeof(VirtualSpaceVMA) <= 64, "VirtualSpaceVMA is too big for internal SLAB." );
static_assert( sizeof(RBAllocator) <= 64, "RBAllocator is too big for internal SLAB." );


int VirtualSpaceComparator( VirtualSpace* space, usize* CR3 ) {
    return (space->CR3 > *CR3) - (space->CR3 < *CR3);
}

// To be called by MapInterface
bool VirtualMap( VirtualSpace* space, uptr pBase, MemoryBlock vTarget, char* mode, bool doHuge, MapInterface* interface );

// To be called by MapInterface
void VirtualUnmap( VirtualSpace* space, MemoryBlock vTarget, MapInterface* interface );

void VirtualBlit( VirtualSpace* from, VirtualSpace* to, uptr vSource, MemoryBlock vTarget, MapInterface* fromInterface, char* mode );

void VirtualPurge( VirtualSpace* to, MemoryBlock vTarget, MapInterface* interface );

static inline void ClaimVirtualSpace( VirtualSpace* space ) {
	space->OwnerCore = SMPID();
}
