/* Supertos Industries (2012 - 2026) 5432 */

#include "VRegion.h"

#define INCLUDE_SIZE sizeof(VirtualInclude)


void* MapMemory( VirtualRegion* region, usize size, char* mode ) {
    return region->Interface->OnMap( region->Interface->Object, region->Owner, size, region->Allocator );
}


void UnmapMemory( VirtualRegion* region, uptr vBegin, usize size ) {
    VirtualPurge( region->Owner, MemoryOfSize( vBegin, AlignUp(size, PAGE) ), region->Interface );
}


VirtualInclude* VRegionInclude( VirtualSpace* self, VirtualRegion* region, MemoryBlock base, char* mode ) {
    ACQUIRE_READER_AUTO(&region->ReadLock);

    VirtualInclude* include = New( self->Service, INCLUDE_SIZE );

    if( !ValidMemory(base) ) base = FromVirtualSpace( self->Allocator, MemorySize(region->Memory), PAGE );

    VirtualBlit( region->Owner, self, region->Memory.Base, base, region->Interface, mode );
    InvalidateVirtualSpace(VNULL, self->PCID);

    *include = (VirtualInclude){ .Memory = base, .Region = region, .Version = region->Version };
    
    VirtualIncludeList* list = self->Includes;
    ACQUIRE_WRITER_AUTO(&list->Lock);
    ToRBTree(&list->Tree, include);

    FetchAdd32(&region->ExternalReferences, 1);
    return include;
}


void VRegionDeclude( VirtualSpace* self, VirtualInclude* include ) {
    VirtualRegion* region = include->Region;

    VirtualPurge( self, include->Memory, region->Interface );
    InvalidateVirtualSpace(VNULL, self->PCID);

    VirtualIncludeList* list = self->Includes;
    ACQUIRE_WRITER_AUTO(&list->Lock);
    FromRBTree(&list->Tree, include);

    Dispose( self->Service, include, sizeof(VirtualInclude) );
    FetchSub32(&region->ExternalReferences, 1);
}


static inline RBAllocator* RBAllocatorCreate( CPUMemoryService* service, LocalVMA* source ) {
    VirtualSpaceVMA* vma = New( service, sizeof(VirtualSpaceVMA) );
    RBAllocator* allocator = New( service, sizeof(RBAllocator) );

    return VirtualSpaceVMANew( vma, allocator, CacheOfSize(service, RBAllocatorCacheSize()), source );
}


VirtualRegion* VRegionNew( VirtualSpace* space, usize size, usize align, MapInterface* interface ) {
    VirtualRegion* region = New(space->Service, sizeof(VirtualRegion) );
    
    MemoryBlock block = FromVirtualSpace(space->Allocator, size, align);

    *region = (VirtualRegion) {
        .Allocator = RBAllocatorCreate(space->Service, space->Service->VMA), 
        .Memory = block, .ExternalReferences = 0,
        .Interface = interface,
        .Owner = space, .Version = 0
    };

    ToVirtualSpace( region->Allocator, block );
    return region;
}


int VSpaceIncludeCompare( VirtualInclude* entry, uptr* addr ) {
    u64 base = entry->Memory.Base;
    return (base > *addr) - (entry->Memory.End < *addr);
}


VirtualSpace* VSpaceNew( CPUMemoryService* self, u16 pcid, u64 ownerCore ) {
    VirtualSpace* space = New( self, sizeof(VirtualSpace) );
    VirtualIncludeList* includes = New( self, sizeof(VirtualIncludeList) );
    
    RBTreeNew( &includes->Tree, offset_of(VirtualInclude, Index), offset_of(VirtualInclude, Memory.Base), VSpaceIncludeCompare );

    void* root = self->Allocate(self->Allocator, PTE_ROOT_SIZE, PTE_ROOT_ALIGN).Base;
    *space = (VirtualSpace){ 
        .Allocator = RBAllocatorCreate(self, self->VMA),
        .CR3 = (uptr)root >> PTE_ROOT_EXP, 
        .Service = self,
        .Includes = includes, 
        .PCID = pcid 
    };

    ACQUIRE_WRITER_AUTO(&self->SpaceTreeLock);
    ToRBTree(&self->VirtualSpaces, space);
    return space;
}


static inline void DisposeRBAllocator( CPUMemoryService* service, RBAllocator* allocator ) {
    Dispose( service, &allocator->Free, sizeof(RBAllocator) );
    Dispose( service, allocator, sizeof(VirtualSpaceVMA) );
}


void VRegionDispose( VirtualRegion* region ) {
    if( region->ExternalReferences > 0 ) KernelCrashUndBurn("Attempt to dispose VirtualRegion in-use!");
    CPUMemoryService* service = region->Owner->Service;
    DisposeRBAllocator( service, region->Allocator );
    Dispose( service, region, sizeof(VirtualRegion) );
}


void VSpaceDispose( VirtualSpace* space ) {
    VirtualInclude* cur;
    while( cur = RBTreeRoot(space->Includes) ) VRegionDeclude(space, cur);
    
    CPUMemoryService* service = space->Service;
    service->Free( service->Allocator, MemoryOfSize(space->CR3, PTE_ROOT_SIZE) );

    Dispose( space->Service, space->Includes, sizeof(RBTree) );
    DisposeRBAllocator( space->Service, space->Allocator );
    Dispose( space->Service, space, sizeof(VirtualSpace) );
}


VirtualInclude* TryUpgradeInclude( VirtualInclude* include, char* mode ) {
    VirtualRegion* region = include->Region;
    AcquireReader(&region->ReadLock);
    bool versionChanged = region->Version != include->Version;
    ReleaseReader(&region->ReadLock);

    if( versionChanged ) {
        MemoryBlock block = include->Memory;
        VirtualSpace* target = include->Region->Owner;
        VRegionDeclude(target, include);
        include = VRegionInclude(target, region, block, mode);
    }

    return versionChanged ? include : NULL;
}


VirtualRegion* SpaceAddressToRegion( VirtualSpace* self, uptr addr ) {
    VirtualIncludeList* list = self->Includes;
    ACQUIRE_READER_AUTO(&list->Lock);
    VirtualInclude* include = SearchRBTree(&list->Tree, &addr);
    return include ? include->Region : NULL;
}