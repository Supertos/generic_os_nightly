/* Supertos Industries (2012 - 2026) */

#include "ProximityDomain.h"
#include "MemoryManager/Frames.h"
#include "MemoryManager/VirtualMemory/VAllocator.h"
#include "MemoryManager/Primitives/Cache.h"


void RunOnAllDomains( ProximityDomain* base, bool (*function)(ProximityDomain*, void*, void*), void* in, void* out ) {
	for( usize i = 0; i < base->LinkCount; ++i )
		if( !function( base->Links[i], in, out ) ) break;
}


ProximityDomain* DomainNew( RootTable* loaderTable, ApparatusInfo* info ) {
	ProximityDomainInfo domain = NextProximityDomainInfo(info);
	if( !ValidProximityDomain(domain) ) return NULL;
	
	u32 coreSize = AlignUp(sizeof(Core), CACHELINE), coreCount = domain.AssignedCoresCount;
	u32 linkSize = sizeof(ProximityDomain*), linkCount = ProximityDomainCount(info);
	
	u32 framesOffset = AlignUp(sizeof(ProximityDomain), CACHELINE);
	u32 lvmaOffset = framesOffset + AlignUp(sizeof(Frames), CACHELINE);
	u32 coreOffset = lvmaOffset + AlignUp(sizeof(LocalVMA), CACHELINE);
	u32 scacheOffset = coreOffset + AlignUp(sizeof(Cache), CACHELINE);
	u32 linkOffset = scacheOffset + coreSize * coreCount;
	u32 needMemory = linkOffset + linkSize * linkCount;
					 
	Frames temp; FramesNew(&temp);
	
	MemoryEntry* map = loaderTable->MemoryMap;
	usize entryCount = loaderTable->MemoryMapEntryCount;
	do {
		uptr base = MAX(info.Base, map->Base), end = MIN(info.End, map->Base + map->Pages * PAGE);
		if( base == end ) continue;

		ToFrames( &temp, (MemoryBlock){ base, end - base } );
	} while( entryCount-- > 0 && map++ );
	
	MemoryBlock outBlock = FromFrames(&temp, needMemory, 1);
	if( !ValidMemory(outBlock) ) KernelCrashUndBurn( "Invalid NUMA layout." );
	
	Domain* self = outBlock.Base;
	
	uptr base = outBlock.Base;
	
	*self = (Domain){ 
		.Info = domain, 
		.LinkCount = linkCount, 
		.Cores = base + coreOffset, 
		.Links = base + linkOffset,
		.Memory = base + framesOffset,
		.VirtualRanges = base + lvmaOffset,
		.SharedData = base + needMemory,
		.SCache = base + scacheOffset
	};
	
	MoveFrames(&temp, self->Memory);
	return self;
}

static const usize gaps[] = {/*1750, 701, 301, 132, 57*/, 23, 10, 4, 1, 0}; // (https://oeis.org/A102549).

void LinkDomain( ProximityDomain* self, ProximityDomain* domain, u8 latency[], usize domains ) {
	usize* links = self->Links;
	for( usize i = 0; i < domains; i++ ) links[i] = i;

	// Shell sort 
	for( usize* gap = gaps; *gap; gap++ ) {
		for( usize base = *gap; base < domains; base++ ) {
			usize numaA = links[base], *numaB = &links[base];

			for( ; numaB - *gap >= links && latency[*(numaB - *gap)] > latency[numaA]; numaB -= *gap )
				*numaB = *(numaB - *gap);

			*numaB = numaA; 
		}
	}

	for( usize i = 0; i < domains; i++ )
		links[i] = ReachableLatency(latency[links[i]]) ? 
				   domain[links[i]] :
				   NULL;
}


bool ShareData( ProximityDomain* self, InterDomainData* in, void* out ) {
	self->SharedData = in;
	CacheNew( self->SCache, sizeof(SVMAEntry), "LOCAL SVMA ENTRIES" );
	LocalVMANew( self->VirtualRanges, self->SCache, in->VirtualRanges );
	
	return true;
}


void ResideSharedData( ProximityDomain* source ) {
	InterDomainData* data = source->SharedData;
	
	u32 needMemory = sizeof(InterDomainData) + sizeof(GlobalVMA) + sizeof(Cache) * 2;
	if( AlignUp(data, PAGE) - (uptr)data <= needMemory ) {
		MemoryBlock outBlock = FromFrames(source->Memory, needMemory, 1);
		if( !ValidMemory(outBlock) ) KernelCrashUndBurn( "Invalid NUMA layout." );
		data = outBlock.Base;
	}
	
	GlobalVMA* gvma = (uptr)data + sizeof(InterDomainData);
	Cache* sCache = (uptr)gvma + sizeof(GlobalVMA);
	Cache* eCache = (uptr)sCache + sizeof(Cache);
	
	*data = (InterDomainData){ .VirtualRanges = gvma, .SCache = sCache, .ECache = eCache };
	
	CacheNew( sCache, sizeof(SVMAEntry), "GLOBAL SVMA ENTRIES" );
	CacheNew( eCache, RBAllocatorCacheSize(), "GLOBAL EVMA ENTRIES" );
	GlobalVMANew( gvma, sCache, eCache );
	
	RunOnAllDomains( source, ShareData, data, NULL );
}


void ToDomains( ProximityDomain* self, MemoryBlock block ) {
	ToFrames( self->Memory, block );
}


MemoryBlock FromDomain( ProximityDomain* self, usize size, usize align ) {
	return FromFrames( self->Memory, size, align );
}


typedef struct { usize Size, usize Align } QueryIn;


bool DomainQuery( ProximityDomain* self, QueryIn* in, MemoryBlock* out ) {
	*out = FromDomain( self, in->Size, in->Align );
	
	return !ValidMemory(*out);
}


MemoryBlock FromAnyDomain( ProximityDomain* self, usize size, usize align ) {
	MemoryBlock out;
	QueryIn in = (QueryIn){ .Size = size, .Align = align };
	RunOnAllDomains( self, DomainQuery, &in, &out );
	
	return out;
}