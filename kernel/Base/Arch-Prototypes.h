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

/* ApparatusInfo functions */

#define APPARATUS_ENTRY_VALID 1

struct ApparatusInfo;
typedef struct ApparatusInfo ApparatusInfo;

typedef struct {
	u32 ProximityID;
	u16 AssignedCoresCount;
	u16 Flags;
	u64 Base;
	u64 End;
	u64 AverageLatency;
} ProximityDomainInfo;

typedef struct {
	u32 SMPID;
	u32 ProximityID;
	u16 Flags;
} CPUInfo;

void ResetCPUCursor( ApparatusInfo* self );

void ResetProximityDomainCursor( ApparatusInfo* self );

bool ValidProximityDomain( ProximityDomainInfo self );

bool ValidCPU( CPUInfo self );

void ProximityDomainLatency( ApparatusInfo* self, ProximityDomainInfo info, u8 out[] );

ProximityDomainInfo NextProximityDomainInfo( ApparatusInfo* self );

CPUInfo NextCPUInfo( ApparatusInfo* self );

ApparatusInfo ApparatusInfoNew( RootTable* loaderInfo );

usize ProximityDomainCount( ApparatusInfo* self );

usize CPUCount( ApparatusInfo* self );

void ProximityDomainLatency( ApparatusInfo* self, ProximityDomainInfo info, usize proximityCount, u8 out[] );

bool ReachableLatency( u8 latency );