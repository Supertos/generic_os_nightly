/* Supertos Industries (2012 - 2026) */
#pragma once
#include "Paging.h"
#include "x86-64.h"

#define ID_BITS (bitsof(uptr) - __VM_VIRT)

#define CAN_FREE true
#define CANT_FREE false

typedef struct {
	PTE** Entry;
	uptr VBase : __VM_VIRT;
	uptr LastAccessID : ID_BITS;
} __VMStack;

typedef struct {
	u8 Level;
	__VMStack Stack[__VM_LEVELS + 1];
	void* Allocator; 
	AllocateFunction Allocate;
	FreeFunction Free;
} PageWalker;

uptr WalkerCurAddress( PageWalker* self );

PTE* WalkerNext( PageWalker* self );

PTE* WalkerEntry( PageWalker* self, uptr vAddr, u64 targetAlign );

PTE* WalkerSkipNULLs( PageWalker* self, uptr maxVAddr );

PageWalker WalkerNew( PageTable* Root, AllocateFunction allocate, void* allocator, FreeFunction free );

void WalkerSetPTE( PageWalker* self, PTE* pte, uptr addr, char* mode, bool allowDeallocate );

static inline usize WalkerStep( PageWalker* self ) { 
    return 1 << (__LEVEL_EXP * self->Level + __PAGE_EXP); 
}