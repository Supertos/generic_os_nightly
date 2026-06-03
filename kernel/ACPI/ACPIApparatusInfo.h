/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Base.h"

struct ApparatusInfo {
	void* XSDP;
	void* SRAT;
	void* SLIT;
	
	void* NUMAEntry;
	void* CPUEntry;
	
	u32 CoreCountCache;
	u32 DomainCountCache;
};
