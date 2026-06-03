/* Supertos Industries (2012 - 2026) */
#include "Base.h"

#include "Structures/Bitmap.h"
#include "SLAB.h"

#define FREE 0
#define USED 1
#define USED_WHOLE ~0ULL


static inline Bitmap* Blocks( SLAB* slab ) {
    return (char*)slab + sizeof(SLAB);
}


SLAB* SLABNew( uptr begin, uptr metaBegin, usize binSize, usize metaSize ) {
	bool internal = (begin == metaBegin);
    SLAB* slab = metaBegin;
    *slab = (SLAB){ .Base = begin, .Size = binSize };

    if( internal )
        begin = AlignUp( SLABMeta(slab) + metaSize, MAX(CACHELINE, binSize) );

    usize slots = (-begin % PAGE) / binSize, maxSlots = PAGE / binSize;

    slab->MaxElements = slots;
    slab->Elements = slots;

    Bitmap* blocks = Blocks(slab);
    FillMemory( blocks, BITMAP_SIZE(slots), USED_WHOLE );

    if( internal )
        SetBitmap( BitmapElement(blocks, 0, maxSlots - slots), FREE );
    return slab;
}


void* FromSLAB( SLAB* slab ) {
    if( slab->Elements == 0 ) return NULL;

    usize slot = NOID;
    while( slot == NOID ) {
        BitmapModify mod = SearchBitmap( Blocks(slab), BITMAP_SIZE(PAGE / slab->MaxElements), 1 );
        if( !ValidModify(mod) ) return NULL;

        slot = TrySetBitmapAtomic(mod, USED);
    }
	
    FetchSub16(&slab->Elements, 1);
    return slab->Base + slot * slab->Size;
}


void ToSLAB( SLAB* slab, uptr addr ) {
    if( addr == NULL ) return;

    usize slot = (addr - slab->Base) / slab->Size;

    BitmapModify mod = BitmapElement( Blocks(slab), slot, FREE );
    while( TrySetBitmapAtomic(mod, 0) == NOID );
    
    FetchAdd16(&slab->Elements, 1);
}