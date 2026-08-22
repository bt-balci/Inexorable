#include <Uefi.h>
#include <Base.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Protocol/MpService.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/BlockIo2.h>
#include <IndustryStandard/PeImage.h>

#include "Efi.h"
#include "x86.h"
#include "Vm.h"
#include "Memory.h"
#include "Firmware.h"

GUID gEfiRngProtocolGuid = EFI_RNG_PROTOCOL_GUID;
GUID gEfiLoadFile2ProtocolGuid = EFI_LOAD_FILE2_PROTOCOL_GUID;
GUID gEfiBlockIo2ProtocolGuid = EFI_BLOCK_IO2_PROTOCOL_GUID;

CHAR8* gEfiCallerBaseName = "BT-BALCI";

CONST UINT8 g_InexorableEfi[] = {
 #embed "../x64/Debug/inexorable.efi"
};
CONST UINTN g_InexorableEfiSize = sizeof(g_InexorableEfi);

UINT64 g_CurrentGuestPhysicalAddress = 0;

CATALYST_VIRTUAL_CPU g_VirtualProcessors[CONFIG_CPU_COUNT] = {0};

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
	ASSERT_EFI_ERROR(Status);

	UINTN TotalProcessors = 0;
	UINTN EnabledProcessors = 0;
	Status = MpServices->GetNumberOfProcessors(MpServices, &TotalProcessors, &EnabledProcessors);

	//
	// This should not fail.
	//
	ASSERT_EFI_ERROR(Status);

	//
	// The number of CPUs is statistically allocated.
	//
	ASSERT(EnabledProcessors == CONFIG_CPU_COUNT);

	DebugPrint0402("PrepareProcessors: Total of %d processors, %d enabled\n", TotalProcessors, EnabledProcessors);

	{
		CATALYST_VIRTUAL_CPU* Cpu = &g_VirtualProcessors[0];

		UINT64 StackGpa;
		Status = GuestFindFreePages(AllocateAnyPages, EFI_SIZE_TO_PAGES(BSP_INITIAL_STACK_SIZE), &StackGpa);
		ASSERT_EFI_ERROR(Status);

		Cpu->Context.Rsp = StackGpa + BSP_INITIAL_STACK_SIZE;
		Cpu->Context.Rip = (UINT64)GuestRip;

		DebugPrint0402("PrepareProcessors: BSP stack allocated at GPA 0x%p - 0x%p\n", StackGpa, Cpu->Context.Rsp - 1);
	}


	//
	// Initialize globals for each guest CPU.
	// Because for APs started via EFI_MP_SERVICES_PROTOCOL, the TR and other segment values are invalid for VMX.
	//
	IA32_DESCRIPTOR Descriptor = { 0 };
	AsmReadIdtr(&Descriptor);

	//
	// Set the interrupt handler for sexy APIC shenanigans.
	//
	((X86_IDT_ENTRY*)Descriptor.Base)[209] = GenerateIdtEntry((UINT64)StartVm, AsmReadCs(), X86_GATE_TYPE_INTERRUPT, 0);

	//
	// Index = 1 to skip the BSP (current processor).
	//
	for (UINTN i = 1; i < CONFIG_CPU_COUNT; i++) {
		CATALYST_VIRTUAL_CPU* Cpu = &g_VirtualProcessors[i];
		Cpu->Index = i;

		//
		// They should halt and wait for mommy SIPI from guest OS like a good little boy.
		//
		Cpu->Context.Rip = g_VmState.Firmware.ApHaltSection;
		
		//
		// Set it. Forget it. APs won't use the stack anyway.
		// Or will they?
		// No.
		//
		Cpu->Context.Rsp = 0;

		DebugPrint0402("PrepareProcessors: Starting AP %d...\n", i);

		//
		// Start in blocking mode. We will wait until each one of them finishes.
		//
		Status = MpServices->StartupThisAP(MpServices, PrepareProcessor, i, NULL, 0, (VOID*)Cpu, NULL);
		if (EFI_ERROR(Status)) {
			DebugPrint0402("PrepareProcessors: FAIL: Failed starting AP %d. Error: %r\n", i, Status);
			return Status;
		}

		DebugPrint0402("PrepareProcessors: AP %d started!\n", i);
	}

	DebugPrint0402("PrepareProcessors: Preparing BSP...\n");

	PrepareProcessor(&g_VirtualProcessors[0]);

	DebugPrint0402("PrepareProcessors: BSP initialized. We are ready.\n");

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
	// We don't want to relocate it. And HP's spyware UEFI implementation will sure as hell relocate it.
	// (Even though this POC won't run in HP computers, I felt an immense impulse just to mess with HP)
	//

	DebugPrint0402("LoadFirmware: Mapping firmware...\n");

	EFI_IMAGE_DOS_HEADER* DosHeader = (EFI_IMAGE_DOS_HEADER*)&g_InexorableEfi;

	if (DosHeader->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
		DebugPrint0402("LoadFirmware: FAIL: Catalyst only supports PE32+ firmwares.\n");
		return EFI_UNSUPPORTED;
	}

	EFI_IMAGE_NT_HEADERS64* NtHeaders = (EFI_IMAGE_NT_HEADERS64*)((UINT8*)&g_InexorableEfi + DosHeader->e_lfanew);

	if (NtHeaders->FileHeader.Machine != EFI_IMAGE_MACHINE_X64) {
		DebugPrint0402("LoadFirmware: FAIL: Catalyst only supports x86_64 firmwares.\n");
		return EFI_UNSUPPORTED;
	}

	EFI_IMAGE_OPTIONAL_HEADER64* OptionalHeader = &NtHeaders->OptionalHeader;

	//
	// Virgins relocate images.
	// Chads put them whereever they want to be.
	// Haters will talk about ASLR.
	// Chad will just virtualize address space via EPTs.
	//
	if (OptionalHeader->ImageBase != FIRMWARE_IMAGE_BASE) {
		DebugPrint0402("LoadFirmware: FAIL: The firmware preferred ImageBase must be exactly 0x%p.\n", FIRMWARE_IMAGE_BASE);
		return EFI_UNSUPPORTED;
	}

	if (OptionalHeader->SectionAlignment != 0x1000) {
		DebugPrint0402("LoadFirmware: FAIL: The firmware executable's section alignment must be 0x1000.\n");
		return EFI_UNSUPPORTED;
	}

	UINT64 GuestBaseAddress = FIRMWARE_IMAGE_BASE;
	UINT8* HostBaseAddress = NULL;

	if (EFI_ERROR(GuestFindFreePages(AllocateAddress, EFI_SIZE_TO_PAGES(OptionalHeader->SizeOfImage), &GuestBaseAddress))) {
		DebugPrint0402("LoadFirmware: FAIL: Cannot allocate memory at 0x%p for firmware image.\n", GuestBaseAddress);
		return EFI_NO_MAPPING;
	}

	if (!TranslateGuestPhysicalToHostPhysical(GuestBaseAddress, (VOID**)&HostBaseAddress)) return EFI_NO_MAPPING;

	DebugPrint0402("LoadFirmware: Firmware image base GPA = 0x%p, HPA = 0x%p\n", GuestBaseAddress, HostBaseAddress);
	
	//
	// This assumes header is less than 4096 bytes.
	// It should be. If your header is bigger than 4096 bytes don't talk to me.
	//
	DebugPrint0402("LoadFirmware: Copying headers from 0x%p to 0x%p with length 0x%p\n", (VOID*)&g_InexorableEfi, HostBaseAddress, OptionalHeader->SizeOfHeaders);
	CopyMem((VOID*)HostBaseAddress, (VOID*)&g_InexorableEfi, OptionalHeader->SizeOfHeaders);
	
	DebugPrint0402("LoadFirmware: Copying sections.\n");

	EFI_IMAGE_SECTION_HEADER* Section = (EFI_IMAGE_SECTION_HEADER*)((UINT8*)OptionalHeader + NtHeaders->FileHeader.SizeOfOptionalHeader);

	for (UINT32 SectionIndex = 0; SectionIndex < NtHeaders->FileHeader.NumberOfSections; SectionIndex++) {
		UINT64	CurrentGuestAddress	= Section->VirtualAddress + GuestBaseAddress;
		UINT8*	Source				= (UINT8*)&g_InexorableEfi + Section->PointerToRawData;
		UINTN	RemainingBytes		= Section->SizeOfRawData;

		//DebugPrint0402("LoadFirmware: Section %.8a. Virtual Address = 0x%p, Size of Raw Data = 0x%p\n", &Section->Name, Section->VirtualAddress, Section->SizeOfRawData);

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

			VOID* HostAddress = NULL;
			if (!TranslateGuestPhysicalToHostPhysical(CurrentGuestAddress, &HostAddress)) return EFI_NO_MAPPING;

			CopyMem(HostAddress, Source, ChunkSize);

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

				VOID* HostAddress = NULL;
				if (!TranslateGuestPhysicalToHostPhysical(CurrentGuestAddress, &HostAddress)) return EFI_NO_MAPPING;

				ZeroMem(HostAddress, ChunkSize);

				CurrentGuestAddress += ChunkSize;
				RemainingLength -= ChunkSize;
			}
		}
		Section++;
	}

	*GuestEntryPoint = (VOID*)(UINT64)OptionalHeader->AddressOfEntryPoint + FIRMWARE_IMAGE_BASE;
	DebugPrint0402("LoadFirmware: Firmware mapped. ImageSize = 0x%p, ImageBase = 0x%p, EntryPointt = 0x%p\n", OptionalHeader->SizeOfImage, FIRMWARE_IMAGE_BASE, *GuestEntryPoint);

	return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MapMemory(
	VOID
) {
	//
	// Initialize globals.
	//
	g_VmState.AddressSpace.Eptp.DirtyAndAccessEnabled = FALSE;
	g_VmState.AddressSpace.Eptp.PageWalkLength = 3;
	g_VmState.AddressSpace.Eptp.MemoryType = 6;
	g_VmState.AddressSpace.Eptp.Pml4Address = ((UINT64)&g_VmState.AddressSpace.Epml4) >> 12;

	DebugPrint0402("MapMemory: g_Eptp = 0x%p\n", g_VmState.AddressSpace.Eptp.All);

	g_VmState.AddressSpace.HostCr3.PhysicalAddress = ((UINT64)&g_VmState.AddressSpace.IdentityPml4[0]) >> 12;

	g_VmState.AddressSpace.IdentityPml4[0].Present = TRUE;
	g_VmState.AddressSpace.IdentityPml4[0].Write = TRUE;
	g_VmState.AddressSpace.IdentityPml4[0].PhysicalAddress = ((UINT64)&g_VmState.AddressSpace.IdentityPdpe[0]) >> 12;


	//
	// Why not map the first 4GB only?
	// Because of the memory holes, there is actually more RAM than there is (approximately 5GB for a 4GB machine)
	// And we start at 5GB~. So we need to map all first 512GBs to make sure we won't bork ourselves.
	// ^^^^
	// This might screw me over, but okay.
	//
	for (UINTN i = 0; i < 512; i++) {
		g_VmState.AddressSpace.IdentityPdpe[i].Present = TRUE;
		g_VmState.AddressSpace.IdentityPdpe[i].Large = TRUE;
		g_VmState.AddressSpace.IdentityPdpe[i].Write = TRUE;
		g_VmState.AddressSpace.IdentityPdpe[i].PhysicalAddress = i;
	}

	DebugPrint0402("MapMemory: HostCr3 = 0x%p\n", g_VmState.AddressSpace.HostCr3.All);

	//
	// Copy current host GDT / IDT into SharedState so the host has valid
	// descriptor tables when a VM exit occurs.
	//
	{
		IA32_DESCRIPTOR HostGdtr;
		AsmReadGdtr(&HostGdtr);
		CopyMem(g_VmState.SharedState.HostGdt, (VOID*)HostGdtr.Base, HostGdtr.Limit + 1);
		g_VmState.SharedState.HostGdtBase = g_VmState.SharedState.HostGdt;
	}
	{
		IA32_DESCRIPTOR HostIdtr;
		AsmReadIdtr(&HostIdtr);
		CopyMem(g_VmState.SharedState.HostIdt, (VOID*)HostIdtr.Base, HostIdtr.Limit + 1);
		g_VmState.SharedState.HostIdtBase = g_VmState.SharedState.HostIdt;
	}
	g_VmState.SharedState.HostTssBase = g_VmState.SharedState.HostTss;

	DebugPrint0402("MapMemory: Mapping out free memory...\n");

	UINTN Pages = EFI_SIZE_TO_PAGES(GUEST_ADDRESS_SPACE_SIZE);
	EFI_PHYSICAL_ADDRESS GuestAddressSpaceBeginning = 0;

	//
	// Just allocate from boot services....
	//
	EFI_STATUS Status = gBS->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, Pages, &GuestAddressSpaceBeginning);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("MapMemory: Failed to allocate pages for guest's address space. %r", Status);
		return Status;
	}

	if (!MapPagesLinear((UINT64)GuestAddressSpaceBeginning, Pages, &g_CurrentGuestPhysicalAddress)) {
		DebugPrint0402("MapMemory: Failed to map %d pages HPA 0x%p to GPA 0x%p\n", Pages, GuestAddressSpaceBeginning, g_CurrentGuestPhysicalAddress);
		return EFI_NO_MAPPING;
	}

	DebugPrint0402("MapMemory: Mapped 0x%p-0x%p for guest\n", GuestAddressSpaceBeginning, g_CurrentGuestPhysicalAddress);

	//
	// Setup guest firmware memory.
	//
	g_VmState.Firmware.Memory.TotalPages = Pages;

	UINT64 Pml4Gpa = 0;
	UINT64 PdpteGpa = 0;

	Status = GuestFindFreePages(AllocateAnyPages, 1, &Pml4Gpa);
	ASSERT_EFI_ERROR(Status);

	Status = GuestFindFreePages(AllocateAnyPages, 1, &PdpteGpa);
	ASSERT_EFI_ERROR(Status);

	PML4* Pml4Host;
	PDPTE* PdpteHost;
	if (!TranslateGuestPhysicalToHostPhysical(Pml4Gpa, (VOID*)&Pml4Host) ||
		!TranslateGuestPhysicalToHostPhysical(PdpteGpa, (VOID*)&PdpteHost)) {
		return EFI_NO_MAPPING;
	}

	Pml4Host->Present = TRUE;
	Pml4Host->Write = TRUE;
	Pml4Host->PhysicalAddress = PdpteGpa >> 12;

	for (UINT32 i = 0; i < 512; i++) {
		PdpteHost[i].Present = TRUE;
		PdpteHost[i].Large = TRUE;
		PdpteHost[i].Write = TRUE;
		PdpteHost[i].PhysicalAddress = i;
	}

	g_VmState.AddressSpace.GuestCr3 = Pml4Gpa;
	DebugPrint0402("MapMemory: Guest CR3 = 0x%p\n", g_VmState.AddressSpace.GuestCr3);

	//
	// Allocate HLT section for APs.
	//
	UINT64 ApHaltGpa;
	Status = GuestFindFreePages(AllocateAnyPages, 1, &ApHaltGpa);
	ASSERT_EFI_ERROR(Status);

	UINT8* ApHaltHost;
	if (!TranslateGuestPhysicalToHostPhysical(ApHaltGpa, (VOID**)&ApHaltHost)) {
		return EFI_NO_MAPPING;
	}
	ApHaltHost[0] = 0xFA;  // cli
	ApHaltHost[1] = 0xF4;  // hlt
	ApHaltHost[2] = 0xEB;  // jmp $-2
	ApHaltHost[3] = 0xFE;  // (opcode for JMP)
	DebugPrint0402("MapMemory: AP HLT page at GPA 0x%p\n", ApHaltGpa);

	g_VmState.Firmware.ApHaltSection = ApHaltGpa;

	//
	// Setup GDT for guest.
	//
	UINT64 GdtGpa;
	Status = GuestFindFreePages(AllocateAnyPages, 1, &GdtGpa);
	ASSERT_EFI_ERROR(Status);

	IA32_SEGMENT_DESCRIPTOR* Gdt = NULL;
	if (!TranslateGuestPhysicalToHostPhysical(GdtGpa, (VOID**)&Gdt)) {
		return EFI_NO_MAPPING;
	}

	gBS->SetMem(Gdt, EFI_PAGE_SIZE, 0);

	//
	// Null descriptor.
	//
	Gdt[0].Uint64 = 0;

	//
	// Data descriptor (index 6, selector 0x30)
	//
	Gdt[6].Bits.BaseLow = 0;
	Gdt[6].Bits.BaseMid = 0;
	Gdt[6].Bits.BaseHigh = 0;
	Gdt[6].Bits.LimitLow = 0xFFFF;
	Gdt[6].Bits.LimitHigh = 0xF;
	Gdt[6].Bits.Type = 0x2;  // Data read/write
	Gdt[6].Bits.S = 1;    // Code/data
	Gdt[6].Bits.DPL = 0;
	Gdt[6].Bits.P = 1;
	Gdt[6].Bits.L = 0;
	Gdt[6].Bits.DB = 1;    // 32-bit
	Gdt[6].Bits.G = 1;    // 4 KB granularity

	//
	// Code descriptor (index 7, selector 0x38)
	//
	Gdt[7].Bits.BaseLow = 0;
	Gdt[7].Bits.BaseMid = 0;
	Gdt[7].Bits.BaseHigh = 0;
	Gdt[7].Bits.LimitLow = 0xFFFF;
	Gdt[7].Bits.LimitHigh = 0xF;
	Gdt[7].Bits.Type = 0xA;  // Code execute/read
	Gdt[7].Bits.S = 1;
	Gdt[7].Bits.DPL = 0;
	Gdt[7].Bits.P = 1;
	Gdt[7].Bits.L = 1;    // Long mode
	Gdt[7].Bits.DB = 0;
	Gdt[7].Bits.G = 1;

	//
	// TSS.
	//
	UINT64 TssGpa;
	Status = GuestFindFreePages(AllocateAnyPages, 1, &TssGpa);
	ASSERT_EFI_ERROR(Status);

	VOID* tssHost;
	if (!TranslateGuestPhysicalToHostPhysical(TssGpa, &tssHost)) {
		return EFI_NO_MAPPING;
	}
	gBS->SetMem(tssHost, EFI_PAGE_SIZE, 0);

	Gdt[8].Bits.BaseLow = (UINT16)(TssGpa & 0xFFFF);
	Gdt[8].Bits.BaseMid = (UINT8)((TssGpa >> 16) & 0xFF);
	Gdt[8].Bits.BaseHigh = (UINT8)((TssGpa >> 24) & 0xFF);
	Gdt[8].Bits.LimitLow = 103;    // Minimal size
	Gdt[8].Bits.LimitHigh = 0;
	Gdt[8].Bits.Type = 0xB;    // 32-bit TSS available
	Gdt[8].Bits.S = 0;      // System descriptor
	Gdt[8].Bits.DPL = 0;
	Gdt[8].Bits.P = 1;
	Gdt[8].Bits.G = 0;      // Byte granularity

	//
	// 64-bit TSS descriptor occupies two GDT slots (entries 8 & 9).
	// GDT[9] was already zeroed by SetMem above; the upper 32 bits of
	// the TSS base address are 0, which is correct for GPA 0x4000.
	//

	g_VmState.SharedState.GuestGdtBase = (VOID*)GdtGpa;
	g_VmState.SharedState.GuestGdtLimit = 10 * 8 - 1;   // 10 descriptors → 79 (TSS needs 16 bytes)
	g_VmState.SharedState.GuestTssBase = (VOID*)TssGpa;
	g_VmState.SharedState.TrSelector = 0x40;   // index 8, RPL 0
	g_VmState.SharedState.TrAccessRights = 0x8B;   // Present, 32-bit TSS available
	g_VmState.SharedState.TrLimit = 103;

	//
	// IDT.
	//
	UINT64 IdtGpa;
	Status = GuestFindFreePages(AllocateAnyPages, 1, &IdtGpa);
	ASSERT_EFI_ERROR(Status);
	VOID* IdtHost;
	if (!TranslateGuestPhysicalToHostPhysical(IdtGpa, &IdtHost)) {
		return EFI_NO_MAPPING;
	}
	gBS->SetMem(IdtHost, EFI_PAGE_SIZE, 0);
	g_VmState.SharedState.GuestIdtBase = (VOID*)IdtGpa;
	g_VmState.SharedState.GuestIdtLimit = 0;

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

	DebugPrint0402("UefiMain: Welcome to Catalyst - Inexorable Hypervisor\n");

	EFI_LOADED_IMAGE_PROTOCOL* LoadedImage = NULL;

	EFI_STATUS Status = gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&LoadedImage);

	//
	// We need this for debugging
	//
	if (Status == EFI_SUCCESS && FALSE) {
		DebugPrint0402("Catalyst.efi loaded at 0x%p, with size 0x%p", LoadedImage->ImageBase, LoadedImage->ImageSize);
		{
			volatile UINT32 Spin = 1;
			while (Spin) {
				__asm__ __volatile__("pause");
			}
		}
	}

	DebugPrint0402("UefiMain: Failed to open LoadedImageProtocol: %r\n", Status);

	//
	// VMX support must be present.
	//
	ASSERT(IsVmxSupported());

	//
	// VMX must be enabled.
	// If the VMX is enabled but BIOS didn't set the lock bit, then screw your BIOS (or OVMF for this context. No don't screw OVMF. It's cool).
	// Update: Screw OVMF and whoever decided to put page tables in EfiConventionalMemory. It took me 4 hours to debug.
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
		DebugPrint0402("UefiMain: FAIL: Failed to map memory: %r\n", Status);
		return Status;
	}

	//
	// Map the firmware to memory. (FIRMWARE_IMAGE_BASE my beloved)
	//
	VOID* GuestRIP = 0;
	Status = LoadFirmware(&GuestRIP);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("UefiMain: FAIL: Failed to load firmware: %r\n", Status);
		return Status;
	}

	DebugPrint0402("UefiMain: Initializing APs...\n");

	//
	// All processors must be woken up and ready.
	//
	Status = PrepareProcessors(GuestRIP);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("UefiMain: FAIL: Failed to prepare processors: %r\n", Status);
		return Status;
	}

	DebugPrint0402("UefiMain: Virtualizing system...\n");

	//
	// Virtualize em' all.
	//
	SendInterrupt(209, ApicDestinationShorthandAllIncludingSelf, 0);

	return EFI_SUCCESS;
}