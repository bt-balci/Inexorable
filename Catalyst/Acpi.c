#include <Uefi.h>
#include <IndustryStandard/Acpi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>


#include "Efi.h"
#include "Memory.h"
#include "Firmware.h"

CONST UINT8 MadtAml[] = {
 #embed "Acpi/madt.aml"
};
CONST UINTN MadtAmlSize = sizeof(MadtAml);

/*
	* Maybe put all of these stuff into g_VmState.Firmware?
	* Generates MADT 4 CPUs with x2APIC enabled
*/
EFI_STATUS
EFIAPI
GenerateAcpiTables(
	VOID
) {
	EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER* Rsdp = NULL;
	UINT64 RsdpPtr = ACPI_RSDP_BASE;
	EFI_ACPI_DESCRIPTION_HEADER* XsdtHeader = NULL;
	UINT64 XsdtPtr = ACPI_XSDT_BASE;
	EFI_ACPI_6_1_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER* Madt = NULL;
	UINT64 MadtPtr = ACPI_MADT_BASE;

	//
	// Allocate at fixed addresses.
	//
	EFI_STATUS Status = GuestFindFreePages(AllocateAddress, EFI_SIZE_TO_PAGES(sizeof(*Rsdp)), &RsdpPtr);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("GenerateAcpiTables: FAIL: Cannnot allocate EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER. %r\n", Status);
		return Status;
	}

	Status = GuestFindFreePages(AllocateAddress, EFI_SIZE_TO_PAGES(sizeof(*XsdtHeader)), &XsdtPtr);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("GenerateAcpiTables: FAIL: Cannnot allocate EFI_ACPI_DESCRIPTION_HEADER for XSDT. %r\n", Status);
		return Status;
	}

	Status = GuestFindFreePages(AllocateAddress, EFI_SIZE_TO_PAGES(MadtAmlSize), &MadtPtr);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("GenerateAcpiTables: FAIL: Cannnot allocate EFI_ACPI_DESCRIPTION_HEADER for XSDT. %r\n", Status);
		return Status;
	}

	//
	// Not TranslateGuestAddressToHostAddress, guest didn't start yet and is in identity map.
	//
	if (!TranslateGuestPhysicalToHostPhysical(RsdpPtr, (VOID**)&Rsdp)) {
		DebugPrint0402("GenerateAcpiTables: FAIL: Cannnot translate RsdpPtr to host address\n");
		return Status;
	}

	if (!TranslateGuestPhysicalToHostPhysical(MadtPtr, (VOID**)&Madt)) {
		DebugPrint0402("GenerateAcpiTables: FAIL: Cannnot translate MadtPtr to host address\n");
		return Status;
	}

	if (!TranslateGuestPhysicalToHostPhysical(XsdtPtr, (VOID**)&XsdtHeader)) {
		DebugPrint0402("GenerateAcpiTables: FAIL: Cannnot translate XsdtPtr to host address\n");
		return Status;
	}

	//
	// Acpimaxxing
	//
	Rsdp->Revision = EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION;
	Rsdp->Signature = EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE;
	CopyMem(Rsdp->OemId, "BALCI ", 6);

	Rsdp->RsdtAddress = 0; // No.

	Rsdp->XsdtAddress = XsdtPtr;

	Rsdp->Length = sizeof(*Rsdp);
	Rsdp->Checksum = 0;
	Rsdp->ExtendedChecksum = 0;

	Rsdp->Checksum = CalculateCheckSum8((UINT8*)Rsdp, 20);
	Rsdp->ExtendedChecksum = CalculateCheckSum8((UINT8*)Rsdp, Rsdp->Length);

	XsdtHeader->Revision = EFI_ACPI_6_1_EXTENDED_SYSTEM_DESCRIPTION_TABLE_REVISION;
	XsdtHeader->Signature = EFI_ACPI_6_1_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE;
	CopyMem(XsdtHeader->OemId, "BALCI ", 6);
	CopyMem((UINT8*)XsdtHeader->OemTableId, "BTBTBTBT", 8);
	XsdtHeader->OemRevision = 1;
	XsdtHeader->CreatorRevision = 1;
	XsdtHeader->CreatorId = SIGNATURE_32('B', 'T', 'B', 'T');

	XsdtHeader->Length = sizeof(*XsdtHeader);
	XsdtHeader->Checksum = 0;

	UINT64* XsdtEntries = (UINT64*)((UINT8*)XsdtHeader + XsdtHeader->Length);
	XsdtEntries[0] = MadtPtr;

	XsdtHeader->Length += 8;

	XsdtHeader->Checksum = CalculateCheckSum8((UINT8*)XsdtHeader, XsdtHeader->Length);

	CopyMem(Madt, MadtAml, MadtAmlSize);

	return EFI_SUCCESS;
}