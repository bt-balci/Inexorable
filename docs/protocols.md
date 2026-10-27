# EFI Subsystem & Protocol Implementations

This document describes how Inexorable implements an autonomous, spec-compliant **UEFI 2.8+** runtime environment without linking against any EDK2 libraries.

All standard definitions, types, and GUIDs are defined in [`include/efi_spec.h`](../include/efi_spec.h).

---

## 1. System Table Architecture

Inexorable initializes an in-memory [`EFI_SYSTEM_TABLE`](../include/efi_spec.h#L464) passed directly to the Linux kernel entry point:

```c
EFI_SYSTEM_TABLE gST = {
    .Hdr = {
        .Signature  = EFI_SYSTEM_TABLE_SIGNATURE, // 0x5453595320494249 ("IBI SYST")
        .Revision   = EFI_2_80_SYSTEM_TABLE_REVISION,
        .HeaderSize = sizeof(EFI_SYSTEM_TABLE),
        .CRC32      = 0,
    },
    .FirmwareVendor = L"Inexorable Bare-Metal Firmware",
    .FirmwareRevision = 0x00010000,
    .BootServices   = &gBS,
    .RuntimeServices= &gRT,
    .NumberOfTableEntries = 0,
    .ConfigurationTable   = gConfigurationTableEntries,
};
```

---

## 2. Boot Services Implementation

### 2.1 Memory Allocation Services
Implemented in [`Inexorable/Memory.c`](../Inexorable/Memory.c):

1. **`AllocatePages(Type, MemoryType, Pages, *Memory)`**:
   - Implements `AllocateAnyPages`, `AllocateMaxAddress`, and `AllocateAddress`.
   - Tracks allocations using a 128KB bitmap located at GPA `0x00050000` (outside low BIOS memory).
   - Each bit represents a 4KB page. Allocation uses fast bitwise scanning (`__builtin_ctzll` / bit shifts).
   - Allocations for firmware staging buffers, page tables, and temporary pools are assigned `EfiBootServicesData` or `EfiBootServicesCode`, ensuring the operating system reclaims them upon handover.

2. **`FreePages(Memory, Pages)`**:
   - Clears corresponding bits in the allocation bitmap.

3. **`AllocatePool(PoolType, Size, **Buffer)` / `FreePool(*Buffer)`**:
   - Wraps `AllocatePages()`. Prepends an 8-byte header storing the page count to allow clean freeing.

4. **`GetMemoryMap(*MemoryMapSize, *MemoryMap, *MapKey, *DescriptorSize, *DescriptorVersion)`**:
   - Performs automated **adjacent descriptor coalescing**: merges contiguous physical memory ranges sharing the exact same memory type into a single descriptor.
   - Emits only **16 clean, unfragmented descriptors** to Linux (compared to 48–64 descriptors under OVMF).

---

### 2.2 `EFI_LOAD_FILE2_PROTOCOL` (On-Demand Zero-Copy DMA Initrd)
Implemented in [`Inexorable/InitrdLoadFile2.c`](../Inexorable/InitrdLoadFile2.c):

- **Protocol GUID**: `LINUX_EFI_INITRD_MEDIA_GUID` (`{0x58de23aa, 0x1149, 0x47e8, {0x80, 0x2e, 0x52, 0x49, 0xa9, 0xab, 0x3a, 0x97}}`).
- **Target Device Path**: `EFI_DEVICE_PATH_PROTOCOL` with Vendor Media Device Path type (`MEDIA_DEVICE_PATH`, `MEDIA_VENDOR_DP`).
- **Operation**:
  1. The Linux kernel EFI stub searches for a device path matching `LINUX_EFI_INITRD_MEDIA_GUID`.
  2. The kernel calls `LoadFile(This, DevicePath, BootPolicy=FALSE, *BufferSize, Buffer)`.
  3. If `Buffer == NULL`, Inexorable returns `EFI_BUFFER_TOO_SMALL` and provides the exact size of the initramfs read from QEMU fw_cfg (`FW_CFG_INITRD_SIZE`).
  4. The kernel allocates the target buffer at high memory (e.g. `0x7EFC1000`) and calls `LoadFile` a second time with the allocated buffer pointer.
  5. Inexorable performs a single, high-speed **QEMU fw_cfg DMA transfer** (`0x514`) directly into the kernel's memory:
     ```c
     QemuFwCfgDmaRead(g_InitrdDmaSelector, Buffer, (UINT32)g_HostBootInfo->InitrdSize);
     ```
  6. **Zero Memory Copies**: The initramfs is transferred directly from hypervisor memory into the guest kernel's pages without any intermediate staging buffers.

---

### 2.3 `EFI_RNG_PROTOCOL` (Hardware RDRAND Entropy)
Implemented in [`Inexorable/RngProtocol.c`](../Inexorable/RngProtocol.c):

- **Protocol GUID**: `EFI_RNG_PROTOCOL_GUID` (`{0x3152bca5, 0xeade, 0x433d, {0x86, 0x2e, 0xc0, 0x1c, 0xdc, 0x29, 0x8f, 0x44}}`).
- **Operation**:
  - Implements `GetRNG(This, *RNGAlgorithm, RNGValueLength, *RNGValue)`.
  - Supports `EFI_RNG_ALGORITHM_RAW` and default RNG algorithm.
  - Generates 64-bit true random numbers using the x86 hardware instruction:
    ```c
    unsigned char ok;
    __asm__ __volatile__("rdrand %0; setc %1" : "=r"(*val), "=qm"(ok));
    ```
  - Supplies entropy to Linux's kernel EFI stub for Kernel Address Space Layout Randomization (KASLR) without boot stalls.

---

### 2.4 Handle Database & Protocol Management
Implemented in [`Inexorable/Protocol.c`](../Inexorable/Protocol.c):

- **`InstallProtocolInterface(*Handle, *Protocol, InterfaceType, *Interface)`**: Registers protocol interfaces on handles.
- **`LocateProtocol(*Protocol, *Registration, **Interface)`**: Locates the first registered protocol matching a given GUID.
- **`HandleProtocol(Handle, *Protocol, **Interface)`**: Queries an interface on a specific handle.
- **`LocateDevicePath(*Protocol, **DevicePath, *Device)`**: Traverses device path hierarchies to find handles supporting protocols (critical for `LOAD_FILE2` discovery).

---

## 3. Configuration Tables

Registered via `InstallConfigurationTable()` into `gST.ConfigurationTable`:

1. **`EFI_ACPI_20_TABLE_GUID`** (`{0x8868e871, 0xe4f1, 0x11d3, {0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81}}`):
   - Points to the Root System Description Pointer (RSDP) at GPA `0x40020000`.
2. **`EFI_RT_PROPERTIES_TABLE`** (`{0xeb668328, 0x3ff1, 0x496e, {0x87, 0x18, 0x49, 0xa7, 0x39, 0xb7, 0x6e, 0x40}}`):
   - Informs the Linux kernel that runtime services are minimal (`RuntimeServicesSupported = 0`), instructing the kernel not to allocate virtual mappings for firmware runtime code.

---

## 4. Runtime Services Implementation

Implemented in [`Inexorable/Misc.c`](../Inexorable/Misc.c), [`Inexorable/Time.c`](../Inexorable/Time.c), and [`Inexorable/Variable.c`](../Inexorable/Variable.c):

1. **`ResetSystem(ResetType, ResetStatus, DataSize, *ResetData)`**:
   - Supports `EfiResetCold`, `EfiResetWarm`, and `EfiResetShutdown`.
   - Executes PCI reset register write: `outb(0xCF9, 0x06)`.
   - Executes ACPI PM1a_CNT power-off write: `outw(0x604, 0x2000)`.
2. **`GetVariable()` / `SetVariable()`**:
   - Provides minimal stubs returning `EFI_NOT_FOUND` / `EFI_UNSUPPORTED`.
3. **`SetVirtualAddressMap()` / `ConvertPointer()`**:
   - Smoothly accommodates Linux virtual address transition stubs, returning `EFI_SUCCESS`.
