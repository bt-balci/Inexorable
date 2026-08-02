#ifndef _FIRMWARE_H_
#define _FIRMWARE_H_
#include "Vm.h"

VOID
EFIAPI
FirmwareGetVariable(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);

VOID
EFIAPI
FirmwareSetVariable(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);

VOID
EFIAPI
FirmwareGetNextVariableName(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);

VOID
EFIAPI
FirmwareQueryVariableInfo(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);

VOID
EFIAPI
FirmwareGetTime(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);

VOID
EFIAPI
FirmwareSetTime(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);

VOID
EFIAPI
FirmwareGetWakeupTime(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);

VOID
EFIAPI
FirmwareSetWakeupTime(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);

VOID
EFIAPI
FirmwareGetNextHighMonotonicCount(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);


VOID
EFIAPI
FirmwareExitBootServices(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
)
#endif