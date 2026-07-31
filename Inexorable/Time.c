#include <Uefi.h>
#include "Vm.h"
#include "x86.h"

EFI_STATUS
EFIAPI
IxGetTime(
	OUT  EFI_TIME* Time,
	OUT  EFI_TIME_CAPABILITIES* Capabilities OPTIONAL
) {
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_GET_TIME, (UINT64)Time, (UINT64)Capabilities, 0, 0, 0, 0);
}

EFI_STATUS
EFIAPI
IxSetTime(
	OUT  EFI_TIME* Time
) {
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_SET_TIME, (UINT64)Time, 0, 0, 0, 0, 0);
}

EFI_STATUS
EFIAPI
IxGetWakeupTime(
	OUT BOOLEAN* Enabled,
	OUT BOOLEAN* Pending,
	OUT EFI_TIME* Time
) {
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_GET_WAKEUP_TIME, (UINT64)Enabled, (UINT64)Pending, (UINT64)Time, 0, 0, 0);
}

EFI_STATUS
EFIAPI
IxSetWakeupTime(
	IN  BOOLEAN	Enable,
	IN  EFI_TIME* Time   OPTIONAL
) {
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_SET_WAKEUP_TIME, (UINT64)Enable, (UINT64)Time, 0, 0, 0, 0);
}