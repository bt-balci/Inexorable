#pragma once
#include "efi_spec.h"

EFI_STATUS
EFIAPI
IxGetNextHighMonotonicCount(
	OUT UINT32*		HighCount
);

VOID
EFIAPI
IxResetSystem(
	IN INT32	ResetType,
	IN EFI_STATUS		ResetStatus,
	IN UINTN			DataSize,
	IN VOID*			ResetData OPTIONAL
);

EFI_STATUS
EFIAPI
IxExitBootServices(
	IN EFI_HANDLE	ImageHandle,
	IN UINTN		MapKey
);

EFI_STATUS
EFIAPI
IxCalculateCrc32(
	IN VOID* Data,
	IN UINTN	DataSize,
	OUT UINT32* Crc32
);

VOID
EFIAPI
IxCopyMem(
	IN VOID* Destination,
	IN VOID* Source,
	IN UINTN    Length
);

VOID
EFIAPI
IxSetMem(
	IN VOID* Buffer,
	IN UINTN    Size,
	IN UINT8    Value
);