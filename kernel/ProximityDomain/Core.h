/* Supertos Industries (2012 - 2026) */
#pragma once

#include "MemoryManager/Primitives/Cache.h"
#include "Prototypes/ProximityDomain.h"
#include "Prototypes/VirtualMemory.h"
#include "Structures/RBTree.h"

#define CACHE_BASE 16
#define CACHE_COUNT LOG2_32(CACHELINE) - LOG2_32(CACHE_BASE) + 1

typedef struct ProximityDomain ProximityDomain;
typedef struct Core Core;

typedef struct {
	ProximityDomain* Owner;
	u32 SMPID;
	
	Cache Caches[CACHE_COUNT];
	
	VirtualSpace* KernelSpace;
	RBTree SpaceTree;
	/* CoreScheduler Scheduler; */
} Core;
