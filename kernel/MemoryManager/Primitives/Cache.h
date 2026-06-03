/* Supertos Industries (2012 - 2026) 
 * Collection of SLABs objects with automatic lifetime control.
 * Crashes kernel if unable to allocate.
 * Do not use where OOM is not critical.
 */
#pragma once

#include "Base.h"

#include "Concurrency/RWLock.h"
#include "Structures/List.h"


typedef struct {
	u16 BinSize;
	RWLock InUse; // Protects everything.
	List SLABs;
	AllocateFunction Allocate;
	FreeFunction Free;
	void* Allocator;
	char* Mark;
} Cache;
