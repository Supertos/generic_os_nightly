/* Supertos Industries (2012 - 2026) 
 * Intrusive Red-Black Tree container.
 * RBIndex interpretes MSB of parent pointer as color, since no modern MMU supports 64-bit addressing and kernel always uses zeroed upper bits.
 *
 * TODO: I'm not writing down all theoretical foundations of RB-Tree. See Linux implementation (they even have ASCII graphics for fixups!) or Wikipedia.
 * Except their code uses explicit mirroring and fixups are iterative (this implementation uses recursion with forced tail optimization (via [[gnu::musttail]])).
 * https://github.com/torvalds/linux/blob/master/lib/rbtree.c - Linux RB-Tree
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct RBIndex RBIndex;
struct RBIndex {
    uptr Parent: 63;
    uptr Color: 1;

    RBIndex* Left;
    RBIndex* Right;
};

typedef struct {
    RBIndex* Root;
    int (*Comparator)(void* item, void* valueAddr);
    u32 IndexOffset;
    u32 ValueOffset;
    RBIndex _NIL;
} RBTree;

void* SearchRBTree( RBTree* tree, void* valueAddr );

void* FromRBTree( RBTree* tree, void* element );

void ToRBTree( RBTree* tree, void* element );

void* RBTreeRoot( RBTree* tree );

void* RBTreeFirst( RBTree* tree );

void* RBTreeNext( RBTree* tree, void* item );

void* RBTreePrev( RBTree* tree, void* item );

RBTree* RBTreeNew( void* begin, u32 indexOffset, u32 valueOffset, int (*comparator)(void* item, void* valueAddr) );