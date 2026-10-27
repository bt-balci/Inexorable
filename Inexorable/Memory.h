#pragma once

#include "efi_spec.h"

extern UINTN g_MemoryMapKey;

void MemoryInit(void);

EFI_STATUS EFIAPI IxAllocatePages(
    IN     EFI_ALLOCATE_TYPE    Type,
    IN     EFI_MEMORY_TYPE      MemoryType,
    IN     UINTN                Pages,
    IN OUT EFI_PHYSICAL_ADDRESS *Memory
);

EFI_STATUS EFIAPI IxFreePages(
    IN EFI_PHYSICAL_ADDRESS Memory,
    IN UINTN                Pages
);

EFI_STATUS EFIAPI IxAllocatePool(
    IN  EFI_MEMORY_TYPE PoolType,
    IN  UINTN           Size,
    OUT VOID            **Buffer
);

EFI_STATUS EFIAPI IxFreePool(
    IN VOID *Buffer
);

EFI_STATUS EFIAPI IxGetMemoryMap(
    IN OUT UINTN                 *MemoryMapSize,
    OUT    EFI_MEMORY_DESCRIPTOR *MemoryMap,
    OUT    UINTN                 *MapKey,
    OUT    UINTN                 *DescriptorSize,
    OUT    UINT32                *DescriptorVersion
);
