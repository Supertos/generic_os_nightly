/* Supertos Industries (2012 - 2026) */
#pragma once

#include "Base.h"

#define ACPI_ENTRY_ANY 0xFF

typedef struct ACPIRecord ACPIRecord;
struct ACPIRecord {
    u8 EntryType;
    u8 EntrySize;
} __attribute__((__may_alias__));

typedef struct {
    u32  Signature;
    u32  Length;
    u8   Revision;
    u8   Checksum;
    char OEMID[6];
    char OEMTableID[8];
    u32  OEMRevision;
    u32  CreatorID;
    u32  CreatorRevision;
} __attribute__((__may_alias__)) ACPIHeader;

typedef struct {
    ACPIHeader Header;
    ACPIHeader* Tables[];
} __attribute__ ((packed)) XSDT;

typedef struct {
    u32  Signature;
    u8   Checksum;
    char OEMID[6];
    u8   Revision;
    u32  RsdtAddress;

    u32  Length;
    XSDT*  XSDT;
    u8   ExtendedChecksum;
    u8   reserved[3];
} __attribute__ ((packed)) XSDP;

static_assert( offset_of(XSDT, Tables) == 36 );
static_assert( offset_of(XSDT, Tables[1]) == 44 );

static inline bool IsACPIChecksumValid( ACPIHeader* header ) {
    u8 check = 0;
    for( usize i = 0; i < header->Length; i++ ) check += ((u8*)header)[i];
    return check == 0;
}

static inline void* FindACPITable( XSDP* root, char* signatureString, char* failureString ) {
    XSDT* xsdt = root->XSDT;
    u32 signature = *((u64*)signatureString), length = xsdt->Header.Length;
    for( usize i = 0; offset_of(XSDT, Tables[i]) < length; i++ ) {
        ACPIHeader* table = xsdt->Tables[i];
        if( table->Signature != signature ) continue;
        if( !IsACPIChecksumValid(table) ) continue;

        return table;
    }

    if( failureString ) KernelCrashUndBurn(failureString);
    return NULL;
}

static inline ACPIRecord* NextACPIEntry( ACPIHeader* head, usize headSize, ACPIRecord* cur, u8 entryType ) {
    if( !cur ) return (uptr)head + headSize;

    ACPIRecord* out;
    do {
        out = (char*)cur + cur->EntrySize;
        if( (uptr)out - (uptr)head >= head->Length ) return NULL;
    } while( out->EntryType == entryType || entryType == ACPI_ENTRY_ANY );
    return out;
}


static inline usize ACPIEntryCount( ACPIHeader* head, usize headSize, u8 entryType, bool (*predicat)(void*, void*), void* predicatArgument ) {
    usize out = 0;
    ACPIRecord* temp = NULL;
    while( temp = NextACPIEntry(head, headSize, temp, entryType) ) 
        if( !predicat || predicat(temp, predicatArgument) ) out++;
    return out;
}