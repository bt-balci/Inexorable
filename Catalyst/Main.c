#include <Uefi.h>
#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Protocol/MpService.h>

#include "Efi.h"
#include "x86.h"
#include "Vm.h"

CATALYST_VIRTUAL_CPU VirtualProcessors[CONFIG_CPU_COUNT];

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
		Status = MpServices->StartupThisAP(MpServices, NULL /* Function */, Index, NULL, 0, (VOID*)Processor, NULL);
		if (EFI_ERROR(Status)) {
			DebugPrint(DEBUG_ERROR, "[CATALYST] Failed starting AP %d. Error: %r\n", Index, Status);
			return Status;
		}

		DebugPrint(DEBUG_VERBOSE, "[CATALYST] AP %d started!\n", Index);
	}

	DebugPrint(DEBUG_VERBOSE, "[CATALYST] Preparing BSP...\n");

	/* Call */

	DebugPrint(DEBUG_VERBOSE, "[CATALYST] BSP initialized. We are ready.\n");

	return EFI_SUCCESS;
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
	// VMX support must be present.
	//
	ASSERT(IsVmxSupported());

	//
	// VMX must be enabled.
	//
	ASSERT(IsVmxEnabled());

	DebugPrint(DEBUG_INFO, "[CATALYST] Initializing APs...\n");

	//
	// All processors must be woken up and ready.
	//
	EFI_STATUS Status = PrepareProcessors();
	if (EFI_ERROR(Status)) {
		DebugPrint(DEBUG_ERROR, "[CATALYST] Failed to prepare processors: %r\n", Status);
		return Status;
	}

	return EFI_SUCCESS;
}