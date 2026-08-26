#include <Uefi.h>
#include <IndustryStandard/Acpi.h>
#include <Guid/Acpi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>


#include "Efi.h"
#include "Memory.h"
#include "Firmware.h"

GUID gEfiAcpi20TableGuid = EFI_ACPI_20_TABLE_GUID;

CONST UINT8 MadtAml[] = {
 #embed "Acpi/madt.aml"
};
CONST UINTN MadtAmlSize = sizeof(MadtAml);


EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER*
EFIAPI
FindAcpiRsdp(
	VOID
) {
	EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER* AcpiTable = NULL;;
	for (UINTN i = 0; i < gST->NumberOfTableEntries; i++) {
		if (CompareGuid(&gST->ConfigurationTable[i].VendorGuid, &gEfiAcpi20TableGuid)) {
			AcpiTable = (EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER*)gST->ConfigurationTable[i].VendorTable;
			break;
		}
	}

	if (AcpiTable == NULL) {
		DebugPrint0402("FindAcpiRsdp: Cannot find RSDP\n");
	}

	return AcpiTable;
}

VOID*
EFIAPI
FindAcpiTable(
	IN UINT32	Signature
) {
	EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER* Rsdp = FindAcpiRsdp();
	EFI_ACPI_DESCRIPTION_HEADER* Xsdt = (EFI_ACPI_DESCRIPTION_HEADER*)Rsdp->XsdtAddress;

	UINTN TableCount = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) / 8;
	UINT64* TableArray = (UINT64*)((UINT8*)Xsdt + Xsdt->Length);
	for (UINTN i = 0; i < TableCount; i++) {
		EFI_ACPI_DESCRIPTION_HEADER* Table = (EFI_ACPI_DESCRIPTION_HEADER*)TableArray[i];
		if (CompareMem(&Table->Signature, &Signature, sizeof(Signature))) {
			return Table;
		}
	}

	DebugPrint0402("FindAcpiTable: Cannot find table with signature 0x%p\n", Signature);

	return NULL;
}

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
	EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE* Fadt = NULL;
	UINT64 FadtPtr = ACPI_FADT_BASE;

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

	Status = GuestFindFreePages(AllocateAddress, EFI_SIZE_TO_PAGES(sizeof(*Fadt)), &FadtPtr);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("GenerateAcpiTables: FAIL: Cannnot allocate EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE for FADT. %r\n", Status);
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

	if (!TranslateGuestPhysicalToHostPhysical(FadtPtr, (VOID**)&Fadt)) {
		DebugPrint0402("GenerateAcpiTables: FAIL: Cannnot translate XsdtPtr to host address\n");
		return Status;
	}

	//
	// Acpimaxxing
	//
	Rsdp->Revision = EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION;
	Rsdp->Signature = EFI_ACPI_6_1_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE;
	CopyMem(Rsdp->OemId, "BALCI ", 6);

	//
	// Since guest has less than 4GB of ram, it is GUARANTEED that this pointer will fit for XSDT and RSDT since both are technically same (except ptr size).
	//
	Rsdp->RsdtAddress = XsdtPtr;
	Rsdp->XsdtAddress = XsdtPtr;

	Rsdp->Length = sizeof(*Rsdp);
	Rsdp->Checksum = 0;
	Rsdp->ExtendedChecksum = 0;

	Rsdp->Checksum = CalculateCheckSum8((UINT8*)Rsdp, 20);
	Rsdp->ExtendedChecksum = CalculateCheckSum8((UINT8*)Rsdp, Rsdp->Length);

	Fadt->Header.Revision = EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE_REVISION;
	Fadt->Header.Signature = EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE;
	CopyMem(Fadt->Header.OemId, "BALCI ", 6);
	CopyMem((UINT8*)Fadt->Header.OemTableId, "BTBTBTBT", 8);
	Fadt->Header.OemRevision = 1;
	Fadt->Header.CreatorRevision = 1;
	Fadt->Header.CreatorId = SIGNATURE_32('B', 'T', 'B', 'T');

	Fadt->Header.Length = sizeof(*Fadt);
	Fadt->Header.Checksum = 0;

	//
	// Q35 doesn't support PM timer.
	//
	/*
		EFI_ACPI_6_1_FIXED_ACPI_DESCRIPTION_TABLE* HostFadt = FindAcpiTable(EFI_ACPI_6_1_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE);
		Fadt->PmTmrBlk = HostFadt->PmTmrBlk;
		Fadt->PmTmLen = HostFadt->PmTmrLen;
	*/

	Fadt->PmTmrBlk = 1;
	Fadt->PmTmLen = 4;

	DebugPrint0402("GenerateAcpiTables: PmTmrBlk = %d, PmTmLen = %d\n", Fadt->PmTmLen, Fadt->PmTmrBlk);

	Fadt->Header.Checksum = CalculateCheckSum8((UINT8*)Fadt, Fadt->Header.Length);
	DebugPrint0402("GenerateAcpiTables FADT: Signature = 0x%p, Length = %d, Revision = %d, Checksum = %d\n",
		Fadt->Header.Signature, Fadt->Header.Length, Fadt->Header.Revision, Fadt->Header.Checksum);

	XsdtHeader->Revision = EFI_ACPI_6_1_EXTENDED_SYSTEM_DESCRIPTION_TABLE_REVISION;
	XsdtHeader->Signature = EFI_ACPI_6_1_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE;
	CopyMem(XsdtHeader->OemId, "BALCI ", 6);
	CopyMem((UINT8*)XsdtHeader->OemTableId, "BTBTBTBT", 8);
	XsdtHeader->OemRevision = 1;
	XsdtHeader->CreatorRevision = 1;
	XsdtHeader->CreatorId = SIGNATURE_32('B', 'T', 'B', 'T');

	//
	// Should be 64.
	//
	UINT32* XsdtEntries = (UINT32*)((UINT8*)XsdtHeader + sizeof(*XsdtHeader));
	XsdtEntries[0] = (UINT32)MadtPtr;
	XsdtEntries[1] = (UINT32)FadtPtr;

	XsdtHeader->Length += (8 * 2) + sizeof(*XsdtHeader);

	XsdtHeader->Checksum = CalculateCheckSum8((UINT8*)XsdtHeader, XsdtHeader->Length);

	DebugPrint0402("GenerateAcpiTables XSDT (RSDT): Signature = 0x%p, Length = %d, Revision = %d, Checksum = %d\n",
		XsdtHeader->Signature, XsdtHeader->Length, XsdtHeader->Revision, XsdtHeader->Checksum);

	CopyMem(Madt, MadtAml, MadtAmlSize);

	return EFI_SUCCESS;
}