/* Supertos Industries (2012 - 2026) 
 * Includes actual architecture definitions.
 */
#pragma once

#if defined(__x86_64__)
	#include "x86-64/x86-64.h"
	#include "ACPI/ACPIAppratusInfo.h"
#else
    #error "Invalid target apparatus."
#endif