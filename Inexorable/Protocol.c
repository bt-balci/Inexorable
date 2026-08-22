#include <Uefi.h>

#include "x86.h"
#include "Vm.h"

EFI_STATUS
EFIAPI
IxOpenProtocol(
	IN EFI_HANDLE	Handle,
	IN EFI_GUID* Protocol,
	OUT VOID** Interface OPTIONAL,
	IN EFI_HANDLE	AgentHandle,
	IN EFI_HANDLE	ControllerHandle,
	IN UINT32		Attributes
) {
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_OPEN_PROTOCOL, (UINT64)Handle, (UINT64)Protocol, (UINT64)Interface, (UINT64)AgentHandle, (UINT64)ControllerHandle, (UINT64)Attributes);
}

EFI_STATUS
EFIAPI
IxHandleProtocol(
	IN EFI_HANDLE	Handle,
	IN EFI_GUID*	Protocol,
	OUT VOID**		Interface
) {
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_HANDLE_PROTOCOL, (UINT64)Handle, (UINT64)Protocol, (UINT64)Interface, 0, 0, 0);
}

EFI_STATUS
EFIAPI
IxLocateProtocol(
	IN EFI_GUID* Protocol,
	IN VOID* Registration OPTIONAL,
	OUT VOID** Interface
) {
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_LOCATE_PROTOCOL, (UINT64)Protocol, (UINT64)Registration, (UINT64)Interface, 0, 0, 0);
}