/* Supertos Industries (2012 - 2026) 
 * Collection of SLABs objects with automatic lifetime control.
 */
#pragma once

#include "Base.h"

#include "Concurrency/RWLock.h"
#include "Structures/List.h"

typedef struct {
	RWLock InUse;
	List* Fragments;
	u16 BinSize;
	
	Allocator Source;
} Cache;

void* FromCache( Cache* self );

void ToCache( uptr addr );

Cache* CacheNew( void* base, u16 binSize, Allocator source );