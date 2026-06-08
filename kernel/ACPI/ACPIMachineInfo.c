/* Supertos Industries (2012 - 2026) 3622 */

#include "ACPIMachineInfo.h"
#include "ACPI.h"
#include "SLIT.h"
#include "SRAT.h"

#define VALID_ENTRY 1

void ResetCPUCursor( MachineInfo* self ) {
	self->CPUEntry = NULL;
}


void ResetProximityDomainCursor( MachineInfo* self ) {
	self->NUMAEntry = NULL;
}


bool ValidProximityDomain( ProximityDomainInfo self ) {
	return self.Flags & VALID_ENTRY;
}


bool ValidCPU( CPUInfo self ) {
	return self.Flags & VALID_ENTRY;
}


bool SameNUMAID( CPUAffinity* self, u32* id ) {
	return CPUAffinityNUMA(self) == *id;
}


usize ProximityDomainCount( MachineInfo* self ) {
	if( !self->SRAT ) return 1;
	
	self->DomainCount = self->DomainCount ?:
						(self->SLIT ? self->SLIT->LocalityCount :
						ACPIEntryCount( self->SRAT, sizeof(SRAT), SRAT_MEM, NULL, NULL ));
	
	return self->DomainCount;
}


usize CPUCount( MachineInfo* self ) {
	if( !self->SRAT ) return 1;
	
	self->CoreCount = self->CoreCount ?:
					  ACPIEntryCount( self->SRAT, sizeof(SRAT), SRAT_CPU, NULL, NULL );
	
	return self->CoreCount;
}


static inline u8* Latency( MachineInfo* self, usize id ) {
	return &slit->Latency[slit->LocalityCount * id];
}


void ProximityDomainLatency( MachineInfo* self, ProximityDomainInfo info, u8 out[] ) {
	SLIT* slit = self->SLIT;
	
	for( usize i = 0; i < ProximityDomainCount(self); i++ )
		out[i] = slit ? Latency(self, info.DomainID)[i] : SLIT_SELF;
}


ProximityDomainInfo NextProximityDomainInfo( MachineInfo* self ) {
	if( !self->SRAT ) {
		self->NUMAEntry = ~self->CPUEntry;
		if( !self->NUMAEntry ) return (ProximityDomainInfo){0};
		
		return (ProximityDomainInfo) {
			.CoreCount = CPUCount(self),
			.Flags = VALID_ENTRY,
			.End = ~0ULL,
			.AverageLatency = SLIT_SELF
		};
	}
	
	self->NUMAEntry = NextACPIEntry( self->SRAT, sizeof(SRAT), self->NUMAEntry, SRAT_MEM );
	if( !self->NUMAEntry ) return (ProximityDomainInfo){0};
	
	MemoryAffinity* cur = self->NUMAEntry;
	u32 id = cur->NUMAID, latencySum = 0;
	
	for( usize i = 0; i < ProximityDomainCount(self); ++i )
		latencySum += self->SLIT ? Latency(self, id)[i] : SLIT_SELF;
	
	return (ProximityDomainInfo){
		.DomainID = id,
		.CoreCount = ACPIEntryCount( self->SRAT, sizeof(SRAT), SRAT_CPU, SameNUMAID, &id ),
		.Flags = VALID_ENTRY,
		.Base = cur->Base,
		.End = cur->Base + cur->Size,
		.AverageLatency = latencySum / ProximityDomainCount(self);
	};
}


CPUInfo NextCPUInfo( MachineInfo* self ) {
	if( !self->SRAT ) {
		self->CPUEntry = ~self->CPUEntry;
		return self->CPUEntry ?
			(CPUInfo){ .SMPID = 1, .DomainID = 1, .Flags = VALID_ENTRY } : 
			(CPUInfo){0};
	}
	
	CPUAffinity* affinity = NextACPIEntry( self->SRAT, sizeof(SRAT), self->CPUEntry, SRAT_CPU );
	
	self->CPUEntry = affinity;
	if( !affinity ) return (CPUInfo){0};
	
	return (CPUInfo){
		.SMPID = affinity->APICID,
		.DomainID = CPUAffinityNUMA(affinity),
		.Flags = VALID_ENTRY
	};
}


MachineInfo MachineInfoNew( RootTable* loaderInfo ) {
	XSDP* xsdp = loaderInfo->XSDP;
	return (MachineInfo){
		.XSDP = xsdp,
		.SRAT = FindACPITable( xsdp, SRAT_SIGNATURE, NULL ),
		.SLIT = FindACPITable( xsdp, SLIT_SIGNATURE, NULL )
	};	
}


bool ReachableLatency( u8 latency ) {
	return latency != 255;
}