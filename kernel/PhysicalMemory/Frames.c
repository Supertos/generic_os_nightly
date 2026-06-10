/* Supertos Industries (2012 - 2025) */

#include "Frames.h"

#define SIZE_LIST_LEVELS 8
#define ADDR_LIST_LEVELS 32

typedef struct {
    uptr End;
    usize Size;
	
	SKIPINDEX(SIZE_LIST_LEVELS) SizeIndex;
	SKIPINDEX(ADDR_LIST_LEVELS) AddrIndex;
} Frame;


static inline void SetSize( Frames* self, Frame* frame, usize size ) {
	Frame* prevs[MAX(SIZE_LIST_LEVELS, ADDR_LIST_LEVELS)];
	
	bool absent = !frame->Size;
	frame->Size = size; frame->End = (uptr)frame + size;
	
	if( absent ) {
		SkipPos( &self->Address, prevs, &frame );
		ToSkip( &self->Address, prevs, frame );
	}else{
		FromSkip(&self->Size, frame);
	}
	
	if( size > 0 ) {
		SkipPos( &self->Size, prevs, &size );
		ToSkip( &self->Size, prevs, frame );
	}else{
		FromSkip(&self->Address, frame);
	}
}


Frame* NewFrame( Frames* self, uptr base, usize size ) {
    Frame* frame = base;
	
    *frame = (Frame){0};
	SetSize(self, frame, size);
    return frame;
}


usize MaxFrameSize( uptr base, usize end ) {
    if( Aligned(base, HUGEPAGE) ) return MIN( end - base, MAX(AlignDown(end - base, HUGEPAGE), HUGEPAGE) );
    return (-base) % HUGEPAGE;
}


static inline bool ShouldSplit( uptr base, usize end ) {
    return end - base > MaxFrameSize(base, end);
}


static inline bool CanMerge( Frame* a, Frame* b ) {
    return a->End == (uptr)b && !ShouldSplit((uptr)a, b->End);
}


Frame* SplitFrame( Frames* self, Frame* frame ) { // Call repeteadly on its return value.
    uptr end = frame->End, base = (uptr)frame, nEnd = MaxFrameSize(base, end);

    SetSize(self, frame, nEnd);
    return NewFrame(self, nEnd, end);
}


Frame* MergeFrames( Frames* self, Frame* a, Frame* b ) {
    SetSize( self, a, b->End - (uptr)a );
	SetSize( self, b, 0 );
    return a;
}


void DrainToFrame( Frames* self, Frame* frame, uptr* base, usize* size ) {
    usize newSize = MaxFrameSize( (uptr)frame, frame->End + *size );
    usize dSize = newSize - frame->Size;

    *size -= dSize;
    *base += dSize;

    SetSize(self, frame, newSize);
}


void ToFrames( Frames* self, MemoryBlock block ) {
	uptr base = block.Base;
	usize size = MemorySize(block);
	if( size == 0 ) return;
	
	ACQUIRE_AUTO(&self->Lock);
	
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
    }

	if( size == 0 ) return;
    Frame* frame = NewFrame(self, base, size);

    while( true ) {
        Frame* next = SkipNext(&self->Address, frame);
        
        if( next && CanMerge(frame, next) ) {
            MergeFrames( self, frame, next );
        }else if( ShouldSplit((uptr)frame, frame->End) ) {
            frame = SplitFrame(self, frame);
        }else{
            break;
        }
    }
}


MemoryBlock FromFrames( Frames* self, usize size, usize align ) {
	if( size == 0 ) return NO_MEMBLOCK;
	ACQUIRE_AUTO(&self->Lock);
	
    size = AlignUp(size, PAGE);

    Frame* prevs[SIZE_LIST_LEVELS];
    SkipPos(&self->Size, prevs, size);
	
    Frame* source = SkipNext(&self->Size, prevs[0]) ?: prevs[0];
    if( !source ) return NO_MEMBLOCK;

    usize allocated = MIN(source->Size, size);

	void* base = source->End - allocated;
    SetSize( self, source, source->Size - allocated );
    
    if( ShouldSplit((uptr)source, source->End) )
        SplitFrame(self, source);

    return MemoryOfSize(base, size);
}


int CmpSize( Frame* item, usize* valueAddr ) { return (item->Size > *valueAddr) - (item->Size < *valueAddr); }
int CmpAddr( Frame* item, uptr* valueAddr ) { return ((uptr)item > *valueAddr) - ((uptr)item < *valueAddr); }


Frames* FramesNew( void* base ) {
    Frames* allocator = base;

    SkipNew( &allocator->Size, offsetof(Frame, SizeIndex), SIZE_LIST_LEVELS, &allocator->SizeSentinel );
    SkipNew( &allocator->Address, offsetof(Frame, AddrIndex), ADDR_LIST_LEVELS, &allocator->AddressSentinel );

    SkipInit( &allocator->Size, P_50, CmpSize );
    SkipInit( &allocator->Address, P_50, CmpAddr );
    return allocator;
}


Frames* MoveFrames( Frames* from, void* toAddr ) {
    Frames* to = FramesNew(toAddr);

    MemoryBlock block;
    do {
        block = FromFrames( from, ~0ULL, 1 );
        if( ValidMemory(block) ) ToFrames(to, block);
    } while( ValidMemory(block) );

    return to;
}
