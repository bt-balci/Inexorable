#ifndef _MEMORY_C_
#define _MEMORY_C_

#include <Uefi.h>

#include "Efi.h"
#include "Memory.h"
#include "x86.h"
#include "Vm.h"

__attribute__((aligned(4096))) EPTP				g_Eptp;
__attribute__((aligned(4096))) EPT_PML4			g_Epml4[512];
__attribute__((aligned(4096))) EPT_PDPTE        g_Epdpt[64][512];
__attribute__((aligned(4096))) EPT_LARGE_PDE    g_Epde[512][512];
__attribute__((aligned(4096))) EPT_PTE			g_Epte[16384][512];
UINT32                                          g_EpteCount;
UINT32                                          g_EpdptCount = 0;
UINT32                                          g_EpdeCount = 0;
UINT32                                          g_EpteCount = 0;

STATIC
VOID
EFIAPI
MapSinglePage(
    IN UINT64   GuestPhysical,
    IN UINT64   HostPhysical,
    IN BOOLEAN  Big
) {
    UINT16 Pml4Idx = (GuestPhysical >> 39) & 0x1FF;
    UINT16 PdptIdx = (GuestPhysical >> 30) & 0x1FF;
    UINT16 PdIdx = (GuestPhysical >> 21) & 0x1FF;
    UINT16 PtIdx = (GuestPhysical >> 12) & 0x1FF;

    //DebugPrint0402("[CATALYST] Mapping GPA = %p to HPA = %p, Big = %d, Pml4Idx = %d, PdptIdx = %d, PdIdx = %d, PtIdx = %d\n",
    //    GuestPhysical, HostPhysical, Big, Pml4Idx, PdptIdx, PdIdx, PtIdx);

    //
    // PML4 Level
    //
    EPT_PML4* Epml4 = &g_Epml4[Pml4Idx];
    if (Epml4->PhysicalAddress == 0) {
        Epml4->Execute = TRUE;
        Epml4->Write = TRUE;
        Epml4->Read = TRUE;
        Epml4->ExecuteUser = TRUE;

        if (g_EpdptCount >= 64) {
            DebugPrint0402("[CATALYST] g_EpdptCount is bigger than 64. Borked\n");
            return;
        }
        UINT32 PdptPoolIndex = g_EpdptCount++;

        Epml4->PhysicalAddress = ((UINT64)&g_Epdpt[PdptPoolIndex][0]) >> 12;

        //DebugPrint0402("[CATALYST] EPT_PML4 = 0x%p at 0x%p\n", Epml4->All, Epml4);
    }

    //
    // PDPT Level
    //
    EPT_PDPTE* Pdpt = &(((EPT_PDPTE*)(Epml4->PhysicalAddress << 12))[PdptIdx]);

    if (Pdpt->PhysicalAddress == 0) {
        Pdpt->Execute = TRUE;
        Pdpt->Write = TRUE;
        Pdpt->Read = TRUE;
        Pdpt->ExecuteUser = TRUE;

        if (g_EpdeCount >= 512) {
            DebugPrint0402("[CATALYST] g_EpdeCount is bigger than 512. Borked\n");
            return;
        }
        UINT32 PdPoolIndex = g_EpdeCount++;

        Pdpt->PhysicalAddress = ((UINT64)&g_Epde[PdPoolIndex][0]) >> 12;
        //DebugPrint0402("[CATALYST] EPT_PDPTE = 0x%p at %p\n", Pdpt->All, Pdpt);
    }

    //
    // Default cast as a LARGE PDE (Used if Big == TRUE)
    //
    EPT_LARGE_PDE* Pd = &(((EPT_LARGE_PDE*)(Pdpt->PhysicalAddress << 12))[PdIdx]);

    //
    // PD Level (Large 2MB Pages)
    //
    if (Big) {
        Pd->Execute = TRUE;
        Pd->Write = TRUE;
        Pd->Read = TRUE;
        Pd->ExecuteUser = TRUE;
        Pd->Large = TRUE;
        Pd->MemoryType = 6; // Writeback memory

        //
        // Large PDE PhysicalAddress starts at bit 21, so shift right by 21.
        //
        Pd->PhysicalAddress = HostPhysical >> 21;

        //DebugPrint0402("[CATALYST] EPT_LARGE_PDE = 0x%p at %p\n", Pd->All, Pd);
        return;
    }

    //
    // PT Level (Fall through for 4KB pages)
    //
    EPT_PDE* NonLargePd = (EPT_PDE*)Pd;

    if (NonLargePd->PhysicalAddress == 0) {
        NonLargePd->Execute = TRUE;
        NonLargePd->Write = TRUE;
        NonLargePd->Read = TRUE;
        NonLargePd->ExecuteUser = TRUE;

        //
        // Hmm. Can we get borked? Better log than hang.
        //
        if (g_EpteCount >= 16384) {
            DebugPrint0402("[CATALYST] g_EpteCount is bigger than 16384. Borked\n");
            return;
        }

        UINT32 PtPoolIndex = g_EpteCount++;

        NonLargePd->PhysicalAddress = ((UINT64)&g_Epte[PtPoolIndex][0]) >> 12;

        //DebugPrint0402("[CATALYST] EPT_PDE = 0x%p at %p. EPTE count = %d\n", NonLargePd->All, NonLargePd, g_EpteCount);
    }

    EPT_PTE* Pt = &(((EPT_PTE*)(NonLargePd->PhysicalAddress << 12))[PtIdx]);

    //
    // Set the 4KB page entry
    //
    Pt->EptMemoryType = 6;
    Pt->Execute = TRUE;
    Pt->Write = TRUE;
    Pt->Read = TRUE;
    Pt->ExecuteUser = TRUE;

    Pt->PhysicalAddress = HostPhysical >> 12;

    //DebugPrint0402("[CATALYST] EPT_PTE = 0x%p at %p\n", Pt->All, Pt);
}

VOID
EFIAPI
MapPagesLinear(
    IN UINT64                       PhysicalStart,
    IN UINT64                       NumberOfPages,
    IN OUT UINT64*                  CurrentGuestPhysical
) {
    UINT64 PhysicalAddr = PhysicalStart;
    UINT64 GuestPhysical = *CurrentGuestPhysical;
    UINT64 BytesRemaining = NumberOfPages * SIZE_4KB;

    DebugPrint0402("[CATALYST] Mapping PhysicalStart = 0x%p and %d pages starting from 0x%p\n", PhysicalAddr, NumberOfPages, GuestPhysical);

    while (BytesRemaining > 0) {
        BOOLEAN IsAligned = ((PhysicalAddr & (SIZE_2MB - 1)) == 0) &&
            ((GuestPhysical & (SIZE_2MB - 1)) == 0);
        BOOLEAN IsLargeEnough = (BytesRemaining >= SIZE_2MB);

        DebugPrint0402("[CATALYST] BytesRemaining = %d, IsAligned = %d, IsLargeEnough = %d\n", BytesRemaining, IsAligned, IsLargeEnough);

        if (IsAligned && IsLargeEnough) {
            //
            // Map 2MB page.
            //
            MapSinglePage(GuestPhysical, PhysicalAddr, TRUE);
            PhysicalAddr += SIZE_2MB;
            GuestPhysical += SIZE_2MB;
            BytesRemaining -= SIZE_2MB;
        }
        else {
            //
            // Map 4KB page.
            //
            MapSinglePage(GuestPhysical, PhysicalAddr, FALSE);
            PhysicalAddr += SIZE_4KB;
            GuestPhysical += SIZE_4KB;
            BytesRemaining -= SIZE_4KB;
        }
    }

    *CurrentGuestPhysical = GuestPhysical;
}

#endif