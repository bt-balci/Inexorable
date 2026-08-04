#ifndef _FIRMWARE_H_
#define _FIRMWARE_H_,

#include <Protocol/LoadFile2.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/Rng.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

#include "Vm.h"

///////////////////////////////////////////////
////		     Runtime Services 		   ////
///////////////////////////////////////////////

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

///////////////////////////////////////////////
////		      Boot Services 		   ////
///////////////////////////////////////////////

VOID
EFIAPI
FirmwareExitBootServices(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);

VOID
EFIAPI
FirmwareFreePool(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);

VOID
EFIAPI
FirmwareAllocatePool(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);

VOID
EFIAPI
FirmwareFreePages(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);

VOID
EFIAPI
FirmwareAllocatePages(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);

VOID
EFIAPI
FirmwareGetMemoryMap(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);

///////////////////////////////////////////////
////				X-Macros	  		   ////
///////////////////////////////////////////////

#define MAX_FUNCS_PER_PROTOCOL 8

typedef struct {
	EFI_GUID* Guid;
	UINTN Size;
	UINTN NumFuncs;
	UINTN Offsets[MAX_FUNCS_PER_PROTOCOL];
} PROTOCOL_REPLICA_INFO;

#define FOR_EACH_PROTOCOL_FUNCTION(PROTOCOL_FUNC, FUNCTION)                              \
	PROTOCOL_FUNC(EFI_LOADED_IMAGE_PROTOCOL, EFI_LOADED_IMAGE_PROTOCOL,                  \
				  gEfiLoadedImageProtocolGuid,                                            \
		FUNCTION(Unload, OFFSET_OF(EFI_LOADED_IMAGE_PROTOCOL, Unload))                   \
	)                                                                                    \
	PROTOCOL_FUNC(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL, EFI_SIMPLE_FILE_SYSTEM_PROTOCOL,      \
				  gEfiSimpleFileSystemProtocolGuid,                                      \
		FUNCTION(OpenVolume, OFFSET_OF(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL, OpenVolume))     \
	)                                                                                    \
	PROTOCOL_FUNC(EFI_GRAPHICS_OUTPUT_PROTOCOL, EFI_GRAPHICS_OUTPUT_PROTOCOL,            \
				  gEfiGraphicsOutputProtocolGuid,                                        \
		FUNCTION(QueryMode, OFFSET_OF(EFI_GRAPHICS_OUTPUT_PROTOCOL, QueryMode))          \
		FUNCTION(SetMode,   OFFSET_OF(EFI_GRAPHICS_OUTPUT_PROTOCOL, SetMode))            \
		FUNCTION(Blt,       OFFSET_OF(EFI_GRAPHICS_OUTPUT_PROTOCOL, Blt))                \
	)                                                                                    \
	PROTOCOL_FUNC(EFI_RNG_PROTOCOL, EFI_RNG_PROTOCOL,                                    \
				  gEfiRngProtocolGuid,                                                   \
		FUNCTION(GetInfo, OFFSET_OF(EFI_RNG_PROTOCOL, GetInfo))                          \
		FUNCTION(GetRNG,  OFFSET_OF(EFI_RNG_PROTOCOL, GetRNG))                           \
	)                                                                                    \
	PROTOCOL_FUNC(EFI_LOAD_FILE2_PROTOCOL, EFI_LOAD_FILE2_PROTOCOL,                      \
				  gEfiLoadFile2ProtocolGuid,                                             \
		FUNCTION(LoadFile, OFFSET_OF(EFI_LOAD_FILE2_PROTOCOL, LoadFile))                 \
	)                                                                                    \
	PROTOCOL_FUNC(EFI_DEVICE_PATH_TO_TEXT_PROTOCOL, EFI_DEVICE_PATH_TO_TEXT_PROTOCOL,    \
				  gEfiDevicePathToTextProtocolGuid,                                      \
		FUNCTION(ConvertDeviceNodeToText,                                                \
				 OFFSET_OF(EFI_DEVICE_PATH_TO_TEXT_PROTOCOL, ConvertDeviceNodeToText))   \
		FUNCTION(ConvertDevicePathToText,                                                \
				 OFFSET_OF(EFI_DEVICE_PATH_TO_TEXT_PROTOCOL, ConvertDevicePathToText))   \
	)

#endif