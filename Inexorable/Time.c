#include "efi_spec.h"

EFI_STATUS EFIAPI IxGetTime(
    OUT VOID *Time,
    OUT VOID *Capabilities OPTIONAL
) {
    (void)Time;
    (void)Capabilities;
    return EFI_UNSUPPORTED;
}

EFI_STATUS EFIAPI IxSetTime(
    IN VOID *Time
) {
    (void)Time;
    return EFI_UNSUPPORTED;
}

EFI_STATUS EFIAPI IxGetWakeupTime(
    OUT BOOLEAN *Enabled,
    OUT BOOLEAN *Pending,
    OUT VOID    *Time
) {
    (void)Enabled;
    (void)Pending;
    (void)Time;
    return EFI_UNSUPPORTED;
}

EFI_STATUS EFIAPI IxSetWakeupTime(
    IN BOOLEAN Enable,
    IN VOID    *Time OPTIONAL
) {
    (void)Enable;
    (void)Time;
    return EFI_UNSUPPORTED;
}
