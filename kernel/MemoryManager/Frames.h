/* Supertos Industries (2012 - 2025)
 * Skip-list based physical memory allocator.
 * Uses address- and size- sorted double-linked skip-lists for coalescing and searching.
 * 
 * Designed to prioritize 2M (on x86-64) hugepages, splitting and coalescing ranges in a maneer that:
 * All frames are either:
 * - have size 2M * k and aligned to 2M;
 * - fully contained within [2M * a, 2M * (a+1)]
 *
 * TryAllocateFrame utilises default first-fit strategy since allocator is designed to delegate small (<2M) chunks management to hot paths.
 *
 * ToFrames algorithm:
 * Find freed frame's address predecessor.
 * Attempt to move pages from freed frame to predecessor.
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
#include "Concurrency/Spinlock.h"

#include "Prototypes/MemoryManager.h"

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
	Spinlock Lock;
} Frames;