/* Supertos Industries (2012 - 2026) */
#pragma once

#include <assert.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

#define __TYPE_SIZE(type, size) static_assert(sizeof(type) == size, "Bad " #type ". Check compiler.");

__TYPE_SIZE(u8, 1);
__TYPE_SIZE(u16, 2);
__TYPE_SIZE(u32, 4);
__TYPE_SIZE(u64, 8);

#define NULL (void*)0
#define NOID (~(usize)0)
#define VNULL ((uptr)~0ULL)

typedef int(*CompareFunction)(void*, void*);