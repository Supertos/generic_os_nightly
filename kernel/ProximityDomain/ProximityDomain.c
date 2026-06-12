/* Supertos Industries (2012 - 2026) */

#include "ProximityDomain.h"
#include "Core.h"
#include "PhysicalMemory/Frames.h"
#include "VirtualMemory/VAllocator.h"
#include "PhysicalMemory/Cache.h"


void RunOnAllDomains( Domain* base, bool (*function)(Domain*, void*, void*), void* in, void* out ) {
	for( usize i = 0; i < base->LinkCount; ++i )
		if( !function( base->Links[i], in, out ) ) break;
}


Domain* DomainNew( RootTable* loaderTable, MachineInfo* info ) {
	MemoryDomainInfo domain = NextMemoryDomainInfo(info);
	if( !ValidProximityDomain(domain) ) return NULL;
	
	u32 coreSize = AlignUp(sizeof(Core), CACHELINE), coreCount = domain.CoreCount;
	u32 linkSize = sizeof(Domain*), linkCount = ProximityDomainCount(info);
	
	u32 framesOffset = AlignUp(sizeof(Domain), CACHELINE);
	u32 lvmaOffset = framesOffset + AlignUp(sizeof(Frames), CACHELINE);
	u32 coreOffset = lvmaOffset + AlignUp(sizeof(LocalVMA), CACHELINE);
	u32 scacheOffset = coreOffset + AlignUp(sizeof(Cache), CACHELINE);
	u32 linkOffset = scacheOffset + coreSize * coreCount;
	u32 needMemory = linkOffset + linkSize * linkCount;
					 
	Frames temp; FramesNew(&temp);
	
	MemoryEntry* map = loaderTable->MemoryMap;
	usize entryCount = loaderTable->MemoryMapEntryCount;
	do {
		uptr base = MAX(domain.Base, map->Base), end = MIN(domain.End, map->Base + map->Pages * PAGE);
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

MemoryDomainInfo* DomainInfo( Domain* self ) {
	return &self->Info;
}

static const usize gaps[] = {/*1750, 701, 301, 132, 57,*/ 23, 10, 4, 1, 0}; // (https://oeis.org/A102549).

void LinkDomain( Domain* self, Domain* domain, u8 latency[], usize domains ) {
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
				   latency[links[i]] :
				   NULL;
}


bool ShareData( Domain* self, InterDomainData* in, void* out ) {
	self->SharedData = in;
	CacheNew( self->SCache, sizeof(SVMAEntry), (Allocator){self, FromAnyDomain, ToDomains} );
	LocalVMANew( self->VirtualRanges, self->SCache, in->VirtualRanges );
	
	return true;
}


void ResideSharedData( Domain* source ) {
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
	
	CacheNew( sCache, sizeof(SVMAEntry), (Allocator){source, FromAnyDomain, ToDomains} );
	CacheNew( eCache, RBAllocatorCacheSize(), (Allocator){source, FromAnyDomain, ToDomains} );
	GlobalVMANew( gvma, sCache, eCache );
	
	RunOnAllDomains( source, ShareData, data, NULL );
}


Core* ResideCore( Domain* self, CPUInfo cpu, usize physMemory ) {
	return CoreNew( &self->Cores[self->CoreCount++], self, cpu.SMPID, physMemory );
}


void ToDomains( Domain* self, MemoryBlock block ) {
	ToFrames( self->Memory, block );
}


MemoryBlock FromDomain( Domain* self, usize size, usize align ) {
	return FromFrames( self->Memory, size, align );
}


typedef struct { usize Size; usize Align } QueryIn;


bool DomainQuery( Domain* self, QueryIn* in, MemoryBlock* out ) {
	*out = FromDomain( self, in->Size, in->Align );
	
	return !ValidMemory(*out);
}


MemoryBlock FromAnyDomain( Domain* self, usize size, usize align ) {
	MemoryBlock out;
	QueryIn in = (QueryIn){ .Size = size, .Align = align };
	RunOnAllDomains( self, DomainQuery, &in, &out );
	
	return out;
}


LocalVMA* DomainVMA( Domain* self ) {
	return self->VirtualRanges;
}