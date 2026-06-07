/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Base.h"

struct ApparatusInfo {
	void* XSDP;
	void* SRAT;
	void* SLIT;
	
	void* NUMAEntry;
	void* CPUEntry;
	
	u32 CoreCountCache;
	u32 DomainCountCache;
};

/* Supertos Industries (2012 - 2026) */

#include "ACPIApparatusInfo.h"
#include "ACPI.h"
#include "SLIT.h"
#include "SRAT.h"

void ResetCPUCursor( ApparatusInfo* self );

void ResetProximityDomainCursor( ApparatusInfo* self );

bool ValidProximityDomain( ProximityDomainInfo self );

bool ValidCPU( CPUInfo self );

bool SameNUMAID( CPUAffinity* self, u32* id );

usize ProximityDomainCount( ApparatusInfo* self );

usize CPUCount( ApparatusInfo* self );

void ProximityDomainLatency( ApparatusInfo* self, ProximityDomainInfo info, u8 out[] );

ProximityDomainInfo NextProximityDomainInfo( ApparatusInfo* self );

CPUInfo NextCPUInfo( ApparatusInfo* self );

ApparatusInfo ApparatusInfoNew( RootTable* loaderInfo );

bool ReachableLatency( u8 latency );