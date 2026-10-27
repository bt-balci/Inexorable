#include "Memory.h"
#include "Serial.h"
#include "inexorable_layout.h"

#define MAX_RAM_SIZE            (4ULL * 1024 * 1024 * 1024) // 4 GB maximum physical space managed
#define MAX_TOTAL_PAGES         (MAX_RAM_SIZE >> 12)        // 1,048,576 pages
#define BITMAP_BYTES            (MAX_TOTAL_PAGES / 8)       // 131,072 bytes (128 KB)
#define MAX_ALLOCATIONS         512
#define PAGE_BITMAP_BASE        0x00050000ULL

typedef struct {
    EFI_PHYSICAL_ADDRESS Start;
    UINTN                Pages;
    EFI_MEMORY_TYPE      Type;
    BOOLEAN              InUse;
} ALLOCATION_RECORD;

static UINT8 *g_PageBitmap = (UINT8 *)PAGE_BITMAP_BASE;
static ALLOCATION_RECORD g_Allocations[MAX_ALLOCATIONS];
UINTN g_MemoryMapKey = 1;
static UINTN g_TotalPages = (SIZE_1GB >> 12);

static inline void set_bit(UINTN bit) {
    if (bit < MAX_TOTAL_PAGES) {
        g_PageBitmap[bit >> 3] |= (1 << (bit & 7));
    }
}

static inline void clear_bit(UINTN bit) {
    if (bit < MAX_TOTAL_PAGES) {
        g_PageBitmap[bit >> 3] &= ~(1 << (bit & 7));
    }
}

static inline int test_bit(UINTN bit) {
    if (bit >= MAX_TOTAL_PAGES) return 1;
    return (g_PageBitmap[bit >> 3] & (1 << (bit & 7))) != 0;
}

static inline EFI_MEMORY_TYPE get_page_type(UINTN page) {
    if (!test_bit(page)) {
        return EfiConventionalMemory;
    }
    EFI_PHYSICAL_ADDRESS addr = (EFI_PHYSICAL_ADDRESS)page << 12;
    for (UINTN i = 0; i < MAX_ALLOCATIONS; i++) {
        if (g_Allocations[i].InUse &&
            addr >= g_Allocations[i].Start &&
            addr < (g_Allocations[i].Start + (g_Allocations[i].Pages << 12))) {
            return g_Allocations[i].Type;
        }
    }
    return (addr == 0) ? EfiReservedMemoryType : EfiLoaderData;
}

void MemoryInit(void) {
    UINT64 ram_size = (g_HostBootInfo && g_HostBootInfo->RamSize > 0) ? g_HostBootInfo->RamSize : SIZE_1GB;
    if (ram_size > MAX_RAM_SIZE) ram_size = MAX_RAM_SIZE;
    g_TotalPages = (UINTN)(ram_size >> 12);

    // Zero out bitmap
    for (UINTN i = 0; i < BITMAP_BYTES; i++) {
        g_PageBitmap[i] = 0;
    }

    // Zero out allocation records
    for (UINTN i = 0; i < MAX_ALLOCATIONS; i++) {
        g_Allocations[i].InUse = FALSE;
    }

    // 1. Reserve Page 0..15 (0x00000 - 0x10000: IVT, BDA, Page Tables at 0x1000/0x2000)
    for (UINTN p = 0; p < 16; p++) set_bit(p);
    g_Allocations[0].Start = 0;
    g_Allocations[0].Pages = 16;
    g_Allocations[0].Type = EfiReservedMemoryType;
    g_Allocations[0].InUse = TRUE;

    // 2. Reserve Stack at 0x70000 - 0x80000 (Pages 112..127: 64KB)
    for (UINTN p = 112; p < 128; p++) set_bit(p);
    g_Allocations[1].Start = 112 << 12;
    g_Allocations[1].Pages = 16;
    g_Allocations[1].Type = EfiBootServicesData;
    g_Allocations[1].InUse = TRUE;

    // 2b. Reserve Page Bitmap at 0x50000 - 0x70000 (Pages 80..111: 128KB) - Reclaimed by OS!
    for (UINTN p = 80; p < 112; p++) set_bit(p);
    g_Allocations[8].Start = 80 << 12;
    g_Allocations[8].Pages = 32;
    g_Allocations[8].Type = EfiBootServicesData;
    g_Allocations[8].InUse = TRUE;

    // 3. Reserve 0xA0000 - 0x100000 (Pages 160..255: Video framebuffer, Option ROMs, BIOS)
    for (UINTN p = 160; p < 256; p++) set_bit(p);
    g_Allocations[2].Start = 160 << 12;
    g_Allocations[2].Pages = 256 - 160;
    g_Allocations[2].Type = EfiReservedMemoryType;
    g_Allocations[2].InUse = TRUE;

    // 4. Reserve Inexorable Firmware (0x40000000 - 0x40080000, 512 KB, 128 pages)
    UINTN fw_start_page = (UINTN)(GUEST_FIRMWARE_IMAGE_BASE >> 12);
    for (UINTN p = fw_start_page; p < fw_start_page + 128; p++) set_bit(p);
    g_Allocations[3].Start = GUEST_FIRMWARE_IMAGE_BASE;
    g_Allocations[3].Pages = 128;
    g_Allocations[3].Type = EfiRuntimeServicesCode;
    g_Allocations[3].InUse = TRUE;

    // 5. Reserve ACPI Tables (0x40020000 - 0x40026000, 24 KB, 6 pages)
    UINTN acpi_start_page = (UINTN)(ACPI_RSDP_BASE >> 12);
    for (UINTN p = acpi_start_page; p < acpi_start_page + 6; p++) set_bit(p);
    g_Allocations[4].Start = ACPI_RSDP_BASE;
    g_Allocations[4].Pages = 6;
    g_Allocations[4].Type = EfiACPIReclaimMemory;
    g_Allocations[4].InUse = TRUE;

    // 6. Reserve Runtime Variable Store (0x40030000 - 0x40040000, 64 KB, 16 pages)
    UINTN rt_start_page = (UINTN)(GUEST_RUNTIME_STORE_BASE >> 12);
    for (UINTN p = rt_start_page; p < rt_start_page + 16; p++) set_bit(p);
    g_Allocations[5].Start = GUEST_RUNTIME_STORE_BASE;
    g_Allocations[5].Pages = 16;
    g_Allocations[5].Type = EfiRuntimeServicesData;
    g_Allocations[5].InUse = TRUE;

    // 7. Staged Kernel (0x40100000 - 0x42100000, 32 MB, 8192 pages) - EfiBootServicesData (Reclaimed by OS!)
    UINTN kstage_start_page = (UINTN)(GUEST_KERNEL_STAGE_BASE >> 12);
    for (UINTN p = kstage_start_page; p < kstage_start_page + 8192; p++) set_bit(p);
    g_Allocations[6].Start = GUEST_KERNEL_STAGE_BASE;
    g_Allocations[6].Pages = 8192;
    g_Allocations[6].Type = EfiBootServicesData;
    g_Allocations[6].InUse = TRUE;

    // 8. Staged Initrd (Only if preloaded in RAM by custom VMM)
    if (g_HostBootInfo && g_HostBootInfo->InitrdAddress != 0 && g_HostBootInfo->InitrdSize > 0) {
        UINTN istage_start_page = (UINTN)(g_HostBootInfo->InitrdAddress >> 12);
        UINTN istage_pages = EFI_SIZE_TO_PAGES(g_HostBootInfo->InitrdSize);
        for (UINTN p = istage_start_page; p < istage_start_page + istage_pages; p++) set_bit(p);
        g_Allocations[7].Start = g_HostBootInfo->InitrdAddress;
        g_Allocations[7].Pages = istage_pages;
        g_Allocations[7].Type = EfiBootServicesData;
        g_Allocations[7].InUse = TRUE;
    }

    SerialPrint("[MEM] Memory manager initialized: %u MB physical RAM (%u total pages, %u pages reserved)\n",
        (UINT32)(ram_size >> 20), g_TotalPages, 16 + 16 + 96 + 128 + 6 + 16 + 8192 + 16384);
}

static ALLOCATION_RECORD* record_alloc(EFI_PHYSICAL_ADDRESS start, UINTN pages, EFI_MEMORY_TYPE type) {
    for (UINTN i = 0; i < MAX_ALLOCATIONS; i++) {
        if (!g_Allocations[i].InUse) {
            g_Allocations[i].Start = start;
            g_Allocations[i].Pages = pages;
            g_Allocations[i].Type = type;
            g_Allocations[i].InUse = TRUE;
            return &g_Allocations[i];
        }
    }
    return NULL;
}

static void remove_alloc(EFI_PHYSICAL_ADDRESS start) {
    for (UINTN i = 0; i < MAX_ALLOCATIONS; i++) {
        if (g_Allocations[i].InUse && g_Allocations[i].Start == start) {
            g_Allocations[i].InUse = FALSE;
            break;
        }
    }
}

EFI_STATUS EFIAPI IxAllocatePages(
    IN     EFI_ALLOCATE_TYPE    Type,
    IN     EFI_MEMORY_TYPE      MemoryType,
    IN     UINTN                Pages,
    IN OUT EFI_PHYSICAL_ADDRESS *Memory
) {
    if (Pages == 0 || Memory == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    SerialPrint("[MEM] AllocatePages: Type=%u, MemType=%u, Pages=%u, Addr=0x%p\n",
        Type, MemoryType, Pages, *Memory);

    UINTN FoundPage = g_TotalPages;

    if (Type == AllocateAddress) {
        UINTN StartPage = (UINTN)(*Memory >> 12);
        if (StartPage + Pages > g_TotalPages) {
            return EFI_OUT_OF_RESOURCES;
        }
        for (UINTN i = 0; i < Pages; i++) {
            if (test_bit(StartPage + i)) {
                return EFI_NOT_AVAILABLE_YET;
            }
        }
        FoundPage = StartPage;
    } else if (Type == AllocateMaxAddress) {
        UINTN MaxPage = (UINTN)(*Memory >> 12);
        if (MaxPage > g_TotalPages) MaxPage = g_TotalPages;

        // Search backward from MaxPage
        if (MaxPage >= Pages) {
            for (INTN p = (INTN)(MaxPage - Pages); p >= 256; p--) {
                BOOLEAN Free = TRUE;
                for (UINTN i = 0; i < Pages; i++) {
                    if (test_bit((UINTN)p + i)) {
                        Free = FALSE;
                        break;
                    }
                }
                if (Free) {
                    FoundPage = (UINTN)p;
                    break;
                }
            }
        }
    } else { // AllocateAnyPages
        // Search forward starting from page 256 (1MB mark)
        for (UINTN p = 256; p + Pages <= g_TotalPages; p++) {
            BOOLEAN Free = TRUE;
            for (UINTN i = 0; i < Pages; i++) {
                if (test_bit(p + i)) {
                    p += i; // Skip forward
                    Free = FALSE;
                    break;
                }
            }
            if (Free) {
                FoundPage = p;
                break;
            }
        }
    }

    if (FoundPage == g_TotalPages) {
        SerialPrint("[MEM] Out of pages! Requested %u pages, type %u\n", Pages, MemoryType);
        return EFI_OUT_OF_RESOURCES;
    }

    for (UINTN i = 0; i < Pages; i++) {
        set_bit(FoundPage + i);
    }

    *Memory = (EFI_PHYSICAL_ADDRESS)(FoundPage << 12);
    record_alloc(*Memory, Pages, MemoryType);
    g_MemoryMapKey++;

    SerialPrint("[MEM] AllocatePages: Succeeded -> 0x%p\n", *Memory);
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI IxFreePages(
    IN EFI_PHYSICAL_ADDRESS Memory,
    IN UINTN                Pages
) {
    if ((Memory & EFI_PAGE_MASK) != 0 || Pages == 0) {
        return EFI_INVALID_PARAMETER;
    }

    UINTN StartPage = (UINTN)(Memory >> 12);
    if (StartPage + Pages > g_TotalPages) {
        return EFI_INVALID_PARAMETER;
    }

    for (UINTN i = 0; i < Pages; i++) {
        clear_bit(StartPage + i);
    }

    remove_alloc(Memory);
    g_MemoryMapKey++;

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI IxAllocatePool(
    IN  EFI_MEMORY_TYPE PoolType,
    IN  UINTN           Size,
    OUT VOID            **Buffer
) {
    if (Buffer == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    UINTN TotalSize = Size + 16;
    UINTN Pages = EFI_SIZE_TO_PAGES(TotalSize);
    EFI_PHYSICAL_ADDRESS Phys = 0;

    EFI_STATUS Status = IxAllocatePages(AllocateAnyPages, PoolType, Pages, &Phys);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    UINT64 *Header = (UINT64 *)Phys;
    Header[0] = 0x504F4F4CULL; // "POOL"
    Header[1] = Pages;

    *Buffer = (VOID *)(Phys + 16);
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI IxFreePool(
    IN VOID *Buffer
) {
    if (Buffer == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    UINT64 *Header = (UINT64 *)((UINT8 *)Buffer - 16);
    if (Header[0] != 0x504F4F4CULL) {
        return EFI_INVALID_PARAMETER;
    }

    UINTN Pages = Header[1];
    return IxFreePages((EFI_PHYSICAL_ADDRESS)Header, Pages);
}

EFI_STATUS EFIAPI IxGetMemoryMap(
    IN OUT UINTN                 *MemoryMapSize,
    OUT    EFI_MEMORY_DESCRIPTOR *MemoryMap,
    OUT    UINTN                 *MapKey,
    OUT    UINTN                 *DescriptorSize,
    OUT    UINT32                *DescriptorVersion
) {
    if (MemoryMapSize == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    // 1. Count normalized contiguous spans across the entire physical address space
    UINTN DescCount = 0;
    UINTN CurrentPage = 0;
    while (CurrentPage < g_TotalPages) {
        EFI_MEMORY_TYPE CurrentType = get_page_type(CurrentPage);
        UINTN SpanPages = 1;
        while (CurrentPage + SpanPages < g_TotalPages && get_page_type(CurrentPage + SpanPages) == CurrentType) {
            SpanPages++;
        }
        DescCount++;
        CurrentPage += SpanPages;
    }

    UINTN RequiredSize = DescCount * sizeof(EFI_MEMORY_DESCRIPTOR);

    if (DescriptorSize)    *DescriptorSize = sizeof(EFI_MEMORY_DESCRIPTOR);
    if (DescriptorVersion) *DescriptorVersion = 1;
    if (MapKey)            *MapKey = g_MemoryMapKey;

    if (*MemoryMapSize < RequiredSize || MemoryMap == NULL) {
        *MemoryMapSize = RequiredSize;
        return EFI_BUFFER_TOO_SMALL;
    }

    // 2. Populate contiguous descriptors
    UINTN OutIndex = 0;
    CurrentPage = 0;
    while (CurrentPage < g_TotalPages) {
        EFI_MEMORY_TYPE CurrentType = get_page_type(CurrentPage);
        UINTN SpanPages = 1;
        while (CurrentPage + SpanPages < g_TotalPages && get_page_type(CurrentPage + SpanPages) == CurrentType) {
            SpanPages++;
        }

        EFI_MEMORY_DESCRIPTOR *Desc = &MemoryMap[OutIndex++];
        Desc->Type = CurrentType;
        Desc->PhysicalStart = (EFI_PHYSICAL_ADDRESS)(CurrentPage << 12);
        Desc->VirtualStart = Desc->PhysicalStart;
        Desc->NumberOfPages = SpanPages;
        Desc->Attribute = EFI_MEMORY_WB;

        if (CurrentType == EfiRuntimeServicesCode || CurrentType == EfiRuntimeServicesData) {
            Desc->Attribute |= EFI_MEMORY_RUNTIME;
        }

        CurrentPage += SpanPages;
    }

    *MemoryMapSize = OutIndex * sizeof(EFI_MEMORY_DESCRIPTOR);
    SerialPrint("[MEM] GetMemoryMap: Succeeded, %u descriptors, MapKey=%u\n", OutIndex, *MapKey);
    return EFI_SUCCESS;
}
