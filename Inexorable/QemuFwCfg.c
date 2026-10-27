#include "QemuFwCfg.h"
#include "inexorable_layout.h"
#include "Serial.h"

#define FW_CFG_CTL_PORT         0x510
#define FW_CFG_DATA_PORT        0x511
#define FW_CFG_DMA_PORT         0x514

#define FW_CFG_SIGNATURE        0x0000
#define FW_CFG_ID               0x0001
#define FW_CFG_NB_CPUS          0x0005
#define FW_CFG_KERNEL_SIZE      0x0008
#define FW_CFG_INITRD_SIZE      0x000B
#define FW_CFG_KERNEL_DATA      0x0011
#define FW_CFG_INITRD_DATA      0x0012
#define FW_CFG_CMDLINE_SIZE     0x0014
#define FW_CFG_CMDLINE_DATA     0x0015
#define FW_CFG_SETUP_SIZE       0x0017
#define FW_CFG_SETUP_DATA       0x0018

#define FW_CFG_DMA_CTL_ERROR    0x01
#define FW_CFG_DMA_CTL_READ     0x02
#define FW_CFG_DMA_CTL_SKIP     0x04
#define FW_CFG_DMA_CTL_SELECT   0x08
#define FW_CFG_DMA_CTL_WRITE    0x10

#pragma pack(push, 1)
typedef struct {
    UINT32 Control;
    UINT32 Length;
    UINT64 Address;
} FW_CFG_DMA_ACCESS;
#pragma pack(pop)

static inline void outb(UINT16 port, UINT8 val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(UINT16 port, UINT16 val) {
    __asm__ __volatile__("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outl(UINT16 port, UINT32 val) {
    __asm__ __volatile__("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline UINT8 inb(UINT16 port) {
    UINT8 ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline UINT32 inl(UINT16 port) {
    UINT32 ret;
    __asm__ __volatile__("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline UINT32 bswap32(UINT32 val) {
    return __builtin_bswap32(val);
}

static inline UINT64 bswap64(UINT64 val) {
    return __builtin_bswap64(val);
}

static void fw_cfg_select(UINT16 key) {
    outw(FW_CFG_CTL_PORT, key);
}

static void fw_cfg_read_bytes(void *buf, UINTN len) {
    UINT8 *p = (UINT8 *)buf;
    for (UINTN i = 0; i < len; i++) {
        p[i] = inb(FW_CFG_DATA_PORT);
    }
}

static BOOLEAN g_DmaSupported = FALSE;

BOOLEAN QemuFwCfgDetect(void) {
    fw_cfg_select(FW_CFG_SIGNATURE);
    char sig[4];
    fw_cfg_read_bytes(sig, 4);
    if (sig[0] == 'Q' && sig[1] == 'E' && sig[2] == 'M' && sig[3] == 'U') {
        SerialPrint("[FW_CFG] QEMU firmware configuration interface detected\n");
        fw_cfg_select(FW_CFG_ID);
        UINT32 id = 0;
        fw_cfg_read_bytes(&id, 4);
        if (id & 0x02) {
            g_DmaSupported = TRUE;
            SerialPrint("[FW_CFG] High-speed DMA interface supported\n");
        }
        return TRUE;
    }
    return FALSE;
}

static BOOLEAN fw_cfg_dma_transfer(UINT16 key, void *dest, UINT32 length) {
    if (!g_DmaSupported) {
        return FALSE;
    }

    volatile FW_CFG_DMA_ACCESS dma;
    dma.Control = bswap32(((UINT32)key << 16) | FW_CFG_DMA_CTL_SELECT | FW_CFG_DMA_CTL_READ);
    dma.Length  = bswap32(length);
    dma.Address = bswap64((UINT64)(uintptr_t)dest);

    UINT64 dma_addr = (UINT64)(uintptr_t)&dma;
    UINT32 high = bswap32((UINT32)(dma_addr >> 32));
    UINT32 low  = bswap32((UINT32)dma_addr);

    // Write 64-bit physical address of descriptor in big-endian to port 0x514 (high) and 0x518 (low)
    outl(FW_CFG_DMA_PORT, high);
    outl(FW_CFG_DMA_PORT + 4, low);

    // Wait until completed with timeout
    UINT32 timeout = 100000000;
    while ((bswap32(dma.Control) & ~FW_CFG_DMA_CTL_ERROR) && --timeout) {
        __asm__ __volatile__("pause");
    }

    if (timeout == 0) {
        SerialPrint("[FW_CFG] DMA timeout waiting for completion of key 0x%x\n", key);
        return FALSE;
    }

    if (bswap32(dma.Control) & FW_CFG_DMA_CTL_ERROR) {
        SerialPrint("[FW_CFG] DMA error during transfer of key 0x%x\n", key);
        return FALSE;
    }

    return TRUE;
}

EFI_STATUS QemuFwCfgFetchBootInfo(void) {
    if (!QemuFwCfgDetect()) {
        return EFI_NOT_FOUND;
    }

    // CPU count
    fw_cfg_select(FW_CFG_NB_CPUS);
    UINT16 cpu_count = 0;
    fw_cfg_read_bytes(&cpu_count, 2);
    g_HostBootInfo->CpuCount = cpu_count;

    // Command line size & string
    fw_cfg_select(FW_CFG_CMDLINE_SIZE);
    UINT32 cmd_size = 0;
    fw_cfg_read_bytes(&cmd_size, 4);
    if (cmd_size > 0 && cmd_size < sizeof(g_HostBootInfo->CmdlineString)) {
        fw_cfg_select(FW_CFG_CMDLINE_DATA);
        fw_cfg_read_bytes(g_HostBootInfo->CmdlineString, cmd_size);
        g_HostBootInfo->CmdlineString[cmd_size] = '\0';
        g_HostBootInfo->CmdlineSize = cmd_size;
    } else {
        g_HostBootInfo->CmdlineSize = 0;
        g_HostBootInfo->CmdlineString[0] = '\0';
    }

    // Kernel size & data (SetupData + KernelData = complete bzImage / PE32+ kernel)
    fw_cfg_select(FW_CFG_SETUP_SIZE);
    UINT32 setup_size = 0;
    fw_cfg_read_bytes(&setup_size, 4);

    fw_cfg_select(FW_CFG_KERNEL_SIZE);
    UINT32 kernel_size = 0;
    fw_cfg_read_bytes(&kernel_size, 4);

    UINT32 total_kernel_size = setup_size + kernel_size;
    g_HostBootInfo->KernelSize = total_kernel_size;

    if (total_kernel_size > 0) {
        SerialPrint("[FW_CFG] Transferring kernel (%u bytes: setup=%u, payload=%u) to GPA 0x%p via DMA...\n",
            total_kernel_size, setup_size, kernel_size, GUEST_KERNEL_STAGE_BASE);
        UINT8 *dest = (UINT8 *)GUEST_KERNEL_STAGE_BASE;
        if (setup_size > 0) {
            if (!fw_cfg_dma_transfer(FW_CFG_SETUP_DATA, dest, setup_size)) {
                fw_cfg_select(FW_CFG_SETUP_DATA);
                fw_cfg_read_bytes(dest, setup_size);
            }
        }
        if (kernel_size > 0) {
            if (!fw_cfg_dma_transfer(FW_CFG_KERNEL_DATA, dest + setup_size, kernel_size)) {
                fw_cfg_select(FW_CFG_KERNEL_DATA);
                fw_cfg_read_bytes(dest + setup_size, kernel_size);
            }
        }
        SerialPrint("[FW_CFG] Kernel transferred successfully\n");
    }

    // Initrd size & data
    fw_cfg_select(FW_CFG_INITRD_SIZE);
    UINT32 initrd_size = 0;
    fw_cfg_read_bytes(&initrd_size, 4);
    g_HostBootInfo->InitrdSize = initrd_size;

    if (initrd_size > 0) {
        SerialPrint("[FW_CFG] Transferring initrd (%u bytes) to GPA 0x%p via DMA...\n",
            initrd_size, GUEST_INITRD_STAGE_BASE);
        if (!fw_cfg_dma_transfer(FW_CFG_INITRD_DATA, (void *)GUEST_INITRD_STAGE_BASE, initrd_size)) {
            // Fallback to port I/O read
            fw_cfg_select(FW_CFG_INITRD_DATA);
            fw_cfg_read_bytes((void *)GUEST_INITRD_STAGE_BASE, initrd_size);
        }
        SerialPrint("[FW_CFG] Initrd transferred successfully\n");
    }

    SerialPrint("[FW_CFG] Boot info ready: CPUs=%u, KernelSize=%u, InitrdSize=%u, Cmdline=\"%s\"\n",
        g_HostBootInfo->CpuCount, g_HostBootInfo->KernelSize, g_HostBootInfo->InitrdSize, g_HostBootInfo->CmdlineString);

    return EFI_SUCCESS;
}
