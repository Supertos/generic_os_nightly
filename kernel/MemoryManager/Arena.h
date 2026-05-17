
#pragma once

typedef struct Arena Arena;

#include "Frames.h"
#include "VirtualMemory/PhysicsInterface.h"

#include "Concurrency/Spinlock.h"
#include "Concurrency/RWLock.h"


struct Arena {
    uptr Base;
    usize End;
	u32 NUMAID;
	Spinlock FramesLock;
	RWLock ReclaimLock;
    Frames Frames;
	List ReclaimList;
	Bump ResBump;
    Arena* LinkedArenas[];
};


// From UEFI Bootloader. See loader/loader.h
typedef struct MemoryEntry MemoryEntry;
struct MemoryEntry {
	uptr Base;
	size_t Pages : 56;
	uint8_t Type : 8;
} __attribute__((__packed__));


typedef struct {
	uptr Base;
	uptr End;
	u32 Flags;
	u32 NUMAID;
} ArenaInfo;


Arena* ArenaNew( MemoryEntry* map, usize entryCount, ArenaInfo info, usize totalArenas, usize reserveMemory ) {
	Frames temp; FramesNew(&temp);
   
	do {
		uptr base = MAX(info.Base, map->Base), end = MIN(info.End, map->Base + map->Pages * PAGE);
		if( base == end ) continue;

		ToFrames( &temp, (MemoryBlock){ base, end - base } );
	} while( entryCount-- > 0 && map++ );

	usize size = sizeof(Arena) + sizeof(Arena*) * (totalArenas + 1);
	
	MemoryBlock outBlock = FromFrames( &temp, size + reserveMemory, true );
	if( !ValidMemory(outBlock) ) KernelCrashUndBurn( "Invalid NUMA layout." );
	Arena* self = outBlock.Base;

	*self = (Arena){ .Base = info.Base, .End = info.End, .NUMAID = info.NUMAID, .ResBump = NewBump(outBlock.Base + size, reserveMemory, "NUMA INIT BUMP") };
	MoveFrames(&temp, &self->Frames);
	
	return self;
}


static const usize gaps[] = {/*1750, 701, 301, 132,*/ 57, 23, 10, 4, 1, 0}; // (https://oeis.org/A102549).

#define LATENCY_UNREACHABLE 255

void LinkArenas( Arena* self, Arena* arenas[], u8 latency[], usize arenaCount ) {
	usize* links = self->LinkedArenas;
	for( usize i = 0; i < arenaCount; i++ ) links[i] = i;

	// Shell sort 
	for( usize* gap = gaps; *gap; gap++ ) {
		for( usize base = *gap; base < arenaCount; base++ ) {
			usize numaA = links[base], *numaB = &links[base];

			for( ; numaB - *gap >= links && latency[*(numaB - *gap)] > latency[numaA]; numaB -= *gap )
				*numaB = *(numaB - *gap);

			*numaB = numaA; 
		}
	}

	usize i = 0;
	for( ; i < arenaCount; i++ ) {
		self->LinkedArenas[i] = arenas[links[i]];
		if( latency[links[i]] != LATENCY_UNREACHABLE ) continue;
		break;
	}

	self->LinkedArenas[i] = NULL;
}


static inline bool ReclaimFromArena( Arena* arena, usize size, volatile u8* readyFlag ) {
	ACQUIRE_READER_AUTO(&arena->ReclaimLock);
	SchedulerInterface* interface = NULL;
	while( interface = ListNext(&arena->ReclaimList, interface) )
		if( interface->ReclaimMemory(interface->MapInterface, size, readyFlag) ) return true;
	return false;
}


MemoryBlock FromArena( Arena* arena, usize size, usize align ) {
	ACQUIRE_AUTO(&arena->FramesLock);
	return FromFrames( &arena->Frames, size, 1 );
}


MemoryBlock FromArenaPartial( Arena* arena, usize size, usize align ) {
	ACQUIRE_AUTO(&arena->FramesLock);
	return FromFramesPartial( &arena->Frames, size, 1 );
}


MemoryBlock FromArenas( Arena* base, usize size, bool whole, volatile u8* readyFlag ) {
	for( Arena* cur = base->LinkedArenas; cur; cur++ ) {
		if( readyFlag && *readyFlag != RECLAIM_CLEAR ) return NO_MEMBLOCK;

		MemoryBlock out = whole ? FromArena( cur, size, 1 ) : FromArenaPartial( cur, size, 1 );
		if( ValidMemory(out) ) return out;

		if( !whole && ReclaimFromArena( cur, size, readyFlag ) ) break;
	}

	return NO_MEMBLOCK;
}


MemoryBlock FromArenasKernel( Arena* base, usize size, usize align ) {
	for( Arena* cur = base->LinkedArenas; cur; cur++ ) {
		MemoryBlock out = FromArena( cur, size, align );
		if( ValidMemory(out) ) return out;
	}

	return NO_MEMBLOCK;
}


void ToArenas( Arena* base, MemoryBlock in ) {
	usize freedSize = 0;
	for( Arena* cur = base->LinkedArenas; cur; cur++ ) {
		uptr freeBase = MAX(in.Base, cur->Base), freeEnd = MIN(in.End, cur->End);
		if( freeBase == freeEnd ) continue;
		freedSize += freeEnd - freeBase;
		
		Acquire(&cur->FramesLock);
		ToFrames( &cur->Frames, (MemoryBlock){ freeBase, freeEnd } );
		Release(&cur->FramesLock);

		if( freedSize >= in.End - in.Base ) break;
	}
}