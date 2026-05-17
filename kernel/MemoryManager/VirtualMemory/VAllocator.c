/* Supertos Industries (2012 - 2026) */
#include "VAllocator.h"
#include "Paging.h"


AllocateFunction FromSharedRanges;
MemoryBlock FromSharedRanges( SVMACursor* self, usize size, usize align ) {
    ACQUIRE(&self->Lock);
    VMASharedRanges* ranges = self->Source;

    ACQUIRE_READER_AUTO(&ranges->Lock);
    SVMAEntry* cur = self->Cur ?: ranges->Last;
    uptr curAddr = self->Top;

    while( curAddr >= cur->End && cur )
        cur = ListPrev(&ranges->Map, cur); // List quirk: insertions are front. Prev - higher addr.

    if( !cur ) return NO_MEMBLOCK;
    *self = (SVMACursor){ .Cur = cur, .Top = cur->End, .Source = self->Source, .Lock = 1 };
    return (MemoryBlock) {
        .Base = MAX(cur->Base, curAddr), 
        .End = cur->End
    };
}


FreeFunction ToSharedRanges;
void ToSharedRanges( SVMACursor* self, MemoryBlock entry ) {
    ACQUIRE(&self->Lock);
    VMASharedRanges* ranges = self->Source;

    ACQUIRE_WRITER_AUTO(&ranges->Lock);
    SVMAEntry* top = ListFirst(&ranges->Map);
    if( top && top->End == entry.Base ) { 
        top->End = entry.End; 
        return; 
    }

    top = FromCache(ranges->SCache);
    *top = (SVMAEntry){ .Base = entry.Base, .End = entry.End };
    ToList( &ranges->Map, top );

    if( !ranges->Last ) ranges->Last = top;
}


MemoryBlock FromVirtualSpace( VirtualSpaceVMA* self, usize size, usize align ) {
    LocalVMA* local = self->Local;
    GlobalVMA* global = local->Global;

    Allocator Flow[] = {
        (Allocator){ self->Free, FromRBAllocatorFirst, ToRBAllocator },
        (Allocator){ &self->Source, FromSharedRanges, ToSharedRanges },
        (Allocator){ &local->Source, FromSharedRanges, ToSharedRanges },
        (Allocator){ &global->Source, FromBump, NULL }
    };

    
    MemoryBlock out = Flow[0].From( Flow[0].Self, size, align );
    if( MemorySuits(out, size, align) && self->Source.Source != NULL ) return out;

    usize i = 1;
    for( ; !MemorySuits(out, size, align); i++ ) {
        Allocator *from = &Flow[i], *to = &Flow[i - 1];
        out = ReplenishAllocator( from, to, size, align );
    }

    for( ; i > 1; i-- ) {
        Allocator *from = &Flow[i - 1], *to = &Flow[i - 2];
        ReplenishAllocator( from, to, size, align );
    }
    
    return Flow[0].From( Flow[0].Self, size, align );
}


void ToVirtualSpace( VirtualSpaceVMA* self, MemoryBlock entry ) {
    ToRBAllocator(self->Free, entry);
}


MemoryBlock FromExclusiveVMA( VirtualSpaceVMA* self, usize size, usize align ) {
    GlobalVMA* vma = self->Local->Global;
    Allocator from = (Allocator){ &vma->Source, FromBump, NULL };
    Allocator to = (Allocator){ &vma->Exclusive, FromRBAllocatorBest, ToRBAllocator };

    MemoryBlock out = FromRBAllocatorBest(&vma->Exclusive, size, align);
    if( !MemorySuits(out, size, align) ) {
        ReplenishAllocator(&from, &to, size, align);
        out = FromRBAllocatorBest(&vma->Exclusive, size, align);
    }

    return out;
}


void ToExclusiveVMA( VirtualSpaceVMA* self, MemoryBlock entry ) {
    GlobalVMA* vma = self->Local->Global;
    ToRBAllocator(&vma->Exclusive, entry);
}


GlobalVMA* GlobalVMANew( void* begin, Cache* sCache, Cache* eCache ) {
	GlobalVMA* self = begin;
	*self = (GlobalVMA) {
		.Shared = (VMASharedRanges){ .SCache = sCache },
		.Source = NewBump(0, MASK(usize, 0, VIRT_BITS), "GLOBAL_VMA")
	};

	RBAllocatorNew(&self->Exclusive, eCache);
	ListNew( &self->Shared.Map, offset_of(SVMAEntry, Index) );

	return self;
}


LocalVMA* LocalVMANew( void* begin, Cache* sCache, GlobalVMA* source ) {
	LocalVMA* self = begin;
	*self = (LocalVMA){
		.Shared = (VMASharedRanges){ .SCache = sCache },
		.Source = (SVMACursor){ .Source = source }
	};

	ListNew( &self->Shared.Map, offset_of(SVMAEntry, Index) );
	return self;
}


VirtualSpaceVMA* VirtualSpaceVMANew( void* begin, RBAllocator* allocator, Cache* eCache, LocalVMA* source ) {
	VirtualSpaceVMA* self = begin;
	*self = (VirtualSpaceVMA){ .Source = (SVMACursor){ .Source = source }, .Free = allocator };
	RBAllocatorNew(allocator, eCache);
	return self;
}