/* Supertos Industries (2012 - 2025) */

#pragma once

#include "Base.h"
#include "ACPI.h"

#define SRAT_CPU_AFFINITY 0x0
#define SRAT_MEM_AFFINITY 0x1

#define SRAT_SIGNATURE "SRAT"

typedef struct MemoryAffinity MemoryAffinity;
struct MemoryAffinity {
    u8  EntryType;
    u8  EntrySize;
    u32 NUMAID;
    u8  __Reserved1[2];
    u64 Base;
    u64 Size;
    u8  __Reserved2[4];
    u32 Flags;
    u8  __Reserved3[8];
} __attribute__ ((packed));

typedef struct CPUAffinity CPUAffinity;
struct CPUAffinity {
    u8  EntryType;
    u8  EntrySize;
    u8  NUMAIDLow;
    u8  APICID;
    u32 Flags;
    u8  SAPIC_EID;
    u8  NUMAIDHigh[3];
    u32 _CDM;
} __attribute__((packed));


static inline bool IsMemoryAffinityEnabled( MemoryAffinity* affinity ) {
    return affinity->Flags & 1;
}


static inline bool IsCPUAffinityEnabled( CPUAffinity* affinity ) {
    return affinity->Flags & 1;
}


u32 CPUAffinityNUMA( CPUAffinity* affinity ) {
    u32 id = affinity->NUMAIDLow;
    id |= (u32)affinity->NUMAIDHigh[0] << 8;
    id |= (u32)affinity->NUMAIDHigh[1] << 16;
    id |= (u32)affinity->NUMAIDHigh[2] << 24;
    return id;
}

typedef struct SRAT SRAT;
struct SRAT {
    ACPIHeader Header;
    u8 __Reserved[12];
} __attribute__((packed));
