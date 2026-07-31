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
	UINTN NameLength	= 0;
	CHAR16* Name		= NULL;
	EFI_GUID VendorGuid = { 0 };
	UINT32 Attributes	= 0;
	UINTN DataSize		= 0;
	VOID* Data			= NULL;
	UINT64 DataPtr		= Cpu->Context.R10;

	//
	// Translate arguments.
	//
	NameLength = StrSizeFromGuest(Cpu->ExtendedContext.Cr3, Cpu->Context.Rcx);
	if (NameLength == MAX_UINTN) {
		Cpu->Context.Rax = EFI_INVALID_PARAMETER;
		return;
	}

	EFI_STATUS Status = gBS->AllocatePool(EfiRuntimeServicesData, NameLength, (VOID**)&Name);
	if (EFI_ERROR(Status)) {
		Cpu->Context.Rax = Status;
		return;
	}

	//
	// These are mandatory.
	//
	if (!CopyFromGuest(Cpu->ExtendedContext.Cr3, Name,			Cpu->Context.Rcx,	NameLength))			goto EndPageFault;
	if (!CopyFromGuest(Cpu->ExtendedContext.Cr3, &VendorGuid,	Cpu->Context.Rdx,	sizeof(VendorGuid)))	goto EndPageFault;
	if (!CopyFromGuest(Cpu->ExtendedContext.Cr3, &DataSize,		Cpu->Context.R9,	sizeof(DataSize)))		goto EndPageFault;

	//
	// Data is OPTIONAL, can be null.
	//
	if (DataSize != 0 && DataPtr != NULL) {
		Status = gBS->AllocatePool(EfiRuntimeServicesData, DataSize, (VOID**)&Data);
		if (EFI_ERROR(Status)) {
			Cpu->Context.Rax = Status;
			goto End;
		}
	}

	//
	// Call.
	//
	Cpu->Context.Rax = gRT->GetVariable(Name, &VendorGuid, &Attributes, &DataSize, Data);

	//
	// We still have to copy DataSize back
	//
	if (!CopyToGuest(Cpu->ExtendedContext.Cr3, &DataSize, Cpu->Context.R9, sizeof(DataSize))) goto EndPageFault;

	//
	// Bail out if failed.
	//
	if (EFI_ERROR(Cpu->Context.Rax)) goto End;

	//
	// Copy OUT arguments back.
	// Attributes is OPTIONAL, can be null. Check for that.
	//
	if (Cpu->Context.R8 != 0) {
		if (!CopyToGuest(Cpu->ExtendedContext.Cr3, &Attributes, Cpu->Context.R8, sizeof(Attributes))) goto EndPageFault;
	}

	//
	// Data is OPTIONAL, can be null.
	//
	if (Cpu->Context.R10 != 0) {
		if (!CopyToGuest(Cpu->ExtendedContext.Cr3, Data, DataPtr, DataSize)) goto EndPageFault;
	}

	goto End;

EndPageFault:
	//
	// Inject #PF into guest.
	//
	PAGE_FAULT_ERROR_CODE Error = {
		.Write = 0
	};
	InjectEvent(Error.All, TRUE, ExceptionInterruptTypePageFault, InterruptionTypeHardwareException, TRUE);

End:
	//
	// Will just return EFI_INVALID_PARAMETER if buffer is invalid. We don't care.
	//
	if (Name != NULL) gBS->FreePool(Name);
	if (Data != NULL) gBS->FreePool(Data);
}