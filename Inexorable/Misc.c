#include <Uefi.h>
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