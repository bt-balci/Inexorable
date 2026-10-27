#include "PeLoader.h"
#include "Memory.h"
#include "Protocol.h"
#include "Serial.h"
#include "Misc.h"
#include "inexorable_layout.h"
#include "Efi.h"

#pragma pack(push, 1)

typedef struct {
    UINT16 e_magic;
    UINT16 e_cblp;
    UINT16 e_cp;
    UINT16 e_crlc;
    UINT16 e_cparhdr;
    UINT16 e_minalloc;
    UINT16 e_maxalloc;
    UINT16 e_ss;
    UINT16 e_sp;
    UINT16 e_csum;
    UINT16 e_ip;
    UINT16 e_cs;
    UINT16 e_lfarlc;
    UINT16 e_ovno;
    UINT16 e_res[4];
    UINT16 e_oemid;
    UINT16 e_oeminfo;
    UINT16 e_res2[10];
    UINT32 e_lfanew;
} IMAGE_DOS_HEADER;

typedef struct {
    UINT16 Machine;
    UINT16 NumberOfSections;
    UINT32 TimeDateStamp;
    UINT32 PointerToSymbolTable;
    UINT32 NumberOfSymbols;
    UINT16 SizeOfOptionalHeader;
    UINT16 Characteristics;
} IMAGE_FILE_HEADER;

typedef struct {
    UINT32 VirtualAddress;
    UINT32 Size;
} IMAGE_DATA_DIRECTORY;

typedef struct {
    UINT16 Magic;
    UINT8  MajorLinkerVersion;
    UINT8  MinorLinkerVersion;
    UINT32 SizeOfCode;
    UINT32 SizeOfInitializedData;
    UINT32 SizeOfUninitializedData;
    UINT32 AddressOfEntryPoint;
    UINT32 BaseOfCode;
    UINT64 ImageBase;
    UINT32 SectionAlignment;
    UINT32 FileAlignment;
    UINT16 MajorOperatingSystemVersion;
    UINT16 MinorOperatingSystemVersion;
    UINT16 MajorImageVersion;
    UINT16 MinorImageVersion;
    UINT16 MajorSubsystemVersion;
    UINT16 MinorSubsystemVersion;
    UINT32 Win32VersionValue;
    UINT32 SizeOfImage;
    UINT32 SizeOfHeaders;
    UINT32 CheckSum;
    UINT16 Subsystem;
    UINT16 DllCharacteristics;
    UINT64 SizeOfStackReserve;
    UINT64 SizeOfStackCommit;
    UINT64 SizeOfHeapReserve;
    UINT64 SizeOfHeapCommit;
    UINT32 LoaderFlags;
    UINT32 NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[16];
} IMAGE_OPTIONAL_HEADER64;

typedef struct {
    UINT8  Name[8];
    union {
        UINT32 PhysicalAddress;
        UINT32 VirtualSize;
    } Misc;
    UINT32 VirtualAddress;
    UINT32 SizeOfRawData;
    UINT32 PointerToRawData;
    UINT32 PointerToRelocations;
    UINT32 PointerToLinenumbers;
    UINT16 NumberOfRelocations;
    UINT16 NumberOfLinenumbers;
    UINT32 Characteristics;
} IMAGE_SECTION_HEADER;

typedef struct {
    UINT32 VirtualAddress;
    UINT32 SizeOfBlock;
} IMAGE_BASE_RELOCATION;

#pragma pack(pop)

EFI_STATUS IxLoadPeImage(
    IN  VOID                   *StagingBase,
    OUT EFI_HANDLE             *OutImageHandle,
    OUT EFI_IMAGE_ENTRY_POINT  *OutEntryPoint
) {
    if (StagingBase == NULL || OutImageHandle == NULL || OutEntryPoint == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)StagingBase;
    if (dos->e_magic != 0x5A4D) { // 'MZ'
        SerialPrint("[PE] Invalid DOS magic: 0x%x (expected 0x5A4D)\n", dos->e_magic);
        return EFI_INVALID_PARAMETER;
    }

    const UINT8 *pe_base = (const UINT8 *)StagingBase + dos->e_lfanew;
    UINT32 pe_sig = *(const UINT32 *)pe_base;
    if (pe_sig != 0x00004550) { // 'PE\0\0'
        SerialPrint("[PE] Invalid PE signature: 0x%x (expected 0x00004550)\n", pe_sig);
        return EFI_INVALID_PARAMETER;
    }

    const IMAGE_FILE_HEADER *file_hdr = (const IMAGE_FILE_HEADER *)(pe_base + 4);
    if (file_hdr->Machine != 0x8664) { // AMD64 / x86_64
        SerialPrint("[PE] Unsupported machine: 0x%x (expected 0x8664)\n", file_hdr->Machine);
        return EFI_UNSUPPORTED;
    }

    const IMAGE_OPTIONAL_HEADER64 *opt_hdr = (const IMAGE_OPTIONAL_HEADER64 *)((const UINT8 *)file_hdr + sizeof(IMAGE_FILE_HEADER));
    if (opt_hdr->Magic != 0x020B) { // PE32+
        SerialPrint("[PE] Unsupported optional header magic: 0x%x (expected 0x020B PE32+)\n", opt_hdr->Magic);
        return EFI_UNSUPPORTED;
    }

    SerialPrint("[PE] PE32+ valid: SizeOfImage=0x%x, PreferredImageBase=0x%p, EntryPointRVA=0x%x, Sections=%u\n",
        opt_hdr->SizeOfImage, opt_hdr->ImageBase, opt_hdr->AddressOfEntryPoint, file_hdr->NumberOfSections);

    // Allocate physical memory for image
    EFI_PHYSICAL_ADDRESS LoadedImageBase = 0;
    UINTN ImagePages = EFI_SIZE_TO_PAGES(opt_hdr->SizeOfImage);
    EFI_STATUS Status = IxAllocatePages(AllocateAnyPages, EfiLoaderCode, ImagePages, &LoadedImageBase);
    if (EFI_ERROR(Status)) {
        SerialPrint("[PE] Failed to allocate %u pages for loaded image: 0x%x\n", ImagePages, Status);
        return Status;
    }

    SerialPrint("[PE] Loaded image allocated at GPA 0x%p (%u pages)\n", LoadedImageBase, ImagePages);

    // Copy headers
    UINT8 *dst = (UINT8 *)LoadedImageBase;
    const UINT8 *src = (const UINT8 *)StagingBase;
    IxCopyMem(dst, (VOID *)src, opt_hdr->SizeOfHeaders);

    // Map sections
    const IMAGE_SECTION_HEADER *sections = (const IMAGE_SECTION_HEADER *)((const UINT8 *)opt_hdr + file_hdr->SizeOfOptionalHeader);
    for (UINT32 s = 0; s < file_hdr->NumberOfSections; s++) {
        UINT8 *sec_dest = dst + sections[s].VirtualAddress;
        UINT32 virt_size = sections[s].Misc.VirtualSize;
        UINT32 raw_size = sections[s].SizeOfRawData;

        // Zero out section virtual space
        IxSetMem(sec_dest, virt_size, 0);

        // Copy raw section data
        UINT32 copy_size = (virt_size < raw_size) ? virt_size : raw_size;
        if (copy_size > 0 && sections[s].PointerToRawData > 0) {
            const UINT8 *sec_src = src + sections[s].PointerToRawData;
            IxCopyMem(sec_dest, (VOID *)sec_src, copy_size);
        }
    }

    // Apply base relocations if loaded at a different address than preferred
    INT64 Delta = (INT64)LoadedImageBase - (INT64)opt_hdr->ImageBase;
    if (Delta != 0 && opt_hdr->NumberOfRvaAndSizes > 5 && opt_hdr->DataDirectory[5].Size > 0) {
        UINT32 reloc_rva = opt_hdr->DataDirectory[5].VirtualAddress;
        UINT32 reloc_size = opt_hdr->DataDirectory[5].Size;
        UINT8 *reloc_ptr = dst + reloc_rva;
        UINT8 *reloc_end = reloc_ptr + reloc_size;

        SerialPrint("[PE] Applying base relocations (Delta = 0x%p, RelocSize = %u)...\n", Delta, reloc_size);

        while (reloc_ptr < reloc_end) {
            const IMAGE_BASE_RELOCATION *base_reloc = (const IMAGE_BASE_RELOCATION *)reloc_ptr;
            if (base_reloc->SizeOfBlock == 0) break;

            UINT32 count = (base_reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(UINT16);
            const UINT16 *entries = (const UINT16 *)(reloc_ptr + sizeof(IMAGE_BASE_RELOCATION));

            for (UINT32 j = 0; j < count; j++) {
                UINT16 entry = entries[j];
                UINT16 type = entry >> 12;
                UINT16 offset = entry & 0xFFF;

                if (type == 10) { // IMAGE_REL_BASED_DIR64
                    UINT64 *patch_addr = (UINT64 *)(dst + base_reloc->VirtualAddress + offset);
                    *patch_addr += Delta;
                }
            }

            reloc_ptr += base_reloc->SizeOfBlock;
        }
        SerialPrint("[PE] Base relocations applied successfully\n");
    }

    // Allocate EFI_LOADED_IMAGE_PROTOCOL
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImageProto = NULL;
    Status = IxAllocatePool(EfiLoaderData, sizeof(EFI_LOADED_IMAGE_PROTOCOL), (VOID **)&LoadedImageProto);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    LoadedImageProto->Revision        = 0x1000;
    LoadedImageProto->ParentHandle    = NULL;
    LoadedImageProto->SystemTable     = &gST;
    LoadedImageProto->DeviceHandle    = NULL;
    LoadedImageProto->FilePath        = NULL;
    LoadedImageProto->Reserved        = NULL;
    LoadedImageProto->ImageBase       = (VOID *)LoadedImageBase;
    LoadedImageProto->ImageSize       = opt_hdr->SizeOfImage;
    LoadedImageProto->ImageCodeType   = EfiLoaderCode;
    LoadedImageProto->ImageDataType   = EfiLoaderData;
    LoadedImageProto->Unload          = NULL;

    // Convert ASCII cmdline to UTF-16LE
    UINTN cmd_len = g_HostBootInfo->CmdlineSize;
    if (cmd_len > 0) {
        CHAR16 *utf16_cmd = NULL;
        Status = IxAllocatePool(EfiLoaderData, (cmd_len + 1) * sizeof(CHAR16), (VOID **)&utf16_cmd);
        if (!EFI_ERROR(Status)) {
            for (UINTN k = 0; k < cmd_len; k++) {
                utf16_cmd[k] = (CHAR16)(UINT8)g_HostBootInfo->CmdlineString[k];
            }
            utf16_cmd[cmd_len] = 0;
            LoadedImageProto->LoadOptions = utf16_cmd;
            LoadedImageProto->LoadOptionsSize = (UINT32)((cmd_len + 1) * sizeof(CHAR16));
        }
    } else {
        LoadedImageProto->LoadOptions = NULL;
        LoadedImageProto->LoadOptionsSize = 0;
    }

    EFI_HANDLE KernelHandle = NULL;
    EFI_GUID LoadedImageGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    Status = IxInstallProtocolInterface(&KernelHandle, &LoadedImageGuid, 0, LoadedImageProto);
    if (EFI_ERROR(Status)) {
        SerialPrint("[PE] Failed to install LoadedImageProtocol on kernel handle: 0x%x\n", Status);
        return Status;
    }

    *OutImageHandle = KernelHandle;
    *OutEntryPoint  = (EFI_IMAGE_ENTRY_POINT)(LoadedImageBase + opt_hdr->AddressOfEntryPoint);

    SerialPrint("[PE] Kernel image ready: Handle=0x%p, EntryPoint=0x%p\n", *OutImageHandle, *OutEntryPoint);
    return EFI_SUCCESS;
}
