#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define SIZE_4KB                        0x00001000ULL
#define SIZE_4MB                        0x00400000ULL
#define SIZE_1GB                        0x40000000ULL

#define GUEST_ADDRESS_SPACE_SIZE        SIZE_1GB
#define MAXIMUM_GUEST_MEMORY_PAGES      (GUEST_ADDRESS_SPACE_SIZE >> 12)

#define GUEST_FIRMWARE_IMAGE_BASE       SIZE_1GB
#define IX_HOST_BOOT_INFO_BASE          (SIZE_1GB + 0x0001F000ULL)
#define ACPI_RSDP_BASE                  (SIZE_1GB + 0x00020000ULL)
#define ACPI_XSDT_BASE                  (SIZE_1GB + 0x00021000ULL)
#define ACPI_MADT_BASE                  (SIZE_1GB + 0x00022000ULL)
#define ACPI_FADT_BASE                  (SIZE_1GB + 0x00023000ULL)
#define ACPI_MCFG_BASE                  (SIZE_1GB + 0x00024000ULL)
#define ACPI_DSDT_BASE                  (SIZE_1GB + 0x00025000ULL)

#define GUEST_RUNTIME_STORE_BASE        (SIZE_1GB + 0x00030000ULL)
#define GUEST_KERNEL_STAGE_BASE         (SIZE_1GB + 0x00100000ULL) // 0x40100000 (32 MB)
#define GUEST_INITRD_STAGE_BASE         (SIZE_1GB + 0x02100000ULL) // 0x42100000 (64 MB)

#define BSP_INITIAL_STACK_SIZE          0x4000
#define HOST_STACK_SIZE                 0x4000
#define HOST_RESERVED_HEAP_SIZE         SIZE_4MB
#define CONFIG_CPU_COUNT                4

// Serial console ports
#define COM1_PORT                       0x3F8
#define DEBUGCON_PORT                   0x402
#define QEMU_DEBUGCON_PORT              0xE9

#pragma pack(push, 1)
typedef struct {
    uint64_t  KernelAddress;
    uint64_t  KernelSize;
    uint64_t  InitrdAddress;
    uint64_t  InitrdSize;
    uint32_t  CpuCount;
    uint32_t  CmdlineSize;
    char      CmdlineString[1024];
    uint64_t  RamSize;
} IX_HOST_BOOT_INFO;
#pragma pack(pop)

extern IX_HOST_BOOT_INFO *g_HostBootInfo;
