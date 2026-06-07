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

void* FromCache( Cache* cache );

void ToCache( Cache* cache, void* addr );

Cache* CacheNew( void* begin, u16 binSize, char* mark );

Cache* CacheSetAllocator( Cache* self, void* allocator, AllocateFunction allocate, FreeFunction free );
