#pragma once

#include "efi_spec.h"

BOOLEAN QemuFwCfgDetect(void);
EFI_STATUS QemuFwCfgFetchBootInfo(void);
