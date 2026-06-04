/* Supertos Industries (2012 - 2026)
 * Bitmap allocator implementation. Shall not be used as standalone allocator.
 * Bitmap modification functions operate through BitmapModify structure, that shall not be modified manualy.
 *
 * Searching:
 * This implementation uses BSF (x86) instruction to navigate through bitmap.
 * 1. Let slotsFree be 0; startUnit, startOffset be nil.
 * 2. For every unit in bitmap:
 *      2.1 If unit is nil, find starting bit inside current unit:
 *          2.1.1 Let unitOffset be 0.
 *          2.1.2 Find first free bit, starting from unitOffset, or jump to 2 if exceeded unit width.
 *          2.1.3 Apply cutoff mask. If mask & unit == mask, we found whole block or it's unit suffix, we can expand in another unit.
 *          2.1.4 If suitable (suffix or whole fit) block is found, jump to 2, else jump to 2.1.2
 *      2.2 If we can't fit remaining (slots - slotsFree) inside current unit (continuation starts from bit 0!) (either whole fit or suffix), set unit to nil, jump to 2.
 *      2.3 Add to slotsFree bits checked. If slotsFree >= requested slots, finish.
 * 3. Finish with failure.
 *
 * TryUpdateSingleBitmapUnitAtomic is used when caller (e.g. SLAB allocator) ensures that free block never crosses unit boundary.
 * UpdateBitmap is used elsewhere.
 *
 * On failure, TryUpdateSingleBitmapUnitAtomic and UpdateBitmap return INVALID_ID, else return starting slotID of BitmapModify (UNIT_ELEMENTS * (Unit - Base) + Offset)
 */

#pragma once

#include "Base.h"

typedef usize Bitmap;

#define BITMAP_BITS bitsof(usize) 
#define BITMAP_SIZE(elements) ((elements + BITMAP_BITS - 1) / BITMAP_BITS) 

typedef struct {
    Bitmap Expect; // All allocations would either overwrite Bitmap[UnitBegin] or won't affect found block entirely.
    Bitmap* Unit;
    usize Offset;
    usize Slots;
    Bitmap* Base;
} BitmapModify;

BitmapModify SearchBitmap( Bitmap* base, usize unitCount, usize slots );

usize TrySetBitmapAtomic( BitmapModify in, bool set );

usize SetBitmap( BitmapModify in, bool set );

BitmapModify BitmapElement( Bitmap* begin, usize elementNo, usize slots );

static inline usize BitAt( Bitmap* begin, usize elementNo ) {
    usize unit = elementNo / BITMAP_BITS;
    return !!( begin[unit] & MASK( Bitmap, elementNo % BITMAP_BITS, 1 ) );
}

static inline bool ValidModify( BitmapModify mod ) {
    return mod.Unit;
}