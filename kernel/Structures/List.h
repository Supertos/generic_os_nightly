/* Supertos Industries (2012 - 2025) */

#pragma once
#include "Base.h"

typedef struct ListIndex ListIndex;
struct ListIndex {
    ListIndex* Prev;
    ListIndex* Next;
};

typedef struct {
    uptr First : PHYS_BITS;
    uptr IndexOffset : (sizeof(u64) * BITS - PHYS_BITS); 
} List;

void ToList( List* list, void* item );

void FromList( List* list, void* item );

void* ListFirst( List* list );

void* ListNext( List* list, void* item );

void* ListPrev( List* list, void* item );

List* ListNew( void* begin, uptr indexOffset );