/* Supertos Industries (2012 - 2025) */
#pragma once

#include "Base.h"
#include "ACPI.h"

#define SRAT_CPU 0x0
#define SRAT_MEM 0x1

#define SRAT_SIGNATURE "SRAT"

typedef struct {
    u8  EntryType;
    u8  EntrySize;
    u32 NUMAID;
    u8  __Reserved1[2];
    u64 Base;
    u64 Size;
    u8  __Reserved2[4];
    u32 Flags;
    u8  __Reserved3[8];
} __attribute__ ((packed)) MemoryAffinity;

typedef struct {
    u8  EntryType;
    u8  EntrySize;
    u8  NUMAIDLow;
    u8  APICID;
    u32 Flags;
    u8  SAPIC_EID;
    u8  NUMAIDHigh[3];
    u32 _CDM;
} __attribute__((packed)) CPUAffinity;

static inline bool MemoryEnabled( MemoryAffinity* affinity ) {
    return affinity->Flags & 1;
}

static inline bool CPUEnabled( CPUAffinity* affinity ) {
    return affinity->Flags & 1;
}

u32 CPUAffinityNUMA( CPUAffinity* self ) {
    u32 id = self->NUMAIDLow;
    id |= (u32)self->NUMAIDHigh[0] << 8;
    id |= (u32)self->NUMAIDHigh[1] << 16;
    id |= (u32)self->NUMAIDHigh[2] << 24;
    return id;
}

typedef struct SRAT {
    ACPIHeader Header;
    u8 __Reserved[12];
} __attribute__((packed)) SRAT;
