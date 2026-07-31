#pragma once

#include <Uefi.h>
EFI_STATUS
EFIAPI
IxGetTime(
	OUT  EFI_TIME* Time,
	OUT  EFI_TIME_CAPABILITIES* Capabilities OPTIONAL
);

EFI_STATUS
EFIAPI
IxSetTime(
	OUT  EFI_TIME* Time
);

EFI_STATUS
EFIAPI
IxGetWakeupTime(
	OUT BOOLEAN* Enabled,
	OUT BOOLEAN* Pending,
	OUT EFI_TIME* Time
);

EFI_STATUS
EFIAPI
IxSetWakeupTime(
	IN  BOOLEAN	Enable,
	IN  EFI_TIME* Time   OPTIONAL
);