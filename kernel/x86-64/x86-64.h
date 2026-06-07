/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Base.h"
#include "Assembly.h"
#include "PageWalker.h"
#include "Validation.h"

usize FirstSetBit64( u64 value ) {
    return BSF64(value);
}

usize LastSetBit64( u64 value ) {
    return 63 - LZCNT(value);
}