/* Supertos Industries (2012 - 2026) */

#include "Base.h"
#include "Validation.h"
#include "LoaderInfo.h"
#include "ACPI.h"
#include "MemoryManager/Arena.h"


// void BootstrapApplicationCore() {

// }

// void InitializeApplicationCore() {

// }


bool CPUAffinityPredicat( CPUAffinity* affinity, u32* id ) {
    return CPUAffinityNUMA(affinity) == *id;
}


Arena* ArenaFromAffinity( MemoryAffinity* affinity, RootTable* root, usize arenaCount, SRAT* srat ) {
    ArenaInfo info = { 
        .Base = affinity->Base, .End = affinity->Base + affinity->Size, 
        .Flags = affinity->Flags, .NUMAID = affinity->NUMAID 
    };

    usize localCPUCount = ACPIEntryCount( srat, sizeof(SRAT), SRAT_CPU_AFFINITY, CPUAffinityPredicat, &info.NUMAID );
    usize reserveMemory = localCPUCount * AlignUp(sizeof(CPUMemoryService), sizeof(usize)) + sizeof(usize);

    return ArenaNew( root->MemoryMap, root->MemoryMapEntryCount, info, arenaCount, reserveMemory );
}

typedef struct { CPUMemoryService* Service; u64 CoreID; } ServiceMapping;


void InitializeBootCore( RootTable* info ) {
    ForbidInterrupts();

    ValidateAndInitialize(info);
    // 1.
    SRAT* srat = FindACPITable( info->XSDP, SRAT_SIGNATURE, "Error locating SRAT (ACPI Table)!" );
    SLIT* slit = FindACPITable( info->XSDP, SLIT_SIGNATURE, "Error locating SLIT (ACPI Table)!" );

    usize arenaCount = slit->LocalityCount, cpuCount = ACPIEntryCount( srat, sizeof(SRAT), SRAT_CPU_AFFINITY, NULL, NULL );

    // 2.
    MemoryAffinity* affinity = NextACPIEntry( srat, sizeof(SRAT), NULL, SRAT_MEM_AFFINITY );
    if( !affinity ) KernelCrashUndBurn("Error locating enabled memory affinity!");

    Arena* arena = ArenaFromAffinity(affinity, info, arenaCount, srat);
    MemoryBlock arenaMemory = FromArena( arena, sizeof(Arena*) * arenaCount, 1 );
    if( !ValidMemory(arenaMemory) ) KernelCrashUndBurn("Faulty configuration: NUMA node either too small or empty!");
    
    Arena** arenaList = arenaMemory.Base;
    arenaList[arena->NUMAID] = arena;

    // 3.
    affinity = NULL;
    do {
        affinity = NextACPIEntry( srat, sizeof(SRAT), affinity, SRAT_MEM_AFFINITY );
        if( !affinity || !IsMemoryAffinityEnabled(affinity) ) continue;
        if( arenaList[affinity->NUMAID] ) continue;

        arenaList[affinity->NUMAID] = ArenaFromAffinity( affinity, info, arenaCount, srat );
    } while( affinity );
   
    for( usize i = 0; i < slit->LocalityCount; i++ ) {
        if( !arenaList[i] ) continue;
        LinkArenas( arenaList[i], arenaList, &slit->LocalityMatrix[i * arenaCount], arenaCount );
    }

    // 4.
    MemoryBlock cpuServices = FromArena( arena, sizeof(ServiceMapping) * cpuCount, 1 );
    if( !ValidMemory(cpuServices) ) KernelCrashUndBurn("Faulty configuration: NUMA node either too small or empty!");
    ServiceMapping* mapping = cpuServices.Base;

    CPUAffinity* curCPU = NULL;
    usize id = 0;
    do {
        curCPU = NextACPIEntry( srat, sizeof(SRAT), curCPU, SRAT_CPU_AFFINITY );
        if( !curCPU || !IsCPUAffinityEnabled(curCPU) ) continue;
        u32 numaID = CPUAffinityNUMA(curCPU);

        Arena* curArena = arenaList[numaID];
        if( !curArena ) KernelCrashUndBurn( "Faulty configuration: CPU has no valid NUMA assigned!" );

        CPUMemoryService* service = FromBump( &curArena->ResBump, sizeof(CPUMemoryService), sizeof(usize) ).Base;
        CPUMemoryServiceNew(service, curArena);

        mapping[id++] = (ServiceMapping){ .CoreID = curCPU->APICID, .Service = service };
    } while( curCPU );
    

    AllowInterrupts();
}