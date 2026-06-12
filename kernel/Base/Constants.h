/* Supertos Industries (2012 - 2026) 
 * Arch-Constants.h specifies values expected from target apparatus.
 * It is imperative, that any architecture implementation (e.g. x86-64/) complies with whatever defined here.
 * Anything not listed here shall be either derived from provided values or being abstracted by implementation.
 * Example:
 * #define VIRT_BITS 57 // Defined in Base/Architecture.h
 * 
 * #define PAGE_EXP 12 // Defined in x86-64/... abstracted with PageWalker
 * #define LEVEL_EXP 9 // Defined in x86-64/... abstracted with PageWalker
 *
 * #define VM_LEVELS ((VIRT_BITS - PAGE_EXP) / LEVEL_EXP) // Derived from VIRT_BITS
 *
 * Notes on implementation:
 * Virtual Memory: Kernel space shall be 1:1 mapped + whatever in higher memory.
 */
#pragma once

#include "Types.h"

#if defined(__x86_64__)
    #define PAGE 4096
    #define HUGE_PAGES 512
    #define GIGA_PAGES (512 * 512)
    #define HUGEPAGE (PAGE * HUGE_PAGES)
    #define GIGAPAGE (PAGE * GIGA_PAGES)
    #define BITS 8
    #define CACHELINE 64
    #define SMP_CORE_ID_BITS 32
	
	#define VIRT_BITS 57
	#define PHYS_BITS 52
	#define PCID_BITS 12
	
	typedef unsigned long __attribute__((__mode__(__pointer__)))  uptr;
	typedef unsigned long usize;
    
	#define architecture_barrier ;
#else
    #error "Invalid target apparatus."
#endif