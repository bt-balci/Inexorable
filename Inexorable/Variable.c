#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#include "Vm.h"
#include "Efi.h"
#include "Firmware.h"
#include "x86.h"

LIST_ENTRY g_VariableList;

EFI_STATUS
EFIAPI
IxGetVariable(
	IN     CHAR16*		VariableName,
	IN     EFI_GUID*	VendorGuid,
	OUT    UINT32*		Attributes     OPTIONAL,
	IN OUT UINTN*		DataSize,
	OUT    VOID*		Data           OPTIONAL
) {
	if (VariableName == NULL || VendorGuid == NULL || DataSize == NULL) {
		return EFI_INVALID_PARAMETER;
	}

	LIST_ENTRY* Link;
	INEXORABLE_FIRMWARE_VARIABLE* FwVariable;
	for (Link = GetFirstNode(&g_VariableList);
		!IsNull(&g_VariableList, Link);
		Link = GetNextNode(&g_VariableList, Link)) {

		FwVariable = (INEXORABLE_FIRMWARE_VARIABLE*)Link;

		if (CompareGuid(&FwVariable->Guid, VendorGuid) && StrCmp(VariableName, FwVariable->Name) == 0) {
			if (Data != NULL) {
				if (*DataSize < FwVariable->BufferSize) {
					*DataSize = FwVariable->BufferSize;
					return EFI_BUFFER_TOO_SMALL;
				}
				CopyMem(Data, FwVariable->Buffer, *DataSize);
			}
			
			*DataSize = FwVariable->BufferSize;
			*Attributes = FwVariable->Attributes;
			return EFI_SUCCESS;
		}
	}

	return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
IxSetVariable(
	IN  CHAR16*		VariableName,
	IN  EFI_GUID*	VendorGuid,
	IN  UINT32      Attributes,
	IN  UINTN		DataSize,
	IN  VOID*		Data
) {
	LIST_ENTRY* Link;
	INEXORABLE_FIRMWARE_VARIABLE* FwVariable;
	for (Link = GetFirstNode(&g_VariableList);
		!IsNull(&g_VariableList, Link);
		Link = GetNextNode(&g_VariableList, Link)) {

		FwVariable = (INEXORABLE_FIRMWARE_VARIABLE*)Link;

		if (CompareGuid(&FwVariable->Guid, VendorGuid) && StrCmp(VariableName, FwVariable->Name)) {
			//
			// Means delete variable
			//
			if (DataSize == 0) {
				RemoveEntryList(&FwVariable->Link);
				if (FwVariable->Buffer)
					gBS.FreePool(FwVariable->Buffer);
				gBS.FreePool(FwVariable);
				return EFI_SUCCESS;
			}

			EFI_STATUS Status = gBS.FreePool(FwVariable->Buffer);
			if (EFI_ERROR(Status)) return EFI_OUT_OF_RESOURCES;

			Status = gBS.AllocatePool(EfiLoaderData, DataSize, &FwVariable->Buffer);
			if (EFI_ERROR(Status)) return EFI_OUT_OF_RESOURCES;

			FwVariable->BufferSize = DataSize;
			FwVariable->Attributes = Attributes;

			CopyMem(FwVariable->Buffer, Data, DataSize);;

			return EFI_SUCCESS;
		}
	}

	//
	// Nothing
	//
	if (DataSize == 0) {
		return EFI_NOT_FOUND;
	}

	EFI_STATUS Status = gBS.AllocatePool(EfiLoaderData, sizeof(INEXORABLE_FIRMWARE_VARIABLE), (VOID**)&FwVariable);
	if (EFI_ERROR(Status)) return EFI_OUT_OF_RESOURCES;

	StrCpyS(FwVariable->Name, 256, VariableName);
	CopyGuid(&FwVariable->Guid, VendorGuid);
	FwVariable->Attributes = Attributes;
	FwVariable->BufferSize = DataSize;
	CopyMem(FwVariable->Buffer, Data, DataSize);

	InsertTailList(&g_VariableList, &FwVariable->Link);

	return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
IxGetNextVariableName(
	IN OUT UINTN* VariableNameSize,
	IN OUT CHAR16* VariableName,
	IN OUT EFI_GUID* VendorGuid
) {
	if (VariableNameSize == NULL || VariableName == NULL || VendorGuid == NULL) {
		return EFI_INVALID_PARAMETER;
	}

	LIST_ENTRY* Link;
	INEXORABLE_FIRMWARE_VARIABLE* FwVariable;
	
	//
	// If VariableName is an empty string, return the FIRST variable.
	//
	if (VariableName[0] == L'\0') {
		if (IsListEmpty(&g_VariableList)) {
			return EFI_NOT_FOUND;
		}

		Link = GetFirstNode(&g_VariableList);
		FwVariable = (INEXORABLE_FIRMWARE_VARIABLE*)Link;
		goto ReturnNextVar;
	}

	for (Link = GetFirstNode(&g_VariableList);
		!IsNull(&g_VariableList, Link);
		Link = GetNextNode(&g_VariableList, Link)) {

		FwVariable = (INEXORABLE_FIRMWARE_VARIABLE*)Link;

		if (CompareGuid(&FwVariable->Guid, VendorGuid) && StrCmp(VariableName, FwVariable->Name) == 0) {

			Link = GetNextNode(&g_VariableList, Link);

			if (IsNull(&g_VariableList, Link)) {
				return EFI_NOT_FOUND;
			}

			FwVariable = (INEXORABLE_FIRMWARE_VARIABLE*)Link;
			goto ReturnNextVar;
		}
	}

	return EFI_INVALID_PARAMETER;

ReturnNextVar:

	//
	// Check if user's VariableName buffer is large enough for the NEXT name
	// StrSize returns the size in bytes including the null terminator.
	//
	UINTN RequiredSize = StrSize(FwVariable->Name);
	if (*VariableNameSize < RequiredSize) {
		*VariableNameSize = RequiredSize;
		return EFI_BUFFER_TOO_SMALL;
	}
	*VariableNameSize = RequiredSize;
	StrCpyS(VariableName, *VariableNameSize / sizeof(CHAR16), FwVariable->Name);
	CopyGuid(VendorGuid, &FwVariable->Guid);

	return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
IxQueryVariableInfo(
	IN  UINT32	Attributes,
	OUT UINT64* MaximumVariableStorageSize,
	OUT UINT64* RemainingVariableStorageSize,
	OUT UINT64* MaximumVariableSize
) {
	if (MaximumVariableStorageSize == NULL || RemainingVariableStorageSize == NULL || MaximumVariableSize == NULL) {
		return EFI_INVALID_PARAMETER;
	}
	
	//
	// Why not?
	//
	const UINT64 TOTAL_STORAGE_SIZE = 1024 * 1024;
	const UINT64 MAX_SINGLE_VAR_SIZE = 64 * 1024;

	UINT64 UsedStorage = 0;
	LIST_ENTRY* Link;
	INEXORABLE_FIRMWARE_VARIABLE* FwVariable;

	//
	// Calculate how much storage is currently used up.
	//
	for (Link = GetFirstNode(&g_VariableList);
		!IsNull(&g_VariableList, Link);
		Link = GetNextNode(&g_VariableList, Link)) {

		FwVariable = (INEXORABLE_FIRMWARE_VARIABLE*)Link;

		UsedStorage += sizeof(INEXORABLE_FIRMWARE_VARIABLE) + FwVariable->BufferSize;
	}

	*MaximumVariableStorageSize = TOTAL_STORAGE_SIZE;
	*MaximumVariableSize = MAX_SINGLE_VAR_SIZE;

	if (UsedStorage < TOTAL_STORAGE_SIZE) {
		*RemainingVariableStorageSize = TOTAL_STORAGE_SIZE - UsedStorage;
	}
	else {
		*RemainingVariableStorageSize = 0;
	}

	return EFI_SUCCESS;
}