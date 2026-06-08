/* Supertos Industries (2012 - 2026) 
 * PageWalker walks the sparse PageTable(s) as if they were fully allocated, allocating (and freeing) tables on demand.
 * Level member of PageWalker struct corresponds to current table in MMU hierarhcy:
 *
 * WalkerEntry selection on each iteration:
 *
 * Level = X. Table. id inside table (1 << WalkerStep)
 * Level = 4. Root, id = vAddr / (1 << 48)
 * Level = 3. 512GB, id = vAddr / (1 << 39)
 * Level = 2. 1GB, id = vAddr / (1 << 30)
 * Level = 1. 2MB, id = vAddr / (1 << 21)
 * Level = 0. 4KB, id = vAddr / (1 << 12)

 */
#pragma once

#include "Base.h"

#define VM_LEVELS ((VIRT_BITS - PAGE_EXP) / LEVEL_EXP)
#define __WALKER_VM_STACK_BYTES (sizeof(PTE*) + sizeof(uptr) + sizeof(usize) ) * VM_LEVELS

struct PTE;
typedef struct PTE PTE;

typedef struct {
	u8 Level;
	char Stack[__WALKER_VM_STACK_BYTES];
	Allocator Source;
} PageWalker;

uptr WalkerCurAddress( PageWalker* self );

usize WalkerStep( PageWalker* self );

PTE* WalkerNext( PageWalker* self );

PTE* WalkerEntry( PageWalker* self, uptr vAddr, u64 targetAlign );

PTE* WalkerSkipNULLs( PageWalker* self, uptr maxVAddr );

PageWalker WalkerNew( PageTable* Root, Allocator allocator );

void WalkerSetEntry( PageWalker* self, uptr addr, char* mode );

void WalkerCopyAttributes( PTE* from, PTE* to );