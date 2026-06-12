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
	
	usize maxAddr = 0;
	for( usize i = 0; i < root->MemoryMapEntryCount; i++ )
		maxAddr = MAX(maxAddr, (uptr)root->MemoryMap[i].Base + root->MemoryMap[i].Pages * PAGE);
	
	MachineInfo info = MachineInfoNew(root);
	usize domainCount = MemoryDomainCount(&info);
	ResetMemoryDomainCursor(&info);
	
	MemoryBlock mDomains, mLatency;
	Domain** domains = NULL;
	u8* latency = NULL;
	Domain* medianDomain = NULL, *sourceDomain = NULL, *domain;
	
	while( domain = DomainNew(root, &info) ) {
		if( !domains ) {
			mDomains = FromDomain( domain, sizeof(*domains) * domainCount, 1 );
			if( !ValidMemory(mDomains) ) KernelCrashUndBurn("Bad topology.");
			domains = (Domain**)mDomains.Base;
			
			mLatency = FromDomain( domain, sizeof(*latency) * domainCount, 1 );
			if( !ValidMemory(mLatency) ) KernelCrashUndBurn( "Bad topology." );
			latency = (u8*)mLatency.Base;
			
			sourceDomain = domain;
		}
		
		MemoryDomainInfo* domainInfo = DomainInfo(domain);
		domains[domainInfo->DomainID] = domain;
		
		if( !medianDomain || DomainInfo(medianDomain)->AverageLatency > domainInfo->AverageLatency ) 
			medianDomain = domain;
	}
	
	for( usize i = 0; i < domainCount; i++ ) {
		MemoryDomainLatency( &info, *DomainInfo(domains[i]), latency );
		LinkDomain( domains[i], domains, latency, domainCount );
	}
	
	ResideSharedData(medianDomain);
	
	ResetCPUCursor(&info);
	CPUInfo cpu;
	while( ValidCPU(cpu = NextCPUInfo(&info)) )
		ResideCore(domains[cpu.DomainID], cpu, maxAddr);
	
	ToDomains(sourceDomain, mDomains);
	ToDomains(sourceDomain, mLatency);

    InterruptsOn();
}