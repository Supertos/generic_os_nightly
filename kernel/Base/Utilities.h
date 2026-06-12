/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Types.h"

static inline u64 AlignUp( u64 value, u64 align ) {
    return ( (value + align - 1) / align ) * align;
}

static inline u64 AlignDown( u64 value, u64 align ) {
    return (value / align) * align;
}

static inline bool Aligned( u64 value, u64 align ) {
    return value % align == 0;
}

static inline char* FillMemory( char* base, u64 count, char val ) {
    for( u64 i = 0; i < count; i++ )
        base[i] = val;
	return base;
}
