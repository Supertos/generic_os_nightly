/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Architecture.h"

#define tail [[gnu::musttail]]
#define compiler_barrier __asm__ __volatile__ ("" ::: "memory");
#define offset_of(type, member) (usize)&(((type *)0)->member)
#define bits_of(type) (sizeof(type) * BITS)