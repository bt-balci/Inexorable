# ACPI & PCIe Architecture

This document details Inexorable's **Hardware-Reduced ACPI 2.0+** implementation, **PCIe MCFG ECAM handoff**, and out-of-the-box **VirtIO device enablement**.

Source code: [`Inexorable/Acpi.c`](../Inexorable/Acpi.c)

---

## 1. Physical Memory Map for ACPI Tables

All ACPI tables are pre-constructed statically at GPA `0x40020000` inside `EfiACPIMemoryNVS` (Non-Volatile Storage) memory, which ensures the operating system preserves these tables across runtime:

```text
Physical GPA        Table        Description
─────────────────────────────────────────────────────────────────────────────
0x40020000          RSDP         Root System Description Pointer (ACPI 2.0+, 36 bytes)
0x40021000          XSDT         Extended System Description Table (64-bit pointers)
0x40022000          MADT         Multiple APIC Description Table (Local APICs + IOAPIC)
0x40023000          FADT         Fixed ACPI Description Table (HW-Reduced profile)
0x40024000          MCFG         PCI Express Memory-Mapped Config (ECAM @ 0xB0000000)
0x40025000          DSDT         Differentiated System Description Table (AML Bytecode)
```

---

## 2. Table Specifications

### 2.1 RSDP (Root System Description Pointer)
- Signature: `"RSD PTR "`
- Revision: `2` (ACPI 2.0+)
- `XsdtAddress`: Points to `0x40021000`.
- Dual Checksums: Standard 20-byte checksum (ACPI 1.0) and 36-byte extended checksum (ACPI 2.0+).
- Registered in `EFI_SYSTEM_TABLE.ConfigurationTable` under `EFI_ACPI_20_TABLE_GUID`.

### 2.2 XSDT (Extended System Description Table)
- Signature: `"XSDT"`
- Contains 64-bit physical pointers to:
  1. `FADT` (`0x40023000`)
  2. `MADT` (`0x40022000`)
  3. `MCFG` (`0x40024000`)

### 2.3 FADT (Fixed ACPI Description Table)
- Signature: `"FACP"`
- Profile: Hardware-Reduced ACPI (`Flags |= (1 << 10)` for `RESET_REG_SUP`).
- PM Control Blocks:
  - `PM1a_EVT`: Port `0x600` (length 4)
  - `PM1a_CNT`: Port `0x604` (length 2)
  - `PM_TMR`: Port `0x608` (length 4)
- Reset Register: Uses PCI reset mechanism at System I/O port `0xCF9` (value `0x06`).
- `X_Dsdt`: 64-bit physical pointer to `0x40025000`.

### 2.4 MADT (Multiple APIC Description Table)
- Signature: `"APIC"`
- Local APIC Address: `0xFEE00000`
- **Dynamic Processor Scaling**:
  - Firmware reads active vCPU count dynamically from QEMU fw_cfg (`g_HostBootInfo->CpuCount`).
  - Emits one `ACPI_LOCAL_APIC` structure (Type 0) for each active core (`ApicId = 0, 1, 2, ...`).
  - Emits one `ACPI_IOAPIC` structure (Type 1) at `0xFEC00000` with `GlobalSystemInterruptBase = 0`.
- Enables full multi-core SMP scaling out of the box (e.g. `make runsmp` for 4 cores).

### 2.5 MCFG (PCI Express Memory-Mapped Configuration Table)
- Signature: `"MCFG"`
- Base Address: `0xB0000000` (Standard 256MB MMCONFIG window).
- PCI Segment: `0`
- Bus Range: Start Bus `0`, End Bus `255` (`0xFF`).

---

## 3. DSDT AML Bytecode Architecture

The Differentiated System Description Table contains compiled AML bytecode declaring the PCI Express Root Bridge, interrupt routing, motherboard resources, and soft-off capabilities.

### 3.1 PCI Express Root Bridge (`PCI0`)
```asl
Scope (_SB)
{
    Device (PCI0)
    {
        Name (_HID, EisaId ("PNP0A08")) // PCI Express Bus
        Name (_CID, EisaId ("PNP0A03")) // Compatible: PCI Bus
        Name (_BBN, Zero)
        Name (_SEG, Zero)
        Name (_UID, Zero)

        Name (_CRS, ResourceTemplate ()
        {
            // Buses 0 to 255
            WordBusNumber (ResourceProducer, MinFixed, MaxFixed, PosDecode,
                0x0000, 0x0000, 0x00FF, 0x0000, 0x0100,,, )
            
            // PCI I/O Ports
            IO (Decode16, 0x0CF8, 0x0CF8, 0x01, 0x08)
            WordIO (ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
                0x0000, 0x0000, 0x0CF7, 0x0000, 0x0CF8,,, , TypeStatic, DenseTranslation)
            WordIO (ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
                0x0000, 0x0D00, 0xFFFF, 0x0000, 0xF300,,, , TypeStatic, DenseTranslation)

            // MMIO Window Below ECAM: 0x80000000 - 0xAFFFFFFF (768 MB)
            DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, Cacheable, ReadWrite,
                0x00000000, 0x80000000, 0xAFFFFFFF, 0x00000000, 0x30000000,,, , AddressRangeMemory, TypeStatic)

            // MMIO Window Above ECAM: 0xC0000000 - 0xFEBFFFFF (998 MB)
            DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, Cacheable, ReadWrite,
                0x00000000, 0xC0000000, 0xFEBFFFFF, 0x00000000, 0x3EC00000,,, , AddressRangeMemory, TypeStatic)
        })
        ...
    }
}
```

---

### 3.2 Linux ECAM Motherboard Resource Reservation (`MMCF`)
Linux enforces strict security verification before activating the PCI Express Memory-Mapped Configuration (ECAM) space:
```text
pci_mmcfg_check_reserved(): Checks if [0xB0000000 - 0xBFFFFFFF] is explicitly reserved
by an ACPI Motherboard Resource device (PNP0C02).
```
If this reservation is missing, Linux displays `PCI: not using ECAM (not reserved)` and falls back to slow CF8/CFC I/O ports.

Inexorable satisfies this check by embedding the `MMCF` child device directly inside `PCI0`:
```asl
Device (MMCF)
{
    Name (_HID, EisaId ("PNP0C02")) // PNP Motherboard Resources
    Name (_CRS, ResourceTemplate ()
    {
        Memory32Fixed (ReadWrite, 0xB0000000, 0x10000000) // 256 MB Window
    })
}
```
Result in Linux dmesg:
```text
[    0.342110] PCI: ECAM [mem 0xb0000000-0xbfffffff] reserved as ACPI motherboard resource
[    0.342512] PCI: Using host bridge windows from ACPI; if necessary, use "pci=nocrs"
[    0.342980] PCI: Using ECAM for [bus 00-ff]
```

---

### 3.3 PCI Interrupt Routing Table (`_PRT`)
Maps PCI INT A, INT B, INT C, and INT D lines across PCI slots 1..4 and 31 to I/O APIC Global System Interrupts (GSIs) 10 and 11:
- Slot 1 (`0x0001FFFF`): INT A -> GSI 10, INT B -> GSI 11
- Slot 2 (`0x0002FFFF`): INT A -> GSI 10, INT B -> GSI 11
- Slot 3 (`0x0003FFFF`): INT A -> GSI 10, INT B -> GSI 11
- Slot 4 (`0x0004FFFF`): INT A -> GSI 10, INT B -> GSI 11
- Slot 31 (`0x001FFFFF`): INT A -> GSI 10, INT B -> GSI 11, INT C -> GSI 10, INT D -> GSI 11

Allows `virtio-pci` to bind interrupts seamlessly without `no GSI found` warnings.

---

### 3.4 Soft-Off / Poweroff Support (`_S5`)
Declares the ACPI S5 Soft-Off state in the root namespace:
```asl
Scope (\)
{
    Name (_S5, Package (0x04)
    {
        Zero, // PM1a_CNT.SLP_TYP
        Zero, // PM1b_CNT.SLP_TYP
        Zero, // Reserved
        Zero  // Reserved
    })
}
```
When Linux executes `poweroff` or receives an ACPI shutdown event, it evaluates `\_S5`, sets `SLP_EN` bit 13, and writes `0x2000` to port `0x604` (`PM1a_CNT`), prompting QEMU to immediately and cleanly power off the virtual machine.

---

## 4. High-Speed PCIe Networking Verification

Inexorable features native PCIe device discovery and full hardware-accelerated networking via our static ACPI MCFG table (`0xB0000000`) and `_PRT` interrupt routing.

### 4.1 Supported Network Controllers
The firmware and standalone rootfs support both paravirtualized and hardware-emulated PCIe NICs:

| Device | QEMU Argument | Driver Module | Dependencies | Carrier Latency | Ping Latency |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **VirtIO Network** | `-device virtio-net-pci,netdev=net0` | `virtio_net` | `net_failover`, `failover` | `0 ms` (Instant) | **0.23 ms** |
| **Intel PCIe Gigabit** | `-device e1000e,netdev=net0` | `e1000e` | `ptp`, `pps_core` | `~1.2 s` (PHY auto-negotiate) | **0.28 ms** |

### 4.2 Automated In-Guest Network Bring-Up
1. **Module Auto-Probing**:
   Upon booting PID 1, `/init` loads network driver modules (`virtio_net`, `e1000e`). Because `virtio-pci` is built into the kernel, the root bridge binds `0000:00:02.0` immediately.
2. **Carrier Synchronization**:
   Physical and emulated PHYs (e.g. Intel 82574L) require a brief link autonegotiation interval before carrier is active (`<BROADCAST,MULTICAST,UP,LOWER_UP>`). The init script polls `/sys/class/net/$iface/carrier` to guarantee link presence before broadcasting DHCP queries.
3. **DHCP & Route Configuration**:
   Busybox `udhcpc` requests an IP lease from QEMU SLIRP (`10.0.2.15/24`) and invokes `/usr/share/udhcpc/default.script` to configure default route `10.0.2.2` and `/etc/resolv.conf`.
4. **Interactive Verification**:
   Running `make testnet` launches an automated test suite across all controllers, validating:
   - Device appearance in `ip link`
   - Leased IP `10.0.2.15/24` in `ip addr`
   - Default gateway in `ip route`
   - Bi-directional ICMP echo (`ping -c 2 10.0.2.2`) with **0% packet loss**
   - Clean guest poweroff upon shell exit.
