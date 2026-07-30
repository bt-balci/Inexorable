#include <Uefi.h>
#include <Base.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Protocol/MpService.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <IndustryStandard/PeImage.h>

#include "Efi.h"
#include "x86.h"
#include "Vm.h"
#include "Memory.h"

//
// Inclusion skill issue.
//
EFI_SYSTEM_TABLE*		gST = NULL;
EFI_BOOT_SERVICES*		gBS = NULL;
EFI_RUNTIME_SERVICES*	gRT = NULL;

CHAR8* gEfiCallerBaseName = "BT-BALCI";

CONST UINT8 g_InexorableEfi[] = {
 #embed "../x64/Debug/inexorable.efi"
};
CONST UINTN g_InexorableEfiSize = sizeof(g_InexorableEfi);

UINT64 g_CurrentGuestPhysicalAddress = 0;

CONST UINT8 g_ApHalt[] = { 0xF4 };

CATALYST_VIRTUAL_CPU VirtualProcessors[CONFIG_CPU_COUNT] = {0};

STATIC
EFI_STATUS
EFIAPI
PrepareProcessors(
	IN VOID* GuestRip
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

	DebugPrint0402("Total of %d processors, %d enabled\n", TotalProcessors, EnabledProcessors);

	//
	// The guest OS will wake up APs. They must "sleep".
	//
	{
		CATALYST_VIRTUAL_CPU* Cpu = &VirtualProcessors[0];
		Cpu->Context.Rip = (UINT64)GuestRip;

		//
		// Map the initial guest stack.
		//
		Status = gBS->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, BSP_INITIAL_STACK_SIZE, (EFI_PHYSICAL_ADDRESS*)&Cpu->Context.Rsp);
		if (EFI_ERROR(Status)) {
			DebugPrint0402("Failed to allocate memory for guest's initial stack: %r\n", Status);
			return Status;
		}

		for (UINT32 i = 0; i < BSP_INITIAL_STACK_SIZE; i++) {
			if (!MapSinglePage(Cpu->Context.Rsp + (i * 0x1000), Cpu->Context.Rsp + (i * 0x1000), FALSE)) {
				DebugPrint0402("Failed to map BSP initial stack!\n");
			}
		}

		//Cpu->Context.Rsp = GUEST_INITIAL_RSP;
	}

	//
	// Initialize globals for each guest CPU.
	// Because for APs started via EFI_MP_SERVICES_PROTOCOL, the TR and other segment values are invalid for VMX.
	//
	IA32_DESCRIPTOR Descriptor = { 0 };
	AsmReadGdtr(&Descriptor);

	//
	// 1:1 map descriptor structures. Read the disclaimer at Main.c for the other screw up.
	// Warning: This will INEVITABLY BREAK if the Catalyst is not mapped over the 4GB limit and guest has more than 4GB memory. 
	//
	g_GuestHostGdtBase = (VOID*)Descriptor.Base;
	MapSinglePage((UINT64)g_GuestHostGdtBase, (UINT64)g_GuestHostGdtBase, FALSE);

	g_GuestHostTssBase = (VOID*)GET_GDT_TSS_BASE((X86_GDT_SYSTEM_ENTRY*)AsmLoadSegmentDescriptor(Descriptor.Base, AsmReadTr()));
	MapSinglePage((UINT64)g_GuestHostTssBase, (UINT64)g_GuestHostTssBase, FALSE);

	AsmReadIdtr(&Descriptor);

	g_GuestHostIdtBase = (VOID*)Descriptor.Base;
	MapSinglePage((UINT64)g_GuestHostIdtBase, (UINT64)g_GuestHostIdtBase, FALSE);

	g_GuestHostTrSelector		= AsmReadTr();
	g_GuestHostTrAccessRights	= AsmLoadSegmentAccessRights(g_GuestHostTrSelector);
	g_GuestHostTrLimit			= AsmLoadSegmentLimit(g_GuestHostTrSelector);

	//
	// Set the interrupt handler for sexy APIC shenanigans.
	//
	((X86_IDT_ENTRY*)Descriptor.Base)[209] = GenerateIdtEntry((UINT64)StartVm, AsmReadCs(), X86_GATE_TYPE_INTERRUPT, 0);

	//
	// Index = 1 to skip the BSP (current processor).
	//
	for (UINTN i = 1; i < CONFIG_CPU_COUNT; i++) {
		CATALYST_VIRTUAL_CPU* Cpu = &VirtualProcessors[i];
		Cpu->Index = i;

		//
		// They should halt and wait for mommy SIPI from guest OS like a good little boy.
		//
		Cpu->Context.Rip = (UINT64)&g_ApHalt;
		
		//
		// Set it. Forget it. APs won't use the stack anyway.
		// Or will they?
		// No.
		//
		Cpu->Context.Rsp = 0;

		DebugPrint0402("Starting AP %d...\n", i);

		//
		// Start in blocking mode. We will wait until each one of them finishes.
		//
		Status = MpServices->StartupThisAP(MpServices, PrepareProcessor, i, NULL, 0, (VOID*)Cpu, NULL);
		if (EFI_ERROR(Status)) {
			DebugPrint0402("Failed starting AP %d. Error: %r\n", i, Status);
			return Status;
		}

		DebugPrint0402("AP %d started!\n", i);
	}

	DebugPrint0402("Preparing BSP...\n");

	PrepareProcessor(&VirtualProcessors[0]);

	DebugPrint0402("BSP initialized. We are ready.\n");

	return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
LoadFirmware(
	OUT VOID**	GuestEntryPoint
) {
	//
	// Could we have just did gBS->LoadImage?
	// Perhaps. Then why I didn't do it? For fixed base addresses.
	// I don't want no implementation specific stuff happening to PE file I load.
	// We don't want to relocate it. And HP's spyware-ass UEFI implementation will sure as hell relocate it.
	// (Even though this POC won't run in HP computers, I felt an immense impulse just to fuck with HP)
	//

	DebugPrint0402("Mapping firmware...\n");

	EFI_IMAGE_DOS_HEADER* DosHeader = (EFI_IMAGE_DOS_HEADER*)&g_InexorableEfi;

	if (DosHeader->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
		DebugPrint0402("Catalyst only supports PE32+ firmwares.\n");
		return EFI_UNSUPPORTED;
	}

	EFI_IMAGE_NT_HEADERS64* NtHeaders = (EFI_IMAGE_NT_HEADERS64*)((UINT8*)&g_InexorableEfi + DosHeader->e_lfanew);

	if (NtHeaders->FileHeader.Machine != EFI_IMAGE_MACHINE_X64) {
		DebugPrint0402("Catalyst only supports x86_64 firmwares.\n");
		return EFI_UNSUPPORTED;
	}

	EFI_IMAGE_OPTIONAL_HEADER64* OptionalHeader = &NtHeaders->OptionalHeader;

	if (OptionalHeader->ImageBase != FIRMWARE_IMAGE_BASE) {
		DebugPrint0402("The firmware preferred ImageBase must be exactly 0x%p.\n", FIRMWARE_IMAGE_BASE);
		return EFI_UNSUPPORTED;
	}

	if (OptionalHeader->SectionAlignment != 0x1000) {
		DebugPrint0402("The firmware executable's section alignment must be 0x1000.\n");
		return EFI_UNSUPPORTED;
	}

	UINT64 GuestBaseAddress = FIRMWARE_IMAGE_BASE;
	UINT8* HostBaseAddress = (UINT8*)TranslateGuestToHostAddress(GuestBaseAddress);

	DebugPrint0402("Firmware image base GPA = 0x%p, HPA = 0x%p\n", GuestBaseAddress, HostBaseAddress);
	
	//
	// This assumes header is less than 4096 bytes.
	// It should be. If your header is bigger than 4096 bytes don't talk to me.
	//
	DebugPrint0402("Copying headers from 0x%p to 0x%p with length 0x%p\n", (VOID*)&g_InexorableEfi, HostBaseAddress, OptionalHeader->SizeOfHeaders);
	CopyMem((VOID*)HostBaseAddress, (VOID*)&g_InexorableEfi, OptionalHeader->SizeOfHeaders);
	
	DebugPrint0402("Copying sections.\n");

	EFI_IMAGE_SECTION_HEADER* Section = (EFI_IMAGE_SECTION_HEADER*)((UINT8*)OptionalHeader + NtHeaders->FileHeader.SizeOfOptionalHeader);

	for (UINT32 SectionIndex = 0; SectionIndex < NtHeaders->FileHeader.NumberOfSections; SectionIndex++) {
		UINT64	CurrentGuestAddress	= Section->VirtualAddress + GuestBaseAddress;
		UINT8*	Source				= (UINT8*)&g_InexorableEfi + Section->PointerToRawData;
		UINTN	RemainingBytes		= Section->SizeOfRawData;

		DebugPrint0402("Section %.8a. Virtual Address = 0x%p, Size of Raw Data = 0x%p\n", &Section->Name, Section->VirtualAddress, Section->SizeOfRawData);

		//
		// Guest's memory is contiguous. But the host's memory is not. We have to recalculate for each page.
		//
		while (RemainingBytes > 0) {
			UINT64 PageOffset = CurrentGuestAddress & 0xFFF;

			//
			// Substract from 0x1000 to deetermine amounts of bytes to write. Because if we only took the PageOffset it could be zero.
			//
			UINT64 ChunkSize = 0x1000 - PageOffset;

			if (ChunkSize > RemainingBytes) {
				ChunkSize = RemainingBytes;
			}

			CopyMem(TranslateGuestToHostAddress(CurrentGuestAddress), Source, ChunkSize);

			RemainingBytes -= ChunkSize;
			CurrentGuestAddress += ChunkSize;
			Source += ChunkSize;
		}

		if (Section->Misc.VirtualSize > Section->SizeOfRawData) {
			UINT64 RemainingLength = Section->Misc.VirtualSize - Section->SizeOfRawData;

			while (RemainingLength > 0) {
				UINT64 PageOffset = CurrentGuestAddress & 0xFFF;
				UINT64 ChunkSize = 0x1000 - PageOffset;
				if (ChunkSize > RemainingLength) {
					ChunkSize = RemainingLength;
				}

				ZeroMem(TranslateGuestToHostAddress(CurrentGuestAddress), ChunkSize);

				CurrentGuestAddress += ChunkSize;
				RemainingLength -= ChunkSize;
			}
		}
		Section++;
	}

	*GuestEntryPoint = (VOID*)(UINT64)OptionalHeader->AddressOfEntryPoint + FIRMWARE_IMAGE_BASE;
	DebugPrint0402("Firmware mapped. ImageSize = 0x%p, ImageBase= 0x%p\n", OptionalHeader->SizeOfImage, FIRMWARE_IMAGE_BASE);

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
		DebugPrint0402("Failed to get memory map size: %r\n", Status);
		return Status;
	}

	//
	// +10 to make sure
	//
	MapSize += DescriptorSize * 10;
	EFI_MEMORY_DESCRIPTOR* DescriptorArray = 0;

	Status = gBS->AllocatePool(EfiBootServicesData, MapSize, (VOID**)&DescriptorArray);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("Failed to allocate for memory map: %r\n", Status);
		return Status;
	}

	Status = gBS->GetMemoryMap(&MapSize, DescriptorArray, &MapKey, &DescriptorSize, &DescriptorVersion);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("Failed to get memory map: %r\n", Status);
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

	DebugPrint0402("g_Eptp = 0x%p\n", g_Eptp.All);

	g_IdentityCr3.PhysicalAddress = ((UINT64)&g_IdentityPml4[0]) >> 12;

	g_IdentityPml4[0].Present = TRUE;
	g_IdentityPml4[0].Write = TRUE;
	g_IdentityPml4[0].PhysicalAddress = ((UINT64)&g_IdentityPdpe[0]) >> 12;

	//
	// Why not map the first 4GB only?
	// Because of the memory holes, there is actually more RAM than there is (approximately 5GB for a 4GB machine)
	// And we start at 5GB~. So we need to map all first 512GBs to make sure we won't bork ourselves.
	// ^^^^
	// This might screw me over, but okay.
	//
	for (UINTN i = 0; i < 512; i++) {
		g_IdentityPdpe[i].Present = TRUE;
		g_IdentityPdpe[i].Large = TRUE;
		g_IdentityPdpe[i].Write = TRUE;
		g_IdentityPdpe[i].PhysicalAddress = i;
	}

	DebugPrint0402("g_IdentityCr3 = 0x%p\n",	g_IdentityCr3.All);

	DebugPrint0402("Mapping out free memory...\n");

	UINT64 TotalBytes = 0;

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
			UINT64 PhysicalStart = Desc->PhysicalStart;
			UINT64 NumberOfPages = Desc->NumberOfPages;
			
			DebugPrint0402("Found memory at %p for %d pages. Mapping to %p\n", PhysicalStart, NumberOfPages, g_CurrentGuestPhysicalAddress);
			
			if (!MapPagesLinear(PhysicalStart, NumberOfPages, &g_CurrentGuestPhysicalAddress)) {
				DebugPrint0402("Failed to map %d pages HPA 0x%p to GPA 0x%p\n", NumberOfPages, PhysicalStart, g_CurrentGuestPhysicalAddress);
			}

			DebugPrint0402("Mapped %d pages, new cursor: %p\n", NumberOfPages, g_CurrentGuestPhysicalAddress);
			TotalBytes += PhysicalStart * 0x1000;
		}

		Desc = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)Desc + DescriptorSize);
	}

	DebugPrint0402("Total of 0x%p bytes of contiguous address space for the guest\n", TotalBytes);

	//
	// Forcefully map the host physical memory holding our Guest Page Tables
	// into the EPT. We will identity map them (GPA == HPA) so we can safely
	// pass their HPA directly to GUEST_CR3. (200 IQ, for now. Considering that we are above the 4GB but I can't be bothered.)
	//
	UINT64 Pml4Hpa = (UINT64)&g_IdentityPml4[0];
	MapSinglePage(Pml4Hpa, Pml4Hpa, FALSE);

	UINT64 PdpeHpa = (UINT64)&g_IdentityPdpe[0];
	//
	// Identity map the PDPEs. Gently...
	//
	for (UINT32 i = 0; i < 512; i++) {
		if (!MapSinglePage(PdpeHpa + (i * 0x1000), PdpeHpa + (i * 0x1000), FALSE)) {
			DebugPrint0402("Failed to map Guest CR3 structures!\n");
			return EFI_NO_MAPPING;
		}
	}

	//
	// Map the HLT area for APs to wake up at.
	//
	if (!MapSinglePage((UINT64)&g_ApHalt, (UINT64)&g_ApHalt, FALSE)) {
		DebugPrint0402("Failed to map Guest CR3 structures!\n");
		return EFI_NO_MAPPING;
	}
End:
	gBS->FreePool(DescriptorArray);
	return Status;
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

	DebugPrint0402("Welcome to Catalyst - Inexorable Hypervisor\n");

	EFI_LOADED_IMAGE_PROTOCOL* LoadedImage = NULL;

	EFI_STATUS Status = gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&LoadedImage);

	//
	// We need this for debugging
	//
	if (Status == EFI_SUCCESS
		&& FALSE) {
		DebugPrint0402("Catalyst.efi loaded at 0x%p, with size 0x%p", LoadedImage->ImageBase, LoadedImage->ImageSize);
		{
			volatile UINT32 Spin = 1;
			while (Spin) {
				__asm__ __volatile__("pause");
			}
		}
	}

	DebugPrint0402("Failed to open LoadedImageProtocol: %r\n", Status);

	//
	// VMX support must be present.
	//
	ASSERT(IsVmxSupported());

	//
	// VMX must be enabled.
	// If the VMX is enabled but BIOS didn't set the lock bit, then screw your BIOS (or OVMF for this context. No don't screw OVMF. It's cool).
	// Update: Screw OVMF and whoever decided to put page tables in EfiConventionalMemory. Shit took me 4 hours to debug.
	//
	ASSERT(IsVmxEnabled());

	//
	// By the golden rule of x86, all logical processors of the same socket must be same. If one logical processor supports VMX, so does the others.
	// This "golden rule" that I just made up might not be reliable when there are more physical cpus that are different, and are in different sockets.
	// But this code will work for our demonstration purposes.
	// Neither way, I don't care. Neither do you.
	//

	//
	// Construct page tables for guest.
	//
	Status = MapMemory();
	if (EFI_ERROR(Status)) {
		DebugPrint0402("Failed to map memory: %r\n", Status);
		return Status;
	}

	//
	// Map the firmware to memory. (FIRMWARE_IMAGE_BASE my beloved)
	//
	VOID* GuestRIP = 0;
	Status = LoadFirmware(&GuestRIP);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("Failed to load firmware: %r\n", Status);
		return Status;
	}

	DebugPrint0402("Initializing APs...\n");

	//
	// All processors must be woken up and ready.
	//
	Status = PrepareProcessors(GuestRIP);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("Failed to prepare processors: %r\n", Status);
		return Status;
	}

	DebugPrint0402("Virtualizing system...\n");

	//
	// Virtualize em' all.
	//
	SendInterrupt(209, ApicDestinationShorthandAllIncludingSelf, 0);

	return EFI_SUCCESS;
}