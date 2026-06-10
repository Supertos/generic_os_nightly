/* Supertos Industries (2012 - 2026) | Ivan Sigaev (@ivan-r-sigaev) */

#include "Base.h"
#include "RWLock.h"


#define READER_COUNT_MASK MASK(u32, 0, 16)
#define READER_COUNT_WRITE_LOCKED READER_COUNT_MASK
#define READER_COUNT_MAX (READER_COUNT_WRITE_LOCKED - 1)
#define ONE_WRITER MASK(u32, 16, 1)


static inline u32 GetReaderCount( RWLock lock ) {
    return lock & READER_COUNT_MASK;
}


static inline u32 GetWaitingWriterCount( RWLock lock ) {
    return lock >> 16;
}


RWLock* AcquireReader( RWLock* lock ) {
    while( true ) {
        RWLock copy = *lock;

        if( GetWaitingWriterCount(copy) != 0 ) goto spin;
        // If readerCount is equal to READER_COUNT_MAX RWLock will wait instead of overflowing.
        if( GetReaderCount(copy) >= READER_COUNT_MAX ) goto spin;
        if( !TryCAS32( lock, &copy, copy + 1 ) ) goto spin;

        break;
    spin:
        Pause();
    }
    return lock;
}


void ReleaseReader( RWLock* lock ) {
    FetchSub32(lock, 1);
}


RWLock* AcquireWriter( RWLock* lock ) {
    RWLock copy = *lock;
    RWLock success = copy | READER_COUNT_WRITE_LOCKED;
    if( GetReaderCount(copy) == 0 )
        if( TryCAS32( lock, &copy, success ) ) return NULL;

    Pause();

    // This may hypothetically overflow if there are 65 535 threads currently waiting to acquire the write lock.
    FetchAdd32(lock, ONE_WRITER);

    while( true ) {
        copy = *lock;
        success = copy | READER_COUNT_WRITE_LOCKED;

        if( GetReaderCount(copy) != 0 ) goto spin;
        if( !TryCAS32(lock, &copy, success) ) goto spin;

        FetchSub32(lock, ONE_WRITER);
        break;
    spin:
        Pause();
    }
    return lock;
}


void ReleaseWriter( RWLock* lock ) {
    while( true ) {
        RWLock copy = *lock;
        RWLock success = copy & !READER_COUNT_WRITE_LOCKED;
        if( TryCAS32( lock, &copy, success ) ) break;
    }
}