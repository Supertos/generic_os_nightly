/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Base.h"

#define __ASM8 b
#define __ASM16 w
#define __ASM32 l
#define __ASM64 q
#define __ASMsize q

#define __ASM_SIZE(size) __ASM##size
#define __ASM_STRING_INT(x) #x
#define __ASM_STRING(x) __ASM_STRING_INT(x)

#define __ASM(mnem, size, ops) mnem __ASM_STRING( __ASM_SIZE(size) ) " " ops ";"


#define DECL_FOR_SIZE( size ) 														\
usize BSF##size( u##size in ) { 													\
	bool fail = false; 																\
	u##size out = 0; 																\
	__asm__ __volatile__ ( 															\
		__ASM("bsf", size, "%2, %1")  												\
		: "=@ccz"(fail), "=r"(out) 													\
        : "r"(in) 																	\
        : 																			\
	); 																				\
	return fail ? NOID : out; 														\
} 																					\
																					\
usize LZCNT##size( u##size in ) {													\
    bool fail = false;																\
    usize out = 0;																	\
    __asm__ __volatile__ (															\
		__ASM("lzcnt", size, "%2, %1")												\
        : "=@ccc"(fail), "=r"(out)													\
        : "r"(in)																	\
        :																			\
    );																				\
    return fail ? NOID : out;														\
}																					\
																					\
void LockOr##size( u##size* in, u##size orV ) {										\
	__asm__ __volatile__ (															\
		__ASM("lock or", size, "%1, %0")											\
        : "+m"(*in)																	\
        : "r"(orV)																	\
        : "memory", "cc"															\
    );																				\
}																					\
																					\
void LockAnd##size( u##size* in, u##size orV ) {									\
	__asm__ __volatile__ (															\
		__ASM("lock and", size, "%1, %0")											\
        : "+m"(*in)																	\
        : "r"(orV)																	\
        : "memory", "cc"															\
    );																				\
}																					\
																					\
bool TryCAS##size( volatile u##size* in, volatile u##size* old, u##size newVal ) {	\
    bool success = false;															\
    __asm__ __volatile__ (															\
		__ASM("lock cmpxchg", size, "%3, %1")										\
        : "=@ccz"(success), "+m"(*in), "+a"(*old)									\
        : "r"(newVal)																\
        : "memory"																	\
    );																				\
    return success;																	\
}																					\
																					\
u##size FetchAdd##size( volatile u##size* in, u##size add ) {						\
    __asm__ __volatile__ (															\
		__ASM("lock xadd", size, "%1, %0")											\
        : "+m"(*in), "+a"(add)														\
        :																			\
        : "cc", "memory"															\
    );																				\
    return add;																		\
}																					\
																					\
u##size FetchSub##size( volatile u##size* in, u##size sub ) {						\
    return FetchAdd##size( in, -sub );												\
}

DECL_FOR_SIZE(16);
DECL_FOR_SIZE(32);
DECL_FOR_SIZE(64);

void Pause() {
    __asm__ __volatile__ ( "pause;" : : : );
}

typedef struct { 
	u32 eax; 
	u32 ebx; 
	u32 ecx; 
	u32 edx; 
} CPUIDResult;

static inline CPUIDResult CPUID( u32 eax, u32 ecx ) {
    CPUIDResult out = { .eax = eax, .ecx = ecx };

    __asm__ __volatile__ ( 
        "cpuid;"
        : "+a"(out.eax), "=b"(out.ebx), "+c"(out.ecx), "=d"(out.edx)
    );

    return out;
}

static inline u32 SMPID() {
    CPUIDResult model = CPUID(0, 0);

    bool isIntelLike = 
        model.ebx == 0x756E6547 &&
        model.edx == 0x49656E69 &&
        model.ecx == 0x6C65746E;

    if( isIntelLike ) return CPUID(0xB, 0).edx;
    
    u32 maxExtendedLeaf = CPUID(0x80000000, 0).eax;
    if( maxExtendedLeaf >= 0x8000001E )
        return CPUID(0x8000001E, 0).eax;

    return (CPUID(1, 0).ebx >> 24) & 0xFF;
}

u32 Random() {
    u32 success = 0, out = 0;
    while( !success ) {
        __asm__ __volatile__ (
            "rdrandl %0;"
            : "=r"(out), "=@ccc"(success)
            :
            : "cc"
        );
    }
    return out;
}

void InterruptsOff() {
    __asm__ __volatile__ ("cli");
}

void InterruptsOn() {
    __asm__ __volatile__ ("sti");
}

void KernelCrashUndBurn( void* messageAddr ) {
    __asm__ __volatile__ (
        "movq %0, %%rax;"
        "ud2;"
        :
        : "m"(messageAddr)
        :
    );
}