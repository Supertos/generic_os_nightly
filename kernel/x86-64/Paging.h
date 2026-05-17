/* Supertos Industries (2012 - 2026)
 * MMU Support Module, including page walker to be used by Virtual Memory Manager.
 *
 * Designed to operate in identity-mapped environment (1:1 + whatever mapped in upper memory).
 * Anticipates pAddr == vAddr when allocating memory and writing PageTables.
 * 
 * PageWalker is designed to work with sparse table. If no allocate method provided, walker iterates through non-existent entries, returning NULLs for PTEs.
 * Identity-mapping enforced to simplify architecture, all mapping operations are conducted in kernel's address space.
 */
#pragma once
#include "Base.h"

#define VIRT_BITS 57
#define PHYS_BITS 52
#define PCID_BITS 12
#define PTE_ROOT_EXP 12
#define PTE_ROOT_SIZE PAGE
#define PTE_ROOT_ALIGN PAGE
#define __PAGE_EXP 12
#define __LEVEL_EXP 9



#define __VM_LEVELS 5

#define __VM_PTE_PHYS (PHYS_BITS - __PAGE_EXP)
#define __VM_VIRT (VIRT_BITS - __PAGE_EXP)
#define __VM_ENTRIES (1 << __LEVEL_EXP)

#define __VM_AVL_MASK (((1 << 12) - 1) << PHYS_BITS)

#define VIRT_ATTRIBUTE_MASK (MASK(u64, 0, 12) | MASK(u64, 63, 1))


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

    u64 PhysicalPage: __VM_PTE_PHYS;
    u64 __Available : 12;

    u64 DisableExecution : 1;
} __attribute__((__may_alias__)) PTE;


typedef struct { PTE Entries[__VM_ENTRIES]; } PageTable;


static inline uptr PTEAddr( PTE* entry ) { 
    if( !entry || entry == VNULL ) return NULL;
    return (entry->PhysicalPage << __PAGE_EXP); 
}

static inline u16 PTEAvailableBits( PTE* pte ) {
	return pte->__Available;
}

static inline void PTESetAvailableBits( PTE* pte, u16 value ) {
	pte->__Available = value;
}



static inline PTE* SetPTE( PTE* pte, uptr addr, char* mode ) {
    *pte = (PTE) { .__Available = pte->__Available, .User = 1, .DisableExecution = 1, .PhysicalPage = (addr >> __PAGE_EXP) };
	
    for( u64 i = 0; mode && mode[i]; i++ ) {
        char m = mode[i];

        if( m == 'R' ) pte->Present = 1;
        if( m == 'W' ) pte->Writable = 1;
        if( m == 'X' ) pte->DisableExecution = 0;

        if( m == 'N' ) pte->CacheDisable = 1; // Non-cacheable
        if( m == 'S' ) pte->WriteThrough = 1; // Shared

        if( m == 'G' ) pte->Global = 1;
        if( m == 'K' ) pte->User = 0; // Kernel

        if( m == 'H' ) pte->Huge = 1;
    }

    return pte;
}

static inline PageTable* TryInitializePT( void* begin ) {
	if( !begin ) return NULL;
    PageTable* tbl = (PageTable*)begin;
    for( u64 i = 0; i < __VM_ENTRIES; i++ ) tbl->Entries[i] = (PTE){0};
    return tbl;
}


static inline void SetVirtualSpace( void* addr, u32 pcid, bool flush ) {
	u64 cr3 = pcid | (u32)addr;
	if( !flush ) cr3 |= (1ULL << 63); // NO_FLUSH
	
    ForbidInterrupts();

	__asm__ __volatile__ (
		"mov %0, %%cr3;"
		:
		: "r"(cr3)
		: "memory"
	);

    AllowInterrupts();
}


static inline void InvalidateVirtualSpace( void* invAddr, u64 pcid ) {    
    u64 pcidType = invAddr == NULL ? 0 : 1;
    if( pcid >= (1 << 12) ) KernelCrashUndBurn("Invalid InvalidateVirtualSpace call: pcid is huge!");
    u64 desc[2] = {pcid, invAddr};

    __asm__ __volatile__ (
        "invpcid %1, %0"
        :
        : "r"(pcidType), "m"(desc)
        : "memory"
    );
}