/* Supertos Industries (2012 - 2026) */

#include "PageWalker.h"

#define PAGE_EXP PCID_BITS
#define LEVEL_EXP 9

#define PT_ENTRIES (1 << LEVEL_EXP)
#define PT_ALIGN (1 << PAGE_EXP)

#define ATTRIBUTE_MASK (MASK(u64, 0, 12) | MASK(u64, 63, 1))

#define PTE_PHYS (PHYS_BITS - PAGE_EXP)

typedef struct {
    u64 Present : 1;
    u64 Writable : 1;
    u64 User : 1;
    u64 WriteThrough : 1;
    u64 CacheDisable : 1;
    u64 Accessed : 1;
    
    u64 Written : 1;
    u64 Huge : 1;
    u64 Global : 1;
    u64 __Reserved1 : 3;

    u64 PAddr: PTE_PHYS;
    u64 __Available : 12;

    u64 DisableExecution : 1;
} __attribute__((__may_alias__)) PTE;


static inline uptr PTEAddr( PTE* entry ) { 
    if( !entry ) return NULL;
    return (entry->PAddr << PAGE_EXP); 
}


static inline PTE* SetPTE( PTE* pte, uptr addr, char* mode ) {
    *pte = (PTE) { .__Available = pte->__Available, .User = 1, .DisableExecution = 1, .PAddr = (addr >> PAGE_EXP) };
	
    for( u64 i = 0; mode && mode[i]; i++ ) {
		switch( mode[i] ) {
			case 'R': pte->Present = 1; break;
			case 'W': pte->Writable = 1; break;
			case 'X': pte->DisableExecution = 0; break;

			case 'N': pte->CacheDisable = 1; break; // Non-cacheable
			case 'S': pte->WriteThrough = 1; break; // Shared

			case 'G': pte->Global = 1; break;
			case 'K': pte->User = 0; break; // Kernel

			case 'H': pte->Huge = 1; break;
		}
    }

    return pte;
}


static inline PTE* TryInitializePT( void* begin ) {
    return begin ? (PTE*)FillMemory(tbl, PAGE, 0) : NULL;
}


typedef struct {
	PTE* Entry;
	uptr Base;
	usize ID;
} VMLevel;


static inline VMLevel* CurLevel( PageWalker* self ) {
    return self->Stack + self->Level * sizeof(VMLevel);
}


usize WalkerStep( PageWalker* self ) {
	usize shift = LEVEL_EXP * self->Level + PAGE_EXP;
	if( shift >= bits_of(usize) ) return ~0ULL;
    return 1ULL << shift;
}


uptr WalkerAddress( PageWalker* self ) {
	return cur->Base + cur->ID * WalkerStep(self);
}


static inline bool WalkerCanAscend( PageWalker* self ) {
	return self->Level < VM_LEVELS - 1;
}


static inline bool WalkerCanDescend( PageWalker* self, u16 id ) {
	VMLevel* cur = CurLevel(self);
	return self->Level > 0 && (!cur->Entry || !cur->Entry[id].Huge);
}


static inline u16 AddToCounter( PTE* pte, u16 delta ) {
    PTE* first = AlignDown(pte, PT_ALIGN);
	first->__Available += delta;
    return first->__Available
}


static inline VMLevel* WalkerDescend( PageWalker* self, u8 id ) {
    VMLevel* top = CurLevel(self);
    top->ID = id;
    if( !WalkerCanDescend(self, id) ) return NULL;
	
	if( !PTEAddr(top->Entry[id]) ) {
		void* data = From( self->Source, PAGE, PT_ALIGN ).Base;
		WalkerSetEntry( self, TryInitializePT(data), "RWNSK" );
	}
	
	VMLevel* old = CurLevel(self);
	self->Level--;
	*CurLevel(self) = (VMLevel) {
		.Entry = old->Entry[id],
		.ID = -1,
		.Base = old->Base + id * WalkerStep(self)
	};
	
	return CurLevel(self);
}


static inline VMLevel* WalkerAscend( PageWalker* self ) {
    self->Level++;
    return CurLevel(self);
}


PTE* WalkerNext( PageWalker* self ) {
	VMLevel* top = CurLevel(self);

	while( top->ID == PT_ENTRIES - 1 && WalkerCanAscend(self) )
        top = WalkerAscend(self);
	
	if( top->ID == PT_ENTRIES - 1 ) return NULL;
	
	while( WalkerCanDescend(self, top->ID + 1) )
		top = WalkerDescend(self, top->ID + 1);
	
    if( !top->Entry ) return NULL;
	return top->Entry[top->ID + 1];
}


void WalkerSetEntry( PageWalker* self, uptr addr, char* mode ) {
	VMLevel* top = CurLevel(self);
	PTE* pte = top->Entry[top->ID];
    bool changed = (!!PTEAddr(pte)) != (!!addr);
    SetPTE( pte, addr, mode );
    if( !changed ) return;

    u16 counter = AddToCounter(pte, addr ? 1 : -1);
    if( counter > 0 || !WalkerCanAscend(self) || !self->Source.To ) return;
	
	To( self->Source, MemoryOfSize(AlignDown(pte, PT_ALIGN), PAGE) );
    WalkerAscend(self);
    tail return WalkerSetEntry(self, NULL, "");
}


PTE* WalkerEntry( PageWalker* self, uptr vAddr, u64 align ) {
    u64 level, id;
	switch( align ) {
		case HUGEPAGE: level = 1; break;
		case GIGAPAGE: level = 2; break;
		default: level = 0; break;
	}
	
	self->Level = VM_LEVELS - 1;
	do {
		id = vAddr / WalkerStep(self);
		vAddr %= WalkerStep(self);
	} while( WalkerDescend(self, id) && level < self->Level );
	return &CurLevel(self)->Entry[id];
}


static inline PTE* WalkerSkipNULLsAtLevel( PageWalker* self ) {
	VMLevel* top = CurLevel(self);
	if( !top->Entry ) return NULL;
	
	while( ++top->ID < PT_ENTRIES ) {
		PTE* cur = top->Entry[top->ID];
		if( PTEAddr(cur) ) return cur;
	}
	
	return NULL;
}


PTE* WalkerSkipNULLs( PageWalker* self, uptr maxVAddr ) {
	while( WalkerCurAddress(self) < maxVAddr ) {
		PTE* out = WalkerSkipNULLsAtLevel(self);
		VMLevel* top = CurLevel(self);
		
		if( PTEAddr(out) ) {
			if( !WalkerCanDescend(self, top->ID) ) return out;
			WalkerDescend( self, top->ID );
			continue;
		}
		
		if( !WalkerCanAscend(self) ) break;
		WalkerAscend(self);
	}
    return NULL;
}


PageWalker WalkerNew( PageTable* Root, Allocator allocator ) {
	PageWalker self = (PageWalker){ 
        .Level = VM_LEVELS - 1, .Allocator = allocator
    };

	*CurLevel(&self) = (VMLevel){ .Entry = Root, .ID = 0, .VBase = 0 };
	return self;
}


void WalkerCopyAttributes( PTE* from, PTE* to ) {
    u64 *fromU64 = (u64*)from, *toU64 = (u64*)to;
    *toU64 = (*toU64 & ~ATTRIBUTE_MASK ) | (*fromU64 & ATTRIBUTE_MASK);
}