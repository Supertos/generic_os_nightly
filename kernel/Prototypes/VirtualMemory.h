/* Supertos Industries (2012 - 2026) 
 * MemoryManager Interface.
 * Collection of forward declarations used outside of MemoryManager.
 */
#pragma once
 
#include "Base.h"

// === //
typedef struct GlobalVMA GlobalVMA;

GlobalVMA* GlobalVMANew( void* begin, Cache* sCache, Cache* eCache );

// === //
typedef struct LocalVMA LocalVMA;

LocalVMA* LocalVMANew( void* begin, Cache* sCache, GlobalVMA* source );

MemoryBlock FromExclusiveVMA( VSpaceVMA* self, usize size, usize align );

void ToExclusiveVMA( VSpaceVMA* self, MemoryBlock entry );

// === //
typedef struct VSpaceVMA VSpaceVMA;

VSpaceVMA* VSpaceVMANew( void* begin, RBAllocator* allocator, Cache* eCache, LocalVMA* source );

RBAllocator* VSpaceVMAAllocator( VSpaceVMA* self );

MemoryBlock FromVSpace( VSpaceVMA* self, usize size, usize align );

void ToVSpace( VSpaceVMA* self, MemoryBlock entry );

// === //
typedef struct VSpace VSpace;
typedef struct VRegion VRegion;

bool VirtualMap( VSpace* self, uptr pBase, MemoryBlock vTarget, char* mode, bool doHuge, MapInterface* interface );

void VirtualUnmap( VSpace* self, MemoryBlock vTarget, MapInterface* interface );

bool VRegionInclude( VSpace* self, VRegion* region, MemoryBlock to, char* mode );

void VRegionDeclude( VSpace* self, VInclude* include );

VSpace* VSpaceNew( void* base, u32 pcid, Core* core );

bool VSpaceDispose( VSpace* self );

VRegion* VSpaceRegion( VSpace* self, uptr addr );

void VSpaceTransfer( VSpace* self, Core* core );

void VSpaceEnter( VSpace* self, Core* core, bool flush );

usize VSpaceRoot( VSpace* self );

int VSpaceCompare( VSpace* self, uptr* a );

usize VSpaceRootOffset();

usize VSpaceIndexOffset();

// === //
VRegion* VRegionNew( void* base, VSpace* space, MemoryBlock where, MapInterface* interface );

bool VRegionDispose( VRegion* self );

bool OutdatedRegionVersion( VRegion* self, RegionVersion compare );

MemoryBlock MapAtVRegion( VRegion* self, MemoryBlock physics, usize align, char* mode, bool allowHuge );

void UnmapAtVRegion( VRegion* self, MemoryBlock virtual );

void LockVRegion( VRegion* self );

void ReleaseVRegion( VRegion* self );

// === //
