/* Supertos Industries (2012 - 2026) */
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
	
	typedef unsigned long uptr;
	typedef unsigned long usize;
    
	/* Mandatory Functions */
	usize FirstSetBit64( u64 value ); // Return NOID on 0ULL;
	usize LastSetBit64( u64 value );  // Return NOID on 0ULL;
	usize KernelCrashUndBurn( char* messageAddr );
	
	u32 SMPID();
	u32 Random();
	
	void InterruptsOff();
	void InterruptsOn();
	
	void LockOr64( u64* value, u64 orVal );
	void LockAnd64( u64* value, u64 andVal );

	bool TryCAS64( volatile u64* value, volatile u64* expectedValue, u64 newValue );
	bool TryCAS32( volatile u32* value, volatile u32* expectedValue, u32 newValue );
	bool TryCAS16( volatile u16* value, volatile u16* expectedValue, u16 newValue );
	
	u32 FetchAdd32( volatile u32* value, u32 other );
	u32 FetchSub32( volatile u32* value, u32 other );
	u16 FetchAdd16( volatile u16* value, u16 other );
	u32 FetchSub16( volatile u16* value, u16 other );

	void Pause();

	#define architecture_barrier ;
#else
    #error "Invalid target apparatus."
#endif