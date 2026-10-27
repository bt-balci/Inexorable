# What Inexorable Strips (Firmware Philosophy)

> *"We strip off everything we don't need."*

Virtual machines running inside modern cloud hypervisors (KVM, Cloud Hypervisor, Firecracker, QEMU microvm) are deterministic software abstractions. Unlike physical motherboards manufactured in the 1990s, virtual machines do not have:
- Unpredictable DRAM topologies requiring SPD memory training.
- Physical floppy disk drives or parallel ATA ribbon cables.
- Expansion cards with 16-bit legacy Option ROMs.
- Mechanical disks needing motor spin-up delays.
- Interactive human operators pressing `F12` to enter a boot menu.

Yet standard virtual machine firmware—most notably **EDK2 / OVMF**—inherits the entire architecture of physical PC firmware, executing millions of lines of obsolete initialization code before the operating system can start.

This document details every major subsystem Inexorable intentionally strips away and why.

---

## 1. Traditional EDK2 / OVMF vs Inexorable Pipeline

```text
EDK2 OVMF Pipeline (~4,200,000 LOC, ~540 ms firmware latency):
[ SEC ] ──> [ PEI Phase ] ──> [ DXE Dispatcher ] ──> [ BDS Menu ] ──> [ GRUB / OS Loader ]
 (16-bit)    (32-bit PEI)     (Loads 60+ drivers)     (Scans Disks)    (Protected Mode)

Inexorable Pipeline (3,103 LOC, ~33 ms firmware latency):
[ ResetVector (16-bit -> 64-bit Long Mode in <0.1ms) ] ──> [ Direct Long Mode Jump into Linux Kernel ]
```

---

## 2. Comprehensive Inventory of Stripped Components

### 1. SEC (Security Phase) & PEI (Pre-EFI Initialization)
- **What it is in EDK2**: Initializes temporary stack in CPU cache (Cache-as-RAM / CAR), runs 32-bit PEI core, and dispatches dozens of PEIMs (PEI Modules) to discover memory and create HOBs (Hand-Off Blocks).
- **Why Inexorable strips it**: In virtual machines, guest physical RAM is mapped and ready the instant the hypervisor creates the vCPU. There is no DRAM training. Inexorable immediately creates 1GB identity page tables at `0x1000` and enters 64-bit Long Mode in **under 100 microseconds**.

### 2. DXE Driver Dispatcher & 60+ Dynamic Drivers
- **What it is in EDK2**: The Driver Execution Environment (DXE) contains a dependency evaluator that scans ROM volumes and dynamically dispatches 60 to 90 individual PE/COFF driver binaries (e.g. `MdeModulePkg`, `PcAtChipsetPkg`, `SecurityPkg`, `NetworkPkg`).
- **Why Inexorable strips it**: Inexorable is compiled as a **single, standalone 28.5 KB PE32+ executable**. There is no dispatcher, no dynamic driver loading, no protocol registration cascades, and no dependency resolution loop.

### 3. BDS (Boot Device Selection) & Interactive Menus
- **What it is in EDK2**: BDS connects consoles, scans all NVMe/SATA/VirtIO block devices, inspects GPT partition tables, searches for `\EFI\BOOT\BOOTX64.EFI`, negotiates boot timeouts, and displays graphic setup menus.
- **Why Inexorable strips it**: Cloud microVMs do not use interactive menus. The kernel and initrd locations are predetermined by the hypervisor. Inexorable streams the kernel directly via QEMU fw_cfg DMA and invokes its entry point immediately.

### 4. Legacy Hardware Probing & Obsolete Chipset Peripherals
- **What it is in EDK2**: Probes for legacy 8259 PIC, 8254 PIT, floppy disk controllers (FDC), parallel ports, PS/2 keyboard/mouse controllers, and legacy IDE controllers.
- **Why Inexorable strips it**: None of these devices exist in modern paravirtualized microVMs. Probing non-existent I/O ports wastes hundreds of milliseconds of hypervisor VM-exits. Inexorable uses direct APIC/IOAPIC, TSC timer, and 16550 UART serial console.

### 5. In-Firmware Dynamic PCI Bus Scanning
- **What it is in EDK2**: Scans all 256 PCI buses, reads vendor/device IDs, assigns BARs (Base Address Registers), and configures bridge windows inside the firmware before OS boot.
- **Why Inexorable strips it**: The guest Linux kernel already possesses an advanced, highly optimized PCI and PCIe subsystem. Performing PCI scanning in firmware and then having the kernel repeat the exact same scan is completely redundant. Inexorable provides static ACPI MCFG and DSDT declarations (`PCI0`), handing the PCIe root bridge directly to Linux.

### 6. Option ROM Scanning & Execution
- **What it is in EDK2**: Scans PCI expansion ROMs for legacy 16-bit or UEFI driver code and executes untrusted third-party code with platform privileges.
- **Why Inexorable strips it**: Option ROMs are a major security vulnerability and virtualization latency penalty. Inexorable executes zero external Option ROMs.

### 7. Compatibility Support Module (CSM)
- **What it is in EDK2**: 16-bit legacy BIOS emulator allowing old operating systems (MS-DOS, Windows 98) to boot via INT 10h/13h/16h interrupts.
- **Why Inexorable strips it**: MicroVMs run modern 64-bit operating systems. Eliminating CSM strips away tens of thousands of lines of fragile real-mode code.

### 8. Bloated External Libraries & Cryptographic Stacks
- **What it is in EDK2**: Pulls in hundreds of megabytes of `MdePkg`, `MdeModulePkg`, and full OpenSSL builds (`CryptoPkg`) for signature verification.
- **Why Inexorable strips it**: Inexorable has **zero external library dependencies**. It does not link with any EDK2 static library. Its standalone C and assembly footprint links cleanly into a 28.5 KB binary.

### 9. Fragmented Dynamic Heap Allocators
- **What it is in EDK2**: Implements a general-purpose heap with memory pools, bucket lists, and fragmentation tracking that emits 48 to 64 fragmented memory descriptors to the OS.
- **Why Inexorable strips it**: Inexorable utilizes a deterministic 4GB page bump allocator with automated adjacent descriptor coalescing. It emits only **16 clean descriptors** and marks all staging areas as `EfiBootServicesData`, allowing Linux to reclaim **100% of the memory** upon `ExitBootServices()`.

### 10. Intermediate Memory Buffering
- **What it is in EDK2**: Reads files from storage into intermediate staging buffers, copies them across memory interfaces, and re-allocates them for kernel handover.
- **Why Inexorable strips it**: Inexorable uses on-demand zero-copy DMA streaming through `EFI_LOAD_FILE2_PROTOCOL`. When Linux's EFI stub requests the initial ramdisk, fw_cfg streams it directly into the kernel's target buffer without intermediate buffering.

---

## 3. Comparative Summary Table

| Feature / Subsystem | Traditional EDK2 OVMF | Inexorable Firmware |
|:---|:---:|:---:|
| **Codebase Complexity** | ~4,200,000 LOC | **3,103 LOC** |
| **Active Binary Size** | 3,653.6 KB | **28.5 KB** |
| **Reset Vector Handoff** | Real Mode -> SEC -> PEI | **Real Mode -> 64-bit Long Mode (<0.1ms)** |
| **Driver Execution Model** | Dynamic DXE Dispatcher (60+ drivers) | **Monolithic Inlined Execution** |
| **PCI Enumeration** | Dynamic Bus Scanning in Firmware | **Static ACPI MCFG Handoff to Kernel** |
| **Option ROM Execution** | Supported (Security liability) | **Stripped (Zero execution)** |
| **Legacy PC Peripherals** | Probed (Floppy, IDE, PS/2) | **Stripped (Zero probing)** |
| **CSM Legacy BIOS** | Supported | **Stripped** |
| **External Dependencies** | MdePkg, CryptoPkg, OpenSSL | **Zero (Autonomous C + NASM)** |
| **Memory Map Descriptors** | 48 – 64 fragmented descriptors | **16 clean, coalesced descriptors** |
| **Staging RAM Reclamation** | Partial | **100% Reclaimed via BootServicesData** |
| **In-Guest Firmware Latency** | ~540 ms | **~33 ms** (16× faster) |
