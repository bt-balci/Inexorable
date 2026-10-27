# Tooling, Synthesis & Benchmark Guide

This document details the complete developer tooling suite, build automation scripts, and empirical benchmark infrastructure included in **Inexorable**.

---

## 1. Tooling Architecture Overview

Inexorable provides a zero-dependency Python and Shell automation layer designed to build, package, test, and benchmark the firmware across both Windows and native Linux/WSL environments:

```text
Inexorable Toolchain
├── tools/
│   ├── build_fd.py            # Synthesizes 2MB Flash ROM image (inexorable.fd)
│   └── package_rootfs.py      # Packages diskless in-memory rootfs (inexorable_rootfs.img)
├── scripts/
│   ├── benchmark.py           # Automated empirical latency benchmark vs EDK2 OVMF
│   ├── test_network.py        # Automated PCIe network verification suite
│   └── run_benchmark.sh       # POSIX launcher wrapper
└── Makefile                   # Cross-platform orchestration
```

---

## 2. Flash ROM Synthesizer (`tools/build_fd.py`)

### Purpose
Synthesizes the complete 2MB (`0x200000` bytes) flash ROM image (`inexorable.fd`) required by QEMU (`-bios inexorable.fd`) without relying on EDK2 `BaseTools` or `GenFv`.

### Architecture & Memory Layout
The synthesized 2MB flash image maps into the upper physical address space from `0xFFE00000` to `0xFFFFFFFF`:

```text
Offset in ROM        Physical Address     Component                   Size
─────────────────────────────────────────────────────────────────────────────
0x00000000           0xFFE00000           Inexorable Core Payload     512 KB
0x00080000           0xFFE80000           Padding (0xFF)              1,520 KB
0x001FF000           0xFFFFF000           Reset Vector Trampoline     4 KB
0x001FFFF0           0xFFFFFFF0           x86 Power-On Reset Vector   16 bytes
```

### Build Process
1. Validates that the compiled 64-bit PE32+ firmware executable (`build/Inexorable.efi`) exists and does not exceed 512 KB.
2. Validates that the 16-bit reset vector trampoline (`build/reset_vector.bin`) exists and fits in the 4KB bootblock.
3. Initializes a 2MB byte buffer pre-filled with erased flash pattern (`0xFF`).
4. Embeds `Inexorable.efi` starting at offset `0x00000000` (`0xFFE00000`).
5. Embeds `reset_vector.bin` starting at offset `0x001FF000` (`0xFFFFF000`), placing the reset entry point directly at `0x001FFFF0` (`0xFFFFFFF0`).
6. Outputs `inexorable.fd` atomically.

### Usage
```bash
python tools/build_fd.py
```
Output:
```text
====================================================================
      INEXORABLE: 2MB FLASH ROM SYNTHESIZER
====================================================================
[BUILD] Firmware Core: build/Inexorable.efi (29,184 bytes)
[BUILD] Reset Vector:  build/reset_vector.bin (4,096 bytes)
[OK] Synthesized 2MB Flash ROM: inexorable.fd (2,097,152 bytes)
```

---

## 3. Standalone Rootfs Packager (`tools/package_rootfs.py`)

### Purpose
Packages an ultra-lean, 100% in-memory standalone Linux root filesystem (`inexorable_rootfs.img`) that runs entirely diskless without any hard drive, NVMe, SATA, or SCSI storage controller.

### Features
- **Zero Disk I/O**: Operates purely in RAM (ramfs / tmpfs).
- **POSIX Shell & Applets**: Includes Busybox applets (`sh`, `ip`, `udhcpc`, `ping`, `dmesg`, `grep`, `cat`, `free`, `poweroff`, `reboot`).
- **Dynamic Runtime Linker**: Packages `ld-linux-x86-64.so.2` and essential glibc shared libraries (`libc.so.6`, `libcrypt.so.2`, `libm.so.6`, `libresolv.so.2`, `libzstd.so.1`, `liblzma.so.5`, `libcrypto.so.3`, `libkmod.so.2`).
- **PCIe Network Drivers**: Automatically stages kernel network modules for PCIe:
  - `virtio_net.ko.zst` (VirtIO paravirtualized network)
  - `net_failover.ko.zst` and `failover.ko.zst`
  - `e1000e.ko.zst` (Intel PCIe Gigabit Ethernet)
  - `ptp.ko.zst` and `pps_core.ko.zst`
  - `e1000.ko.zst` (Intel PRO/1000 legacy)
- **Compact Module Index**: Executes `depmod -b $ROOT` to create minimal binary dependency trees.
- **Automated Init & Network Bring-Up**: Installs an optimized `/init` script that mounts virtual filesystems (`devtmpfs`, `procfs`, `sysfs`, `tmpfs`, `devpts`), probes network drivers, waits for link carrier, requests DHCP leases via `udhcpc`, displays system telemetry, and provides clean serial shutdown.

### Usage
```bash
# Package the complete in-memory rootfs
python tools/package_rootfs.py

# Internal flag: Assemble directory skeleton only
python tools/package_rootfs.py --stage /path/to/staging
```

---

## 4. Empirical Performance Benchmark Engine (`scripts/benchmark.py`)

### Purpose
Measures and compares cold-boot execution phases between **Inexorable** and standard **EDK2 / OVMF** (`OVMF.4m.fd`) with nanosecond-precision host timers and in-guest hardware TSC cycles.

### Execution Phases Measured
1. **In-Guest Firmware Latency (Hardware TSC Cycles)**:
   Measures cycles elapsed from hardware reset (`T0`) through C core initialization (`T1`), PE kernel mapping (`T2`), and long mode jump into Linux `CONFIG_EFI_STUB`.
2. **Host FW Latency to Kernel Entry**:
   Measures host wall-clock time from QEMU process invocation until the first kernel printk timestamp (`[ 0.000000]`).
3. **Linux Kernel Execution Phase**:
   Measures time from kernel entry until the first userspace shell prompt (`[INEXORABLE-USERSPACE]`).
4. **Total Cold-Boot Latency**:
   Complete end-to-end wall-clock time from QEMU invocation until interactive `/bin/sh` prompt.

### Command-Line Arguments
| Option | Description | Default |
| :--- | :--- | :--- |
| *(None)* | Executes standard 10-iteration comparative benchmark | 10 runs |
| `--quick` | Executes fast 3-iteration smoke benchmark | 3 runs |
| `--runs N` | Executes benchmark with custom iteration count $N$ | 10 runs |

### Usage
```bash
# Run standard 10-iteration benchmark
python scripts/benchmark.py

# Run quick 3-iteration smoke test
python scripts/benchmark.py --quick

# Run 20-iteration benchmark
python scripts/benchmark.py --runs 20
```

### Sample Output
```text
=====================================================================
   INEXORABLE vs EDK2 OVMF: EMPIRICAL COLD-BOOT BENCHMARK ENGINE     
=====================================================================

[Codebase & Binary Footprint]
  - Inexorable Codebase LOC:      3,103 lines
  - EDK2 OVMF Equivalent LOC:     ~4,200,000 lines (>99.9% reduction)
  - Inexorable Active Binary:     28.5 KB (PE32+ core)
  - EDK2 OVMF Active Binary:      3,653.6 KB (125x larger)
  - Inexorable Flash ROM Image:   2048 KB
  - EDK2 OVMF Flash ROM Image:    4096 KB

[Benchmarking Inexorable (10 runs)]...
  Run  1: In-Guest FW= 30.6 ms | Host FW=321.4 ms | Kernel=895.2 ms | Total=1216.6 ms
  ...

=====================================================================
              EMPIRICAL COLD-BOOT PERFORMANCE SUMMARY                
=====================================================================
Metric                              | Inexorable      | EDK2 OVMF       | Delta / Speedup
-----------------------------------------------------------------------------------
Codebase Size (LOC)                 | 3,103           | 4,200,000       | 1354x smaller
Active Firmware Size                | 28.5         KB | 3653.6          KB | 128.2x smaller
In-Guest Firmware Latency (Mean)    | 31.97        ms | 640.88       ms | 20.0x faster
In-Guest Firmware Latency (Min)     | 28.57        ms | 630.32       ms | 22.1x faster
Host FW Latency to Kernel (Mean)    | 344.88       ms | 720.88       ms | 2.1x faster
Linux Kernel Execution Phase        | 895.12       ms | 912.44       ms | Identical payload
Total Time to Shell (Mean)          | 1201.17      ms | 1552.39      ms | 351.2 ms faster
Total Time to Shell (Min)           | 1178.40      ms | 1493.09      ms | 314.7 ms faster
=====================================================================
```

---

## 5. PCIe Network Verification Suite (`scripts/test_network.py`)

### Purpose
Automates continuous integration testing for high-throughput PCI Express networking inside the guest OS, validating driver attachment, DHCP IP leasing, and sub-millisecond gateway routing.

### Verified Hardware Models
- **`virtio-net-pci`**: VirtIO paravirtualized network controller.
- **`e1000e`**: Intel 82574L Gigabit PCIe network controller.

### Test Workflow
1. Launches QEMU with Inexorable firmware, Linux kernel, and the specified NIC attached to a SLIRP virtual network (`10.0.2.0/24`).
2. Waits for userspace shell readiness and link carrier state.
3. Automatically delivers diagnostic shell commands via stdin:
   - `ip link`
   - `ip addr`
   - `ip route`
   - `ping -c 2 10.0.2.2`
   - `exit`
4. Parses guest output to confirm:
   - `eth0` appears with `<BROADCAST,MULTICAST,UP,LOWER_UP>`
   - `10.0.2.15` is assigned via DHCP
   - Default route `10.0.2.2` is installed
   - Ping packet loss is **0%**
   - Guest VM initiates clean poweroff

### Usage
```bash
python scripts/test_network.py
```
Output:
```text
====================================================================
                      TEST RESULTS SUMMARY
====================================================================
VirtIO-Net (virtio-net-pci):         [PASSED]
Intel PCIe Gigabit (e1000e):         [PASSED]
====================================================================
All PCIe networking tests completed successfully!
```

---

## 6. Makefile Command Reference

The top-level `Makefile` orchestrates compilation, ROM synthesis, rootfs packaging, virtual machine execution, and automated testing across Windows and Linux:

| Target | Command | Description |
| :--- | :--- | :--- |
| `all` | `make` | Compiles C and assembly sources and synthesizes `inexorable.fd` |
| `efi` | `make efi` | Compiles and links freestanding PE32+ `build/Inexorable.efi` |
| `reset_vector`| `make reset_vector` | Assembles 16-bit reset vector trampoline to `build/reset_vector.bin` |
| `fd` | `make fd` | Synthesizes 2MB flash ROM `inexorable.fd` |
| `rootfs` | `make rootfs` | Packages standalone in-memory rootfs `inexorable_rootfs.img` |
| `run` | `make run` | Boots Linux microVM under QEMU with KVM and VirtIO networking (1 vCPU) |
| `runsmp` | `make runsmp` | Boots Linux microVM under QEMU with KVM, 4 vCPUs (SMP), and VirtIO net |
| `testnet` | `make testnet` | Runs the automated PCIe network verification suite |
| `benchmark` | `make benchmark` | Runs the automated comparative latency benchmark against OVMF |
| `clean` | `make clean` | Removes `build/` directory and generated `inexorable.fd` |
