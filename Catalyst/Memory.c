#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>

#include "Efi.h"
#include "Memory.h"
#include "x86.h"
#include "Vm.h"

UINTN
StrSizeFromGuest(
	IN CR3      GuestCr3,
	IN UINT64   GuestAddress
) {
	CONST UINT64 ChunkChars = 256;
	CONST UINT64 ChunkBytes = ChunkChars * sizeof(CHAR16);
	CONST UINT64 MaxReturn = (ChunkChars + 1) * 2;

	UINT64 Source = GuestAddress;
	CHAR16 Temp[256] = { 0 };

	UINTN TotalLength = 0;
	UINTN Length = MaxReturn;

	while (Length == MaxReturn) {
		if (!CopyFromGuest(GuestCr3, (VOID*)&Temp, Source, ChunkBytes)) {
			return MAX_UINTN;
		}

		if (Length == MaxReturn) {
			//
			// Not null-terminated in this chunk. Add only the real characters, no phantom null terminator.
			//
			TotalLength += ChunkBytes;
			Source += ChunkBytes;
		}
		else {
			TotalLength += Length;   // Includes the real null terminator
		}
	}

	return TotalLength;
}

UINTN
StrLenFromGuest(
	IN CR3      GuestCr3,
	IN UINT64   GuestAddress
) {
	CONST UINT64 ChunkChars = 256;
	CONST UINT64 ChunkBytes = ChunkChars * sizeof(CHAR16);

	UINT64 Source = GuestAddress;
	CHAR16 Temp[256] = { 0 };

	UINTN TotalLength = 0;
	UINTN Length = 256;

	while (Length == 256) {
		if (!CopyFromGuest(GuestCr3, (VOID*)&Temp, Source, ChunkBytes)) {
			return MAX_UINTN;
		}

		Length = StrnLenS(Temp, 256);
		Source += 256;
		TotalLength += Length;
	}

	return TotalLength;
}

BOOLEAN
EFIAPI
CopyFromGuest(
	IN CR3      GuestCr3,
	IN VOID*	HostBuffer,
	IN UINT64   GuestVirtualAddress,
	IN UINTN    Size
) {
	UINT8* Destination = (UINT8*)HostBuffer;
	UINT64 Source = GuestVirtualAddress;
	UINTN BytesRemaining = Size;

	while (BytesRemaining > 0) {
		VOID* HostPointer = NULL;

		if (!TranslateGuestAddressToHostAddress(GuestCr3, Source, &HostPointer)) {
			//
			// Maybe inject pagefault.
			//
			return FALSE;
		}

		//
		// Calculate how many bytes we can safely copy before hitting the next page boundary.
		//
		UINTN OffsetInPage = Source & 0xFFF;
		UINTN BytesToCopyThisPage = 4096 - OffsetInPage;
		if (BytesToCopyThisPage > BytesRemaining) {
			BytesToCopyThisPage = BytesRemaining;
		}

		CopyMem(Destination, HostPointer, BytesToCopyThisPage);

		Destination += BytesToCopyThisPage;
		Source += BytesToCopyThisPage;
		BytesRemaining -= BytesToCopyThisPage;
	}

	return TRUE;
}

BOOLEAN
EFIAPI
CopyToGuest(
	IN CR3      GuestCr3,
	IN VOID*	HostBuffer,
	IN UINT64   GuestVirtualAddress,
	IN UINTN    Size
) {
	UINT8* Source = (UINT8*)HostBuffer;
	UINT64 Destination = GuestVirtualAddress;
	UINTN BytesRemaining = Size;

	//
	// There's arbitrary read/write exploit because I didn't check the page tables for permission bits.
	// I can't be bothered. This is for a PoC.
	//

	while (BytesRemaining > 0) {
		VOID* HostPointer = NULL;

		if (!TranslateGuestAddressToHostAddress(GuestCr3, Destination, &HostPointer)) return FALSE;

		UINTN Offset = Destination & 0xFFF;
		UINTN BytesToCopy = 0x1000 - Offset;
		if (BytesToCopy > BytesRemaining) {
			BytesToCopy = BytesRemaining;
		}

		CopyMem(HostPointer, Source, BytesToCopy);

		Destination += BytesToCopy;
		Source += BytesToCopy;
		BytesRemaining -= BytesToCopy;
	}

	return TRUE;
}

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

	EPT_PML4* Epml4 = &g_VmState.AddressSpace.Epml4[Pml4Idx];
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
	EPT_PML4* Epml4 = &g_VmState.AddressSpace.Epml4[Pml4Idx];
	if (!Epml4->Present) {
		Epml4->Execute = TRUE;
		Epml4->Write = TRUE;
		Epml4->Read = TRUE;
		Epml4->ExecuteUser = TRUE;

		if (g_VmState.AddressSpace.EpdptCount >= 64) {
			DebugPrint0402("g_EpdptCount is bigger than 64. Borked\n");
			return FALSE;
		}
		UINT32 PdptPoolIndex = g_VmState.AddressSpace.EpdptCount++;

		Epml4->PhysicalAddress = ((UINT64)&g_VmState.AddressSpace.Epdpt[PdptPoolIndex][0]) >> 12;
		Epml4->Present = TRUE;

		//DebugPrint0402("EPT_PML4 = 0x%p at 0x%p\n", Epml4->All, Epml4);
	}

	//
	// PDPT Level
	//
	EPT_PDPTE* Epdpt = &(((EPT_PDPTE*)(Epml4->PhysicalAddress << 12))[PdptIdx]);

	if (!Epdpt->Present) {
		Epdpt->Execute = TRUE;
		Epdpt->Write = TRUE;
		Epdpt->Read = TRUE;
		Epdpt->ExecuteUser = TRUE;

		if (g_VmState.AddressSpace.EpdeCount >= 512) {
			DebugPrint0402("g_EpdeCount is bigger than 512. Borked\n");
			return FALSE;
		}
		UINT32 PdPoolIndex = g_VmState.AddressSpace.EpdeCount++;

		Epdpt->PhysicalAddress = ((UINT64)&g_VmState.AddressSpace.Epde[PdPoolIndex][0]) >> 12;
		Epdpt->Present = TRUE;
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
	EPT_PDE* Epd = &(((EPT_PDE*)(Epdpt->PhysicalAddress << 12))[PdIdx]);

	if (!Epd->Present) {
		Epd->Execute = TRUE;
		Epd->Write = TRUE;
		Epd->Read = TRUE;
		Epd->ExecuteUser = TRUE;

		//
		// Hmm. Can we get borked? Better log than hang.
		//
		if (g_VmState.AddressSpace.EpteCount >= 16384) {
			DebugPrint0402("g_EpteCount is bigger than 16384. Borked\n");
			return FALSE;
		}

		UINT32 PtPoolIndex = g_VmState.AddressSpace.EpteCount++;

		Epd->PhysicalAddress = ((UINT64)&g_VmState.AddressSpace.Epte[PtPoolIndex][0]) >> 12;
		Epd->Present = TRUE;

		//DebugPrint0402("EPT_PDE = 0x%p at %p. EPTE count = %d\n", NonLargePd->All, NonLargePd, g_EpteCount);
	}

	EPT_PTE* Ept = &(((EPT_PTE*)(Epd->PhysicalAddress << 12))[PtIdx]);
	if (Ept->Present) {
		DebugPrint0402("Ept is already present. Failed to map. GPA = 0x%p to HPA = 0x%p, Big = %d, Pml4Idx = %d, PdptIdx = %d, PdIdx = %d, PtIdx = %d, Ept = 0x%p\n",
			GuestPhysical, HostPhysical, Big, Pml4Idx, PdptIdx, PdIdx, PtIdx, Ept->All);
		return FALSE;
	}

	//
	// Set the 4KB page entry
	//
	Ept->EptMemoryType = 6;
	Ept->Execute = TRUE;
	Ept->Write = TRUE;
	Ept->Read = TRUE;
	Ept->ExecuteUser = TRUE;

	Ept->PhysicalAddress = HostPhysical >> 12;
	Ept->Present = TRUE;

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