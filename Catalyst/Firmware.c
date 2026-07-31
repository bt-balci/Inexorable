#include <Uefi.h>
#include <Library/BaseLib.h>

#include "Efi.h"
#include "Memory.h"
#include "Vm.h"

VOID
EFIAPI
FirmwareGetVariable(
	IN OUT CATALYST_VIRTUAL_CPU*	Cpu
) {
	//
	// Arguments.
	//
	CHAR16 Name[256]	= { 0 };
	EFI_GUID VendorGuid = { 0 };
	UINT32 Attributes	= 0;
	UINTN DataSize		= 0;
	VOID* Data			= NULL;

	//
	// Copy from guest.
	//
	if (!CopyFromGuest(Cpu->ExtendedContext.Cr3, Name,			Cpu->Context.Rcx,	256))					return;
	if (!CopyFromGuest(Cpu->ExtendedContext.Cr3, &VendorGuid,	Cpu->Context.Rdx,	sizeof(VendorGuid)))	return;
	if (!CopyFromGuest(Cpu->ExtendedContext.Cr3, &Attributes,	Cpu->Context.R8,	sizeof(Attributes)))	return;
	if (!CopyFromGuest(Cpu->ExtendedContext.Cr3, &DataSize,		Cpu->Context.R9,	sizeof(DataSize)))		return;
	if (!CopyFromGuest(Cpu->ExtendedContext.Cr3, Data,			Cpu->Context.R10,	DataSize))				return;

	//
	// Call.
	//
	Cpu->Context.Rax = gRT->GetVariable(Name, &VendorGuid, &Attributes, &DataSize, Data);

	//
	// Copy arguments back.
	//
	if (!EFI_ERROR(Cpu->Context.Rax)) {
		if (!CopyToGuest(Cpu->ExtendedContext.Cr3, Name,		Cpu->Context.Rcx,	256))					return;
		if (!CopyToGuest(Cpu->ExtendedContext.Cr3, &VendorGuid, Cpu->Context.Rdx,	sizeof(VendorGuid)))	return;
		if (!CopyToGuest(Cpu->ExtendedContext.Cr3, &Attributes, Cpu->Context.R8,	sizeof(Attributes)))	return;
		if (!CopyToGuest(Cpu->ExtendedContext.Cr3, &DataSize,	Cpu->Context.R9,	sizeof(DataSize)))		return;
		if (!CopyToGuest(Cpu->ExtendedContext.Cr3, Data,		Cpu->Context.R10,	DataSize))				return;
	}
}