/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Base.h"
#include "MemoryManager.h"
#include "VirtualMemory.h"

typedef struct Domain Domain;
typedef struct Cache Cache;
typedef struct Core Core;

typedef struct LocalVMA LocalVMA;

/* Return false from function to finish early. */
void RunOnAllDomains( Domain* base, bool (*function)( Domain* domain, void* in, void* out ), void* in, void* out );

FreeFunction ToDomains;

AllocateFunction FromDomain;

AllocateFunction FromAnyDomain;

Domain* DomainNew( RootTable* loaderTable, MachineInfo* info );

MemoryDomainInfo* DomainInfo( Domain* self );

void LinkDomain( Domain* self, Domain* domains[], u8 latency[], usize domainCount );

void ResideSharedData( Domain* source );

LocalVMA* DomainVMA( Domain* self );

typedef struct Core Core;

usize CoreMetaSize();

Core* CoreMeta();

Cache* CoreCache( Core* self, usize size );

Core* CoreNew( void* base, Domain* owner, u32 smpID, usize physMemory );

LocalVMA* CoreVMA( Core* self );

u64 CoreSMPID( Core* self );

Allocator GetFramesAllocator( Core* self );

void RegisterCoreVSpace( Core* self, void* item );

void UnregisterCoreVSpace( Core* self, void* item );

#define NEW(type, ...) type##New( FromCache(CoreCache(CoreMeta(), sizeof(type))), __VA_ARGS__ )

#define DISPOSE(obj) ToCache( obj )

#define NEW_REMOTE(core, type, ...) type##New( FromCache(CoreCache(core, sizeof(type))), __VA_ARGS__ )

#define DISPOSE_REMOTE(core, obj) ToCache( obj )

Core* ResideCore( Domain* self, CPUInfo cpu, usize physMemory );