#pragma once

#include "efi_spec.h"

void VariableInit(void);

EFI_STATUS EFIAPI IxGetVariable(
    IN     CHAR16   *VariableName,
    IN     EFI_GUID *VendorGuid,
    OUT    UINT32   *Attributes OPTIONAL,
    IN OUT UINTN    *DataSize,
    OUT    VOID     *Data       OPTIONAL
);

EFI_STATUS EFIAPI IxSetVariable(
    IN CHAR16   *VariableName,
    IN EFI_GUID *VendorGuid,
    IN UINT32   Attributes,
    IN UINTN    DataSize,
    IN VOID     *Data
);

EFI_STATUS EFIAPI IxGetNextVariableName(
    IN OUT UINTN    *VariableNameSize,
    IN OUT CHAR16   *VariableName,
    IN OUT EFI_GUID *VendorGuid
);

EFI_STATUS EFIAPI IxQueryVariableInfo(
    IN  UINT32 Attributes,
    OUT UINT64 *MaximumVariableStorageSize,
    OUT UINT64 *RemainingVariableStorageSize,
    OUT UINT64 *MaximumVariableSize
);
