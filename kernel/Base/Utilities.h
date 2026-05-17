/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Types.h"
#include "Architecture.h"

#define MIN(x, y) ({ typeof(x) _x = x; typeof(y) _y = y; _x < _y ? _x : _y; })
#define MAX(x, y) ({ typeof(x) _x = x; typeof(y) _y = y; _x > _y ? _x : _y; })

#define MASK(type, pos, length) ({ 							\
	usize _length = length, _pos = pos;					    \
	type _0 = 0, _1 = 1;									\
															\
	bool _ubShift = _length >= sizeof(type) * BITS;	        \
	type _mask = _ubShift ? ~_0 : ((_1 << _length) - _1);	\
	_mask << _pos; 											\
})

static inline usize AlignUp( usize value, usize align ) {
    return ( (value + align - 1) / align ) * align;
}

static inline usize AlignDown( usize value, usize align ) {
    return (value / align) * align;
}

static inline bool Aligned( usize value, usize align ) {
    return value % align == 0;
}

static inline void FillMemory( char* base, usize count, char val ) {
    for( usize i = 0; i < count; i++ )
        base[i] = val;
}
