#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <Uefi.h>

#include "Vm.h"

///////////////////////////////////////////////
////				Methods				   ////
///////////////////////////////////////////////


BOOLEAN
EFIAPI
MapSinglePage(
	IN UINT64   GuestPhysical,
	IN UINT64   HostPhysical,
	IN BOOLEAN  Big
);

BOOLEAN
EFIAPI
MapPagesLinear(
	IN UINT64       PhysicalStart,
	IN UINT64       NumberOfPages,
	IN OUT UINT64*  CurrentGuestPhysical
);

BOOLEAN
EFIAPI
CopyToGuest(
	IN CR3      GuestCr3,
	IN VOID* HostBuffer,
	IN UINT64   GuestVirtualAddress,
	IN UINTN    Size
);

BOOLEAN
EFIAPI
CopyFromGuest(
	IN CR3      GuestCr3,
	IN VOID* HostBuffer,
	IN UINT64   GuestVirtualAddress,
	IN UINTN    Size
);

UINTN
StrSizeFromGuest(
	IN CR3      GuestCr3,
	IN UINT64   GuestAddress
);

UINTN
StrLenFromGuest(
	IN CR3      GuestCr3,
	IN UINT64   GuestAddress
);

BOOLEAN
EFIAPI
GetGuestEptEntry(
	IN UINT64		GuestPhysicalAddress,
	OUT EPT_PML4** Pml4					OPTIONAL,
	OUT EPT_PDPTE** Pdpt					OPTIONAL,
	OUT EPT_PDE** Pd						OPTIONAL,
	OUT EPT_PTE** Pt						OPTIONAL,
	OUT UINT16* Offset					OPTIONAL
);

BOOLEAN
EFIAPI
TranslateGuestPhysicalToHostPhysical(
	IN UINT64	GuestPhysicalAddress,
	OUT VOID**	HostPhysicalAddress
);

BOOLEAN
EFIAPI
TranslateGuestVirtualToGuestPhysical(
	IN CR3      GuestCr3,
	IN UINT64   GuestVirtualAddress,
	OUT UINT64* GuestPhysicalAddress
);

BOOLEAN
EFIAPI
TranslateGuestAddressToHostAddress(
	IN CR3      GuestCr3,
	IN UINT64   GuestVirtual,
	OUT VOID**  HostVirtual
);

#endif