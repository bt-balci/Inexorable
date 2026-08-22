#ifndef _EFI_H_
#define _EFI_H_

#include <Uefi.h>
#include <Protocol/Rng.h>
#include <Protocol/LoadFile2.h>

#ifndef EXTERNAL_PROTOCOLS_DEFINED
#define EXTERNAL_PROTOCOLS_DEFINED
extern GUID gEfiRngProtocolGuid;
extern GUID gEfiLoadFile2ProtocolGuid;
extern GUID gEfiBlockIo2ProtocolGuid;
#endif

//
// Cringe.
//
#ifndef TABLES_DEFINED
#define TABLES_DEFINED
extern EFI_SYSTEM_TABLE*		gST;
extern EFI_BOOT_SERVICES*		gBS;
extern EFI_RUNTIME_SERVICES*	gRT;
#endif

extern CHAR8* gEfiCallerBaseName;

extern CONST UINT8 g_InexorableEfi[];

extern CONST UINTN g_InexorableEfiSize;

extern UINT64 g_CurrentGuestPhysicalAddress;

UINTN
EFIAPI
DebugPrint0402(
    IN CONST CHAR8* Format,
    ...
);

#endif