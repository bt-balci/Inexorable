#pragma once

#include "efi_spec.h"

EFI_STATUS IxLoadPeImage(
    IN  VOID                   *StagingBase,
    OUT EFI_HANDLE             *OutImageHandle,
    OUT EFI_IMAGE_ENTRY_POINT  *OutEntryPoint
);
