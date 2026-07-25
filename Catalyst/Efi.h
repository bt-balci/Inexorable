#ifndef _EFI_H_
#define _EFI_H_

#include <Uefi.h>

#define CONFIG_CPU_COUNT	4

extern EFI_SYSTEM_TABLE*		gST;
extern EFI_BOOT_SERVICES*		gBS;
extern EFI_RUNTIME_SERVICES*	gRT;

extern CHAR8* gEfiCallerBaseName;

extern CONST UINT8 g_InexorableEfi[];

extern CONST UINTN g_InexorableEfiSize;

extern UINT64 g_CurrentVirtualAddress;

UINTN
EFIAPI
DebugPrint0402(
    IN CONST CHAR8* Format,
    ...
);

#endif