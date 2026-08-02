#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>

#include "Vm.h"
#include "x86.h"

EFI_STATUS
EFIAPI
IxGetNextHighMonotonicCount(
	OUT UINT32* HighCount
) {
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_GET_NEXT_HIGH_MONO_COUNT, (UINT64)HighCount, 0, 0, 0, 0, 0);
}

VOID
EFIAPI
IxResetSystem(
	IN EFI_RESET_TYPE           ResetType,
	IN EFI_STATUS               ResetStatus,
	IN UINTN                    DataSize,
	IN VOID* ResetData OPTIONAL
) {
	__vmx_vmcall(CATALYST_VMCALL_RESET_SYSTEM, (UINT64)ResetType, (UINT64)ResetStatus, (UINT64)DataSize, (UINT64)ResetData, 0, 0);
}

EFI_STATUS
EFIAPI
IxExitBootServices(
	IN EFI_HANDLE	ImageHandle,
	IN UINTN		MapKey
) {
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_EXIT_BOOT_SERVICES, (UINT64)ImageHandle, MapKey, 0, 0, 0, 0);
}

EFI_STATUS
EFIAPI
IxCalculateCrc32(
	IN VOID*	Data,
	IN UINTN	DataSize,
	OUT UINT32* Crc32
) {
	if (Data == NULL) return EFI_INVALID_PARAMETER;
	if (Crc32 == NULL) return EFI_INVALID_PARAMETER;
	if (DataSize == 0) return EFI_INVALID_PARAMETER;

	*Crc32 = CalculateCrc32(Data, DataSize);

	return EFI_SUCCESS;
}

VOID
EFIAPI
IxCopyMem(
	IN VOID* Destination,
	IN VOID* Source,
	IN UINTN    Length
) {
	CopyMem(Destination, Source, Length);
}

VOID
EFIAPI
IxSetMem(
	IN VOID* Buffer,
	IN UINTN    Size,
	IN UINT8    Value
) {
	IxSetMem(Buffer, Size, Value);
}