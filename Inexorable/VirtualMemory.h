#pragma once
#include <Uefi.h>

EFI_STATUS
EFIAPI
IxSetVirtualAddressMap(
	IN  UINTN                        MemoryMapSize,
	IN  UINTN                        DescriptorSize,
	IN  UINT32                       DescriptorVersion,
	IN  EFI_MEMORY_DESCRIPTOR* VirtualMap
);

EFI_STATUS
EFIAPI
IxConvertPointer(
	IN     UINTN	DebugDisposition,
	IN OUT VOID** Address
);