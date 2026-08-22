#ifndef _EFI_H_
#define _EFI_H_
#include <Uefi.h>

extern EFI_SYSTEM_TABLE			gST;
extern EFI_BOOT_SERVICES		gBS;
extern EFI_RUNTIME_SERVICES		gRT;

extern LIST_ENTRY g_ProtocolList;
extern LIST_ENTRY g_HandleList;
extern LIST_ENTRY g_VariableList;
#endif