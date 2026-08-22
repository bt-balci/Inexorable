#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#include "Efi.h"
#include "Firmware.h"
#include "x86.h"
#include "Vm.h"

LIST_ENTRY g_ProtocolList = { 0 };
LIST_ENTRY g_HandleList = { 0 };
UINTN HandleListHint = 0;

EFI_STATUS
EFIAPI
IxHandleProtocol(
	IN EFI_HANDLE	Handle,
	IN EFI_GUID*	Protocol,
	OUT VOID**		Interface
) {
	LIST_ENTRY* Link;
	INEXORABLE_FIRMWARE_HANDLE* FwHandle;
	for (Link = GetFirstNode(&g_HandleList);
		!IsNull(&g_HandleList, Link);
		Link = GetNextNode(&g_HandleList, Link)) {

		FwHandle = (INEXORABLE_FIRMWARE_HANDLE*)Link;

		if (FwHandle->Value == Handle) {
			LIST_ENTRY* ProtocolLink;
			INEXORABLE_FIRMWARE_PROTOCOL* FwProtocol;
			for (ProtocolLink = GetFirstNode(&FwHandle->InstalledProtocols);
				!IsNull(&FwHandle->InstalledProtocols, ProtocolLink);
				ProtocolLink = GetNextNode(&FwHandle->InstalledProtocols, ProtocolLink)) {
				
				FwProtocol = (INEXORABLE_FIRMWARE_PROTOCOL*)Link;

				if (CompareGuid(&FwProtocol->Guid, Protocol)) {
					*Interface = FwProtocol->Pointer;
					return EFI_SUCCESS;
				}
			}

			return EFI_NOT_FOUND;
		}
	}

	return EFI_NOT_FOUND;
}

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
	if ((Attributes & EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL) != 0)
		return IxHandleProtocol(Handle, Protocol, Interface);
}

EFI_STATUS
EFIAPI
IxLocateProtocol(
	IN EFI_GUID* Protocol,
	IN VOID* Registration OPTIONAL,
	OUT VOID** Interface
) {
	LIST_ENTRY* Link;
	INEXORABLE_FIRMWARE_PROTOCOL* FwProtocol;
	for (Link = GetFirstNode(&g_ProtocolList);
		!IsNull(&g_ProtocolList, Link);
		Link = GetNextNode(&g_ProtocolList, Link)) {

		FwProtocol = (INEXORABLE_FIRMWARE_PROTOCOL*)Link;

		if (CompareGuid(&FwProtocol->Guid, Protocol)) {
			*Interface = FwProtocol->Pointer;
			return EFI_SUCCESS;
		}
	}

	return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
IxInstallProtocolInterface(
	IN OUT EFI_HANDLE* Handle,
	IN EFI_GUID* Protocol,
	IN EFI_INTERFACE_TYPE	InterfaceType,
	IN VOID* Interface
) {
	INEXORABLE_FIRMWARE_PROTOCOL* FwProtocol = NULL;
	INEXORABLE_FIRMWARE_HANDLE* FwHandle = NULL;

	EFI_STATUS Status = gBS.AllocatePool(EfiRuntimeServicesData, sizeof(INEXORABLE_FIRMWARE_PROTOCOL), (VOID**)&FwProtocol);
	if (EFI_ERROR(Status)) goto Fail;

	if (*Handle == NULL) {
		Status = gBS.AllocatePool(EfiRuntimeServicesData, sizeof(INEXORABLE_FIRMWARE_PROTOCOL), (VOID**)&FwHandle);
		if (EFI_ERROR(Status)) goto Fail;

		HandleListHint += 1;
		FwHandle->Value = (EFI_HANDLE)HandleListHint;
		*Handle = FwHandle->Value;

		InsertTailList(&g_HandleList, &FwHandle->Link);

		InitializeListHead(&FwHandle->InstalledProtocols);
		InsertTailList(&FwHandle->InstalledProtocols, &FwProtocol->Link);
	}
	else {
		LIST_ENTRY* Link;
		INEXORABLE_FIRMWARE_HANDLE* FwHandle;
		for (Link = GetFirstNode(&g_HandleList);
			!IsNull(&g_HandleList, Link);
			Link = GetNextNode(&g_HandleList, Link)) {

			FwHandle = (INEXORABLE_FIRMWARE_HANDLE*)Link;

			if (FwHandle->Value == *Handle) {
				InsertTailList(&FwHandle->InstalledProtocols, &FwProtocol->Link);
				break;
			}
		}

		return EFI_INVALID_PARAMETER;
	}

	InsertTailList(&g_ProtocolList, &FwProtocol->Link);

	return EFI_SUCCESS;

Fail:
	if (FwProtocol != NULL) gBS.FreePool(FwProtocol);
	if (FwHandle != NULL) gBS.FreePool(FwHandle);
}