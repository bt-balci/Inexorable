#ifndef _MEMORY_C_
#define _MEMORY_C_

#include <Uefi.h>

#include "Efi.h"
#include "Memory.h"

UINT64* g_Pml4Table = NULL;

STATIC
UINT64*
EFIAPI
AllocateZeroedPageTable(
    VOID
) {
    EFI_PHYSICAL_ADDRESS NewPage;
    EFI_STATUS Status = gBS->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, 1, &NewPage);
    if (EFI_ERROR(Status)) {
        //
        // Borked.
        //
        __asm__ __volatile__("int3");
        return NULL;
    }

    //
    // Zero the memory. Fiirmware returns 0xAF for uninitialized memory.
    //
    gBS->SetMem((VOID*)NewPage, SIZE_4KB, 0);

    return (UINT64*)NewPage;
}

STATIC
VOID
EFIAPI
MapSinglePage(
    IN UINT64   VirtualAddr,
    IN UINT64   PhysicalAddr,
    IN BOOLEAN  Big
) {
    //
    // Calculate table indices from the Virtual Address.
    //
    UINT16 Pml4Idx = (VirtualAddr >> 39) & 0x1FF;
    UINT16 PdptIdx = (VirtualAddr >> 30) & 0x1FF;
    UINT16 PdIdx = (VirtualAddr >> 21) & 0x1FF;
    UINT16 PtIdx = (VirtualAddr >> 12) & 0x1FF;

    //
    // PML4 Level
    //
    if (!(g_Pml4Table[Pml4Idx] & PAGE_PRESENT)) {
        g_Pml4Table[Pml4Idx] = (UINT64)AllocateZeroedPageTable() | PAGE_PRESENT | PAGE_RW;
    }
    UINT64* Pdpt = (UINT64*)(g_Pml4Table[Pml4Idx] & PAGE_ADDRESS_MASK);
    
    //
    // PDPT Level
    //
    if (!(Pdpt[PdptIdx] & PAGE_PRESENT)) {
        Pdpt[PdptIdx] = (UINT64)AllocateZeroedPageTable() | PAGE_PRESENT | PAGE_RW;
    }
    UINT64* Pd = (UINT64*)(Pdpt[PdptIdx] & PAGE_ADDRESS_MASK);

    //
    // PD Level
    //
    if (Big) {
        //
        // Map as huge page and stop.
        //
        Pd[PdIdx] = PhysicalAddr | PAGE_PRESENT | PAGE_RW | PAGE_HUGE;
        return;
    }

    //
    // PT Level (Fall through for 4KB pages)
    //
    if (!(Pd[PdIdx] & PAGE_PRESENT)) {
        Pd[PdIdx] = (UINT64)AllocateZeroedPageTable() | PAGE_PRESENT | PAGE_RW;
    }
    UINT64* Pt = (UINT64*)(Pd[PdIdx] & PAGE_ADDRESS_MASK);

    //
    // Set the 4KB page entry
    //
    Pt[PtIdx] = PhysicalAddr | PAGE_PRESENT | PAGE_RW;
}

VOID
EFIAPI
MapPagesLinear(
    IN UINT64       PhysicalStart,
    IN UINT64       NumberOfPages,
    IN OUT UINT64*  CurrentVirtualAddr
) {
    UINT64 PhysicalAddr = PhysicalStart;
    UINT64 VirtualAddr  = *CurrentVirtualAddr;

    UINT64 BytesRemaining = NumberOfPages * SIZE_4KB;

    //
    // This can be moved to an initializer.
    //
    if (g_Pml4Table == NULL) {
        g_Pml4Table = AllocateZeroedPageTable();
    }

    while (BytesRemaining > 0) {
        //
        // Check if both Physical and Virtual addresses are 2MB aligned
        //
        BOOLEAN IsAligned = ((PhysicalAddr & (SIZE_2MB - 1)) == 0) &&
            ((VirtualAddr & (SIZE_2MB - 1)) == 0);

        //
        // Check if we have at least 2MB of memory left to map
        //
        BOOLEAN IsLargeEnough = (BytesRemaining >= SIZE_2MB);

        //
        // This can be beautified even more I thiink.
        //
        if (IsAligned && IsLargeEnough) {
            //
            // Map a 2MB Page
            //
            MapSinglePage(VirtualAddr, PhysicalAddr, TRUE);

            PhysicalAddr += SIZE_2MB;
            VirtualAddr += SIZE_2MB;
            BytesRemaining -= SIZE_2MB;
        }
        else {
            // Map a 4KB Page
            MapSinglePage(VirtualAddr, PhysicalAddr, FALSE);

            PhysicalAddr += SIZE_4KB;
            VirtualAddr += SIZE_4KB;
            BytesRemaining -= SIZE_4KB;
        }
    }

    *CurrentVirtualAddr = VirtualAddr;
}

#endif