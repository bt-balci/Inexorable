#pragma once
#include <Uefi.h>

///////////////////////////////////////////////
////				Structs				   ////
///////////////////////////////////////////////

typedef struct {
	LIST_ENTRY Link;
	EFI_GUID Guid;
	CHAR16 Name[256];
	VOID* Buffer;
	UINTN BufferSize;
	UINT32 Attributes;
} INEXORABLE_FIRMWARE_VARIABLE;

typedef struct {
	LIST_ENTRY Link;
	EFI_HANDLE Value;
	LIST_ENTRY InstalledProtocols;
} INEXORABLE_FIRMWARE_HANDLE;

typedef struct {
	LIST_ENTRY Link;
	EFI_GUID Guid;
	VOID* Pointer;
} INEXORABLE_FIRMWARE_PROTOCOL;