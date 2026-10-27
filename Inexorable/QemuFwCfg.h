#pragma once

#include "efi_spec.h"

#define FW_CFG_INITRD_DATA      0x0012

BOOLEAN QemuFwCfgDetect(void);
EFI_STATUS QemuFwCfgFetchBootInfo(void);
BOOLEAN QemuFwCfgDmaRead(UINT16 key, VOID *dest, UINT32 length);
