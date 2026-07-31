#include <Uefi.h>
#include "Vm.h"
#include "x86.h"

EFI_STATUS
EFIAPI
IxGetVariable(
	IN     CHAR16*		VariableName,
	IN     EFI_GUID*	VendorGuid,
	OUT    UINT32*		Attributes     OPTIONAL,
	IN OUT UINTN*		DataSize,
	OUT    VOID*		Data           OPTIONAL
) {
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_GET_VARIABLE, (UINT64)VariableName, (UINT64)VendorGuid, (UINT64)Attributes, (UINT64)DataSize, (UINT64)Data, NULL);
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
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_SET_VARIABLE, (UINT64)VariableName, (UINT64)VendorGuid, Attributes, DataSize, (UINT64)Data, NULL);
}

EFI_STATUS
EFIAPI
IxGetNextVariableName(
	IN OUT UINTN* VariableNameSize,
	IN OUT CHAR16* VariableName,
	IN OUT EFI_GUID* VendorGuid
) {
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_GET_NEXT_VARIABLE_NAME, (UINT64)VariableNameSize, (UINT64)VariableName, (UINT64)VendorGuid, NULL, NULL, NULL);
}

EFI_STATUS
EFIAPI
IxQueryVariableInfo(
	IN  UINT32	Attributes,
	OUT UINT64* MaximumVariableStorageSize,
	OUT UINT64* RemainingVariableStorageSize,
	OUT UINT64* MaximumVariableSize
) {
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_QUERY_VARIABLE_INFO, (UINT64)Attributes, (UINT64)MaximumVariableStorageSize, (UINT64)RemainingVariableStorageSize, (UINT64)MaximumVariableSize, NULL, NULL);
}