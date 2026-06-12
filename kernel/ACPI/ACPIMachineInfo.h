/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Base.h"
#include "ACPI.h"
#include "SLIT.h"
#include "SRAT.h"

struct MachineInfo {
	void* XSDP;
	void* SRAT;
	void* SLIT;
	
	void* NUMAEntry;
	void* CPUEntry;
	
	u32 CoreCount;
	u32 DomainCount;
};