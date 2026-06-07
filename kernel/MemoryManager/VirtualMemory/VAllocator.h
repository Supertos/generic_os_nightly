/* Supertos Industries (2012 - 2026) 
 * VMA Hierarchy implementation.
 *
 * Shared Virtual Memory: normal vmem, no guarantees.
 * Exclusive Virtual Memory: vmem that is guaranteed to be free in every VirtualSpace. Used when consistency of pointers across different VirtualSpaces is needed.
 * 
 * 
 * GlobalVMA:
 *  Resides in Median NUMA (lowest avg. latency),
 *  Monotonically grows Bump's base and marks memory either shared or exclusive,
 *  Supports list of every shared memory ever created, used when new LocalVMA's created to catch up with others.
 *  Is the only entity to provide exclusive ranges.
 *  Shared list tends to slow down (or stop entirely) growing after a while since it accumulates huge amounts of virtual memory enough for any process.
 * 
 * LocalVMA:
 * Resides locally in every NUMA.
 * Caches GlobalVMA's shared memory list to provide newly created VirtualSpaces with usable shared virtual memory.
 * Local list growth slows down as well.
 * 
 * VSpaceVMA:
 * Created for every Process Physical Memory Manager.
 * Tracks free memory memory to map to.
 * Supports free since it tracks actual memory usage.
 * If Source is NULL, relies on external code to replenish.
 */

#pragma once
 
#include "Base.h"

#include "MemoryManager/Primitives/Cache.h"
#include "MemoryManager/Primitives/Bump.h"
#include "MemoryManager/Primitives/RBAllocator.h"
#include "Structures/List.h"
#include "Structures/RBTree.h"

#include "Concurrency/Spinlock.h"
#include "Concurrency/RWLock.h"

typedef struct {
    uptr Base;
    uptr End;
    ListIndex Index;
} SVMAEntry;

typedef struct {
    Cache* SCache;
    SVMAEntry* Last;
    List Map;
    RWLock Lock;
} VMASharedRanges;

typedef struct {
    SVMAEntry* Cur;
    uptr Top;
    Spinlock Lock;
    VMASharedRanges* Source;
} SVMACursor;

typedef struct {
    VMASharedRanges Shared;
    Bump Source;
    RBAllocator Exclusive;
} GlobalVMA;

typedef struct {
    VMASharedRanges Shared;
    SVMACursor Source;
    GlobalVMA* Global;
} LocalVMA;

typedef struct {
    RBAllocator* Free;
    SVMACursor Source;
    LocalVMA* Local;
} VSpaceVMA;

MemoryBlock FromVSpace( VSpaceVMA* self, usize size, usize align );

void ToVSpace( VSpaceVMA* self, MemoryBlock entry );

MemoryBlock FromExclusiveVMA( VSpaceVMA* self, usize size, usize align );

void ToExclusiveVMA( VSpaceVMA* self, MemoryBlock entry );

VSpaceVMA* VSpaceVMANew( void* begin, RBAllocator* allocator, Cache* eCache, LocalVMA* source );