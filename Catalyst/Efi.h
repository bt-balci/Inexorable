#pragma once
#include <Uefi.h>

#define CONFIG_CPU_COUNT 4

EFI_SYSTEM_TABLE*		gST = NULL;
EFI_BOOT_SERVICES*		gBS = NULL;
EFI_RUNTIME_SERVICES*	gRT = NULL;

CHAR8* gEfiCallerBaseName = "BT-BALCI";