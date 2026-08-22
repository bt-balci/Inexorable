#ifndef _FIRMWARE_H_
#define _FIRMWARE_H_,

#include <Protocol/LoadFile2.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/Rng.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/BlockIo2.h>

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

VOID
EFIAPI
FirmwareLocateProtocol(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);

VOID
EFIAPI
FirmwareHandleProtocol(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);

VOID
EFIAPI
FirmwareExecuteProtocol(
	IN OUT CATALYST_VIRTUAL_CPU* Cpu
);


///////////////////////////////////////////////
////			  Guest Services	  	   ////
///////////////////////////////////////////////

EFI_STATUS
EFIAPI
GuestFindFreePages(
	IN EFI_ALLOCATE_TYPE	AllocationType,
	IN UINTN				Pages,
	IN OUT UINT64* OutPageBase
);

EFI_STATUS
EFIAPI
GuestAllocatePool(
	IN UINTN	Size,
	OUT UINT64* Buffer
);

EFI_STATUS
EFIAPI
GuestFreePages(
	IN UINT64	BasePage,
	IN UINTN	Pages
);

EFI_STATUS
EFIAPI
GuestFreePool(
	IN UINT64 Buffer
);



///////////////////////////////////////////////
////			   Marshalling	  		   ////
///////////////////////////////////////////////

#define MAX_FUNCS_PER_PROTOCOL			8
#define MAX_PTR_FIELDS_PER_PRROTOCOL	16

typedef enum {
	ArgDirectionIn,
	ArgDirectionOut,
	ArgDirectionInOut,
	ArgDirectionConstPtr
} MarshalArgumentDirection;

typedef enum {
	ArgTypeUIntn,
	ArgTypePointer,
	ArgTypeBuffer,
	ArgTypeGuidPtr,
	ArgTypeProtocolPtr,
	ArgTypeStringAlloc,
	ArgTypeEvent,
	ArgTypeHandle
} MarshalArgumentType;

typedef struct {
	MarshalArgumentDirection Direction;
	MarshalArgumentType Type;
	INTN SizeArgIndex;
} FUNC_ARG_DESC;

typedef struct {
	EFI_GUID Guid;
	UINTN Size;
	struct {
		UINTN Count;
		UINTN Offsets[MAX_FUNCS_PER_PROTOCOL];
		UINTN NumArgs[MAX_FUNCS_PER_PROTOCOL];
		CONST FUNC_ARG_DESC* ArgTables[MAX_FUNCS_PER_PROTOCOL];
	} Functions;
	struct {
		UINTN Count;
		UINTN Offsets[MAX_FUNCS_PER_PROTOCOL];
		UINTN Sizes[MAX_FUNCS_PER_PROTOCOL];
	} PtrFields;
} PROTOCOL_REPLICA_INFO;

typedef struct {
	PROTOCOL_REPLICA_INFO Info;
	UINTN FuncIndex;
	VOID* HostFunc;
	VOID* HostThis;
} PROTOCOL_STUB_CONTEXT;

#ifndef FUNC_ARGS_DEFINED
#define FUNC_ARGS_DEFINED
STATIC CONST PROTOCOL_REPLICA_INFO mReplicaTable[] = {

	// ===== EFI_BLOCK_IO2_PROTOCOL =====
	{
		.Guid = EFI_BLOCK_IO2_PROTOCOL_GUID,
		.Size = sizeof(EFI_BLOCK_IO2_PROTOCOL),
		.PtrFields = {
			.Count = 1,
			.Offsets = {
				OFFSET_OF(EFI_BLOCK_IO2_PROTOCOL, Media)
			},
			.Sizes = {
				sizeof(EFI_BLOCK_IO_MEDIA)
			}
		},
		.Functions = {
			.Count = 4,
			.Offsets = {
				OFFSET_OF(EFI_BLOCK_IO2_PROTOCOL, Reset),
				OFFSET_OF(EFI_BLOCK_IO2_PROTOCOL, ReadBlocksEx),
				OFFSET_OF(EFI_BLOCK_IO2_PROTOCOL, WriteBlocksEx),
				OFFSET_OF(EFI_BLOCK_IO2_PROTOCOL, FlushBlocksEx)
			},
			.NumArgs = { 2, 6, 6, 2, 0,0,0,0 },
			.ArgTables = {
				(CONST FUNC_ARG_DESC[]) {          // Reset
					{ ArgDirectionConstPtr, ArgTypePointer, -1 },   // This
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // ExtendedVerification
				},
				(CONST FUNC_ARG_DESC[]) {          // ReadBlocksEx
					{ ArgDirectionConstPtr, ArgTypePointer, -1 },   // This
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // MediaId
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // LBA
					{ ArgDirectionInOut,    ArgTypePointer, -1 },   // Token
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // BufferSize
					{ ArgDirectionOut,       ArgTypeBuffer,   4 },   // Buffer (size = arg 4)
				},
				(CONST FUNC_ARG_DESC[]) {          // WriteBlocksEx
					{ ArgDirectionConstPtr, ArgTypePointer, -1 },   // This
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // MediaId
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // LBA
					{ ArgDirectionInOut,    ArgTypePointer, -1 },   // Token
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // BufferSize
					{ ArgDirectionIn,        ArgTypeBuffer,   4 },   // Buffer
				},
				(CONST FUNC_ARG_DESC[]) {          // FlushBlocksEx
					{ ArgDirectionConstPtr, ArgTypePointer, -1 },   // This
					{ ArgDirectionInOut,    ArgTypePointer, -1 },   // Token
				},
		  }
		},
	},

	// ===== EFI_LOADED_IMAGE_PROTOCOL =====
	{
		.Guid = EFI_LOADED_IMAGE_PROTOCOL_GUID,
		.Size = sizeof(EFI_LOADED_IMAGE_PROTOCOL),
		.PtrFields = {
			.Count = 1,
			.Offsets = {
				OFFSET_OF(EFI_BLOCK_IO2_PROTOCOL, Media)
			},
			.Sizes = {
				sizeof(EFI_BLOCK_IO_MEDIA)
			}
		},
		.Functions = {
			.Count = 1,
			.Offsets = {
				OFFSET_OF(EFI_LOADED_IMAGE_PROTOCOL, Unload)
			},
			.NumArgs = { 1, 0,0,0,0,0,0,0 },
			.ArgTables = {
				(CONST FUNC_ARG_DESC[]) {          // Unload
					{ ArgDirectionConstPtr, ArgTypePointer, -1 },   // This
				},
			}
		},
	},

	// ===== EFI_SIMPLE_FILE_SYSTEM_PROTOCOL =====
	{
		.Guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID,
		.Size = sizeof(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL),
		.Functions = {
			.Count = 1,
			.Offsets = {
				OFFSET_OF(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL, OpenVolume)
			},
			.NumArgs = { 2, 0,0,0,0,0,0,0 },
			.ArgTables = {
				(CONST FUNC_ARG_DESC[]) {          // OpenVolume
					{ ArgDirectionConstPtr, ArgTypePointer,      -1 },  // This
					{ ArgDirectionOut,       ArgTypeProtocolPtr, -1 },  // Root
				},
			}
		},
	},

	// ===== EFI_GRAPHICS_OUTPUT_PROTOCOL =====
	{
		.Guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID,
		.Size = sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL),
		.Functions = {
			.Count = 3,
			.Offsets = {
				OFFSET_OF(EFI_GRAPHICS_OUTPUT_PROTOCOL, QueryMode),
				OFFSET_OF(EFI_GRAPHICS_OUTPUT_PROTOCOL, SetMode),
				OFFSET_OF(EFI_GRAPHICS_OUTPUT_PROTOCOL, Blt)
			},
			.NumArgs = { 4, 2, 10, 0,0,0,0,0 },
			.ArgTables = {
				(CONST FUNC_ARG_DESC[]) {          // QueryMode
					{ ArgDirectionConstPtr, ArgTypePointer, -1 },   // This
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // ModeNumber
					{ ArgDirectionOut,       ArgTypePointer, -1 },   // SizeOfInfo
					{ ArgDirectionOut,       ArgTypeBuffer,   2 },   // Info (size from *arg[2])
				},
				(CONST FUNC_ARG_DESC[]) {          // SetMode
					{ ArgDirectionConstPtr, ArgTypePointer, -1 },   // This
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // ModeNumber
				},
				(CONST FUNC_ARG_DESC[]) {          // Blt
					{ ArgDirectionConstPtr, ArgTypePointer, -1 },   // This
					{ ArgDirectionInOut,    ArgTypeBuffer,  -1 },   // BltBuffer
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // BltOperation
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // SourceX
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // SourceY
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // DestinationX
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // DestinationY
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // Width
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // Height
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // Delta
				},
			}
		},
	},

	// ===== EFI_RNG_PROTOCOL =====
	{
		.Guid = EFI_RNG_PROTOCOL_GUID,
		.Size = sizeof(EFI_RNG_PROTOCOL),
		.Functions = {
			.Count = 2,
			.Offsets = {
				OFFSET_OF(EFI_RNG_PROTOCOL, GetInfo),
				OFFSET_OF(EFI_RNG_PROTOCOL, GetRNG)
			},
			.NumArgs = { 3, 4, 0,0,0,0,0,0 },
			.ArgTables = {
				(CONST FUNC_ARG_DESC[]) {          // GetInfo
					{ ArgDirectionConstPtr, ArgTypePointer, -1 },   // This
					{ ArgDirectionInOut,    ArgTypePointer, -1 },   // RNGAlgorithmListSize
					{ ArgDirectionOut,       ArgTypeBuffer,   1 },   // RNGAlgorithmList
				},
				(CONST FUNC_ARG_DESC[]) {          // GetRNG
					{ ArgDirectionConstPtr, ArgTypePointer, -1 },   // This
					{ ArgDirectionIn,        ArgTypePointer, -1 },   // RNGAlgorithm
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // RNGValueLength
					{ ArgDirectionOut,       ArgTypeBuffer,   2 },   // RNGValue
				},
			}
		},
	},

	// ===== EFI_LOAD_FILE2_PROTOCOL =====
	{
		.Guid = EFI_LOAD_FILE2_PROTOCOL_GUID,
		.Size = sizeof(EFI_LOAD_FILE2_PROTOCOL),
		.Functions = {
			.Count = 1,
			.Offsets = {
				OFFSET_OF(EFI_LOAD_FILE2_PROTOCOL, LoadFile)
			},
			.NumArgs = { 5, 0,0,0,0,0,0,0 },
			.ArgTables = {
				(CONST FUNC_ARG_DESC[]) {          // LoadFile
					{ ArgDirectionConstPtr, ArgTypePointer, -1 },   // This
					{ ArgDirectionIn,        ArgTypePointer, -1 },   // FilePath
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // BootPolicy
					{ ArgDirectionInOut,    ArgTypePointer, -1 },   // BufferSize
					{ ArgDirectionOut,       ArgTypeBuffer,   3 },   // Buffer
				},
			}
		},
	},

	// ===== EFI_DEVICE_PATH_TO_TEXT_PROTOCOL =====
	{
		.Guid = EFI_DEVICE_PATH_TO_TEXT_PROTOCOL_GUID,
		.Size = sizeof(EFI_DEVICE_PATH_TO_TEXT_PROTOCOL),
		.Functions = {
			.Count = 2,
			.Offsets = {
				OFFSET_OF(EFI_DEVICE_PATH_TO_TEXT_PROTOCOL, ConvertDeviceNodeToText),
				OFFSET_OF(EFI_DEVICE_PATH_TO_TEXT_PROTOCOL, ConvertDevicePathToText)
			},
			.NumArgs = { 3, 3, 0,0,0,0,0,0 },
			.ArgTables = {
				(CONST FUNC_ARG_DESC[]) {          // ConvertDeviceNodeToText
					{ ArgDirectionConstPtr, ArgTypePointer, -1 },   // This
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // DisplayOnly
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // AllowShortcuts
				},
				(CONST FUNC_ARG_DESC[]) {          // ConvertDevicePathToText
					{ ArgDirectionConstPtr, ArgTypePointer, -1 },   // This
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // DisplayOnly
					{ ArgDirectionIn,        ArgTypeUIntn,   -1 },   // AllowShortcuts
				},
			}
		},
	},
};
#endif

#endif