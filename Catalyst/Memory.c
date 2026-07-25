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

UINT32 g_EpdptCount    = 0;
UINT32 g_EpdeCount     = 0;
UINT32 g_EpteCount     = 0;

VOID
EFIAPI
DumpEpts(
    VOID
) {
#define DUMP_EPT_FIELD(Entry, Field, Name) \
    DebugPrint0402("[CATALYST] %a = 0x%x\n", Name, (Entry).Field)

    DebugPrint0402("--- BEGIN EPT DUMP ---\n");

    //
    // EPTP
    //
    DebugPrint0402("[CATALYST] EPTP:\n");
    DUMP_EPT_FIELD(g_Eptp, MemoryType, "MemoryType");
    DUMP_EPT_FIELD(g_Eptp, PageWalkLength, "PageWalkLength");
    DUMP_EPT_FIELD(g_Eptp, DirtyAndAccessEnabled, "DirtyAndAccessEnabled");
    DebugPrint0402("[CATALYST] Pml4Address (raw) = 0x%x\n", g_Eptp.Pml4Address);
    DebugPrint0402("[CATALYST] PML4 base = 0x%p\n", (UINT64)(g_Eptp.Pml4Address) << 12);

    //
    // Walk the PML4 table
    //
    for (UINT32 Pml4Idx = 0; Pml4Idx < 512; Pml4Idx++) {
        EPT_PML4* Pml4e = &g_Epml4[Pml4Idx];
        if (!Pml4e->Present) {
            continue;
        }

        DebugPrint0402("[CATALYST] PML4[%d]:\n", Pml4Idx);
        DUMP_EPT_FIELD((*Pml4e), Read, "Read");
        DUMP_EPT_FIELD((*Pml4e), Write, "Write");
        DUMP_EPT_FIELD((*Pml4e), Execute, "Execute");
        DUMP_EPT_FIELD((*Pml4e), Present, "Present");
        DUMP_EPT_FIELD((*Pml4e), ExecuteUser, "ExecuteUser");
        DebugPrint0402("[CATALYST] PhysicalAddress (raw) = 0x%x\n", Pml4e->PhysicalAddress);

        UINT64 PdptBase = (UINT64)Pml4e->PhysicalAddress << 12;
        DebugPrint0402("[CATALYST] PDPT base = 0x%p\n", PdptBase);

        //
        // Locate the PDPT pool used by this PML4 entry
        //
        INT32 PdptPool = -1;
        for (UINT32 j = 0; j < g_EpdptCount; j++) {
            if (PdptBase == (UINT64)&g_Epdpt[j][0]) {
                PdptPool = (INT32)j;
                break;
            }
        }
        if (PdptPool < 0) {
            DebugPrint0402("[CATALYST] ERROR: PDPT pool not found!\n");
            continue;
        }

        //
        // Walk the PDPT entries
        //
        for (UINT32 PdptIdx = 0; PdptIdx < 512; PdptIdx++) {
            EPT_PDPTE* Pdpte = &g_Epdpt[PdptPool][PdptIdx];
            if (!Pdpte->Present) {
                continue;
            }

            DebugPrint0402("[CATALYST] PDPT[%d] (pool %d):\n", PdptIdx, PdptPool);
            DUMP_EPT_FIELD((*Pdpte), Read, "Read");
            DUMP_EPT_FIELD((*Pdpte), Write, "Write");
            DUMP_EPT_FIELD((*Pdpte), Execute, "Execute");
            DUMP_EPT_FIELD((*Pdpte), Present, "Present");
            DUMP_EPT_FIELD((*Pdpte), ExecuteUser, "ExecuteUser");
            DebugPrint0402("[CATALYST] PhysicalAddress (raw) = 0x%x\n", Pdpte->PhysicalAddress);

            UINT64 PdBase = (UINT64)Pdpte->PhysicalAddress << 12;
            DebugPrint0402("[CATALYST] PD base = 0x%p\n", PdBase);

            //
            // Locate the PDE pool used by this PDPT entry
            //
            INT32 PdePool = -1;
            for (UINT32 m = 0; m < g_EpdeCount; m++) {
                if (PdBase == (UINT64)&g_Epde[m][0]) {
                    PdePool = (INT32)m;
                    break;
                }
            }
            if (PdePool < 0) {
                DebugPrint0402("[CATALYST] ERROR: PDE pool not found!\n");
                continue;
            }

            //
            // Walk the PDE entries (only non‑large 4 KB mappings are used by MapSinglePage. Remember, it's so over)
            //
            for (UINT32 PdeIdx = 0; PdeIdx < 512; PdeIdx++) {
                //
                // MapSinglePage writes these as EPT_PDE, so read them as such.
                //
                EPT_PDE* Pde = (EPT_PDE*)&g_Epde[PdePool][PdeIdx];
                if (!Pde->Present) {
                    continue;
                }

                DebugPrint0402("[CATALYST] PDE[%d] (pool %d):\n", PdeIdx, PdePool);
                DUMP_EPT_FIELD((*Pde), Read, "Read");
                DUMP_EPT_FIELD((*Pde), Write, "Write");
                DUMP_EPT_FIELD((*Pde), Execute, "Execute");
                DUMP_EPT_FIELD((*Pde), Present, "Present");
                DUMP_EPT_FIELD((*Pde), ExecuteUser, "ExecuteUser");
                DebugPrint0402("[CATALYST] PhysicalAddress (raw) = 0x%x\n", Pde->PhysicalAddress);

                UINT64 PtBase = (UINT64)Pde->PhysicalAddress << 12;
                DebugPrint0402("[CATALYST] PT base = 0x%p\n", PtBase);

                //
                // Locate the PTE pool used by this PDE entry
                //
                INT32 PtePool = -1;
                for (UINT32 p = 0; p < g_EpteCount; p++) {
                    if (PtBase == (UINT64)&g_Epte[p][0]) {
                        PtePool = (INT32)p;
                        break;
                    }
                }
                if (PtePool < 0) {
                    DebugPrint0402("[CATALYST] ERROR: PTE pool not found!\n");
                    continue;
                }

                //
                // Walk the PTE entries
                //
                for (UINT32 PteIdx = 0; PteIdx < 512; PteIdx++) {
                    EPT_PTE* Pte = &g_Epte[PtePool][PteIdx];
                    if (!Pte->Present) {
                        continue;
                    }

                    DebugPrint0402("[CATALYST] PTE[%d] (pool %d):\n", PteIdx, PtePool);
                    DUMP_EPT_FIELD((*Pte), Read, "Read");
                    DUMP_EPT_FIELD((*Pte), Write, "Write");
                    DUMP_EPT_FIELD((*Pte), Execute, "Execute");
                    DUMP_EPT_FIELD((*Pte), EptMemoryType, "EptMemoryType");
                    DUMP_EPT_FIELD((*Pte), Present, "Present");
                    DUMP_EPT_FIELD((*Pte), ExecuteUser, "ExecuteUser");
                    DebugPrint0402("[CATALYST] PhysicalAddress (raw) = 0x%x\n", Pte->PhysicalAddress);
                    DebugPrint0402("[CATALYST] HPA = 0x%p\n", (UINT64)Pte->PhysicalAddress << 12);
                }
            }
        }
    }

    DebugPrint0402("--- END EPT DUMP ---\n");

#undef DUMP_EPT_FIELD
}

/*
    Maybe this shouuld return a BOOLEAN?
*/
VOID*
EFIAPI
TranslateAddress(
    IN UINT64 GuestPhysicalAddress
) {
    UINT16 Pml4Idx = (GuestPhysicalAddress >> 39) & 0x1FF;
    UINT16 PdptIdx = (GuestPhysicalAddress >> 30) & 0x1FF;
    UINT16 PdIdx = (GuestPhysicalAddress >> 21) & 0x1FF;
    UINT16 PtIdx = (GuestPhysicalAddress >> 12) & 0x1FF;
    UINT16 Offset = (GuestPhysicalAddress) & 0xFFF;

    DebugPrint0402("[CATALYST] Translating GPA = 0x%p to HPA. Pml4Idx = %d, PdptIdx = %d, PdIdx = %d, PtIdx = %d\n", 
        GuestPhysicalAddress, Pml4Idx, PdptIdx, PdIdx, PtIdx);

    EPT_PML4* Epml4 = &g_Epml4[Pml4Idx];
    DebugPrint0402("[CATALYST] Epml4 = 0x%x at 0x%p\n", Epml4->All, Epml4);
    if (!Epml4->Present)
        return NULL;

    EPT_PDPTE* Pdpt = &(((EPT_PDPTE*)(Epml4->PhysicalAddress << 12))[PdptIdx]);
    DebugPrint0402("[CATALYST] Pdpt = 0x%x at 0x%p\n", Pdpt->All, Pdpt);
    if (!Pdpt->Present)
        return NULL;

    EPT_PDE* Pd = &(((EPT_PDE*)(Pdpt->PhysicalAddress << 12))[PdIdx]);
    DebugPrint0402("[CATALYST] Pd = 0x%x at 0x%p\n", Pd->All, Pd);
    if (!Pd->Present)
        return NULL;

    /*
    if (Pd->Large)
        return (VOID*)(UINT64)(Pd->PhysicalAddress << 21);
    */

    EPT_PTE* Pt = &(((EPT_PTE*)(((EPT_PDE*)Pd)->PhysicalAddress << 12))[PtIdx]);
    DebugPrint0402("[CATALYST] Pt = 0x%x at 0x%p\n", Pt->All, Pt);
    if (!Pt->Present)
        return NULL;

    UINT64 Result = (UINT64)(Pt->PhysicalAddress << 12) + Offset;

    DebugPrint0402("[CATALYST] GPA = 0x%p, translates to HPA = 0x%p\n", GuestPhysicalAddress, Result);
    return (VOID*)Result;
}

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
    if (!Epml4->Present) {
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
        Epml4->Present = TRUE;

        //DebugPrint0402("[CATALYST] EPT_PML4 = 0x%p at 0x%p\n", Epml4->All, Epml4);
    }

    //
    // PDPT Level
    //
    EPT_PDPTE* Pdpt = &(((EPT_PDPTE*)(Epml4->PhysicalAddress << 12))[PdptIdx]);

    if (!Pdpt->Present) {
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
        Pdpt->Present = TRUE;
        //DebugPrint0402("[CATALYST] EPT_PDPTE = 0x%p at %p\n", Pdpt->All, Pdpt);
    }

    /*
    //
    // Default cast as a LARGE PDE (Used if Big == TRUE)
    //
    EPT_LARGE_PDE* Pd = &(((EPT_LARGE_PDE*)(Pdpt->PhysicalAddress << 12))[PdIdx]);

    //
    // PD Level (Large 2MB Pages)
    // "GG's man it's over"
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
        Pd->Present = TRUE;

        //DebugPrint0402("[CATALYST] EPT_LARGE_PDE = 0x%p at %p\n", Pd->All, Pd);
        return;
    }
    */

    //
    // PT Level (Fall through for 4KB pages)
    //
    EPT_PDE* NonLargePd = &(((EPT_PDE*)(Pdpt->PhysicalAddress << 12))[PdIdx]);

    if (!NonLargePd->Present) {
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
        NonLargePd->Present = TRUE;

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
    Pt->Present = TRUE;

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

    //DebugPrint0402("[CATALYST] Mapping PhysicalStart = 0x%p and %d pages starting from 0x%p\n", PhysicalAddr, NumberOfPages, GuestPhysical);

    while (BytesRemaining > 0) {
        BOOLEAN IsAligned = ((PhysicalAddr & (SIZE_2MB - 1)) == 0) &&
            ((GuestPhysical & (SIZE_2MB - 1)) == 0);
        BOOLEAN IsLargeEnough = (BytesRemaining >= SIZE_2MB);

        //DebugPrint0402("[CATALYST] BytesRemaining = %d, IsAligned = %d, IsLargeEnough = %d\n", BytesRemaining, IsAligned, IsLargeEnough);

        if (IsAligned && IsLargeEnough && 0) {
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