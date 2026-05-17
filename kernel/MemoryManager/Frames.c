/* Supertos Industries (2012 - 2025) */

#include "Frames.h"
 

typedef struct {
    usize End;
    uptr Base;
    uptr Size;

    SkipIndex SizeIndex;
    SkipIndex* SizeIndexLists[SKIPINDEX_LINKS(SIZE_LIST_LEVELS)];

    SkipIndex AddrIndex;
    SkipIndex* AddrIndexLists[SKIPINDEX_LINKS(ADDR_LIST_LEVELS)];
} Frame;


static inline void SetSize( Frames* self, Frame* frame, usize size ) {
    frame->Size = size; frame->End = frame->Base + size;
    UpdateSizeList(self, frame);
}


Frame* NewFrame( Frames* self, uptr base, usize end ) {
    Frame* frame = base;
    *frame = (Frame){ .Base = base, .End = end, .Size = end - base };

    UpdateSizeList(self, frame);

    Frame* prevs[ADDR_LIST_LEVELS];
    SkipPos( &self->Address, prevs, &frame->Base );
    ToSkip( &self->Address, prevs, frame );
    return frame;
}


usize MaxFrameSize( uptr base, usize end ) {
    if( IsAligned(base, HUGEPAGE) ) return MAX( HUGEPAGE, AlignDownTo(end - base, HUGEPAGE) );
    return (-base) % HUGEPAGE;
}


static inline bool ShouldSplit( uptr base, usize end ) {
    return end - base > MaxFrameSize(base, end);
}


static inline bool CanMerge( Frame* a, Frame* b ) {
    return a->End == b->Base && !ShouldSplit(a->Base, b->End);
}


void UpdateSizeList( Frames* self, Frame* frame ) {
    if( frame->SizeIndex.Levels != 0 ) FromSkip(&self->Size, frame);

    Frame* prevs[SIZE_LIST_LEVELS];
    SkipPos( &self->Size, prevs, &frame->Size );
    InsertSkipItem( self->Size, prevs, frame );
}


Frame* SplitFrame( Frames* self, Frame* frame ) { // Call repeteadly on its return value.
    usize base = frame->Base, end = frame->End;

    SetSize( self, frame, MaxFrameSize(base, end) );
    return NewFrame(self, frame->End, end);
}


Frame* MergeFrames( Frames* self, Frame* a, Frame* b ) {
    FromSkip(&self->Size, b);
    FromSkip(&self->Address, b);

    SetSize( self, a, b->End - a->Base );
    return a;
}


void DrainToFrame( Frames* self, Frame* frame, uptr* base, usize* size ) {
    usize newSize = MaxFrameSize( frame->Base, frame->End + *size );
    usize dSize = newSize - frame->Size;

    *size -= dSize;
    *base += dSize;

    SetSize(self, frame, newSize);
}


void ToFrames( Frames* self, MemoryBlock block ) {
    uptr base = block.Base; usize size = block.End - block.Base;
    if( size == 0 ) return;

    self->Bytes += size;
    self->TotalBytes = MAX(self->Bytes, self->TotalBytes);

    Frame* prevs[ADDR_LIST_LEVELS];
    SkipPos(&self->Address, prevs, &base);
    
    Frame* father = prevs[0];
    if( father && father->End == base ) {
        DrainToFrame( self, father, &base, &size );

        Frame* grand = SkipPrev(&self->Address, prevs[0]);
        if( grand && CanMerge(grand, father) ) // No consequtive blocks may satisfy this twice.
            father = MergeFrames( self, grand, father );

        if( size == 0 ) return;
    }

    Frame* frame = NewFrame(self, base, size);

    while( true ) {
        Frame* next = SkipNext(&self->Address, frame);
        
        if( next && CanMerge(frame, next) ) {
            MergeFrames( self, frame, next );
        }else if( ShouldSplit(frame->Base, frame->End) ) {
            frame = SplitFrame(self, frame);
        }else{
            break;
        }
    }
}


MemoryBlock __FromFrames( Frames* self, usize size, bool whole ) {
    size = AlignUp(size, PAGE);
    if( size == 0 ) return NO_MEMBLOCK;

    Frame* prevs[SIZE_LIST_LEVELS];
    SkipPos(&self->Size, prevs, size);
    Frame* source = prevs[0];
    if( !source ) return NO_MEMBLOCK;

    source = SkipNext(&self->Size, source) ?: source;
    if( source->Size < size && whole ) return NO_MEMBLOCK;

    usize allocated = MIN(source->Size, size);

    SetSize( self, source, source->Size - allocated );
    void* base = source->End;

    if( source->Size == 0 ) {
        FromSkip(&self->Size, source);
        FromSkip(&self->Address, source);
    }else if( ShouldSplit(source->Base, source->End) ) {
        SplitFrame(self, source);
    }

    return (MemoryBlock){ .Base = base, .End = base + size };
}


MemoryBlock FromFrames( Frames* self, usize size, usize align ) {
    return __FromFrames( self, size, true );
}


MemoryBlock FromFramesPartial( Frames* self, usize size, usize align ) {
    return __FromFrames( self, size, false );
}


int CmpSize( Frame* item, usize* valueAddr ) { return (item->Size > *valueAddr) - (item->Size < *valueAddr); }
int CmpAddr( Frame* item, uptr* valueAddr ) { return (item->Base > *valueAddr) - (item->Base < *valueAddr); }


Frames* FramesNew( void* base ) {
    Frames* allocator = base;

    SkipNew( &allocator->Size, offset_of(Frame, SizeIndex), SIZE_LIST_LEVELS, &allocator->SizeSentinel );
    SkipNew( &allocator->Address, offset_of(Frame, AddrIndex), ADDR_LIST_LEVELS, &allocator->AddressSentinel );

    SkipInit( &allocator->Size, P_50, CmpSize );
    SkipInit( &allocator->Address, P_50, CmpAddr );
    return allocator;
}


Frames* MoveFrames( Frames* from, void* toAddr ) {
    Frames* to = FramesNew(toAddr);

    MemoryBlock block;
    do {
        block = FromFrames( from, ~0ULL, false );
        if( ValidMemory(block) ) ToFrames(to, block);
    } while( ValidMemory(block) );

    return to;
}