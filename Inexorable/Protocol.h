#pragma once
#include <Uefi.h>

EFI_STATUS
EFIAPI
IxOpenProtocol(
	IN EFI_HANDLE	Handle,
	IN EFI_GUID* Protocol,
	OUT VOID** Interface OPTIONAL,
	IN EFI_HANDLE	AgentHandle,
	IN EFI_HANDLE	ControllerHandle,
	IN UINT32		Attributes
);

EFI_STATUS
EFIAPI
IxHandleProtocol(
	IN EFI_HANDLE	Handle,
	IN EFI_GUID* Protocol,
	OUT VOID** Interface
);

EFI_STATUS
EFIAPI
IxLocateProtocol(
	IN EFI_GUID* Protocol,
	IN VOID* Registration OPTIONAL,
	OUT VOID** Interface
);

EFI_STATUS
EFIAPI
IxInstallProtocolInterface(
	IN OUT EFI_HANDLE*		Handle,
	IN EFI_GUID*			Protocol,
	IN EFI_INTERFACE_TYPE	InterfaceType,
	IN VOID*				Interface
);