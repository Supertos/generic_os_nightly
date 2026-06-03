/* Supertos Industries (2012 - 2026) */

#include "Base.h"
#include "Bitmap.h"

static inline bool CanFitFromEnd( Bitmap unit, usize slotNo, usize slots ) {
    Bitmap mask = MASK(Bitmap, slotNo, slots);
    return ((~unit) & mask) == mask;
}

 
static inline bool TryFit( Bitmap* unit, usize slots, BitmapModify* out ) {
    Bitmap expect = *unit, cur = expect;
    usize base = 0;

    while( !CanFitFromEnd(cur, base, slots) && base < BITMAP_BITS ) {
        base = FirstSetBit(cur) + 1;
        cur &= MASK(Bitmap, base, BITMAP_BITS);
    }

    if( base >= BITMAP_BITS ) unit = NULL;

    *out = (BitmapModify){ .Expect = expect, .Offset = base, .Unit = unit };
    return !!unit;
}


BitmapModify SearchBitmap( Bitmap* base, usize unitCount, usize slots ) {
    BitmapModify out = (BitmapModify){0};
    if( slots == 0 || unitCount * BITMAP_BITS <= slots ) return out;

    usize fit = 0, i = -1;
    while( ++i < unitCount && fit < slots ) {
        if( !out.Unit || !CanFitFromEnd(base[i], 0, slots - fit) ) { 
            if( TryFit(&base[i], slots, &out) ) fit = BITMAP_BITS - out.Offset;
            continue;
        }

        fit += BITMAP_BITS;
    }

    if( fit < slots ) out.Unit = NULL;
    out.Slots = slots;
    return out;
}


usize TrySetBitmapAtomic( BitmapModify in, bool set ) {
    usize slots = in.Slots, offset = in.Offset;
    if( !in.Unit ) return NOID;
    Bitmap mask = MASK( Bitmap, offset, slots ), expect = in.Expect;

    Bitmap new = set ? (expect | mask) : (expect & (~mask));
    if( !TryCAS64(in.Unit, &expect, new) ) return NOID;

    return (in.Unit - in.Base) * BITMAP_BITS + offset;
}


static inline void FlipWithMask( Bitmap* target, Bitmap mask, bool set ) {
    set ? (*target |= mask) : (*target &= ~mask);
}


usize SetBitmap( BitmapModify in, bool set ) {
    if( in.Slots == 0 || !in.Unit ) return NOID;

    usize offset = in.Offset, slots = in.Slots;

    Bitmap* base = in.Unit;
    Bitmap* end = base + (offset + slots) / BITMAP_BITS;

    FlipWithMask( base, MASK(Bitmap, offset, slots), set );
    if( base > end ) {
        FlipWithMask( end, MASK(Bitmap, 0, slots % BITMAP_BITS), set );
        FillMemory( base + 1, (end - base - 1) * sizeof(Bitmap), set ? ~0ULL : 0 );
    }

    return (in.Unit - in.Base) * BITMAP_BITS + offset;
}


BitmapModify BitmapElement( Bitmap* begin, usize elementNo, usize slots ) {
    usize unit = elementNo / BITMAP_BITS, offset = elementNo % BITMAP_BITS;
    return (BitmapModify) {
        .Unit = &begin[unit], .Expect = begin[unit],
        .Slots = slots, .Offset = offset,
        .Base = begin
    };
}