/* Supertos Industries (2012 - 2026) */
/*
#pragma once

#include "VSpace.h"
#include "VRegion.h"

#include "Structures/List.h"

#define RECLAIM_CLEAR 0
#define RECLAIM_AWAIT 1
#define RECLAIM_RECLAIM 2

typedef struct {
	// Called on OOM. Unused physical memory of total size [size] shall me marked as unusable internally. Returns true on success. Set *readyFlag to RECLAIM_AWAIT.
	bool (*ReclaimMemory)( void* ppm, usize size, u8* readyFlag );

	// Called when Scheduler finishes (or prepares for) process' execution kwant. 
	// If you were asked to reserve and free memory: unmap and return it back to the Arena. Set provided *readyFlag(s) to RECLAIM_RECLAIM.
	// If you were asking for memory: check *readyFlag(s) is RECLAIM_RECLAIM. Try allocating memory from Arena and mapping it at reserved vaddr. Set *readyFlag back to RECLAIM_CLEAR. Repeat allocation until requested memory is recieved.
	void (*SynchroPoint)( void* ppm );

	// Must be set by scheduler. ReclaimMemory must free memory immediately if InUseByCoreID == 0 and set *readyFlag to RELCAIM_RECLAIM.
	u8 InUseByCoreID;

	void* MapInterface;
	ListIndex Index;
} SchedulerInterface;
*/