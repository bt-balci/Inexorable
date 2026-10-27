#include "Acpi.h"
#include "inexorable_layout.h"
#include "Serial.h"
#include "Protocol.h"

#pragma pack(push, 1)

// Standard ACPI Table Header
typedef struct {
    CHAR8   Signature[4];
    UINT32  Length;
    UINT8   Revision;
    UINT8   Checksum;
    CHAR8   OemId[6];
    CHAR8   OemTableId[8];
    UINT32  OemRevision;
    CHAR8   CreatorId[4];
    UINT32  CreatorRevision;
} ACPI_HEADER;

// ACPI 2.0+ RSDP
typedef struct {
    CHAR8   Signature[8];       // "RSD PTR "
    UINT8   Checksum;
    CHAR8   OemId[6];
    UINT8   Revision;           // 2 for ACPI 2.0+
    UINT32  RsdtAddress;        // 32-bit physical address
    UINT32  Length;             // Total structure length (36)
    UINT64  XsdtAddress;        // 64-bit physical address
    UINT8   ExtendedChecksum;
    UINT8   Reserved[3];
} ACPI_RSDP;

// XSDT Table
typedef struct {
    ACPI_HEADER Header;
    UINT64      Entry[3];       // Pointers to FADT, MADT, MCFG
} ACPI_XSDT;

// Generic Address Structure (GAS)
typedef struct {
    UINT8   AddressSpaceId;     // 0=Memory, 1=System I/O, etc.
    UINT8   RegisterBitWidth;
    UINT8   RegisterBitOffset;
    UINT8   AccessSize;
    UINT64  Address;
} ACPI_GAS;

// FADT Revision 6
typedef struct {
    ACPI_HEADER Header;
    UINT32      FirmwareCtrl;
    UINT32      Dsdt;
    UINT8       Reserved1;
    UINT8       PreferredPmProfile;
    UINT16      SciInt;
    UINT32      SmiCmd;
    UINT8       AcpiEnable;
    UINT8       AcpiDisable;
    UINT8       S4BiosReq;
    UINT8       PstateCnt;
    UINT32      Pm1aEvtBlk;
    UINT32      Pm1bEvtBlk;
    UINT32      Pm1aCntBlk;
    UINT32      Pm1bCntBlk;
    UINT32      Pm2CntBlk;
    UINT32      PmTmrBlk;
    UINT32      Gpe0Blk;
    UINT32      Gpe1Blk;
    UINT8       Pm1EvtLen;
    UINT8       Pm1CntLen;
    UINT8       Pm2CntLen;
    UINT8       PmTmrLen;
    UINT8       Gpe0BlkLen;
    UINT8       Gpe1BlkLen;
    UINT8       Gpe1Base;
    UINT8       CstCnt;
    UINT16      PLvl2Lat;
    UINT16      PLvl3Lat;
    UINT16      FlushSize;
    UINT16      FlushStride;
    UINT8       DutyOffset;
    UINT8       DutyWidth;
    UINT8       DayAlrm;
    UINT8       MonAlrm;
    UINT8       Century;
    UINT16      IaPcBootArch;
    UINT8       Reserved2;
    UINT32      Flags;          // Bit 20: HW_REDUCED_ACPI, Bit 10: RESET_REG_SUP
    ACPI_GAS    ResetReg;
    UINT8       ResetValue;
    UINT16      ArmBootArch;
    UINT8       MinorVersion;
    UINT64      X_FirmwareCtrl;
    UINT64      X_Dsdt;
    ACPI_GAS    X_Pm1aEvtBlk;
    ACPI_GAS    X_Pm1bEvtBlk;
    ACPI_GAS    X_Pm1aCntBlk;
    ACPI_GAS    X_Pm1bCntBlk;
    ACPI_GAS    X_Pm2CntBlk;
    ACPI_GAS    X_PmTmrBlk;
    ACPI_GAS    X_Gpe0Blk;
    ACPI_GAS    X_Gpe1Blk;
    ACPI_GAS    SleepControlReg;
    ACPI_GAS    SleepStatusReg;
    UINT64      HypervisorVendorIdentity;
} ACPI_FADT;

// MADT Table
typedef struct {
    ACPI_HEADER Header;
    UINT32      LocalApicAddress;   // 0xFEE00000
    UINT32      Flags;              // 1 = PCAT_COMPAT
} ACPI_MADT_HEADER;

typedef struct {
    UINT8   Type;                   // 0 = Processor Local APIC
    UINT8   Length;                 // 8
    UINT8   AcpiProcessorId;
    UINT8   ApicId;
    UINT32  Flags;                  // 1 = Enabled
} ACPI_LOCAL_APIC;

typedef struct {
    UINT8   Type;                   // 1 = I/O APIC
    UINT8   Length;                 // 12
    UINT8   IoApicId;
    UINT8   Reserved;
    UINT32  IoApicAddress;          // 0xFEC00000
    UINT32  GlobalSystemInterruptBase; // 0
} ACPI_IOAPIC;

// MCFG Table
typedef struct {
    ACPI_HEADER Header;
    UINT64      Reserved;
    struct {
        UINT64  BaseAddress;        // 0xB0000000
        UINT16  PciSegmentGroup;    // 0
        UINT8   StartBusNumber;     // 0
        UINT8   EndBusNumber;       // 0
        UINT32  Reserved;
    } ConfigSpace[1];
} ACPI_MCFG;

#pragma pack(pop)

static void ComputeChecksum(void *table, UINT32 length, UINT32 checksum_offset) {
    UINT8 *bytes = (UINT8 *)table;
    bytes[checksum_offset] = 0;
    UINT8 sum = 0;
    for (UINT32 i = 0; i < length; i++) {
        sum += bytes[i];
    }
    bytes[checksum_offset] = (UINT8)(0 - sum);
}

static void InitHeader(ACPI_HEADER *hdr, const char *sig, UINT32 len, UINT8 rev) {
    hdr->Signature[0] = sig[0];
    hdr->Signature[1] = sig[1];
    hdr->Signature[2] = sig[2];
    hdr->Signature[3] = sig[3];
    hdr->Length = len;
    hdr->Revision = rev;
    hdr->Checksum = 0;
    hdr->OemId[0] = 'I'; hdr->OemId[1] = 'N'; hdr->OemId[2] = 'E';
    hdr->OemId[3] = 'X'; hdr->OemId[4] = 'O'; hdr->OemId[5] = 'R';
    hdr->OemTableId[0] = 'V'; hdr->OemTableId[1] = 'M'; hdr->OemTableId[2] = 'F';
    hdr->OemTableId[3] = 'I'; hdr->OemTableId[4] = 'R'; hdr->OemTableId[5] = 'M';
    hdr->OemTableId[6] = 'W'; hdr->OemTableId[7] = ' ';
    hdr->OemRevision = 1;
    hdr->CreatorId[0] = 'I'; hdr->CreatorId[1] = 'N'; hdr->CreatorId[2] = 'E'; hdr->CreatorId[3] = 'X';
    hdr->CreatorRevision = 1;
}

EFI_STATUS AcpiInit(void) {
    SerialPrint("[ACPI] Building Hardware-Reduced Static ACPI Tables at 0x%p...\n", ACPI_RSDP_BASE);

    // 1. DSDT at ACPI_DSDT_BASE (0x40025000)
    // Embed minimal valid AML table with Scope(\_SB)
    UINT8 *dsdt_ptr = (UINT8 *)ACPI_DSDT_BASE;
    ACPI_HEADER *dsdt_hdr = (ACPI_HEADER *)dsdt_ptr;
    // Standard 36-byte header + 6-byte Scope(_SB) AML
    // ScopeOp (0x10), PkgLength (0x05), '_', 'S', 'B', '_'
    const UINT8 aml_scope[] = { 0x10, 0x05, 0x5F, 0x53, 0x42, 0x5F };
    UINT32 dsdt_len = sizeof(ACPI_HEADER) + sizeof(aml_scope);

    // Zero out memory
    for (UINT32 i = 0; i < 0x1000; i++) dsdt_ptr[i] = 0;

    InitHeader(dsdt_hdr, "DSDT", dsdt_len, 2);
    for (UINT32 i = 0; i < sizeof(aml_scope); i++) {
        dsdt_ptr[sizeof(ACPI_HEADER) + i] = aml_scope[i];
    }
    ComputeChecksum(dsdt_ptr, dsdt_len, offsetof(ACPI_HEADER, Checksum));

    // 2. FADT at ACPI_FADT_BASE (0x40023000)
    ACPI_FADT *fadt = (ACPI_FADT *)ACPI_FADT_BASE;
    UINT8 *fadt_raw = (UINT8 *)fadt;
    for (UINT32 i = 0; i < 0x1000; i++) fadt_raw[i] = 0;

    InitHeader(&fadt->Header, "FACP", sizeof(ACPI_FADT), 6);
    // RESET_REGISTER_SUPPORTED (bit 10)
    fadt->Flags = (1 << 10);
    fadt->Pm1aEvtBlk = 0x600;
    fadt->Pm1EvtLen = 4;
    fadt->Pm1aCntBlk = 0x604;
    fadt->Pm1CntLen = 2;
    fadt->PmTmrBlk = 0x608;
    fadt->PmTmrLen = 4;

    fadt->X_Pm1aEvtBlk.AddressSpaceId = 1; // System I/O
    fadt->X_Pm1aEvtBlk.Address = 0x600;
    fadt->X_Pm1aEvtBlk.RegisterBitWidth = 32;

    fadt->X_Pm1aCntBlk.AddressSpaceId = 1; // System I/O
    fadt->X_Pm1aCntBlk.Address = 0x604;
    fadt->X_Pm1aCntBlk.RegisterBitWidth = 16;

    fadt->X_PmTmrBlk.AddressSpaceId = 1; // System I/O
    fadt->X_PmTmrBlk.Address = 0x608;
    fadt->X_PmTmrBlk.RegisterBitWidth = 32;

    fadt->ResetReg.AddressSpaceId = 1; // System I/O
    fadt->ResetReg.RegisterBitWidth = 8;
    fadt->ResetReg.RegisterBitOffset = 0;
    fadt->ResetReg.AccessSize = 1;
    fadt->ResetReg.Address = 0xCF9;     // PCI reset register
    fadt->ResetValue = 0x06;
    fadt->X_Dsdt = ACPI_DSDT_BASE;
    fadt->Dsdt = (UINT32)ACPI_DSDT_BASE;
    ComputeChecksum(fadt, sizeof(ACPI_FADT), offsetof(ACPI_HEADER, Checksum));

    // 3. MADT at ACPI_MADT_BASE (0x40022000)
    UINT8 *madt_raw = (UINT8 *)ACPI_MADT_BASE;
    for (UINT32 i = 0; i < 0x1000; i++) madt_raw[i] = 0;

    ACPI_MADT_HEADER *madt = (ACPI_MADT_HEADER *)madt_raw;
    madt->LocalApicAddress = 0xFEE00000;
    madt->Flags = 1; // PCAT_COMPAT

    UINT32 madt_offset = sizeof(ACPI_MADT_HEADER);

    // Local APIC structures (one for each actual CPU detected via fw_cfg)
    UINT32 cpu_count = (g_HostBootInfo && g_HostBootInfo->CpuCount > 0) ? g_HostBootInfo->CpuCount : 1;
    if (cpu_count > 64) cpu_count = 64;

    for (UINT32 c = 0; c < cpu_count; c++) {
        ACPI_LOCAL_APIC *lapic = (ACPI_LOCAL_APIC *)(madt_raw + madt_offset);
        lapic->Type = 0; // Local APIC
        lapic->Length = sizeof(ACPI_LOCAL_APIC);
        lapic->AcpiProcessorId = (UINT8)c;
        lapic->ApicId = (UINT8)c;
        lapic->Flags = 1; // Enabled
        madt_offset += sizeof(ACPI_LOCAL_APIC);
    }

    // 1 I/O APIC structure
    ACPI_IOAPIC *ioapic = (ACPI_IOAPIC *)(madt_raw + madt_offset);
    ioapic->Type = 1; // I/O APIC
    ioapic->Length = sizeof(ACPI_IOAPIC);
    ioapic->IoApicId = 0;
    ioapic->Reserved = 0;
    ioapic->IoApicAddress = 0xFEC00000;
    ioapic->GlobalSystemInterruptBase = 0;
    madt_offset += sizeof(ACPI_IOAPIC);

    InitHeader(&madt->Header, "APIC", madt_offset, 4);
    ComputeChecksum(madt_raw, madt_offset, offsetof(ACPI_HEADER, Checksum));

    // 4. MCFG at ACPI_MCFG_BASE (0x40024000)
    ACPI_MCFG *mcfg = (ACPI_MCFG *)ACPI_MCFG_BASE;
    UINT8 *mcfg_raw = (UINT8 *)mcfg;
    for (UINT32 i = 0; i < 0x1000; i++) mcfg_raw[i] = 0;

    InitHeader(&mcfg->Header, "MCFG", sizeof(ACPI_MCFG), 1);
    mcfg->ConfigSpace[0].BaseAddress = 0xB0000000; // Standard MMCONFIG window
    mcfg->ConfigSpace[0].PciSegmentGroup = 0;
    mcfg->ConfigSpace[0].StartBusNumber = 0;
    mcfg->ConfigSpace[0].EndBusNumber = 0;
    ComputeChecksum(mcfg, sizeof(ACPI_MCFG), offsetof(ACPI_HEADER, Checksum));

    // 5. XSDT at ACPI_XSDT_BASE (0x40021000)
    ACPI_XSDT *xsdt = (ACPI_XSDT *)ACPI_XSDT_BASE;
    UINT8 *xsdt_raw = (UINT8 *)xsdt;
    for (UINT32 i = 0; i < 0x1000; i++) xsdt_raw[i] = 0;

    InitHeader(&xsdt->Header, "XSDT", sizeof(ACPI_XSDT), 1);
    xsdt->Entry[0] = ACPI_FADT_BASE;
    xsdt->Entry[1] = ACPI_MADT_BASE;
    xsdt->Entry[2] = ACPI_MCFG_BASE;
    ComputeChecksum(xsdt, sizeof(ACPI_XSDT), offsetof(ACPI_HEADER, Checksum));

    // 6. RSDP at ACPI_RSDP_BASE (0x40020000)
    ACPI_RSDP *rsdp = (ACPI_RSDP *)ACPI_RSDP_BASE;
    UINT8 *rsdp_raw = (UINT8 *)rsdp;
    for (UINT32 i = 0; i < 0x1000; i++) rsdp_raw[i] = 0;

    rsdp->Signature[0] = 'R'; rsdp->Signature[1] = 'S'; rsdp->Signature[2] = 'D'; rsdp->Signature[3] = ' ';
    rsdp->Signature[4] = 'P'; rsdp->Signature[5] = 'T'; rsdp->Signature[6] = 'R'; rsdp->Signature[7] = ' ';
    rsdp->OemId[0] = 'I'; rsdp->OemId[1] = 'N'; rsdp->OemId[2] = 'E';
    rsdp->OemId[3] = 'X'; rsdp->OemId[4] = 'O'; rsdp->OemId[5] = 'R';
    rsdp->Revision = 2; // ACPI 2.0+
    rsdp->RsdtAddress = 0;
    rsdp->Length = sizeof(ACPI_RSDP); // 36 bytes
    rsdp->XsdtAddress = ACPI_XSDT_BASE;

    // Checksum 1: first 20 bytes (ACPI 1.0 fields)
    ComputeChecksum(rsdp, 20, offsetof(ACPI_RSDP, Checksum));

    // Checksum 2: all 36 bytes (Extended checksum)
    ComputeChecksum(rsdp, sizeof(ACPI_RSDP), offsetof(ACPI_RSDP, ExtendedChecksum));

    // Register EFI_ACPI_20_TABLE_GUID in gST.ConfigurationTable
    EFI_GUID Acpi20Guid = EFI_ACPI_20_TABLE_GUID;
    EFI_STATUS Status = IxInstallConfigurationTable(&Acpi20Guid, (VOID *)ACPI_RSDP_BASE);
    if (EFI_ERROR(Status)) {
        SerialPrint("[ACPI] Failed to register ACPI 2.0 table in ConfigurationTable: 0x%x\n", Status);
        return Status;
    }

    SerialPrint("[ACPI] Static ACPI tables installed successfully (RSDP=0x%p, XSDT=0x%p, FADT=0x%p)\n",
        ACPI_RSDP_BASE, ACPI_XSDT_BASE, ACPI_FADT_BASE);
    return EFI_SUCCESS;
}
