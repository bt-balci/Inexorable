# Inexorable Documentation Portal

Welcome to the technical documentation for **Inexorable**—the zero-enumeration, direct-long-mode standalone UEFI firmware engineered for cloud hypervisors, microVMs, and confidential computing.

---

## Documentation Index

1. [**Architecture & Boot Flow**](boot-flow.md)
   - Detailed execution pipeline from power-on hardware reset vector (`0xFFFFFFF0`) to Linux userspace (`/init`).
   - Physical memory map, identity paging, staging buffers, and 100% memory reclamation.

2. [**What We Strip (Firmware Philosophy)**](what-we-strip.md)
   - Deep dive into why traditional UEFI (EDK2 / OVMF) is slow in virtualized environments.
   - Comprehensive breakdown of all eliminated legacy components (SEC, PEI, DXE, BDS, CSM, dynamic bus probing, option ROMs, bloated heap allocators).

3. [**EFI Subsystem & Protocol Implementations**](protocols.md)
   - How Inexorable implements a spec-compliant minimal UEFI environment without any external EDK2 dependencies.
   - Memory allocation services, `EFI_LOAD_FILE2_PROTOCOL` zero-copy DMA streaming, `EFI_RNG_PROTOCOL` (hardware RDRAND), and runtime services.

4. [**ACPI & PCIe Architecture**](acpi-and-pcie.md)
   - Static ACPI 2.0+ tables (RSDP, XSDT, FADT, MADT, MCFG, DSDT).
   - PCIe root bridge (`PCI0`), `PNP0C02` ECAM reservation, `_PRT` interrupt routing, and `_S5` soft-off shutdown support.
   - Out-of-the-box VirtIO (`virtio-net-pci`) and Intel Gigabit (`e1000e`) PCIe device enablement without `pci=off`.

5. [**Performance Analysis & Benchmarks**](performance-analysis.md)
   - Rigorous empirical latency measurements comparing Inexorable against EDK2 OVMF.
   - Deep breakdown of the boot timeline: firmware phase vs kernel initialization vs rootfs startup.
   - Single-core and multi-core (SMP 4) telemetry data and memory density metrics.

6. [**Tooling, Synthesis & Benchmark Guide**](tooling-and-benchmarks.md)
   - Comprehensive reference for `build_fd.py`, `package_rootfs.py`, `benchmark.py`, and `test_network.py`.
   - Command-line arguments, options, automated testing workflows, and `Makefile` target catalog.
