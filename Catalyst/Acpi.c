#include <Uefi.h>
#include <IndustryStandard/Acpi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>


#include "Efi.h"
#include "Memory.h"
#include "Firmware.h"

/*
	* Maybe put all of these stuff into g_VmState.Firmware?
	* Replicates MADT 4 CPUs with x2APIC enabled
*/
EFI_STATUS
EFIAPI
ReplicateAcpiTables(
	OUT EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER* Rsdp
) {
	EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER* Rsdp = NULL;
	EFI_ACPI_6_1_FIXED_ACPI_DESCRIPTION_TABLE* Fadt = NULL;
	EFI_ACPI_DESCRIPTION_HEADER* XsdtHeader = NULL;

	EFI_STATUS Status = gBS->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, EFI_SIZE_TO_PAGES(sizeof(EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER)),
		(EFI_PHYSICAL_ADDRESS*)&Rsdp);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("ReplicateAcpiTables: FAIL: Cannnot allocate EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER\n");
		return Status;
	}

	Status = gBS->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, EFI_SIZE_TO_PAGES(sizeof(EFI_ACPI_6_1_FIXED_ACPI_DESCRIPTION_TABLE)),
		(EFI_PHYSICAL_ADDRESS*)&Fadt);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("ReplicateAcpiTables: FAIL: Cannnot allocate EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER\n");
		return Status;
	}

	//
	// +8 for FADT
	//
	Status = gBS->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, EFI_SIZE_TO_PAGES(sizeof(EFI_ACPI_DESCRIPTION_HEADER) + 8),
		(EFI_PHYSICAL_ADDRESS*)&XsdtHeader);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("ReplicateAcpiTables: FAIL: Cannnot allocate EFI_ACPI_DESCRIPTION_HEADER (for XSDT)\n");
		return Status;
	}

	//
	// Acpimaxxing
	//

	Rsdp->Revision = EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION;
	Rsdp->Signature = EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE;
	CopyMem(Rsdp->OemId, "BALCI ", 6);


	Rsdp->XsdtAddress = 0;
	Rsdp->RsdtAddress = 0; // No.

	Rsdp->Length = sizeof(*Rsdp);
	Rsdp->Checksum = 0;

	XsdtHeader->Revision = EFI_ACPI_6_1_EXTENDED_SYSTEM_DESCRIPTION_TABLE_REVISION;
	XsdtHeader->Signature = EFI_ACPI_6_1_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE;
	CopyMem(XsdtHeader->OemId, "BALCI ", 6);
	XsdtHeader->OemRevision = 1;
	XsdtHeader->CreatorRevision = 1;
	XsdtHeader->CreatorId = SIGNATURE_32('B', 'T', 'B', 'T');

	XsdtHeader->Length = sizeof(*XsdtHeader);
	XsdtHeader->Checksum = 0;

	Fadt->Header.Revision = EFI_ACPI_6_1_FIXED_ACPI_DESCRIPTION_TABLE_REVISION;
	Fadt->Header.Signature = EFI_ACPI_6_1_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE;
	CopyMem(Fadt->Header.OemId, "BALCI ", 6);
	CopyMem(&Fadt->Header.OemTableId, "BALCIFA", 8);
	Fadt->Header.OemRevision = 1;
	Fadt->Header.CreatorRevision = 1;
	Fadt->Header.CreatorId = SIGNATURE_32('B', 'T', 'B', 'T');

	Fadt->Header.Length = sizeof(*Fadt);
	Fadt->Header.Checksum = 0;


	return EFI_SUCCESS;
}