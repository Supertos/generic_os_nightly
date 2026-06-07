/* Supertos Industries (2012 - 2026) */

#include "VSpace.h"
#include "VRegion.h"
#include "Architecture.h"

#include "Prototypes/ProximityDomain.h"
#include "Primitives/RBAllocator.h"

#include "Prototypes/MapInterface.h"

typedef struct {
	RBIndex Index;
    MemoryBlock Memory;
    RegionVersion Version;
    VRegion* Region;
} VInclude;


static_assert( sizeof(VInclude) <= 64, "VIncludes is too big for internal SLAB." );


void AssertCorrectCore( VSpace* self ) {
    if( SMPID() != CoreSMPID(self->Core) )
        KernelCrashUndBurn("Can't modify VSpace from another core.");
}


static inline usize SelectAlign( uptr pBase, uptr vBase, usize pages ) {
    usize align = (pBase | vBase);
    usize aligns[] = {PAGE, HUGEPAGE, GIGAPAGE};

    for( usize i = 1; i < 3; i++ )
        if( align % aligns[i] != 0 || pages < aligns[i] / PAGE ) return aligns[i - 1];

    return GIGAPAGE;
}


bool VirtualMap( VSpace* self, uptr pBase, MemoryBlock vTarget, char* mode, bool doHuge, MapInterface* interface ) {
    AssertCorrectCore(self);

    uptr vEnd = vTarget.End, vBase = vTarget.Base, length = vEnd - vBase;
    if( !pBase || vBase == VNULL ) return false;

    PageWalker walk = WalkerNew( self->CR3, GetFramesAllocator(self->Core) );

    while( vEnd > vBase ) {
        usize align = SelectAlign(pBase, vBase, length / PAGE);

        PTE* entry = WalkerEntry( &walk, vBase, align );
        if( !entry ) return false;

        WalkerSetEntry( &walk, pBase, mode );
		
		vBase += align;
		pBase += align;
		length -= align;
    }

    return true;
}


void VirtualUnmap( VSpace* self, MemoryBlock vTarget, MapInterface* interface ) {
    AssertCorrectCore(self);
    usize vEnd = vTarget.End, vBase = vTarget.Base;
    if( vBase == VNULL ) return;

    PageWalker walk = WalkerNew( self->CR3, GetFramesAllocator(self->Core) );
    
	PTE* entry = WalkerEntry( &walk, vBase, PAGE );
    if( !PTEAddr(entry) ) entry = WalkerSkipNULLs(&walk, vEnd);

	while( entry && WalkerCurAddress(&walk) < vEnd ) {
		WalkerSetEntry( &walk, NULL, "" );
		entry = WalkerSkipNULLs(&walk, vEnd);
	}
}


void VirtualBlit( VSpace* from, VSpace* to, uptr vSource, MemoryBlock vTarget, MapInterface* fromInterface, char* mode ) {
    AssertCorrectCore(from);
	
    uptr vEnd = vTarget.End, vBase = vTarget.Base;
    if( vEnd == vBase ) return;

    PageWalker walk = WalkerNew( from->CR3, NO_ALLOCATOR ), 
			   tmpWalk = WalkerNew( to->CR3, NO_ALLOCATOR );
	PTE* entry = WalkerEntry( &walk, vSource, PAGE );

    do {
        if( !PTEAddr(entry) ) entry = WalkerSkipNULLs(&walk, vEnd);
        usize vCur = vBase + WalkerCurAddress(&walk) - vSource;
        
        fromInterface->OnMapAt( fromInterface->Object, to, PTEAddr(entry), vCur, WalkerStep(&walk), mode );
        if( mode ) continue;

        WalkerCopyAttributes( entry, WalkerEntry(&tmpWalk, vCur, PAGE) );
    } while( WalkerCurAddress(&walk) < vEnd && (entry = WalkerNext(&walk)) );
}


void VirtualPurge( VSpace* to, MemoryBlock vTarget, MapInterface* interface ) {
    AssertCorrectCore(to);
    PageWalker walk = WalkerNew( to->CR3, GetFramesAllocator(to->Core) );
    uptr vEnd = vTarget.End, vBase = vTarget.Base;

    PTE* cur = WalkerEntry(&walk, vBase, PAGE);
    uptr vContigous = WalkerCurAddress(&walk), pContigous = PTEAddr(cur);

    while( vContigous < vEnd ) {
        bool adjacent = PTEAddr(cur) && PTEAddr( cur=WalkerNext(&walk) );

        uptr vRange = WalkerCurAddress(&walk) - vContigous;
        if( adjacent && PTEAddr(cur) - pContigous == vRange ) continue;

		interface->OnUnmap( interface->Object, to, pContigous, vContigous, vRange );
        
        if( !PTEAddr(cur) ) cur = WalkerSkipNULLs(&walk, vEnd);
		vContigous = WalkerCurAddress(&walk);
		pContigous = PTEAddr(cur);
	}
}


VInclude* VIncludeNew( void* base, MemoryBlock to, VRegion* source ) {
	VInclude* self = base;
	*self = (VInclude){ 
		.Memory = to,
		.Region = source,
	};
	
	return self;
}


bool VRegionInclude( VSpace* self, VRegion* region, MemoryBlock to, char* mode ) {
	AssertCorrectCore(self);
	
	if( !ValidMemory(to) ) to = FromVSpace( self->Allocator, region->Memory.Size, PAGE );
	if( !ValidMemory(to) ) return false;
		
	VInclude* include = NEW(VInclude, to, region);
	ToRBTree(self->Includes, include);
	
	VRegionMap(region, self, to, &include->Version, mode);
	return true;
}


void VRegionDeclude( VSpace* self, VInclude* include ) {
	AssertCorrectCore(self);
	
	VRegionUnmap(include->Region, self, include->Memory);
	
	FromRBTree(self->Includes, include);
	DISPOSE(include);
}


int VIncludeCompare( VInclude* entry, uptr* addr ) {
    u64 base = entry->Memory.Base;
    return (base > *addr) - (entry->Memory.End < *addr);
}


VSpace* VSpaceNew( void* base, u32 pcid, Core* core ) {
	VSpace* out = base;
	
	MemoryBlock root = From( GetFramesAllocator(core), PTE_ROOT_SIZE, PTE_ROOT_ALIGN );
	if( !ValidMemory(root) ) return NULL;
	
	RBAllocator* rbAlloc = NEW_REMOTE( core, RBAllocator, CoreCache(core, RBAllocatorCacheSize()) );
	
	*out = (VSpace) {
		.CR3 = root.Base,
		.PCID = pcid,
		.Core = core,
		.Allocator = NEW_REMOTE( core, VSpaceVMA, rbAlloc, CoreVMA(core) ),
		.Includes = NEW_REMOTE( core, RBTree, offsetof(VInclude, Index), offsetof(VInclude, Memory.Base), VIncludeCompare )
	};
	
	RegisterCoreVSpace(core, out);
	return out;
}


bool VSpaceDispose( VSpace* self ) {
	UnregisterCoreVSpace(self->Core, self);
	
	VirtualInclude* inc;
	while( inc = RBTreeRoot(self->Includes) )
		VRegionDeclude( self, inc );
	
	DISPOSE( VSpaceVMAAllocator(self->Allocator) );
	DISPOSE( self->Allocator );
	DISPOSE( self->Includes );
	
	return true;
}


VRegion* VSpaceRegion( VSpace* self, uptr addr ) {
	AssertCorrectCore(self);
    VInclude* include = SearchRBTree(self->Includes, &addr);
    return include ? include->Region : NULL;
}


void VSpaceTransfer( VSpace* self, Core* core ) {
	AssertCorrectCore(self);
	RBAllocatorSetCache(self->Allocator, CoreCache(core, RBAllocatorCacheSize()));

	VInclude* cur = NULL;
	while( cur = RBTreeNext(self->Includes, cur) )
		if( VRegionSpace(cur) == self ) VRegionTransfer(cur, core);
	
	compiler_barrier;
	architecture_barrier;
	
	UnregisterCoreVSpace( self->Core, self );
	RegisterCoreVSpace( core, self );
	self->Core = core;
}


void VSpaceEnter( VSpace* self, Core* core, bool flush ) {
	SetVirtualSpace( self->CR3, self->PCID, flush );
}


usize VSpaceRoot( VSpace* self ) {
	return self->CR3;
}


int VSpaceCompare( VSpace* self, uptr* a ) {
	return (self->CR3 > *a) - (self->CR3 < *a);
}


usize VSpaceRootOffset() {
	return offsetof(VSpace, CR3);
}


usize VSpaceIndexOffset() {
	return offsetof(VSpace, Index);
}


VSpaceVMA VSpaceAllocator( VSpace* self ) {
	return self->Allocator;
}