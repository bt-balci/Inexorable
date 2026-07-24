#include <Uefi.h>
#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Protocol/MpService.h>

#include "Efi.h"
#include "x86.h"
#include "Vm.h"

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

	DebugPrint(DEBUG_VERBOSE, "[CATALYST] Total of %ull processors, %ull enabled\n", TotalProcessors, EnabledProcessors);

	//
	// Index = 1 to skip the BSP (current processor).
	//
	for (UINT32 Index = 1; Index < CONFIG_CPU_COUNT; Index++) {
		CATALYST_VIRTUAL_CPU* Processor = &VirtualProcessors[Index];
		Processor->Index = Index;

		DebugPrint(DEBUG_VERBOSE, "[CATALYST] Starting AP %d...\n", Index);

		//
		// Start in blocking mode. We will wait until each one of them finishes.
		//
		Status = MpServices->StartupThisAP(MpServices, PrepareProcessor, Index, NULL, 0, (VOID*)Processor, NULL);
		if (EFI_ERROR(Status)) {
			DebugPrint(DEBUG_ERROR, "[CATALYST] Failed starting AP %d. Error: %r\n", Index, Status);
			return Status;
		}

		DebugPrint(DEBUG_VERBOSE, "[CATALYST] AP %d started!\n", Index);
	}

	DebugPrint(DEBUG_VERBOSE, "[CATALYST] Preparing BSP...\n");

	//
	// They are all zero initialized. So Index for the first processor is zero.
	//
	PrepareProcessor(&VirtualProcessors[0]);

	DebugPrint(DEBUG_VERBOSE, "[CATALYST] BSP initialized. We are ready.\n");

	return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
FindFirmware(
	IN EFI_HANDLE	ImageHandle
) {

}

EFI_STATUS
EFIAPI
UefiMain(
	IN EFI_HANDLE			ImageHandle,
	IN EFI_SYSTEM_TABLE*	SystemTable
) {
	gST = SystemTable;
	gBS = SystemTable->BootServices;
	gRT = SystemTable->RuntimeServices;

	DebugPrint(DEBUG_INFO, "[CATALYST] Welcome to Catalyst - Inexorable Hypervisor\n");

	//
	// Find virtual machine firmware.
	//
	EFI_STATUS Status = FindFirmware(ImageHandle);
	if (EFI_ERROR(Status)) {
		DebugPrint(DEBUG_ERROR, "[CATALYST] Failed to prepare processors: %r\n", Status);
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

	DebugPrint(DEBUG_INFO, "[CATALYST] Initializing APs...\n");

	//
	// All processors must be woken up and ready.
	//
	Status = PrepareProcessors();
	if (EFI_ERROR(Status)) {
		DebugPrint(DEBUG_ERROR, "[CATALYST] Failed to prepare processors: %r\n", Status);
		return Status;
	}

	//
	// Virtualize all processors.
	//

	return EFI_SUCCESS;
}