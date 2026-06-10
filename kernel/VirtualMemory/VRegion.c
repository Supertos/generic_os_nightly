/* Supertos Industries (2012 - 2026) */
#include "VRegion.h"

#include "PhysicsInterface.h"

void VRegionMap( VRegion* self, VSpace* trg, MemoryBlock to, RegionVersion* version, char* mode ) {
	ACQUIRE_READER_AUTO(&self->ReadLock);
	VirtualBlit( self->Space, trg, self->Memory.Base, to, self->Interface, mode );
	
	*version = self->Version;
	FetchAdd32(&self->ExternalReferences, 1);
	InvalidateVirtualSpace(VNULL, trg->PCID);
}


void VRegionUnmap( VRegion* self, VSpace* trg, MemoryBlock where ) {
	ACQUIRE_READER_AUTO(&self->ReadLock);
	VirtualPurge( self, where, self->Interface );
	
	FetchSub32(&self->ExternalReferences, 1);
	InvalidateVirtualSpace(VNULL, trg->PCID);
}


VRegion* VRegionNew( void* base, VSpace* space, MemoryBlock where, MapInterface* interface ) {
	AssertCorrectCore(space);
	VRegion* self = base;
	
	if( !ValidMemory(where) ) return NULL;
	
	Core* core = space->Core;
	
	*self = (VRegion){
		.Allocator = NEW( RBAllocator, CoreCache(CoreMeta(), RBAllocatorCacheSize()) ),
		.Interface = interface,
		.Space = space,
		.Memory = where
	};
	
	ToRBAllocator(self->Allocator, where);
	
    return self;
}


bool VRegionDispose( VRegion* self ) {
	AssertCorrectCore(self->Space);
    if( self->ExternalReferences > 0 ) return false;
	
	VirtualPurge( self->Space, self->Memory, self->Interface );
	ToVSpace( self->Space, self->Memory );
	
	DISPOSE(self->Allocator);
	return true;
}


bool OutdatedRegionVersion( VRegion* self, RegionVersion compare ) {
	return !TryCAS16(&self->Version, &compare, compare);
}


MemoryBlock MapAtVRegion( VRegion* self, MemoryBlock physics, usize align, char* mode, bool allowHuge ) {
	AssertCorrectCore(self->Core);
	
	MemoryBlock virtual = FromRBAllocatorFirst( self->Allocator, MemorySize(physics), align );
	if( !ValidMemory(virtual) ) return NO_MEMBLOCK;
	
	VirtualMap( self->Space, physics.Base, virtual, mode, allowHuge, self->Interface );
	return virtual;
}


void UnmapAtVRegion( VRegion* self, MemoryBlock virtual ) {
	AssertCorrectCore(self->Core);
	VirtualPurge( self->Space, virtual, self->Interface );
}


void LockVRegion( VRegion* self ) {
	AssertCorrectCore(self->Core);
	AcquireWriter(&self->ReadLock);
}


void ReleaseVRegion( VRegion* self ) {
	AssertCorrectCore(self->Core);
	FetchAdd16(&self->Version, 1);
	ReleaseWriter(&self->ReadLock);
}


void VRegionTransfer( VRegion* self, Core* core ) {
	RBAllocatorSetCache(self->Allocator, CoreCache(core, RBAllocatorCacheSize()));
}


VSpace* VRegionSpace( VRegion* self ) {
	return self->Space;
}