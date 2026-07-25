#ifndef _MAIN_C_
#define _MAIN_C_

#include <Uefi.h>
#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Protocol/MpService.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

#include "Efi.h"
#include "x86.h"
#include "Vm.h"
#include "Memory.h"

//
// Inclusion skill issue.
//
EFI_SYSTEM_TABLE* gST = NULL;
EFI_BOOT_SERVICES* gBS = NULL;
EFI_RUNTIME_SERVICES* gRT = NULL;
CHAR8* gEfiCallerBaseName = "BT-BALCI";
CONST UINT8 g_InexorableEfi[] = {
 #embed "../x64/Debug/inexorable.efi"
};
CONST UINTN g_InexorableEfiSize = sizeof(g_InexorableEfi);
UINT64 g_CurrentVirtualAddress;

CATALYST_VIRTUAL_CPU VirtualProcessors[CONFIG_CPU_COUNT] = {0};

STATIC
EFI_STATUS
EFIAPI
PrepareProcessors(
	VOID
) {
	EFI_MP_SERVICES_PROTOCOL* MpServices = NULL;

	EFI_STATUS Status = gBS->LocateProtocol(
		&gEfiMpServiceProtocolGuid,
		NULL,
		(VOID**)&MpServices
	);

	//
	//	EFI_MP_SERVICES is used to wakeup APs.
	//
	ASSERT(Status == EFI_SUCCESS);

	UINTN TotalProcessors = 0;
	UINTN EnabledProcessors = 0;
	Status = MpServices->GetNumberOfProcessors(MpServices, &TotalProcessors, &EnabledProcessors);

	//
	// This should not fail.
	//
	ASSERT(Status == EFI_SUCCESS);

	//
	// The number of CPUs is statistically allocated.
	//
	ASSERT(EnabledProcessors == CONFIG_CPU_COUNT);

	DebugPrint0402("[CATALYST] Total of %d processors, %d enabled\n", TotalProcessors, EnabledProcessors);

	//
	// Fill each processor.
	//
	for (UINT32 i = 0; i < CONFIG_CPU_COUNT; i++) {
		CATALYST_VIRTUAL_CPU* Processor = &VirtualProcessors[i];
		Processor->Index = i;
	}

	//
	// Index = 1 to skip the BSP (current processor).
	//
	for (UINTN i = 1; i < CONFIG_CPU_COUNT; i++) {
		CATALYST_VIRTUAL_CPU* Processor = &VirtualProcessors[i];

		DebugPrint0402("[CATALYST] Starting AP %d...\n", i);

		//
		// Start in blocking mode. We will wait until each one of them finishes.
		//
		Status = MpServices->StartupThisAP(MpServices, PrepareProcessor, i, NULL, 0, (VOID*)Processor, NULL);
		if (EFI_ERROR(Status)) {
			DebugPrint0402("[CATALYST] Failed starting AP %d. Error: %r\n", i, Status);
			return Status;
		}

		DebugPrint0402("[CATALYST] AP %d started!\n", i);
	}

	DebugPrint0402("[CATALYST] Preparing BSP...\n");

	PrepareProcessor(&VirtualProcessors[0]);

	DebugPrint0402("[CATALYST] BSP initialized. We are ready.\n");

	return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MapMemory(
	VOID
) {
	UINTN	MapSize				= 0;
	UINTN	MapKey				= 0;
	UINT32	DescriptorVersion	= 0;
	UINTN	DescriptorSize		= 0;

	EFI_STATUS Status = gBS->GetMemoryMap(&MapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);
	if (Status != EFI_BUFFER_TOO_SMALL) {
		DebugPrint0402("[CATALYST] Failed to get memory map size: %r\n", Status);
		return Status;
	}

	//
	// +10 to make sure
	//
	MapSize += DescriptorSize * 10;
	EFI_MEMORY_DESCRIPTOR* DescriptorArray = 0;

	Status = gBS->AllocatePool(EfiBootServicesData, MapSize, (VOID**)&DescriptorArray);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("[CATALYST] Failed to allocate for memory map: %r\n", Status);
		return Status;
	}

	Status = gBS->GetMemoryMap(&MapSize, DescriptorArray, &MapKey, &DescriptorSize, &DescriptorVersion);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("[CATALYST] Failed to get memory map: %r\n", Status);
		goto End;
	}

	UINTN NumEntries = MapSize / DescriptorSize;
	EFI_MEMORY_DESCRIPTOR* Desc = DescriptorArray;

	//
	// Initialize globals.
	//
	g_Eptp.DirtyAndAccessEnabled = FALSE;
	g_Eptp.PageWalkLength = 3;
	g_Eptp.MemoryType = 6;
	g_Eptp.Pml4Address = ((UINT64)&g_Epml4) >> 12;

	DebugPrint0402("[CATALYST] g_Eptp = 0x%p\n", g_Eptp);

	DebugPrint0402("[CATALYST] Mapping out free memory...\n");

	//
	// We will map everything from 0. As if there is contiguous, linear memory for the guest.
	//
	for (UINTN i = 0; i < NumEntries; i++) {
		//
		// Exclude everything except EfiConventionalMemory. We don't want to bork ourselves.
		//
		if (Desc->Type == EfiConventionalMemory /*||
			Desc->Type == EfiBootServicesCode ||
			Desc->Type == EfiBootServicesData ||
			Desc->Type == EfiLoaderCode ||
			Desc->Type == EfiLoaderData*/)
		{
			UINT64 PhysicalStart	= Desc->PhysicalStart;
			UINT64 NumberOfPages	= Desc->NumberOfPages;
			
			DebugPrint0402("[CATALYST] Found memory at %p for %d pages. Mapping to %p\n", PhysicalStart, NumberOfPages, g_CurrentVirtualAddress);
			
			MapPagesLinear(PhysicalStart, NumberOfPages, &g_CurrentVirtualAddress);

			DebugPrint0402("[CATALYST] Mapped %d pages, new cursor: %p\n", NumberOfPages, g_CurrentVirtualAddress);
		}

		Desc = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)Desc + DescriptorSize);
	}

End:
	gBS->FreePool(DescriptorArray);
	return Status;
}

EFI_STATUS
EFIAPI
UefiMain(
	IN EFI_HANDLE			ImageHandle,
	IN EFI_SYSTEM_TABLE* SystemTable
) {
	gST = SystemTable;
	gBS = SystemTable->BootServices;
	gRT = SystemTable->RuntimeServices;

	DebugPrint0402("[CATALYST] Welcome to Catalyst - Inexorable Hypervisor\n");

	//
	// Construct page tables for guest.
	//
	EFI_STATUS Status = MapMemory();
	if (EFI_ERROR(Status)) {
		DebugPrint0402("[CATALYST] Failed to map memory: %r\n", Status);
		return Status;
	}

	//
	// VMX support must be present.
	//
	ASSERT(IsVmxSupported());

	//
	// VMX must be enabled.
	// If the VMX is enabled but BIOS didn't set the lock bit, then screw your BIOS (or OVMF for this context. No don't screw OVMF. It's cool).
	//
	ASSERT(IsVmxEnabled());

	//
	// By the golden rule of x86, all logical processors of the same socket must be same. If one logical processor supports VMX, so does the others.
	// This "golden rule" that I just made up might not be reliable when there are more physical cpus that are different, and are in different sockets.
	// But this code will work for our demonstration purposes.
	// Neither way, I don't care. Neither do you.
	//

	DebugPrint0402("[CATALYST] Initializing APs...\n");

	//
	// All processors must be woken up and ready.
	//
	Status = PrepareProcessors();
	if (EFI_ERROR(Status)) {
		DebugPrint0402("[CATALYST] Failed to prepare processors: %r\n", Status);
		return Status;
	}

	return EFI_SUCCESS;
}

#endif