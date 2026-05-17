/* Supertos Industries (2012 - 2026) */

#include "PageWalker.h"

static inline __VMStack* __StackTop( PageWalker* self ) { 
    return &self->Stack[self->Level]; 
}


uptr WalkerCurAddress( PageWalker* self ) { 
    __VMStack* top = __StackTop(self);
    return ((u64)top->VBase << __PAGE_EXP) + top->LastAccessID * WalkerStep(self); 
}


static inline bool WalkerCanAscend( PageWalker* self ) {
	return self->Level < __VM_LEVELS;
}


static inline bool WalkerCanDescend( PageWalker* self, u8 id ) {
    __VMStack* parent = __StackTop(self);
    return parent && self->Level > 0 && (!parent->Entry || !parent->Entry[id]->Huge);
}


static inline u16 IncreaseCounter( PTE* pte ) {
    PTE* first = (PTE*)AlignDown(pte, PAGE);
    PTESetAvailableBits(first, PTEAvailableBits(first) + 1);
    return PTEAvailableBits(first);
}


static inline u16 DecreaseCounter( PTE* pte ) {
    PTE* first = (PTE*)AlignDown(pte, PAGE);
    PTESetAvailableBits(first, PTEAvailableBits(first) - 1);
    return PTEAvailableBits(first);
}


void WalkerSetPTE( PageWalker* self, PTE* pte, uptr addr, char* mode, bool allowDeallocate ) {
    bool changed = (!!PTEAddr(pte)) != (!!addr);
    SetPTE( pte, addr, mode );

    if( !changed ) return;

    u16 counter;
    if( addr )
        counter = IncreaseCounter(pte);
    else
        counter = DecreaseCounter(pte);

    if( counter > 0 || !allowDeallocate || !WalkerCanAscend(self) ) return;

    self->Free( self->Allocator, MemoryOfSize(AlignDown(pte, PAGE), PAGE) );
    __VMStack* top = __StackTop(self);
	if( top->Entry != AlignDown(pte, PTE_ROOT_ALIGN) ) return;
	
    WalkerAscend(self);
    WalkerSetPTE( self, top->Entry[top->LastAccessID], NULL, "", allowDeallocate );
}


static inline __VMStack* WalkerDescend( PageWalker* self, u8 id ) {
    __VMStack* top = __StackTop(self);
    top->LastAccessID = id;
    if( !WalkerCanDescend(self, id) ) return NULL;

    PTE** parent = top->Entry;
    PTE* trg = parent ? &parent[id] : NULL;

    if( trg && !PTEAddr(trg) && self->Allocate ) {
        void* data = self->Allocate( self->Allocator, PAGE, PAGE ).Base;
        WalkerSetPTE( self, trg, TryInitializePT(data), "RWNSK", CANT_FREE );
    }

    self->Level--;
    *__StackTop(self) = (__VMStack){
        .Entry = parent ? PTEAddr(&parent[id]) : NULL,
        .LastAccessID = -1,
        .VBase = top->VBase + (id * WalkerStep(self)) >> __PAGE_EXP
    };

	return __StackTop(self);
}


static inline __VMStack* WalkerAscend( PageWalker* self ) {
    self->Level = MIN(__VM_LEVELS, self->Level + 1);
    return __StackTop(self);
}


PTE* WalkerNext( PageWalker* self ) {
	__VMStack* top = __StackTop(self);

	while( top->LastAccessID == __VM_ENTRIES - 1 && WalkerCanAscend(self) )
        top = WalkerAscend(self);
	
	while( WalkerCanDescend(self, top->LastAccessID + 1) )
		top = WalkerDescend(self, top->LastAccessID + 1);
	
    if( !top->Entry ) return NULL;
	return top->Entry[top->LastAccessID + 1];
}


PTE* WalkerEntry( PageWalker* self, uptr vAddr, u64 targetAlign ) {
    u64 level = 0, id;
    if( targetAlign == HUGEPAGE ) level = 1;
    if( targetAlign == GIGAPAGE ) level = 2;
	self->Level = __VM_LEVELS;
	
    PTE* entry;
	bool descended = false;
	do {
		id = vAddr / WalkerStep(self);
		vAddr %= WalkerStep(self);
		
		descended = WalkerCanDescend(self, id);
		entry = WalkerDescend(self, id);
	} while( descended && level < self->Level ); // Selects entry on targetLevel, not targetLevel entry.
    // (targetAlign == PAGE) => 4K PTE.
	return entry;
}


static inline PTE* WalkerNextAtLevel( PageWalker* self ) {
	__VMStack* top = __StackTop(self);
    
    u64 id = top->LastAccessID++;  // Overflow here is unimportant.
	if( id >= __VM_ENTRIES || !top->Entry ) return NULL;
	return &top->Entry[id];
}


static inline PTE* WalkerSkipNULLsAtLevel( PageWalker* self ) {
	__VMStack* top = __StackTop(self);
	if( !top->Entry ) return NULL;
	
	PTE* next = NULL;
	do {
		if( top->LastAccessID >= __VM_ENTRIES ) return NULL;
		next = WalkerNextAtLevel(self);
	} while( !next || !PTEAddr(next) );
	return next;
}


PTE* WalkerSkipNULLs( PageWalker* self, uptr maxVAddr ) {
	while( WalkerCurAddress(self) < maxVAddr ) {
		PTE* out = WalkerSkipNULLsAtLevel(self);
		__VMStack* top = __StackTop(self);
		
		if( out && PTEAddr(out) ) {
			if( !WalkerCanDescend(self, top->LastAccessID) ) return out;
			
			WalkerDescend( self, top->LastAccessID );
			continue;
		}
		
		if( !WalkerCanAscend(self) ) return NULL;
		WalkerAscend(self);
	}
    return NULL;
}


PageWalker WalkerNew( PageTable* Root, AllocateFunction allocate, void* allocator, FreeFunction free ) {
	PageWalker self = (PageWalker){ 
        .Level = __VM_LEVELS, .Allocator = allocator,
        .Free = free, .Allocate = allocate 
    };

	*__StackTop(&self) = (__VMStack){ .Entry = Root, .LastAccessID = 0, .VBase = 0 };
	return self;
}


void WalkerCopyAttributes( PTE* from, PTE* to ) {
    u64 *fromU64 = (u64*)from, *toU64 = (u64*)to;
    *toU64 = (*toU64 & ~VIRT_ATTRIBUTE_MASK ) | (*fromU64 & VIRT_ATTRIBUTE_MASK);
}