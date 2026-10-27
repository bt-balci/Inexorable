#include "efi_spec.h"
#include "inexorable_layout.h"
#include "Serial.h"
#include "Memory.h"
#include "Protocol.h"
#include "Variable.h"
#include "Acpi.h"
#include "InitrdLoadFile2.h"
#include "RngProtocol.h"
#include "PeLoader.h"
#include "QemuFwCfg.h"

// External prototypes from Misc.c
EFI_STATUS EFIAPI IxExitBootServices(EFI_HANDLE ImageHandle, UINTN MapKey);
VOID EFIAPI IxResetSystem(INT32 ResetType, EFI_STATUS ResetStatus, UINTN DataSize, VOID *ResetData);
EFI_STATUS EFIAPI IxStall(UINTN Microseconds);
EFI_STATUS EFIAPI IxSetWatchdogTimer(UINTN Timeout, UINT64 WatchdogCode, UINTN DataSize, CHAR16 *WatchdogData);
VOID EFIAPI IxCopyMem(VOID *Destination, VOID *Source, UINTN Length);
VOID EFIAPI IxSetMem(VOID *Buffer, UINTN Size, UINT8 Value);
EFI_STATUS EFIAPI IxCalculateCrc32(VOID *Data, UINTN DataSize, UINT32 *Crc32);

// External prototypes from VirtualMemory.c
EFI_STATUS EFIAPI IxSetVirtualAddressMap(UINTN MemoryMapSize, UINTN DescriptorSize, UINT32 DescriptorVersion, EFI_MEMORY_DESCRIPTOR *VirtualMap);
EFI_STATUS EFIAPI IxConvertPointer(UINTN DebugDisposition, VOID **Address);

// External prototypes from Time.c
EFI_STATUS EFIAPI IxGetTime(VOID *Time, VOID *Capabilities);
EFI_STATUS EFIAPI IxSetTime(VOID *Time);
EFI_STATUS EFIAPI IxGetWakeupTime(BOOLEAN *Enabled, BOOLEAN *Pending, VOID *Time);
EFI_STATUS EFIAPI IxSetWakeupTime(BOOLEAN Enable, VOID *Time);

const CHAR8 *gEfiCallerBaseName = "Inexorable";

IX_HOST_BOOT_INFO *g_HostBootInfo = (IX_HOST_BOOT_INFO *)IX_HOST_BOOT_INFO_BASE;

static EFI_STATUS EFIAPI IxConReset(VOID *This, BOOLEAN ExtendedVerification) {
    (void)This;
    (void)ExtendedVerification;
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI IxConOutputString(VOID *This, CHAR16 *String) {
    (void)This;
    if (!String) return EFI_SUCCESS;
    while (*String) {
        CHAR16 ch = *String++;
        if (ch == L'\n') {
            SerialPutc('\r');
        }
        SerialPutc((char)(ch & 0x7F));
    }
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI IxConClearScreen(VOID *This) {
    (void)This;
    return EFI_SUCCESS;
}

#pragma pack(push, 1)
typedef struct {
    EFI_STATUS (EFIAPI *Reset)(VOID *This, BOOLEAN ExtendedVerification);
    EFI_STATUS (EFIAPI *OutputString)(VOID *This, CHAR16 *String);
    VOID *TestString;
    VOID *QueryMode;
    VOID *SetMode;
    VOID *SetAttribute;
    EFI_STATUS (EFIAPI *ClearScreen)(VOID *This);
    VOID *SetCursorPosition;
    VOID *EnableCursor;
    VOID *Mode;
} IX_TEXT_OUTPUT_PROTOCOL;
#pragma pack(pop)

static IX_TEXT_OUTPUT_PROTOCOL g_ConOut = {
    .Reset = IxConReset,
    .OutputString = IxConOutputString,
    .TestString = NULL,
    .QueryMode = NULL,
    .SetMode = NULL,
    .SetAttribute = NULL,
    .ClearScreen = IxConClearScreen,
    .SetCursorPosition = NULL,
    .EnableCursor = NULL,
    .Mode = NULL
};

EFI_SYSTEM_TABLE     gST = { 0 };
EFI_BOOT_SERVICES    gBS = { 0 };
EFI_RUNTIME_SERVICES gRT = { 0 };

static EFI_RT_PROPERTIES_TABLE g_RtProperties = {
    .Version = 0x0001,
    .Length  = sizeof(EFI_RT_PROPERTIES_TABLE),
    .RuntimeServicesSupported = 0x00000000 // Informs Linux no runtime calls needed!
};

static inline UINT64 rdtsc(void) {
    UINT32 lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((UINT64)hi << 32) | lo;
}

static EFI_STATUS EFIAPI IxUnsupportedStub(void) {
    return EFI_UNSUPPORTED;
}

static EFI_TPL EFIAPI IxRaiseTPL(EFI_TPL NewTpl) {
    (void)NewTpl;
    return 4; // TPL_APPLICATION
}

static VOID EFIAPI IxRestoreTPL(EFI_TPL OldTpl) {
    (void)OldTpl;
}

static BOOLEAN IsDebugMode(void) {
    const char *cmd = g_HostBootInfo->CmdlineString;
    if (!cmd) return FALSE;
    while (*cmd) {
        if (cmd[0] == 'd' && cmd[1] == 'e' && cmd[2] == 'b' && cmd[3] == 'u' && cmd[4] == 'g') {
            return TRUE;
        }
        cmd++;
    }
    return FALSE;
}

void InexorableInit(void) {
    UINT64 T0 = rdtsc();

    SerialInit();
    SerialPrint("\n");
    SerialPrint("====================================================================\n");
    SerialPrint("   INEXORABLE: Zero-Enumeration Direct-Long-Mode UEFI Firmware\n");
    SerialPrint("====================================================================\n");
    SerialPrint("[TELEMETRY] T0 (Firmware Reset -> Long Mode Entry): %u cycles\n", T0);

    // Initialize Host Boot Info structure
    // If not running under custom VMM, detect QEMU and fetch via fw_cfg DMA
    QemuFwCfgFetchBootInfo();

    if (IsDebugMode()) {
        SerialPrint("\n");
        SerialPrint("  ___                               _     _      \n");
        SerialPrint(" |_ _|_ __   _____  _____  _ __ __ _| |__ | | ___ \n");
        SerialPrint("  | || '_ \\ / _ \\ \\/ / _ \\| '__/ _` | '_ \\| |/ _ \\\n");
        SerialPrint("  | || | | |  __/>  < (_) | | | (_| | |_) | |  __/\n");
        SerialPrint(" |___|_| |_|\\___/_/\\_\\___/|_|  \\__,_|_.__/|_|\\___|\n");
        SerialPrint(" [FIRMWARE DEBUG MODE ACTIVE - FULL HARDWARE TELEMETRY]\n\n");
        SerialPrint("[DEBUG] Active vCPUs: %u (Local APIC IDs 0..%u)\n", g_HostBootInfo->CpuCount, g_HostBootInfo->CpuCount - 1);
        SerialPrint("[DEBUG] Physical RAM: %u MB (Page Allocator Bitmap @ 0x00050000)\n",
            (UINT32)(g_HostBootInfo->RamSize / (1024 * 1024)));
        SerialPrint("[DEBUG] High-Speed fw_cfg DMA: Port 0x514 Active (32-bit/64-bit DMA)\n");
        SerialPrint("[DEBUG] ACPI 2.0+ Layout: RSDP=0x%p, XSDT=0x%p, FADT=0x%p, MADT=0x%p, MCFG=0x%p\n",
            ACPI_RSDP_BASE, ACPI_XSDT_BASE, ACPI_FADT_BASE, ACPI_MADT_BASE, ACPI_MCFG_BASE);
        SerialPrint("[DEBUG] PCIe ECAM Window: Base 0xB0000000 (Length 256MB, Buses 0..255)\n");
        SerialPrint("[DEBUG] Kernel Stage GPA: 0x%p (Size: %u bytes, 100%% Reclaimable)\n",
            GUEST_KERNEL_STAGE_BASE, (UINT32)g_HostBootInfo->KernelSize);
        SerialPrint("[DEBUG] Initrd Size: %u bytes (LINUX_EFI_INITRD_MEDIA_GUID On-Demand DMA)\n\n",
            (UINT32)g_HostBootInfo->InitrdSize);
    }

    // 1. Initialize Autonomous Physical Memory Allocator
    MemoryInit();

    // 2. Setup System Table and Services
    gST.Hdr.Signature = EFI_SYSTEM_TABLE_SIGNATURE;
    gST.Hdr.Revision = EFI_SYSTEM_TABLE_REVISION;
    gST.Hdr.HeaderSize = sizeof(EFI_SYSTEM_TABLE);
    gST.BootServices = &gBS;
    gST.RuntimeServices = &gRT;

    // Boot Services
    gBS.Hdr.Signature = EFI_BOOT_SERVICES_SIGNATURE;
    gBS.Hdr.Revision = EFI_BOOT_SERVICES_REVISION;
    gBS.Hdr.HeaderSize = sizeof(EFI_BOOT_SERVICES);

    VOID **bs_ptrs = (VOID **)&gBS;
    for (UINTN i = sizeof(EFI_TABLE_HEADER)/sizeof(VOID *); i < sizeof(EFI_BOOT_SERVICES)/sizeof(VOID *); i++) {
        bs_ptrs[i] = (VOID *)IxUnsupportedStub;
    }

    gBS.RaiseTPL                = IxRaiseTPL;
    gBS.RestoreTPL              = IxRestoreTPL;
    gBS.AllocatePages           = IxAllocatePages;
    gBS.FreePages               = IxFreePages;
    gBS.GetMemoryMap            = IxGetMemoryMap;
    gBS.AllocatePool            = IxAllocatePool;
    gBS.FreePool                = IxFreePool;
    gBS.ExitBootServices        = IxExitBootServices;
    gBS.Stall                   = IxStall;
    gBS.SetWatchdogTimer        = IxSetWatchdogTimer;
    gBS.CopyMem                 = IxCopyMem;
    gBS.SetMem                  = IxSetMem;
    gBS.CalculateCrc32          = IxCalculateCrc32;

    gBS.HandleProtocol          = IxHandleProtocol;
    gBS.OpenProtocol            = (VOID *)IxOpenProtocol;
    gBS.CloseProtocol           = (VOID *)IxCloseProtocol;
    gBS.LocateProtocol          = IxLocateProtocol;
    gBS.LocateHandle            = IxLocateHandle;
    gBS.LocateHandleBuffer      = (VOID *)IxLocateHandleBuffer;
    gBS.LocateDevicePath        = (VOID *)IxLocateDevicePath;
    gBS.InstallProtocolInterface   = (VOID *)IxInstallProtocolInterface;
    gBS.UninstallProtocolInterface = (VOID *)IxUninstallProtocolInterface;
    gBS.InstallConfigurationTable  = IxInstallConfigurationTable;

    // Runtime Services
    gRT.Hdr.Signature = EFI_RUNTIME_SERVICES_SIGNATURE;
    gRT.Hdr.Revision = EFI_RUNTIME_SERVICES_REVISION;
    gRT.Hdr.HeaderSize = sizeof(EFI_RUNTIME_SERVICES);

    VOID **rt_ptrs = (VOID **)&gRT;
    for (UINTN i = sizeof(EFI_TABLE_HEADER)/sizeof(VOID *); i < sizeof(EFI_RUNTIME_SERVICES)/sizeof(VOID *); i++) {
        rt_ptrs[i] = (VOID *)IxUnsupportedStub;
    }

    gRT.GetTime                 = (VOID *)IxGetTime;
    gRT.SetTime                 = (VOID *)IxSetTime;
    gRT.GetWakeupTime           = (VOID *)IxGetWakeupTime;
    gRT.SetWakeupTime           = (VOID *)IxSetWakeupTime;
    gRT.SetVirtualAddressMap    = (VOID *)IxSetVirtualAddressMap;
    gRT.ConvertPointer          = (VOID *)IxConvertPointer;
    gRT.GetVariable             = IxGetVariable;
    gRT.SetVariable             = IxSetVariable;
    gRT.GetNextVariableName     = IxGetNextVariableName;
    gRT.QueryVariableInfo       = IxQueryVariableInfo;
    gRT.ResetSystem             = IxResetSystem;

    // 3. Initialize Protocol and Variable Subsystems
    ProtocolInit();
    VariableInit();

    gST.ConsoleOutHandle = (EFI_HANDLE)0x1005;
    gST.ConOut = &g_ConOut;
    gST.StandardErrorHandle = (EFI_HANDLE)0x1005;
    gST.StdErr = &g_ConOut;

    EFI_GUID TextOutGuid = { 0x387477c2, 0x69c7, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } };
    IxInstallProtocolInterface(&gST.ConsoleOutHandle, &TextOutGuid, 0, &g_ConOut);

    // 4. Install EFI_RT_PROPERTIES_TABLE
    EFI_GUID RtPropertiesGuid = EFI_RT_PROPERTIES_TABLE_GUID;
    IxInstallConfigurationTable(&RtPropertiesGuid, &g_RtProperties);

    // 5. Build and Install Static Hardware-Reduced ACPI Tables
    AcpiInit();

    // 6. Install Direct Memory Initrd Protocol (LOAD_FILE2)
    InitrdProtocolInit();

    // 7. Install Hardware RNG Protocol (rdrand)
    RngProtocolInit();

    UINT64 T1 = rdtsc();
    SerialPrint("[TELEMETRY] T1 (Firmware Core & Protocols Ready): %u cycles (+%u cycles from T0)\n",
        T1, T1 - T0);

    // 8. Load Kernel Image via PE32+ Loader
    EFI_HANDLE KernelHandle = NULL;
    EFI_IMAGE_ENTRY_POINT EntryPoint = NULL;

    SerialPrint("[BOOT] Loading 64-bit PE32+ Linux Kernel from GPA 0x%p...\n", GUEST_KERNEL_STAGE_BASE);
    EFI_STATUS Status = IxLoadPeImage((VOID *)GUEST_KERNEL_STAGE_BASE, &KernelHandle, &EntryPoint);
    if (EFI_ERROR(Status)) {
        SerialPrint("[FATAL] IxLoadPeImage failed with status 0x%x\n", Status);
        while (1) __asm__ __volatile__("cli; hlt");
    }

    UINT64 T2 = rdtsc();
    SerialPrint("[TELEMETRY] T2 (Kernel Mapped & Relocated): %u cycles (+%u cycles from T1)\n",
        T2, T2 - T1);

    SerialPrint("[BOOT] Jumping directly to 64-bit Linux EntryPoint 0x%p in Long Mode...\n", EntryPoint);
    if (IsDebugMode()) {
        SerialPrint("[DEBUG] Linux EFI Stub Handoff Architecture:\n");
        SerialPrint("  ├─ ABI Convention:     Microsoft x64 (RCX = Handle, RDX = SystemTable)\n");
        SerialPrint("  ├─ Target EntryPoint:  0x%p (Long Mode 64-bit)\n", EntryPoint);
        SerialPrint("  ├─ EFI System Table:   0x%p (Revision 2.80)\n", &gST);
        SerialPrint("  ├─ Boot Services:      0x%p\n", &gBS);
        SerialPrint("  └─ ExitBootServices:   Reclaims 100%% of firmware staging buffers\n");
    }
    SerialPrint("--------------------------------------------------------------------\n");

    // Call Linux CONFIG_EFI_STUB entry point directly in Long Mode!
    // Per Microsoft x64 ABI: arguments are passed in RCX (ImageHandle), RDX (SystemTable)
    Status = EntryPoint(KernelHandle, &gST);

    SerialPrint("[BOOT] Kernel returned with status: 0x%x\n", Status);
    while (1) {
        __asm__ __volatile__("cli; hlt");
    }
}

// Entry points satisfying both MSBuild linker and direct jump
EFI_STATUS EFIAPI UefiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    (void)ImageHandle;
    (void)SystemTable;
    InexorableInit();
    return EFI_SUCCESS;
}

void InexorableMain(void) {
    InexorableInit();
}
