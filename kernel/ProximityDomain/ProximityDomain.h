/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Base.h"
#include "Prototypes/MemoryManager.h"

typedef struct ProximityDomain ProximityDomain;

typedef struct {
	ProximityDomain* Owner;
	u32 SMPID;
	
	Cache* Caches;
	/* CoreScheduler Scheduler; */
} Core;

typedef struct {
	GlobalVMA* VirtualRanges;
	Cache* SCache;
	Cache* ECache;
	/* GlobalScheduler Scheduler; */
} InterDomainData;

struct ProximityDomain {
    ProximityDomainInfo Info;
	
	InterDomainData* SharedData;
	
	Frames* Memory;
	LocalVMA* VirtualRanges;
	Cache* SCache;
	/* DomainScheduler Scheduler; */
	
	u32 LinkCount;
	
	Core* Cores;
	ProximityDomain** Links;
};

/* Return false from function to finish early. */
void RunOnAllDomains( ProximityDomain* base, bool (*function)( ProximityDomain* domain, void* in, void* out ), void* in, void* out );

FreeFunction ToDomains;

AllocateFunction FromDomain;

AllocateFunction FromAnyDomain;

ProximityDomain* DomainNew( RootTable* loaderTable, ApparatusInfo* info );

void LinkDomain( ProximityDomain* self, ProximityDomain* arenas[], u8 latency[], usize domainCount );

void ResideSharedData( ProximityDomain* source );