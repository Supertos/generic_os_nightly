/* Supertos Industries (2012 - 2026) 
 * Arch-Prototypes.h specifies functions expected to be implemented by HAL.
 *
 * Notes on implementation:
 * Virtual Memory: Kernel space shall be 1:1 mapped + whatever in higher memory.
 */
#pragma once

#include "Types.h"
#include "LoaderInfo.h"

/* Assembly Functions */
void ValidateAndInitialize( RootTable* loaderTable );

usize FirstSetBit64( u64 value ); // Return NOID on 0ULL;
usize LastSetBit64( u64 value );  // Return NOID on 0ULL;
usize KernelCrashUndBurn( char* messageAddr );
	
u32 SMPID();
u32 Random();

void SetVirtualSpace( void* addr, u32 pcid, bool flush );
void InvalidateVirtualSpace( void* invAddr, u64 pcid );
	
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
u16 FetchSub16( volatile u16* value, u16 other );

u64 ThreadLocalValue();

void SwapThreadLocalValue();

void SetThreadLocalValue( bool kernel, uptr addr );

void Pause();

void FlushCachelines( uptr addr, usize size );