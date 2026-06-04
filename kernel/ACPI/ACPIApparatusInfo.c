/* Supertos Industries (2012 - 2026) */

#include "ACPIApparatusInfo.h"
#include "ACPI.h"
#include "SLIT.h"
#include "SRAT.h"

void ResetCPUCursor( ApparatusInfo* self ) {
	self->CPUEntry = NULL;
}

void ResetProximityDomainCursor( ApparatusInfo* self ) {
	self->NUMAEntry = NULL;
}

bool ValidProximityDomain( ProximityDomainInfo self ) {
	return self.Flags & APPARATUS_ENTRY_VALID;
}

bool ValidCPU( CPUInfo self ) {
	return self.Flags & APPARATUS_ENTRY_VALID;
}


bool SameNUMAID( CPUAffinity* self, u32* id ) {
	return CPUAffinityNUMA(self) == *id;
}


usize ProximityDomainCount( ApparatusInfo* self ) {
	if( !self->SRAT ) return 1;
	if( self->DomainCountCache == 0 )
		self->DomainCountCache = ACPIEntryCount( self->SRAT, sizeof(SRAT), SRAT_MEM_AFFINITY, NULL, NULL );
	
	return self->DomainCountCache;
}


usize CPUCount( ApparatusInfo* self ) {
	if( !self->SRAT ) return 1;
	if( self->CoreCountCache == 0 )
		self->CoreCountCache = ACPIEntryCount( self->SRAT, sizeof(SRAT), SRAT_CPU_AFFINITY, NULL, NULL );
	
	return self->CoreCountCache;
}


static inline u8* PDLatency( ApparatusInfo* self, usize id ) {
	return &slit->LocalityMatrix[slit->LocalityCount * id];
}


void ProximityDomainLatency( ApparatusInfo* self, ProximityDomainInfo info, u8 out[] ) {
	SLIT* slit = self->SLIT;
	usize count = slit ? slit->LocalityCount : ProximityDomainCount(self);
	
	for( usize i = 0; i < slit->LocalityCount; i++ )
		out[i] = slit ? PDLatency(self, info.ProximityID)[i] : SLIT_LATENCY_SAME_NODE;
}


ProximityDomainInfo NextProximityDomainInfo( ApparatusInfo* self ) {
	if( !self->SRAT ) {
		self->NUMAEntry = ~self->CPUEntry;
		if( !self->NUMAEntry ) return (ProximityDomainInfo){0};
		
		return (ProximityDomainInfo) {
			.ProximityID = 1,
			.AssignedCoresCount = CPUCount(self),
			.Flags = APPARATUS_ENTRY_VALID,
			.Base = 0,
			.End = ~0ULL,
			.AverageLatency = SLIT_LATENCY_SAME_NODE
		};
	}
	
	self->NUMAEntry = NextACPIEntry( self->SRAT, sizeof(SRAT), self->NUMAEntry, SRAT_MEM_AFFINITY );
	if( !self->NUMAEntry ) return (ProximityDomainInfo){0};
	
	MemoryAffinity* affinity = self->NUMAEntry;
	usize id = affinity->NUMAID, latencySum = 0;
	
	for( usize i = 0; i < ProximityDomainCount(self); ++i )
		latencySum += self->SLIT ? PDLatency(self, id) : SLIT_LATENCY_SAME_NODE;
	
	return (ProximityDomainInfo){
		.ProximityID = id,
		.AssignedCoresCount = ACPIEntryCount( self->SRAT, sizeof(SRAT), SRAT_CPU_AFFINITY, SameNUMAID, id ),
		.Flags = APPARATUS_ENTRY_VALID,
		.Base = affinity->Base,
		.End = affinity->Base + affinity->Size,
		.AverageLatency = latencySum / ProximityDomainCount(self);
	};
}


CPUInfo NextCPUInfo( ApparatusInfo* self ) {
	if( !self->SRAT ) {
		self->CPUEntry = ~self->CPUEntry;
		return self->CPUEntry ?
			(CPUInfo){ .SMPID = 1, .ProximityID = 1, .Flags = APPARATUS_ENTRY_VALID } : 
			(CPUInfo){0};
	}
	
	CPUAffinity* affinity = NextACPIEntry( self->SRAT, sizeof(SRAT), self->CPUEntry, SRAT_CPU_AFFINITY );
	
	self->CPUEntry = affinity;
	if( !affinity ) return (CPUInfo){0};
	
	return (CPUInfo){
		.SMPID = affinity->APICID,
		.ProximityID = CPUAffinityNUMA(affinity),
		.Flags = APPARATUS_ENTRY_VALID
	};
}


ApparatusInfo ApparatusInfoNew( RootTable* loaderInfo ) {
	XSDP* xsdp = loaderInfo->XSDP;
	return (ApparatusInfo){
		.XSDP = xsdp,
		.SRAT = FindACPITable( xsdp, SRAT_SIGNATURE, NULL ),
		.SLIT = FindACPITable( xsdp, SLIT_SIGNATURE, NULL )
	};	
}

bool ReachableLatency( u8 latency ) {
	return latency == 255;
}