/* Supertos Industries (2012 - 2026) */

#include "Base.h"
#include "Architecture.h"
#include "Prototypes/ProximityDomain.h"
#include "Prototypes/MemoryManager.h"
#include "Prototypes/VirtualMemory.h"


// void BootstrapApplicationCore() {

// }

// void InitializeApplicationCore() {

// }



void InitializeBootCore( RootTable* root ) {
    InterruptsOff();
    ValidateAndInitialize(root);
	
	ApparatusInfo info = ApparatusInfoNew(root);
	usize domainCount = ProximityDomainCount(&info);
	ResetProximityDomainCursor(&info);
	
	MemoryBlock mDomains, mLatency;
	ProximityDomain** domains = NULL;
	u8* latency = NULL;
	ProximityDomain* medianDomain = NULL, sourceDomain = NULL, domain;
	
	while( domain = ProximityDomainNew(root, &info) ) {
		if( !domains ) {
			mDomains = FromDomain( domain, sizeof(*domains) * domainCount, 1 );
			if( !ValidMemory(mDomains) ) KernelCrashUndBurn("Bad topology.");
			domains = mDomains.Base;
			
			mLatency = FromDomain( domain, sizeof(*latency) * domainCount, 1 );
			if( !IsValidMemory(mLatency) ) KernelCrashUndBurn( "Bad topology." );
			latency = mLatency.Base;
			
			sourceDomain = domain;
		}
		
		ProximityDomainInfo* domainInfo = DomainInfo(domain);
		domains[domainInfo->ProximityID] = domain;
		
		if( !medianDomain || DomainInfo(medianDomain)->AverageLatency > domainInfo->AverageLatency ) 
			medianDomain = domain;
	}
	
	for( usize i = 0; i < domainCount; i++ ) {
		ProximityDomainLatency( &info, *DomainInfo(domains[i]), latency );
		LinkDomain( domains[i], domains, latency, domainCount );
	}
	
	ResideSharedData(medianDomain);
	
	ResetCPUCursor(&info);
	CPUInfo cpu;
	while( ValidCPU(cpu = NextCPUInfo(&info)) )
		ResideCore(domains[cpu.ProximityID], cpu);
	
	ToDomains(sourceDomain, mDomains);
	ToDomains(sourceDomain, mLatency);

    InterruptsOn();
}