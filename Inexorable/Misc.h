#pragma once
#include <Uefi.h>

EFI_STATUS
EFIAPI
IxGetNextHighMonotonicCount(
	OUT UINT32*		HighCount
);

VOID
EFIAPI
IxResetSystem(
	IN EFI_RESET_TYPE	ResetType,
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