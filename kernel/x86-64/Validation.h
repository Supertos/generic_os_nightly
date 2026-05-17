/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Base.h"
#include "LoaderInfo.h"

static inline void ValidateAndInitialize(RootTable* loaderTable) {
    CPUIDResult features = CPUID(0x01, 0x0);

    if( !(features.edx & (1 << 9)) ) KernelCrashUndBurn("Apparatus does not support APIC!");
    if( !(features.ecx & (1 << 17)) ) KernelCrashUndBurn("Apparatus does not support PCID!");
    if( !(features.ecx & (1 << 30)) ) KernelCrashUndBurn("Apparatus does not support RDRAND!");

    usize old = 0;
    usize duplicates = 0;
    for( usize i = 15; i; i-- ) {
        usize rand = Random();
        if( old == rand ) duplicates++;
        old = rand;
    }

    if( duplicates == 15 ) KernelCrashUndBurn("Apparatus has buggy RDRAND implementation. (AMD?)");


    __asm__ __volatile__ ( // Enable PCID.
		"mov %%cr4, %%rax;"
		"bts $17, %%rax;"
		"mov %%rax, %%cr4;"
		:
		:
		: "rax", "flags", "memory"
	);


    __asm__ __volatile__ ( // Enable APIC.
		"mov $0x1B, %%ecx;"
        "rdmsr;"
        "bts $11, %%eax;"
        "wrmsr;"
		:
		:
		: "ecx", "edx", "eax", "memory"
	);


    __asm__ __volatile__ ( // Mask PIC interrupts
		"out $0xFF, $0x21;"
		"out $0xFF, $0xA1;"
	);
}
