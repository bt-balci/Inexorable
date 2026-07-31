#include <Uefi.h>
#include "Vm.h"
#include "x86.h"

EFI_STATUS
EFIAPI
IxSetVirtualAddressMap(
	IN  UINTN                        MemoryMapSize,
	IN  UINTN                        DescriptorSize,
	IN  UINT32                       DescriptorVersion,
	IN  EFI_MEMORY_DESCRIPTOR* VirtualMap
) {
	return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
IxConvertPointer(
	IN     UINTN	DebugDisposition,
	IN OUT VOID** Address
) {
	return EFI_SUCCESS;
}