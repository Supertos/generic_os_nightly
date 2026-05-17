/* Supertos Industries (2012 - 2025) 
 * FrameAllocator - physical memory sequential page allocator.
 * Address-based double linked skip-list storage for quick O(logN) in-place defragmentation purposes.
 * Size-based double linked list for size and linear best-fit search complexity O(n).
 * 
 * FrameAllocator respects hugepage boundaries, supporting three separate lists (2M for default x86_64 hugepage size):
 * Size2MList                - 2M * k frames, aligned to 2M boundary.
 * SizeSmallList             - <2M frames, fully contained in single aligned 2M region.
 * SizeSmallAlignedList      - <2M frames, aligned and fully contained in single aligned 2M region.
 * While Size2MList and SizeSmallList are primary search places, SizeSmallAlignedList is used to de-prioritize aligned frames for further coleasce.
 *
 * All frames, as follows, either:
 * - Are multiple of 2M and aligned to 2M;
 * - Or fully contained inside single aligned 2M memory region.
 *
 * TryAllocateFrame utilises default first-fit strategy since allocator is designed to delegate small (<2M) chunks management to hot paths.
 *
 * MergeFrames removes consumed frame from size and addr list automatically.
 * SplitFrames adds created frame to addr list, but does nothing in regards of size lists. SplitFrames is expected to be called only in FreeFrame.
 *
 * FreeFrame algorithm as follows:
 * Find freed frame's predcessor in skip-list based on its address.
 * Try to transfer pages from freed frame to its predcessor.
 * If successful, try merge predcessor with its predcessor and update size of either predcessor, or its predcessor.
 * If size of freed block is 0, return.
 * Insert freed frame in skip-list.
 * Let remainder be frame.
 * While we should split remainder (boundary violation) or we can merge remainder with the next block:
 *      - Merge if we're able to, remainder is remainder + successor.
 *      - Else split, remainder is the successor out of two parts. Add remainder's predcessor to size list.
 * Add remainder to size list.
 *
 * While loop will not run more than two times. Since all frames are subject to either one of two invariants
 * and can only be merged with frame of the same property, only two adjancent frames may exist at the same time. e.g.:
 * [2M .. 2.5M] [2.5M .. 4M] -> [2M .. 4M], one iteration;
 * [2.2M .. 4M] [4M .. 8M] -> can't coleasce, no iterations;
 * [2M .. 2.2M] [2.2M .. 4M] [4M .. 8M] -> [2M .. 8M] in two iterations.
 */

#pragma once

#include "Structures/Skip.h"

#define SIZE_LIST_LEVELS 8
#define ADDR_LIST_LEVELS 32


typedef struct {
    Skip Size;
    SkipIndex SizeSentinel;
    SkipIndex* SizeIndexLists[SKIPINDEX_LINKS(SIZE_LIST_LEVELS)];
    Skip Address;
    SkipIndex AddressSentinel;
    SkipIndex* AddressIndexLists[SKIPINDEX_LINKS(ADDR_LIST_LEVELS)];

    usize TotalBytes;
    usize Bytes; 
} Frames;


void ToFrames( Frames* self, MemoryBlock block );

MemoryBlock FromFrames( Frames* self, usize size, usize align );

MemoryBlock FromFramesPartial( Frames* self, usize size, usize align );

Frames* FramesNew( void* base );

Frames* MoveFrames( Frames* from, void* toAddr );