#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

#include "Efi.h"
#include "Memory.h"
#include "x86.h"
#include "Vm.h"

BOOLEAN
EFIAPI
TranslateGuestPhysicalToHostPhysical(
    IN UINT64   GuestPhysicalAddress,
    OUT VOID**  HostPhysicalAddress
) {
    *HostPhysicalAddress = NULL;

    UINT16 Pml4Idx  = (GuestPhysicalAddress >> 39) & 0x1FF;
    UINT16 PdptIdx  = (GuestPhysicalAddress >> 30) & 0x1FF;
    UINT16 PdIdx    = (GuestPhysicalAddress >> 21) & 0x1FF;
    UINT16 PtIdx    = (GuestPhysicalAddress >> 12) & 0x1FF;
    UINT16 Offset   = (GuestPhysicalAddress) & 0xFFF;

    EPT_PML4* Epml4 = &g_VmState.Memory.Epml4[Pml4Idx];
    //DebugPrint0402("Epml4 = 0x%x at 0x%p\n", Epml4->All, Epml4);
    if (!Epml4->Present)
        return FALSE;

    EPT_PDPTE* Pdpt = &(((EPT_PDPTE*)(Epml4->PhysicalAddress << 12))[PdptIdx]);
    //DebugPrint0402("Pdpt = 0x%x at 0x%p\n", Pdpt->All, Pdpt);
    if (!Pdpt->Present)
        return FALSE;

    EPT_PDE* Pd = &(((EPT_PDE*)(Pdpt->PhysicalAddress << 12))[PdIdx]);
    //DebugPrint0402("Pd = 0x%x at 0x%p\n", Pd->All, Pd);
    if (!Pd->Present)
        return FALSE;

    /*
    if (Pd->Large)
        return (VOID*)(UINT64)(Pd->PhysicalAddress << 21);
    */

    EPT_PTE* Pt = &(((EPT_PTE*)(((EPT_PDE*)Pd)->PhysicalAddress << 12))[PtIdx]);
    //DebugPrint0402("Pt = 0x%x at 0x%p\n", Pt->All, Pt);
    if (!Pt->Present)
        return FALSE;

    UINT64 Result = (UINT64)(Pt->PhysicalAddress << 12) + Offset;

    DebugPrint0402("GPA = 0x%p, translates to HPA = 0x%p. Pml4Idx = %d, PdptIdx = %d, PdIdx = %d, PtIdx = %d\n", GuestPhysicalAddress, Result, Pml4Idx, PdptIdx, PdIdx, PtIdx);

    *HostPhysicalAddress = (VOID*)Result;

    return TRUE;
}

BOOLEAN
EFIAPI
TranslateGuestVirtualToGuestPhysical(
    IN CR3      GuestCr3,
    IN UINT64   GuestVirtualAddress,
    OUT UINT64* GuestPhysicalAddress
) {
    *GuestPhysicalAddress = NULL;

    UINT16 Pml4Idx = (GuestVirtualAddress >> 39) & 0x1FF;
    UINT16 PdptIdx = (GuestVirtualAddress >> 30) & 0x1FF;
    UINT16 PdIdx = (GuestVirtualAddress >> 21) & 0x1FF;
    UINT16 PtIdx = (GuestVirtualAddress >> 12) & 0x1FF;
    UINT16 Offset = (GuestVirtualAddress) & 0xFFF;

    PML4* Pml4 = NULL;
    if (!TranslateGuestPhysicalToHostPhysical(GuestCr3.PhysicalAddress << 12, (VOID**)&Pml4)) return FALSE;
    if (!Pml4[Pml4Idx].Present) return FALSE;

    PDPTE* Pdpt = NULL;
    if (!TranslateGuestPhysicalToHostPhysical(Pml4[Pml4Idx].PhysicalAddress << 12, (VOID**)&Pdpt)) return FALSE;
    if (!Pdpt[PdptIdx].Present) return FALSE;

    if (Pdpt[PdptIdx].Large) {
        PDPE_LARGE* PdpeLarge = (PDPE_LARGE*)&Pdpt[PdptIdx];
        UINT64 Offset1GB = GuestVirtualAddress & 0x3FFFFFFF; // 30-bit offset

        *GuestPhysicalAddress = (UINT64)(PdpeLarge->PhysicalAddress << 30) + Offset1GB;
        return TRUE;
    }

    PDE* Pde = NULL;
    if (!TranslateGuestPhysicalToHostPhysical(Pdpt[PdptIdx].PhysicalAddress << 12, (VOID**)&Pde)) return FALSE;
    if (!Pde[PdIdx].Present) return FALSE;

    if (Pde[PdIdx].Large) {
        PDE_LARGE* PdeLarge = (PDE_LARGE*)&Pde[PdIdx];
        UINT64 Offset2MB = GuestVirtualAddress & 0x1FFFFF; // 21-bit offset

        *GuestPhysicalAddress = (UINT64)(PdeLarge->PhysicalAddress << 21) + Offset2MB;
        return TRUE;
    }

    PTE* Pte = NULL;
    if (!TranslateGuestPhysicalToHostPhysical(Pde[PdIdx].PhysicalAddress << 12, (VOID**)&Pte)) return FALSE;
    if (!Pte[PtIdx].Present) return FALSE;

    *GuestPhysicalAddress = (UINT64)((Pte[PtIdx].PhysicalAddress << 12)) + Offset;

    return TRUE;
}

BOOLEAN
EFIAPI
TranslateGuestAddressToHostAddress(
    IN CR3      GuestCr3,
    IN UINT64   GuestVirtual,
    OUT VOID**  HostVirtual
) {

    UINT64 HostPhysical = 0;
    if (!TranslateGuestVirtualToGuestPhysical(GuestCr3, GuestVirtual, &HostPhysical)) return FALSE;

    return TranslateGuestPhysicalToHostPhysical(HostPhysical, HostVirtual);
}


BOOLEAN
EFIAPI
MapSinglePage(
    IN UINT64   GuestPhysical,
    IN UINT64   HostPhysical,
    IN BOOLEAN  Big
) {
    UINT16 Pml4Idx  = (GuestPhysical >> 39) & 0x1FF;
    UINT16 PdptIdx  = (GuestPhysical >> 30) & 0x1FF;
    UINT16 PdIdx    = (GuestPhysical >> 21) & 0x1FF;
    UINT16 PtIdx    = (GuestPhysical >> 12) & 0x1FF;

    //DebugPrint0402("Mapping GPA = %p to HPA = %p, Big = %d, Pml4Idx = %d, PdptIdx = %d, PdIdx = %d, PtIdx = %d\n",
        //GuestPhysical, HostPhysical, Big, Pml4Idx, PdptIdx, PdIdx, PtIdx);

    //
    // PML4 Level
    //
    EPT_PML4* Epml4 = &g_VmState.Memory.Epml4[Pml4Idx];
    if (!Epml4->Present) {
        Epml4->Execute = TRUE;
        Epml4->Write = TRUE;
        Epml4->Read = TRUE;
        Epml4->ExecuteUser = TRUE;

        if (g_VmState.Memory.EpdptCount >= 64) {
            DebugPrint0402("g_EpdptCount is bigger than 64. Borked\n");
            return FALSE;
        }
        UINT32 PdptPoolIndex = g_VmState.Memory.EpdptCount++;

        Epml4->PhysicalAddress = ((UINT64)&g_VmState.Memory.Epdpt[PdptPoolIndex][0]) >> 12;
        Epml4->Present = TRUE;

        //DebugPrint0402("EPT_PML4 = 0x%p at 0x%p\n", Epml4->All, Epml4);
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

        if (g_VmState.Memory.EpdeCount >= 512) {
            DebugPrint0402("g_EpdeCount is bigger than 512. Borked\n");
            return FALSE;
        }
        UINT32 PdPoolIndex = g_VmState.Memory.EpdeCount++;

        Pdpt->PhysicalAddress = ((UINT64)&g_VmState.Memory.Epde[PdPoolIndex][0]) >> 12;
        Pdpt->Present = TRUE;
        //DebugPrint0402("EPT_PDPTE = 0x%p at %p\n", Pdpt->All, Pdpt);
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

        //DebugPrint0402("EPT_LARGE_PDE = 0x%p at %p\n", Pd->All, Pd);
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
        if (g_VmState.Memory.EpteCount >= 16384) {
            DebugPrint0402("g_EpteCount is bigger than 16384. Borked\n");
            return FALSE;
        }

        UINT32 PtPoolIndex = g_VmState.Memory.EpteCount++;

        NonLargePd->PhysicalAddress = ((UINT64)&g_VmState.Memory.Epte[PtPoolIndex][0]) >> 12;
        NonLargePd->Present = TRUE;

        //DebugPrint0402("EPT_PDE = 0x%p at %p. EPTE count = %d\n", NonLargePd->All, NonLargePd, g_EpteCount);
    }

    EPT_PTE* Pt = &(((EPT_PTE*)(NonLargePd->PhysicalAddress << 12))[PtIdx]);
    if (Pt->Present) {
        return FALSE;
    }

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

    //DebugPrint0402("EPT_PTE = 0x%p at %p\n", Pt->All, Pt);
    return TRUE;
}

BOOLEAN
EFIAPI
MapPagesLinear(
    IN UINT64       PhysicalStart,
    IN UINT64       NumberOfPages,
    IN OUT UINT64*  CurrentGuestPhysical
) {
    UINT64 PhysicalAddr     = PhysicalStart;
    UINT64 GuestPhysical    = *CurrentGuestPhysical;
    UINT64 BytesRemaining   = NumberOfPages * SIZE_4KB;

    //DebugPrint0402("Mapping PhysicalStart = 0x%p and %d pages starting from 0x%p\n", PhysicalAddr, NumberOfPages, GuestPhysical);

    while (BytesRemaining > 0) {
        BOOLEAN IsAligned = ((PhysicalAddr & (SIZE_2MB - 1)) == 0) &&
            ((GuestPhysical & (SIZE_2MB - 1)) == 0);
        BOOLEAN IsLargeEnough = (BytesRemaining >= SIZE_2MB);

        //DebugPrint0402("BytesRemaining = %d, IsAligned = %d, IsLargeEnough = %d\n", BytesRemaining, IsAligned, IsLargeEnough);

        //
        // Never map large pages.
        //
        if (IsAligned && IsLargeEnough && FALSE) {
            //
            // Map 2MB page.
            //
            if (!MapSinglePage(GuestPhysical, PhysicalAddr, TRUE)) {
                return FALSE;
            }
            PhysicalAddr += SIZE_2MB;
            GuestPhysical += SIZE_2MB;
            BytesRemaining -= SIZE_2MB;
        }
        else {
            //
            // Map 4KB page.
            //
            if (!MapSinglePage(GuestPhysical, PhysicalAddr, FALSE)) {
                return FALSE;
            }
            PhysicalAddr += SIZE_4KB;
            GuestPhysical += SIZE_4KB;
            BytesRemaining -= SIZE_4KB;
        }
    }

    *CurrentGuestPhysical = GuestPhysical;

    return TRUE;
}