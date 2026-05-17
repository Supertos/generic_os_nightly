/* Supertos Industries (2012 - 2026) */

#include "VSpace.h"
#include "Paging.h"


static inline void AssertCorrectCore( VirtualSpace* self ) {
    u64 CoreID = SMPID();
    if( CoreID != self->OwnerCore ) 
        KernelCrashUndBurn("Inter-core PageTable modification attempt! VirtualSpace can be modified by its owner only.");
}


static inline usize SelectAlign( uptr pBase, uptr vBase, usize pages ) {
    usize align = (pBase | vBase);
    usize aligns[] = {PAGE, 1, HUGEPAGE, HUGE_PAGES, GIGAPAGE, GIGA_PAGES};

    for( usize i = 2; i < sizeof(aligns) / sizeof(usize); i += 2 )
        if( align % aligns[i] != 0 || pages < aligns[i + 1] ) return aligns[i - 2];

    return GIGAPAGE;
}


bool VirtualMap( VirtualSpace* space, uptr pBase, MemoryBlock vTarget, char* mode, bool doHuge, MapInterface* interface ) {
    AssertCorrectCore(space);

    uptr vEnd = vTarget.End, vBase = vTarget.Base, pages = (vEnd - vBase) / PAGE;
    CPUMemoryService* service = space->Service;
    if( !pBase || vBase == VNULL ) return false;

    PageWalker walk = WalkerNew( SpaceCR3(space), service->Allocate, service->Allocator, NULL );

    while( vEnd > vBase ) {
        usize align = SelectAlign(pBase, vBase, pages);

        PTE* entry = WalkerEntry( &walk, vBase, align );
        if( !entry ) return false;

        WalkerSetPTE( &walk, entry, pBase, mode, CANT_FREE );

        vBase += align;
        pBase += align;
        pages -= align / PAGE;
    }

    return true;
}


void VirtualUnmap( VirtualSpace* space, MemoryBlock vTarget, MapInterface* interface ) {
    AssertCorrectCore(space);
    usize vEnd = vTarget.End, vBase = vTarget.Base;
    CPUMemoryService* service = space->Service;
    if( vBase == VNULL ) return;

    PageWalker walk = WalkerNew( SpaceCR3(space), NULL, service->Allocator, service->Free );
    
	PTE* entry = WalkerEntry( &walk, vBase, PAGE );
    if( !entry || !PTEAddr(entry) ) entry = WalkerSkipNULLs(&walk, vEnd);

	while( entry && WalkerCurAddress(&walk) < vEnd ) {
        WalkerSetPTE( &walk, entry, NULL, "", CAN_FREE );
		entry = WalkerSkipNULLs(&walk, vEnd);
	}
}


void VirtualBlit( VirtualSpace* from, VirtualSpace* to, uptr vSource, MemoryBlock vTarget, MapInterface* fromInterface, char* mode ) {
    AssertCorrectCore(from);
    CPUMemoryService* service = to->Service;
    uptr vEnd = vTarget.End, vBase = vTarget.Base;
    if( vEnd == vBase ) return;

    PageWalker walk = WalkerNew( SpaceCR3(from), NULL, NULL, NULL );
	PTE* entry = WalkerEntry( &walk, vSource, PAGE );

    do {
        if( !entry || !PTEAddr(entry) ) entry = WalkerSkipNULLs(&walk, vEnd);
        usize offset = WalkerCurAddress(&walk) - vSource;
        
        fromInterface->OnMapAt( fromInterface->Object, to, PTEAddr(entry), vBase + offset, WalkerStep(&walk), mode );
        
        if( !mode ) {
            PageWalker tempWalk = WalkerNew( SpaceCR3(from), NULL, NULL, NULL );
            PTE* trgEntry = WalkerEntry( &tempWalk, vBase + offset, PAGE );
            WalkerCopyAttributes( entry, trgEntry );
        }
    } while( WalkerCurAddress(&walk) < vEnd && (entry = WalkerNext(&walk)) );
}


void VirtualPurge( VirtualSpace* to, MemoryBlock vTarget, MapInterface* interface ) {
    AssertCorrectCore(to);
    PageWalker walk = WalkerNew( SpaceCR3(to), NULL, to->Service->Allocator, to->Service->Free );
    uptr vEnd = vTarget.End, vBase = vTarget.Base;

    PTE* cur = WalkerEntry(&walk, vBase, PAGE);
    uptr vContigous = WalkerCurAddress(&walk), pContigous = PTEAddr(cur);

    while( vContigous < vEnd ) {
        bool adjancent = PTEAddr(cur) && PTEAddr( cur=WalkerNext(&walk) );

        uptr vRange = WalkerCurAddress(&walk) - vContigous;
        if( adjancent && PTEAddr(cur) - pContigous == vRange ) continue;

		interface->OnUnmap( interface->Object, to, pContigous, vContigous, vRange );
        
        if( !PTEAddr(cur) ) cur = WalkerSkipNULLs(cur, vEnd);
		vContigous = WalkerCurAddress(&walk);
		pContigous = PTEAddr(cur);
	}
}