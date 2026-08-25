#include <Uefi.h>
#include <Base.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/LoadedImage.h>

#include "Efi.h"
#include "Util.h"
#include "Variable.h"
#include "Time.h"
#include "Misc.h"
#include "VirtualMemory.h"
#include "Memory.h"
#include "Protocol.h"
#include "Elf.h"
#include "Boot.h"

CONST UINT8 Kernel[] = {
#embed "../WorkhorseRT-main/build/workhorse.elf"
};

CHAR8* gEfiCallerBaseName = "BT-BALCI";

EFI_SYSTEM_TABLE		gST = { 0 };
EFI_BOOT_SERVICES	    gBS = { 0 };
EFI_RUNTIME_SERVICES	gRT = { 0 };

VOID
EFIAPI
BootKernel(
	VOID
) {
	UINT8* MbiBuffer = NULL;
	gBS.AllocatePool(EfiLoaderData, 4096, (VOID**)&MbiBuffer);
	SetMem(MbiBuffer, 4096, 0);

	UINT32 CurrentOffset = 8;

	ELF64_HEADER* Header = (ELF64_HEADER*)Kernel;
	if (!(Header->Ident[ElfIdentMagic0] == ELF_MAG0 &&
		Header->Ident[ElfIdentMagic1] == ELF_MAG1 &&
		Header->Ident[ElfIdentMagic2] == ELF_MAG2 &&
		Header->Ident[ElfIdentMagic3] == ELF_MAG3)) {
		DebugPrint0402("[GUEST] Specified kernel is not a valid ELF file\n");
		return;
	}

	if (Header->Machine != EM_X86_64) {
		DebugPrint0402("[GUEST] Specified kernel is not a x86_64 executable\n");
		return;
	}

	if (Header->Ident[ElfIdentVersion] != EV_CURRENT) {
		DebugPrint0402("[GUEST] Specified kernel has invalid ELF version\n");
		return;
	}

	if (Header->Type != ElfTypeRelocatable && Header->Type != ElfTypeExecutable) {
		DebugPrint0402("[GUEST] Unsupported ELF file\n");
		return;
	}

	UINT32 SectionsTotalSize = Header->Shnum * Header->Shentsize;
	UINT32 ElfTagSize = sizeof(MULTIBOOT2_TAG_ELF_SECTIONS) + SectionsTotalSize;

	ELF64_PHDR* Phdrs = (ELF64_PHDR*)((UINT8*)Kernel + Header->Phoff);
	DebugPrint0402("[GUEST] Loading ELF: Phoff=0x%lx, Phnum=%lu, Entry=0x%lx\n",
		Header->Phoff, Header->Phnum, Header->Entry);

	for (UINTN i = 0; i < Header->Phnum; i++) {
		DebugPrint0402("[GUEST] Segment %lu: Type=0x%x, Offset=0x%lx, Vaddr=0x%lx, Paddr=0x%lx, "
			"FileSize=0x%lx, MemSize=0x%lx, Align=0x%lx\n",
			i, Phdrs[i].Type, Phdrs[i].Offset, Phdrs[i].Vaddr, Phdrs[i].Paddr,
			Phdrs[i].FileSize, Phdrs[i].MemSize, Phdrs[i].Align);

		if (Phdrs[i].Type == PT_LOAD) {

			UINTN Pages = EFI_SIZE_TO_PAGES(Phdrs[i].MemSize);
			EFI_PHYSICAL_ADDRESS TargetAddress = Phdrs[i].Paddr;

			DebugPrint0402("[GUEST] Segment %lu is PT_LOAD. Pages=%lu, TargetAddress=0x%p\n",
				i, Pages, TargetAddress);

			//
			// Force allocation at the exact physical address
			//
			EFI_STATUS Status = gBS.AllocatePages(
				AllocateAddress,
				EfiLoaderData,
				Pages,
				&TargetAddress
			);

			if (EFI_ERROR(Status)) {
				DebugPrint0402("[GUEST] 0x%p mark is not empty. Cannot load kernel here\n", Phdrs[i].Paddr);
				return;
			}

			UINT8* Dest = (UINT8*)TargetAddress;
			UINT8* Src = ((UINT8*)Kernel + Phdrs[i].Offset);
			DebugPrint0402("[GUEST] Copying segment %lu: Src=0x%p, Dest=0x%p, FileSize=0x%lx\n",
				i, Src, Dest, Phdrs[i].FileSize);
			CopyMem(Dest, Src, Phdrs[i].FileSize);

			//
			// Zero BSS
			//
			if (Phdrs[i].MemSize > Phdrs[i].FileSize) {
				UINTN BssSize = Phdrs[i].MemSize - Phdrs[i].MemSize;
				SetMem(Dest + Phdrs[i].FileSize, BssSize, 0);
			}

			DebugPrint0402("[GUEST] Segment %lu loaded successfully at 0x%p\n", i, Dest);
			continue;
		}

		DebugPrint0402("[GUEST] Segment %lu is not PT_LOAD (type=0x%x), skipping.\n", i, Phdrs[i].Type);
	}

	UINTN MemoryMapSize = 0;
	EFI_MEMORY_DESCRIPTOR* Descriptors = NULL;
	UINTN MapKey = 0;
	UINTN DescriptorSize = 0;
	UINT32 DescriptorVersion = 0;
	if (gBS.GetMemoryMap(&MemoryMapSize, Descriptors, &MapKey, &DescriptorSize, &DescriptorVersion) != EFI_BUFFER_TOO_SMALL) {
		DebugPrint0402("[GUEST] WHAT\n");
		return;
	}

	DebugPrint0402("[GUEST] DescriptorSize = 0x%p, DescriptorVersion = %d, MemoryMapSize = 0x%p",
		DescriptorSize, DescriptorVersion, MemoryMapSize);

	//
	// Allocation may change size.
	//
	MemoryMapSize += DescriptorSize * 4;

	EFI_STATUS Status = gBS.AllocatePool(EfiRuntimeServicesData, MemoryMapSize, (VOID**)&Descriptors);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("[GUEST] Cannot allocate memory for memory map descriptors %r\n", Status);
		return;
	}

	DebugPrint0402("[GUEST] Allocated descriptor buffer 0x%p at 0x%p",
		MemoryMapSize, Descriptors);

	Status = gBS.GetMemoryMap(&MemoryMapSize, Descriptors, &MapKey, &DescriptorSize, &DescriptorVersion);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("[GUEST] Cannot get memory map descriptors %r\n", Status);
		return;
	}

	gBS.ExitBootServices(0, MapKey);

	DebugPrint0402("[GUEST] Kernel dynamically mapped successfully\n");

	MULTIBOOT2_TAG_ELF_SECTIONS* ElfTag = (MULTIBOOT2_TAG_ELF_SECTIONS*)(MbiBuffer + CurrentOffset);
	ElfTag->Type = MULTIBOOT_TAG_TYPE_ELF_SECTIONS;
	ElfTag->Size = ElfTagSize;
	ElfTag->Num = Header->Shnum;
	ElfTag->Entsize = Header->Shentsize;
	ElfTag->Shndx = Header->Shstrndx;

	DebugPrint0402("[GUEST] ElfTag at 0x%p, Type=0x%x, Size=%d, Num=%d, Entsize=%d, Shndx=%d\n",
		ElfTag, ElfTag->Type, ElfTag->Size, ElfTag->Num, ElfTag->Entsize, ElfTag->Shndx);

	UINT8* RawSections = (UINT8*)Kernel + Header->Shoff;
	gBS.CopyMem(ElfTag->Sections, RawSections, SectionsTotalSize);

	DebugPrint0402("[GUEST] Copied %d bytes of ELF sections from 0x%p to 0x%p\n",
		SectionsTotalSize, RawSections, ElfTag->Sections);

	CurrentOffset += ALIGN_UP_8(ElfTagSize);
	DebugPrint0402("[GUEST] CurrentOffset after ELF tag = 0x%p\n", CurrentOffset);

	MULTIBOOT2_TAG_MMAP* MmapTag = (MULTIBOOT2_TAG_MMAP*)(MbiBuffer + CurrentOffset);
	MmapTag->Type = MULTIBOOT_TAG_TYPE_MMAP;
	MmapTag->EntrySize = sizeof(MULTIBOOT_MMAP_ENTRY);
	MmapTag->EntryVersion = 0;

	DebugPrint0402("[GUEST] MmapTag at 0x%p, Type=0x%p, EntrySize=%d, EntryVersion=%d\n",
		MmapTag, MmapTag->Type, MmapTag->EntrySize, MmapTag->EntryVersion);

	UINT32 MmapEntryCount = 0;
	UINTN NumEntries = MemoryMapSize / DescriptorSize;

	DebugPrint0402("[GUEST] MemoryMapSize=%d, DescriptorSize=%d, NumEntries=%d\n",
		MemoryMapSize, DescriptorSize, NumEntries);

	for (UINTN i = 0; i < NumEntries; i++) {
		EFI_MEMORY_DESCRIPTOR* Desc = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)Descriptors + (i * DescriptorSize));

		UINT32 MbType = MULTIBOOT_MEMORY_RESERVED;
		
		//
		// We only have 2 types of memory :)
		//
		if (Desc->Type == EfiConventionalMemory) {
			MbType = MULTIBOOT_MEMORY_AVAILABLE;
		}
		else {
			MbType = MULTIBOOT_MEMORY_RESERVED;
		}

		MmapTag->Entries[MmapEntryCount].Address = Desc->PhysicalStart;
		MmapTag->Entries[MmapEntryCount].Length = Desc->NumberOfPages * 4096;
		MmapTag->Entries[MmapEntryCount].Type = MbType;
		MmapTag->Entries[MmapEntryCount].Zero = 0;

		DebugPrint0402("[GUEST] MMAP[%d]: PhysStart=0x%p, Pages=%d, Length=0x%p, EFI_Type=%d -> MB_Type=%d\n",
			i, Desc->PhysicalStart, Desc->NumberOfPages, Desc->NumberOfPages * 4096, Desc->Type, MbType);

		MmapEntryCount++;
	}

	DebugPrint0402("[GUEST] Total MmapEntryCount = %d\n", MmapEntryCount);

	UINT32 MmapTagSize = sizeof(MULTIBOOT2_TAG_MMAP) + (MmapEntryCount * sizeof(MULTIBOOT_MMAP_ENTRY));
	MmapTag->Size = MmapTagSize;
	DebugPrint0402("[GUEST] MmapTagSize = 0x%p\n", MmapTagSize);

	CurrentOffset += ALIGN_UP_8(MmapTagSize);
	DebugPrint0402("[GUEST] CurrentOffset after ELF tag = 0x%p\n", CurrentOffset);

	MULTIBOOT2_TAG_OLD_ACPI* AcpiTag = (MULTIBOOT2_TAG_OLD_ACPI*)(MbiBuffer + CurrentOffset);
	AcpiTag->Type = MULTIBOOT_TAG_TYPE_ACPI_OLD;
	AcpiTag->Size = sizeof(*AcpiTag) + 36;
	CopyMem((VOID**)&AcpiTag->Rsdp, (VOID*)ACPI_RSDP_BASE, 36);

	DebugPrint0402("[GUEST] AcpiTag at 0x%p, Type=0x%p, Size=%d, Rsdp=0x%p\n",
		AcpiTag, AcpiTag->Type, AcpiTag->Size, AcpiTag->Rsdp);

	CurrentOffset += (ALIGN_UP_8(AcpiTag->Size));
	DebugPrint0402("[GUEST] CurrentOffset after ELF tag = 0x%p\n", CurrentOffset);

	MULTIBOOT2_INFO_TAG* EndTag = (MULTIBOOT2_INFO_TAG*)(MbiBuffer + CurrentOffset);
	EndTag->Type = 0;
	EndTag->Size = 8;

	DebugPrint0402("[GUEST] EndTag at 0x%p, Type=0, Size=8\n", EndTag);

	CurrentOffset += 8;
	DebugPrint0402("[GUEST] CurrentOffset after ELF tag = 0x%p\n", CurrentOffset);

	MULTIBOOT2_INFO* FinalMbi = (MULTIBOOT2_INFO*)MbiBuffer;
	FinalMbi->TotalSize = CurrentOffset;
	FinalMbi->Reserved = 0;

	DebugPrint0402("[GUEST] Dropping to Protected Mode because Multiboot2 demands so\n");
	DebugPrint0402("[GUEST] MbiBuffer = 0x%p, 0x%x\n", FinalMbi, FinalMbi->TotalSize);
	DebugPrint0402("[GUEST] Booting to 0x%p with Multiboot2Info = 0x%p\n", Header->Entry, MbiBuffer);

	Multiboot2Boot((VOID*)Header->Entry, FinalMbi);
}

VOID
EFIAPI
UefiMain(
	VOID
) {
	DebugPrint0402("[GUEST] Inexorable - As if it was always there. (It never was)\n");

	//
	// Setup system table.
	//
	gST.Hdr.HeaderSize	= sizeof(gST);
	gST.Hdr.Revision	= EFI_SYSTEM_TABLE_REVISION;
	gST.Hdr.Signature	= EFI_SYSTEM_TABLE_SIGNATURE;

	gST.Hdr.CRC32 = CalculateCrc32(&gST.Hdr, gST.Hdr.HeaderSize);

	gST.BootServices                = &gBS;
	gST.RuntimeServices             = &gRT;

	//
	// Setup runtime services.
	//
	gRT.Hdr.HeaderSize	= sizeof(gRT);
	gRT.Hdr.Revision	= EFI_RUNTIME_SERVICES_REVISION;
	gRT.Hdr.Signature	= EFI_RUNTIME_SERVICES_SIGNATURE;

	gRT.Hdr.CRC32 = CalculateCrc32(&gRT.Hdr, gRT.Hdr.HeaderSize);

	gRT.GetVariable                 = IxGetVariable;
	gRT.SetVariable                 = IxSetVariable;
	gRT.QueryVariableInfo           = IxQueryVariableInfo;
	gRT.GetNextVariableName         = IxGetNextVariableName;

	gRT.SetTime                     = IxSetTime;
	gRT.GetTime                     = IxGetTime;
	gRT.GetWakeupTime               = IxGetWakeupTime;
	gRT.SetWakeupTime               = IxSetWakeupTime;

	gRT.GetNextHighMonotonicCount   = IxGetNextHighMonotonicCount;
	gRT.ResetSystem                 = IxResetSystem;

	gRT.SetVirtualAddressMap		= IxSetVirtualAddressMap;
	gRT.ConvertPointer				= IxConvertPointer;

	//
	// Setup boot services.
	//
	gBS.Hdr.HeaderSize	= sizeof(gBS);
	gBS.Hdr.Revision	= EFI_BOOT_SERVICES_REVISION;
	gBS.Hdr.Signature	= EFI_BOOT_SERVICES_SIGNATURE;

	gBS.Hdr.CRC32 = CalculateCrc32(&gBS.Hdr, gBS.Hdr.HeaderSize);

	gBS.ExitBootServices			= IxExitBootServices;

	gBS.AllocatePages				= IxAllocatePages;
	gBS.FreePages					= IxFreePages;

	gBS.AllocatePool				= IxAllocatePool;
	gBS.FreePool					= IxFreePool;

	gBS.GetMemoryMap				= IxGetMemoryMap;

	gBS.CalculateCrc32				= IxCalculateCrc32;
	gBS.CopyMem						= IxCopyMem;
	gBS.SetMem						= IxSetMem;

	gBS.OpenProtocol				= IxOpenProtocol;
	gBS.LocateProtocol				= IxLocateProtocol;
	gBS.HandleProtocol				= IxHandleProtocol;
	gBS.InstallProtocolInterface	= IxInstallProtocolInterface;

	InitializeListHead(&g_ProtocolList);
	InitializeListHead(&g_HandleList);
	InitializeListHead(&g_VariableList);
	BootKernel();
}