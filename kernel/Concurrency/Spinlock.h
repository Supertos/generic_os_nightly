/* Supertos Industries (2012 - 2026) */
#pragma once
#include "Base.h"

#define ACQUIRE_AUTO(lock) Spinlock* __internalLock##__LINE__ __attribute__((cleanup(__SpinlockReleaseMacro))) = Acquire(lock);

typedef volatile u16 Spinlock;


static inline Spinlock* Acquire( Spinlock* lock ) {
    u16 expected = 0;
    while( !TryCAS16( lock, &expected, 1 ) ) {
        expected = 0;
        Pause();
    }

    return lock;
}


static inline void Release( Spinlock* lock ) {
    compiler_barrier;
    *lock = 0;
    architecture_barrier;
}


void __SpinlockReleaseMacro( Spinlock** lock ) {
    Release(*lock);
}


static inline InitializeSpinlock( Spinlock* lock ) {
    *lock = 0;
}