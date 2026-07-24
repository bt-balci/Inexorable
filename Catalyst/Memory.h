#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <Uefi.h>

//
// Maybe move to x86.h?
//

///////////////////////////////////////////////
////			   Definitions			   ////
///////////////////////////////////////////////

#define PAGE_PRESENT       (1ULL << 0)
#define PAGE_RW            (1ULL << 1)
#define PAGE_HUGE          (1ULL << 7)
#define PAGE_ADDRESS_MASK  (0x000FFFFFFFFFF000ULL)

#define PAGE_SIZE_4KB           0x1000ULL
#define PAGE_SIZE_2MB           0x200000ULL

#define CR3_ADDRESS_MASK    0x000FFFFFFFFFF000ULL
#define CR3_PWT             (1ULL << 3ULL)
#define CR3_PCD             (1ULL << 5ULL)

///////////////////////////////////////////////
////				Globals				   ////
///////////////////////////////////////////////

extern UINT64* g_Pml4Table;

///////////////////////////////////////////////
////				Methods				   ////
///////////////////////////////////////////////

VOID
EFIAPI
MapPagesLinear(
    IN UINT64       PhysicalStart,
    IN UINT64       NumberOfPages,
    IN OUT UINT64*  CurrentVirtualAddr
);

#endif