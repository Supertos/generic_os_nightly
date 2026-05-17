/* Supertos Industries (2012 - 2026) */

#include "List.h"
#include "Base.h"


static inline ListIndex* Index( List* list, void* item ) { 
    return (char*)item + list->IndexOffset; 
}


static inline void* Item( List* list, ListIndex* index ) { 
    if( !index ) return NULL;
    return (char*)index - list->IndexOffset;
}


void ToList( List* list, void* item ) {
    ListIndex* index = Index(list, item), *first = list->First;

    *index = (ListIndex){ .Next = first };
    first->Prev = index;
    list->First = index;
}


void* FromList( List* list, void* item ) {
    ListIndex* index = Index(list, item);
    ListIndex *next = index->Next, *prev = index->Prev;

    if( next ) next->Prev = prev;

    if( prev )
        prev->Next = next;
    else
        list->First = next;
    return item;
}


void* ListFirst( List* list ) {
    return Item(list, list->First);
}


void* ListNext( List* list, void* item ) {
    if( !item ) return ListFirst(list);
    
    ListIndex* index = Index(list, item);
    return Item(list, index->Next);
}


void* ListPrev( List* list, void* item ) {
    ListIndex* index = Index(list, item);
    return Item(list, index->Prev);
}


List* ListNew( void* begin, uptr indexOffset ) {
    List* list = begin;
    *list = (List){ .IndexOffset = indexOffset };
    return list;
}