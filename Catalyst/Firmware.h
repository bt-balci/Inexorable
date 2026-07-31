#ifndef _FIRMWARE_H_
#define _FIRMWARE_H_
#include "Vm.h"

VOID
EFIAPI
FirmwareGetVariable(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);
#endif