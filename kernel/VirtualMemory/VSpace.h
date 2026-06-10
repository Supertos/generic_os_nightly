/* Supertos Industries (2012 - 2026) 
 * VSpace - Abstraction layer over MMU mechanism commonly found on x86 and ARM architectures.
 * The main difference from typical VMM subsystems is responsibility pyramid:
 * User -> VMM (Locate region and its MapInterface) -> MapInterface (Execute policy over VMM) -> VMM (Mapping, unmapping, vm allocation)
 */
#pragma once

#include "Base.h"
#include "Architecture.h"

#include "Prototypes/ProximityDomain.h"
#include "Prototypes/MemoryManager.h"
#include "Prototypes/VirtualMemory.h"

#include "PhysicsInterface.h"

#include "VAllocator.h"

typedef struct {
	u64 CR3;
	u32 PCID;
	Core* Core;
    VSpaceVMA* Allocator;
	RBTree* Includes;
	RBIndex Index;
} VSpace;

static_assert( sizeof(VSpace) <= 64, "VSpace is too big for internal SLAB." );
static_assert( sizeof(VSpaceVMA) <= 64, "VSpaceVMA is too big for internal SLAB." );
static_assert( sizeof(RBAllocator) <= 64, "RBAllocator is too big for internal SLAB." );

void AssertCorrectCore( VSpace* self );

bool VirtualMap( VSpace* self, uptr pBase, MemoryBlock vTarget, char* mode, bool doHuge, MapInterface* interface );

void VirtualUnmap( VSpace* self, MemoryBlock vTarget, MapInterface* interface );

void VirtualBlit( VSpace* from, VSpace* to, uptr vSource, MemoryBlock vTarget, MapInterface* fromInterface, char* mode );

void VirtualPurge( VSpace* to, MemoryBlock vTarget, MapInterface* interface );

VInclude* VIncludeNew( void* base, MemoryBlock to, VRegion* source );

bool VRegionInclude( VSpace* self, VRegion* region, MemoryBlock to, char* mode );

void VRegionDeclude( VSpace* self, VInclude* include );

VSpace* VSpaceNew( void* base, u32 pcid, Core* core );

bool VSpaceDispose( VSpace* self );

VRegion* VSpaceRegion( VSpace* self, uptr addr );

void VSpaceTransfer( VSpace* self, Core* core );

void VSpaceEnter( VSpace* self, Core* core, bool flush );

usize VSpaceRoot( VSpace* self );

int VSpaceCompare( VSpace* self, uptr* a );

usize VSpaceRootOffset( VSpace* self );

usize VSpaceIndexOffset( VSpace* self );

VSpaceVMA VSpaceAllocator( VSpace* self );