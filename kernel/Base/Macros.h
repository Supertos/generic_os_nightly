/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Architecture.h"
#include "Types.h"

#define tail [[gnu::musttail]]
#define compiler_barrier __asm__ __volatile__ ("" ::: "memory");
#define offsetof(type, member) (usize)&(((type *)0)->member)
#define bitsof(type) (sizeof(type) * BITS)

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

#define LOG2_32(x) ( \
    ((x) & 0xFFFF0000 ? 16 : 0) + \
    (((x) & 0xFF00FF00 ? 8 : 0)) + \
    (((x) & 0xF0F0F0F0 ? 4 : 0)) + \
    (((x) & 0xCCCCCCCC ? 2 : 0)) + \
    (((x) & 0xAAAAAAAA ? 1 : 0)) \
)