/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Types.h"

typedef struct ProximityDomain ProximityDomain;

/* Return false from function to finish early. */
void RunOnAllDomains( ProximityDomain* base, bool (*function)( ProximityDomain* domain, void* in, void* out ), void* in, void* out );

FreeFunction ToDomains;

AllocateFunction FromDomain;

AllocateFunction FromAnyDomain;

ProximityDomain* DomainNew( RootTable* loaderTable, ApparatusInfo* info );

void LinkDomain( ProximityDomain* self, ProximityDomain* arenas[], u8 latency[], usize domainCount );

void ResideSharedData( ProximityDomain* source );

typedef struct Core Core;

usize CoreMetaSize();