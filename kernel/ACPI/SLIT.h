/* Supertos Industries (2012 - 2025) */
#pragma once

#include "Base.h"
#include "ACPI.h"

#define SLIT_SIGNATURE "SLIT"

#define SLIT_SELF 10

typedef struct {
    ACPIHeader Header;
    u64 LocalityCount;
    u8 Latency[];
} __attribute__ ((packed)) SLIT;