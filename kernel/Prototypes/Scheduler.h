/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Types.h"

// typedef struct CoreScheduler CoreScheduler;
// typedef struct DomainScheduler DomainScheduler;
// typedef struct GlobalScheduler GlobalScheduler;

// #define RECLAIM_READY 0
// #define RECLAIM_IN_PROCESS 1
// #define RECLAIM_SUCCESS 2

/* To be defined elsewhere. */
/* Called on local OOM. Set *readySignal to RECLAIM_IN_PROCESS if successfully reserved memory for further reclaim. */
/* If reclamant is the same core running ProcessPhysicalMemory, free memory immediately and set readySignal to RECLAIM_SUCCESS. */
/* Do not try to modify PageTables (Single PageTable Operator Principle) inside PPM's if core is different from owner, wait for SynchroPoint to be ran on its owner core. */
/* Return ProximityDomain to which memory will be freed once *readySignal is RECLAIM_SUCCESS. */
/* Return NULL on failure. */
/* Reclamant may continue execution and check state on either entry/exit from VirtualSpace or PageFault. */
// ProximityDomain SchedulerRequestReclaim( void* Scheduler, usize size, usize align, volatile u8* readySignal );
