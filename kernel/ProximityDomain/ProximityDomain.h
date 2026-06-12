/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Base.h"
#include "Prototypes/MemoryManager.h"
#include "Prototypes/VirtualMemory.h"
#include "Prototypes/ProximityDomain.h"

typedef struct {
	GlobalVMA* VirtualRanges;
	Cache* SCache;
	Cache* ECache;
	/* GlobalScheduler Scheduler; */
} InterDomainData;

struct MemoryDomain {
    MemoryDomainInfo Info;
	
	InterDomainData* SharedData;
	
	Frames* Memory;
	LocalVMA* VirtualRanges;
	Cache* SCache;
	/* DomainScheduler Scheduler; */
	
	u32 LinkCount;
	u32 CoreCount;
	
	Core* Cores;
	MemoryDomain** Links;
};
