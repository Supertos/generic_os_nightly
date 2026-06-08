/* Supertos Industries (2012 - 2025)
 * Bump Allocator used in early stages.
 * Used to place heterogenous data inside big memory region.
 * Always aligned to usize.
 * Crashes kernel exhausted.
 */
#pragma once

#include "Base.h"
#include "Concurrency/Spinlock.h"

typedef struct {
    char* Mark;
    uptr Base;
    uptr End;
    Spinlock Lock;
} Bump;

static inline bool CanFromBump( Bump* bump, usize size ) {
    return bump->Base + size <= bump->End;
}

MemoryBlock FromBump( Bump* bump, usize size, usize align ) {
    ACQUIRE_AUTO(&bump->Lock);
    align = align ?: 1;
    uptr out = bump->Base;
    size += (-out % align);

    if( !CanFromBump(bump, size) ) KernelCrashUndBurn(bump->Mark);
    bump->Base = AlignUp( out + size, sizeof(usize) );
    return (MemoryBlock){ out, out + size };
}

static inline Bump NewBump( uptr begin, usize size, char* mark ) {
    return (Bump){ 
        .Base = AlignUp(begin, sizeof(usize)),
        .End = begin + size, 
        .Mark = mark
    };
}
