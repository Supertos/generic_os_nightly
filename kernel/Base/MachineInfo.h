/* Supertos Industries (2012 - 2026) 
 * MachineInfo is an abstraction over machine-generated tables.
 */
#pragma once

#include "Types.h"

struct MachineInfo;
typedef struct MachineInfo MachineInfo;

typedef struct {
	u32 DomainID;
	u16 CoreCount;
	u16 Flags;
	u64 Base;
	u64 End;
	u64 AverageLatency;
} MemoryDomainInfo;

typedef struct {
	u32 SMPID;
	u32 DomainID;
	u16 Flags;
} CPUInfo;

void ResetCPUCursor( MachineInfo* self );

void ResetMemoryDomainCursor( MachineInfo* self );

bool ValidMemoryDomain( MemoryDomainInfo self );

bool ValidCPU( CPUInfo self );

bool ReachableLatency( u8 latency );

void MemoryDomainLatency( MachineInfo* self, MemoryDomainInfo info, u8 out[] );

MemoryDomainInfo NextMemoryDomainInfo( MachineInfo* self );

CPUInfo NextCPUInfo( MachineInfo* self );

MachineInfo MachineInfoNew( RootTable* loaderInfo );

usize MemoryDomainCount( MachineInfo* self );

usize CPUCount( MachineInfo* self );

void MemoryDomainLatency( MachineInfo* self, MemoryDomainInfo info, u8 out[] );
