/* Supertos Industries (2012 - 2026) 
 * MemoryManager Interface.
 * Collection of forward declarations used outside of MemoryManager.
 */
#pragma once
 
#include "Base.h"

typedef struct Frames Frames;

FreeFunction ToFrames;
AllocateFunction FromFrames;

Frames* FramesNew( void* base );
Frames* MoveFrames( Frames* from, void* toAddr );

typedef struct Cache Cache;

void* FromCache( Cache* self );

void ToCache( void* addr );

Cache* CacheNew( void* base, u16 binSize, Allocator source );

Cache* CacheSetAllocator( Cache* self, void* allocator, AllocateFunction allocate, FreeFunction free );
