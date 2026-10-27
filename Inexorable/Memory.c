#include "Memory.h"
#include "Serial.h"
#include "inexorable_layout.h"

#define TOTAL_PAGES (GUEST_ADDRESS_SPACE_SIZE >> 12) // 262144 pages for 1GB
#define BITMAP_BYTES (TOTAL_PAGES / 8)               // 32768 bytes
#define MAX_ALLOCATIONS 512

typedef struct {
    EFI_PHYSICAL_ADDRESS Start;
    UINTN                Pages;
    EFI_MEMORY_TYPE      Type;
    BOOLEAN              InUse;
} ALLOCATION_RECORD;

static UINT8 g_PageBitmap[BITMAP_BYTES];
static ALLOCATION_RECORD g_Allocations[MAX_ALLOCATIONS];
UINTN g_MemoryMapKey = 1;

static inline void set_bit(UINTN bit) {
    g_PageBitmap[bit >> 3] |= (1 << (bit & 7));
}

static inline void clear_bit(UINTN bit) {
    g_PageBitmap[bit >> 3] &= ~(1 << (bit & 7));
}

static inline int test_bit(UINTN bit) {
    return (g_PageBitmap[bit >> 3] & (1 << (bit & 7))) != 0;
}

void MemoryInit(void) {
    // Zero out bitmap
    for (UINTN i = 0; i < BITMAP_BYTES; i++) {
        g_PageBitmap[i] = 0;
    }

    // Zero out allocation records
    for (UINTN i = 0; i < MAX_ALLOCATIONS; i++) {
        g_Allocations[i].InUse = FALSE;
    }

    // 1. Reserve Page 0..15 (0x00000 - 0x10000: IVT, BDA, Page Tables at 0x1000/0x2000)
    for (UINTN p = 0; p < 16; p++) {
        set_bit(p);
    }
    g_Allocations[0].Start = 0;
    g_Allocations[0].Pages = 16;
    g_Allocations[0].Type = EfiReservedMemoryType;
    g_Allocations[0].InUse = TRUE;

    // 2. Reserve Stack at 0x70000 - 0x80000 (Pages 112..127: 64KB)
    for (UINTN p = 112; p < 128; p++) {
        set_bit(p);
    }
    g_Allocations[1].Start = 112 << 12;
    g_Allocations[1].Pages = 16;
    g_Allocations[1].Type = EfiBootServicesData;
    g_Allocations[1].InUse = TRUE;

    // 3. Reserve 0xA0000 - 0x100000 (Pages 160..255: Video framebuffer, Option ROMs, BIOS)
    for (UINTN p = 160; p < 256; p++) {
        set_bit(p);
    }
    g_Allocations[2].Start = 160 << 12;
    g_Allocations[2].Pages = 256 - 160;
    g_Allocations[2].Type = EfiReservedMemoryType;
    g_Allocations[2].InUse = TRUE;

    SerialPrint("[MEM] Memory manager initialized: 1GB RAM (%u pages reserved in low 1MB, %u pages free)\n",
        16 + 16 + 96, TOTAL_PAGES - (16 + 16 + 96));
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

    UINTN FoundPage = TOTAL_PAGES;

    if (Type == AllocateAddress) {
        UINTN StartPage = (UINTN)(*Memory >> 12);
        if (StartPage + Pages > TOTAL_PAGES) {
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
        if (MaxPage > TOTAL_PAGES) MaxPage = TOTAL_PAGES;

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
        for (UINTN p = 256; p + Pages <= TOTAL_PAGES; p++) {
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

    if (FoundPage == TOTAL_PAGES) {
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
    if (StartPage + Pages > TOTAL_PAGES) {
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

    // Fixed descriptors:
    // 1. Low reserved 1MB
    // 2. Allocations array
    // 3. Free conventional memory spans
    // 4. Inexorable firmware at 0x40000000 (128 KB) -> EfiBootServicesCode
    // 5. ACPI tables at 0x40020000 (24 KB)         -> EfiACPIReclaimMemory
    // 6. Runtime store at 0x40030000 (64 KB)        -> EfiRuntimeServicesData
    // 7. Staged kernel at 0x40100000 (32 MB)        -> EfiLoaderData
    // 8. Staged initrd at 0x42100000 (64 MB)        -> EfiLoaderData

    // We can emit a normalized map by scanning the 1GB range in chunks.
    // Count required descriptors:
    UINTN DescCount = 0;

    // Scan the bitmap to count contiguous allocated and free spans
    UINTN CurrentPage = 0;
    while (CurrentPage < TOTAL_PAGES) {
        int IsAlloc = test_bit(CurrentPage);
        UINTN SpanPages = 1;
        while (CurrentPage + SpanPages < TOTAL_PAGES && test_bit(CurrentPage + SpanPages) == IsAlloc) {
            SpanPages++;
        }
        DescCount++;
        CurrentPage += SpanPages;
    }

    // Add the 5 out-of-band fixed entries above 1GB
    DescCount += 5;

    UINTN RequiredSize = DescCount * sizeof(EFI_MEMORY_DESCRIPTOR);

    if (DescriptorSize)    *DescriptorSize = sizeof(EFI_MEMORY_DESCRIPTOR);
    if (DescriptorVersion) *DescriptorVersion = 1;
    if (MapKey)            *MapKey = g_MemoryMapKey;

    if (*MemoryMapSize < RequiredSize || MemoryMap == NULL) {
        *MemoryMapSize = RequiredSize;
        return EFI_BUFFER_TOO_SMALL;
    }

    // Populate descriptors
    UINTN OutIndex = 0;
    CurrentPage = 0;
    while (CurrentPage < TOTAL_PAGES) {
        int IsAlloc = test_bit(CurrentPage);
        UINTN SpanPages = 1;
        while (CurrentPage + SpanPages < TOTAL_PAGES && test_bit(CurrentPage + SpanPages) == IsAlloc) {
            SpanPages++;
        }

        EFI_MEMORY_DESCRIPTOR *Desc = &MemoryMap[OutIndex++];
        Desc->PhysicalStart = (EFI_PHYSICAL_ADDRESS)(CurrentPage << 12);
        Desc->VirtualStart = Desc->PhysicalStart;
        Desc->NumberOfPages = SpanPages;
        Desc->Attribute = EFI_MEMORY_WB;

        if (!IsAlloc) {
            Desc->Type = EfiConventionalMemory;
        } else {
            // Check if this matches a specific allocation record
            Desc->Type = EfiLoaderData; // default
            for (UINTN i = 0; i < MAX_ALLOCATIONS; i++) {
                if (g_Allocations[i].InUse && g_Allocations[i].Start == Desc->PhysicalStart) {
                    Desc->Type = g_Allocations[i].Type;
                    break;
                }
            }
            if (Desc->PhysicalStart == 0) {
                Desc->Type = EfiReservedMemoryType;
            }
        }

        CurrentPage += SpanPages;
    }

    // Out-of-band fixed descriptors above 1GB:
    // Inexorable Firmware (512 KB)
    {
        EFI_MEMORY_DESCRIPTOR *Desc = &MemoryMap[OutIndex++];
        Desc->Type = EfiRuntimeServicesCode;
        Desc->PhysicalStart = GUEST_FIRMWARE_IMAGE_BASE;
        Desc->VirtualStart = Desc->PhysicalStart;
        Desc->NumberOfPages = EFI_SIZE_TO_PAGES(512 * 1024); // 512 KB
        Desc->Attribute = EFI_MEMORY_WB | EFI_MEMORY_RUNTIME;
    }

    // ACPI Tables (0x40020000 - 0x40026000, 24 KB)
    {
        EFI_MEMORY_DESCRIPTOR *Desc = &MemoryMap[OutIndex++];
        Desc->Type = EfiACPIReclaimMemory;
        Desc->PhysicalStart = ACPI_RSDP_BASE;
        Desc->VirtualStart = Desc->PhysicalStart;
        Desc->NumberOfPages = EFI_SIZE_TO_PAGES(0x6000); // 24 KB
        Desc->Attribute = EFI_MEMORY_WB;
    }

    // Runtime Store (0x40030000 - 0x40040000, 64 KB)
    {
        EFI_MEMORY_DESCRIPTOR *Desc = &MemoryMap[OutIndex++];
        Desc->Type = EfiRuntimeServicesData;
        Desc->PhysicalStart = GUEST_RUNTIME_STORE_BASE;
        Desc->VirtualStart = Desc->PhysicalStart;
        Desc->NumberOfPages = EFI_SIZE_TO_PAGES(0x10000); // 64 KB
        Desc->Attribute = EFI_MEMORY_WB | EFI_MEMORY_RUNTIME;
    }

    // Staged Kernel (0x40100000 - 0x42100000, 32 MB)
    {
        EFI_MEMORY_DESCRIPTOR *Desc = &MemoryMap[OutIndex++];
        Desc->Type = EfiLoaderData;
        Desc->PhysicalStart = GUEST_KERNEL_STAGE_BASE;
        Desc->VirtualStart = Desc->PhysicalStart;
        Desc->NumberOfPages = EFI_SIZE_TO_PAGES(32 * 1024 * 1024);
        Desc->Attribute = EFI_MEMORY_WB;
    }

    // Staged Initrd (0x42100000 - 0x46100000, 64 MB)
    {
        EFI_MEMORY_DESCRIPTOR *Desc = &MemoryMap[OutIndex++];
        Desc->Type = EfiLoaderData;
        Desc->PhysicalStart = GUEST_INITRD_STAGE_BASE;
        Desc->VirtualStart = Desc->PhysicalStart;
        Desc->NumberOfPages = EFI_SIZE_TO_PAGES(64 * 1024 * 1024);
        Desc->Attribute = EFI_MEMORY_WB;
    }

    *MemoryMapSize = OutIndex * sizeof(EFI_MEMORY_DESCRIPTOR);
    SerialPrint("[MEM] GetMemoryMap: Succeeded, %u descriptors, MapKey=%u\n", OutIndex, *MapKey);
    return EFI_SUCCESS;
}
