/* Supertos Industries (2012 - 2025) */
#pragma once

#include "Base.h"
#include "ACPI.h"

#define SLIT_LATENCY_SAME_NODE 10
#define SLIT_SIGNATURE "SLIT"

typedef struct {
    ACPIHeader Header;
    u64 LocalityCount;
    u8 LocalityMatrix[];
} __attribute__ ((packed)) SLIT;