#pragma once
#include <Uefi.h>

#define ALIGN_UP_8(x) (((x) + 7) & ~7)

#define MULTIBOOT_TAG_TYPE_END               0
#define MULTIBOOT_TAG_TYPE_CMDLINE           1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME  2
#define MULTIBOOT_TAG_TYPE_MODULE            3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO     4
#define MULTIBOOT_TAG_TYPE_BOOTDEV           5
#define MULTIBOOT_TAG_TYPE_MMAP              6
#define MULTIBOOT_TAG_TYPE_VBE               7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER       8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS      9
#define MULTIBOOT_TAG_TYPE_APM               10
#define MULTIBOOT_TAG_TYPE_EFI32             11
#define MULTIBOOT_TAG_TYPE_EFI64             12
#define MULTIBOOT_TAG_TYPE_SMBIOS            13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD          14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW          15
#define MULTIBOOT_TAG_TYPE_NETWORK           16
#define MULTIBOOT_TAG_TYPE_EFI_MMAP          17
#define MULTIBOOT_TAG_TYPE_EFI_BS            18

#define MULTIBOOT_HEADER_TAG_END  0
#define MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST  1
#define MULTIBOOT_HEADER_TAG_ADDRESS  2
#define MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS  3
#define MULTIBOOT_HEADER_TAG_CONSOLE_FLAGS  4
#define MULTIBOOT_HEADER_TAG_FRAMEBUFFER  5
#define MULTIBOOT_HEADER_TAG_MODULE_ALIGN  6
#define MULTIBOOT_HEADER_TAG_EFI_BS  7

#pragma pack(push, 1)
typedef struct {
    UINT32 Type;
    UINT16 Flags;
    UINT32 Size;
} MULTIBOOT2_HEADER_TAG;

typedef struct {
    UINT16 Type;
    UINT16 Flags;
    UINT32 Size;
    UINT32 Requests[0];
} MULTIBOOT2_HEADER_TAG_INFORMATION_REQUEST;

typedef struct {
    UINT32 Type;
    UINT32 Size;
    UINT8 Rsdp[0];
} MULTIBOOT2_TAG_OLD_ACPI;

typedef struct {
    UINT16 Type;
    UINT16 Flags;
    UINT32 Size;
    UINT32 HeaderAddress;
    UINT32 LoadAddress;
    UINT32 LoadEndAddress;
    UINT32 BssEndAddress;
} MULTIBOOT2_HEADER_TAG_ADDRESS;

#define MULTIBOOT_MEMORY_AVAILABLE          1
#define MULTIBOOT_MEMORY_RESERVED		    2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE   3
#define MULTIBOOT_MEMORY_NVS                4
#define MULTIBOOT_MEMORY_BADRAM             5

typedef struct {
    UINT64 Address;
    UINT64 Length;
    UINT32 Type;
    UINT32 Zero;
} MULTIBOOT_MMAP_ENTRY;

typedef struct {
    UINT32 Type;
    UINT32 Size;
    UINT32 EntrySize;
    UINT32 EntryVersion;
    MULTIBOOT_MMAP_ENTRY Entries[0];
} MULTIBOOT2_TAG_MMAP;

typedef struct {
    UINT32 Type;
    UINT32 Size;
    UINT32 Num;
    UINT32 Entsize;
    UINT32 Shndx;
    UINT8 Sections[0];
} MULTIBOOT2_TAG_ELF_SECTIONS;

typedef struct {
    UINT32 Type;
    UINT32 Size;
} MULTIBOOT2_INFO_TAG;

typedef struct {
    UINT32 TotalSize;
    UINT32 Reserved;
} MULTIBOOT2_INFO;

typedef struct {
    UINT32 Magic;
    UINT32 Architecture;
    UINT32 HeaderLength;
    UINT32 Checksum;
} MULTIBOOT2_HEADER;
#pragma pack(pop)

VOID
Multiboot2Boot(
    VOID* Entry, 
    VOID* MultibootInfoAddress
);