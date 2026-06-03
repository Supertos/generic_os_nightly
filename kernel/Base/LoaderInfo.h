/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Types.h"

typedef struct VideoInfo VideoInfo;
struct VideoInfo {
	void* BufferBegin;
	usize BufferSize;
	usize Width;
	usize Height;
	usize PixelsPerScanline;
	bool IsBGR;
};

typedef struct MemoryEntry MemoryEntry;
struct MemoryEntry {
	void* Base;
	usize Pages : 56;
	u8 Type : 8;
} __attribute__((__packed__));

typedef struct RootTable RootTable;
struct RootTable {
	void* XSDP;
	usize MemoryMapEntryCount;
	MemoryEntry* MemoryMap;
	VideoInfo Video;
	usize BlockSizeInPages;
};