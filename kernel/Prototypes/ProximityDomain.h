/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Base.h"
#include "MemoryManager.h"

typedef struct ProximityDomain ProximityDomain;
typedef struct Cache Cache;

/* Return false from function to finish early. */
void RunOnAllDomains( ProximityDomain* base, bool (*function)( ProximityDomain* domain, void* in, void* out ), void* in, void* out );

FreeFunction ToDomains;

AllocateFunction FromDomain;

AllocateFunction FromAnyDomain;

ProximityDomain* ProximityDomainNew( RootTable* loaderTable, ApparatusInfo* info );

ProximityDomainInfo* DomainInfo( ProximityDomain* self );

void LinkDomain( ProximityDomain* self, ProximityDomain* domains[], u8 latency[], usize domainCount );

void ResideSharedData( ProximityDomain* source );

LocalVMA* ProximityDomainVMA( ProximityDomain* self );

typedef struct Core Core;

usize CoreMetaSize();

Cache* CoreCache( Core* self, usize size );

Core* CoreNew( void* base, ProximityDomain* owner, u32 smpID );

LocalVMA* CoreVMA( Core* self );

u64 CoreSMPID( Core* self );

Allocator GetFramesAllocator( Core* self );

void RegisterCoreVSpace( Core* self, void* item );

void UnregisterCoreVSpace( Core* self, void* item );

#define NEW(type, ...) type##New( FromCache(CacheOfSize(CoreMeta(), sizeof(type))), ... )

#define DISPOSE(obj) ToCache( obj )

#define NEW_REMOTE(core, type, ...) type##New( FromCache(CacheOfSize(core, sizeof(type))), ... )

#define DISPOSE_REMOTE(core, obj) ToCache( obj )

Core* ResideCore( ProximityDomain* self, CPUInfo cpu );