/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Base.h"

typedef struct MapInterface MapInterface;
struct MapInterface {
	// Map specific physics at specific virtual address. MapInterface is expected to map pBegin at vBegin.
	void (*OnMapAt)( void* ppm, void* space, uptr pBegin, uptr vBegin, usize length, char* mode );

	// Map any physics at any address withing provided allocactor. MapInterface is expected to return mapped vBegin.
	// Return VNULL if memory is unavailable.
	// Return any valid vaddr even if memory is yet to be mapped.
	void* (*OnMap)( void* ppm, void* space, usize length, void* allocator );

	// Unmap vBegin (and decrease reference count if supported)
	// API guarantees that [vBegin, vBegin + length] corresponds to [pBegin, pBegin + length]
	void (*OnUnmap)( void* ppm, void* space, uptr pBegin, void* vBegin, usize length );
	
	// Return false on critical failure, return true on continue operation.
	// May be used as SynchroPoint to map reserved physics.
	bool (*PageFault)( void* ppm, void* region, uptr pAddr, void* vAddr );
	void* Object; 
};