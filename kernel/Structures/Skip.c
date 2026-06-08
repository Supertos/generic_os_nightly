/* Supertos Industries (2012 - 2025) */


#include "Skip.h"


static inline SkipIndex* Index( Skip* list, void* item ) {
	if( !item ) return &list->Sentinel;
	return (char*)item + list->IndexOffset;
}


static inline void* Item( Skip* list, SkipIndex* index ) { 
	if( index == &list->Sentinel ) return NULL;
	return (char*)index - list->IndexOffset;
}


static inline void** Next( SkipIndex* index ) { return index + 1; }
static inline void** Prev( SkipIndex* index ) { return Next(index) + index->MaxLevels; }
void* SkipFirst( Skip* list ) { return Next(&list->Sentinel)[0]; }


void* SkipPrev( Skip* list, void* item ) {
	void* prev = Prev( Index(list, item) )[0];
	return Item(list, prev);
}


void* SkipNext( Skip* list, void* item ) {
	if( !item ) return FirstSkipItem(list);
	void* next = Next( Index(list, item) )[0];
	return Item(list, next);
}


static inline void Link( SkipIndex* a, SkipIndex* b, u16 lvl) {
	Next(a)[lvl] = b;
	if( b ) Prev(b)[lvl] = a;
}


void ToSkip( Skip* list, void* prevs[], void* item ) {
	SkipIndex* index = Index(list, item);
	u16 lvl = 0;
	do {
		SkipIndex *prev = Index(list, prevs[lvl]), *next = Next(prev)[lvl];

		Link(prev, index, lvl);
		Link(index, next, lvl);
	} while( ++lvl < list->Levels && list->P > Random() );
	index->Levels = lvl;
	index->MaxLevels = list->Levels;
}


void* FromSkip( Skip* list, void* item ) {
	SkipIndex* index = Index(list, item);
	for( u16 lvl = 0; lvl < index->Levels; lvl++ )
		Link( Prev(index)[lvl], Next(index)[lvl], lvl );

	index->Levels = 0;
	return item;
}


void SkipPos( Skip* list, void* prevs[], void* value ) {
	SkipIndex *cur = NULL, *next;
	for( u16 lvl = 0; lvl < list->Levels; lvl++ ) {
		while( (next = SkipNext(list, cur)) && list->Compare(next, value) < 0 ) cur = next;
		prevs[lvl] = cur;
	}
}


Skip* SkipNew( void* begin, u32 indexOffset, u16 lvls, void* sentinel ) {
	Skip* list = begin;
	*list = (Skip) { .IndexOffset = indexOffset, .Levels = lvls, .Sentinel = sentinel };
	
	FillMemory( Next(&list->Sentinel), sizeof(void*) * lvls * 2, 0 );
	list->Sentinel.Levels = lvls;
	return list;
}


void SkipInit( Skip* list, u32 p, int (*compare)(void*, void*) ) {
	list->P = p; list->Compare = compare;
}