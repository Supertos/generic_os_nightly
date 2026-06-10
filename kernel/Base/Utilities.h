/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Types.h"

static inline usize AlignUp( usize value, usize align ) {
    return ( (value + align - 1) / align ) * align;
}

static inline usize AlignDown( usize value, usize align ) {
    return (value / align) * align;
}

static inline bool Aligned( usize value, usize align ) {
    return value % align == 0;
}

static inline char* FillMemory( char* base, usize count, char val ) {
    for( usize i = 0; i < count; i++ )
        base[i] = val;
	return base;
}
