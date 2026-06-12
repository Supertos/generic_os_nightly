/* Supertos Industries (2012 - 2026) */

#include "Base.h"
#include "Bitmap.h"


static inline usize TryFit( Bitmap unit, usize slots ) {
	unit = ~unit;
	for( usize i = 0; i < bitsof(unit); i = FirstSetBit64(unit) ) {
		Bitmap mask = MASK(Bitmap, i, slots);
		if( (unit & mask) == mask ) return i;
		unit &= MASK(Bitmap, i + 1, bitsof(unit));
	}
	return NOID;
}


static inline void FlipWithMask( Bitmap* unit, Bitmap mask, bool set ) {
    set ? LockOr64(unit, mask) : LockAnd64(unit, mask);
}


usize TrySetBitsAtomic( Bitmap* self, usize unitCount, usize slots ) {
	if( slots == 0 || slots > bitsof(Bitmap) ) return NOID;
	
	for( usize i = 0; i < unitCount; ++i ) {
		usize id;
		Bitmap temp = self[i];
		while( (id = TryFit(temp, slots)) != NOID ) {
			Bitmap new = temp | MASK(Bitmap, id, slots);
			if( TryCAS64(&self[i], &temp, new) ) return i * bitsof(Bitmap) + id;
		}
	}
	
	return NOID;
}


// Must be atomic for single machine word.
void SetBits( Bitmap* self, usize id, usize slots, bool set ) {
    if( slots == 0 ) return;

	usize unit = id / bitsof(Bitmap), offset = id % bitsof(Bitmap);

    Bitmap* base = self + unit;
    Bitmap* end = base + (offset + slots) / bitsof(Bitmap);

    FlipWithMask( base, MASK(Bitmap, offset, slots), set );
    if( base < end ) {
		usize leftSlots = (slots + offset) % bitsof(Bitmap);
		
        FlipWithMask( end, MASK(Bitmap, 0, leftSlots), set );
        FillMemory( base + 1, (end - base - 1) * sizeof(Bitmap), set ? ~0ULL : 0 );
    }
}