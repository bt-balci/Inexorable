#include "efi_spec.h"
#include "inexorable_layout.h"

extern EFI_SYSTEM_TABLE     gST;
extern EFI_RUNTIME_SERVICES gRT;

EFI_STATUS EFIAPI IxSetVirtualAddressMap(
    IN UINTN                 MemoryMapSize,
    IN UINTN                 DescriptorSize,
    IN UINT32                DescriptorVersion,
    IN EFI_MEMORY_DESCRIPTOR *VirtualMap
) {
    (void)DescriptorVersion;
    if (VirtualMap == NULL || DescriptorSize < sizeof(EFI_MEMORY_DESCRIPTOR)) {
        return EFI_INVALID_PARAMETER;
    }

    UINTN Count = MemoryMapSize / DescriptorSize;
    INT64 Delta = 0;

    for (UINTN i = 0; i < Count; i++) {
        EFI_MEMORY_DESCRIPTOR *Desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)VirtualMap + i * DescriptorSize);
        if (Desc->PhysicalStart == GUEST_FIRMWARE_IMAGE_BASE) {
            Delta = (INT64)Desc->VirtualStart - (INT64)Desc->PhysicalStart;
            break;
        }
    }

    if (Delta != 0) {
        if (gRT.GetTime)             gRT.GetTime             = (VOID *)((uintptr_t)gRT.GetTime + Delta);
        if (gRT.SetTime)             gRT.SetTime             = (VOID *)((uintptr_t)gRT.SetTime + Delta);
        if (gRT.GetWakeupTime)       gRT.GetWakeupTime       = (VOID *)((uintptr_t)gRT.GetWakeupTime + Delta);
        if (gRT.SetWakeupTime)       gRT.SetWakeupTime       = (VOID *)((uintptr_t)gRT.SetWakeupTime + Delta);
        if (gRT.SetVirtualAddressMap) gRT.SetVirtualAddressMap = (VOID *)((uintptr_t)gRT.SetVirtualAddressMap + Delta);
        if (gRT.ConvertPointer)      gRT.ConvertPointer      = (VOID *)((uintptr_t)gRT.ConvertPointer + Delta);
        if (gRT.GetVariable)         gRT.GetVariable         = (VOID *)((uintptr_t)gRT.GetVariable + Delta);
        if (gRT.GetNextVariableName) gRT.GetNextVariableName = (VOID *)((uintptr_t)gRT.GetNextVariableName + Delta);
        if (gRT.SetVariable)         gRT.SetVariable         = (VOID *)((uintptr_t)gRT.SetVariable + Delta);
        if (gRT.ResetSystem)         gRT.ResetSystem         = (VOID *)((uintptr_t)gRT.ResetSystem + Delta);
        if (gRT.QueryVariableInfo)   gRT.QueryVariableInfo   = (VOID *)((uintptr_t)gRT.QueryVariableInfo + Delta);

        gST.RuntimeServices = (EFI_RUNTIME_SERVICES *)((uintptr_t)gST.RuntimeServices + Delta);
    }

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI IxConvertPointer(
    IN     UINTN DebugDisposition,
    IN OUT VOID  **Address
) {
    (void)DebugDisposition;
    (void)Address;
    return EFI_SUCCESS;
}
