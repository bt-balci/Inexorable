# INEXORABLE: Cross-Platform Makefile
# Target: Zero-Enumeration Direct-Long-Mode UEFI Firmware (inexorable.fd)
# Compatible with: Linux (Ubuntu/Debian/Arch/Fedora), WSL2, macOS, Windows (LLVM/MinGW/MSYS2)

CC          := clang
LD          := lld-link
NASM        := nasm
PYTHON      := python3

ifeq ($(OS),Windows_NT)
    PYTHON   = python
    MKDIR    = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
    RM       = if exist "$(subst /,\,$(1))" rmdir /s /q "$(subst /,\,$(1))"
    DEL      = if exist "$(subst /,\,$(1))" del /q "$(subst /,\,$(1))"
else
    MKDIR    = mkdir -p $(1)
    RM       = rm -rf $(1)
    DEL      = rm -f $(1)
endif

BUILD_DIR   := build
SRC_DIR     := Inexorable
INCLUDE_DIR := include

# Compiler & Linker Flags for UEFI PE32+ (x86_64)
CFLAGS      := -target x86_64-unknown-windows \
               -ffreestanding \
               -fno-stack-protector \
               -fshort-wchar \
               -mno-red-zone \
               -O2 \
               -Wall \
               -Wextra \
               -Wno-unused-parameter \
               -Wno-unused-function \
               -I$(INCLUDE_DIR) \
               -I$(SRC_DIR)

LDFLAGS     := /ENTRY:UefiMain \
               /SUBSYSTEM:EFI_APPLICATION \
               /BASE:0x40000000 \
               /ALIGN:4096 \
               /DYNAMICBASE:NO \
               /NODEFAULTLIB

# Sources
C_SRCS      := $(wildcard $(SRC_DIR)/*.c)
C_OBJS      := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.obj,$(C_SRCS))
ASM_SRCS    := $(SRC_DIR)/Boot.asm
ASM_OBJS    := $(BUILD_DIR)/Boot.obj

TARGET_EFI  := $(BUILD_DIR)/Inexorable.efi
TARGET_FD   := inexorable.fd
TARGET_ROOTFS := inexorable_rootfs.img

.PHONY: all efi reset_vector fd rootfs run runsmp benchmark clean

all: $(TARGET_FD)

$(BUILD_DIR):
	@$(call MKDIR,$(BUILD_DIR))

# Compile C Files to Windows PE COFF Objects
$(BUILD_DIR)/%.obj: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "[CC] $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble 64-bit Assembly to Windows PE COFF Object
$(BUILD_DIR)/Boot.obj: $(SRC_DIR)/Boot.asm | $(BUILD_DIR)
	@echo "[NASM] $< -> $@"
	$(NASM) -f win64 $< -o $@

# Link PE32+ UEFI Core Executable
$(TARGET_EFI): $(C_OBJS) $(ASM_OBJS)
	@echo "[LD] Linking $@"
	$(LD) $(LDFLAGS) /OUT:$@ $^
	@$(call MKDIR,$(SRC_DIR)/x64/Release)
	@cp $@ $(SRC_DIR)/x64/Release/Inexorable.efi 2>/dev/null || copy $(subst /,\,$@) $(subst /,\,$(SRC_DIR)/x64/Release/Inexorable.efi) >nul 2>&1 || true

efi: $(TARGET_EFI)

# Assemble 16-bit Reset Vector Trampoline
$(BUILD_DIR)/reset_vector.bin: qemu_target/reset_vector.asm | $(BUILD_DIR)
	@echo "[NASM] $< -> $@"
	$(NASM) -f bin $< -o $@

reset_vector: $(BUILD_DIR)/reset_vector.bin

# Synthesize 2MB Flash ROM (inexorable.fd)
$(TARGET_FD): $(TARGET_EFI) $(BUILD_DIR)/reset_vector.bin
	@echo "[FD] Building $@"
	$(PYTHON) tools/build_fd.py

fd: $(TARGET_FD)

# Generate Standalone In-Memory Rootfs
$(TARGET_ROOTFS): tools/package_rootfs.py
	@echo "[ROOTFS] Packaging $@"
	$(PYTHON) tools/package_rootfs.py

rootfs: $(TARGET_ROOTFS)

# Run QEMU with KVM and VirtIO Networking (1 CPU)
run: $(TARGET_FD) $(TARGET_ROOTFS)
	qemu-system-x86_64 \
	    -enable-kvm \
	    -machine q35 \
	    -cpu host \
	    -m 2G \
	    -bios $(TARGET_FD) \
	    -kernel /boot/vmlinuz-linux \
	    -initrd $(TARGET_ROOTFS) \
	    -device virtio-net-pci,netdev=net0 \
	    -netdev user,id=net0 \
	    -append "console=ttyS0 earlycon=uart8250,io,0x3f8 quiet loglevel=3 tsc=reliable clocksource=tsc no_timer_check lpj=3187195 audit=0 mitigations=off" \
	    -serial stdio \
	    -display none \
	    -no-reboot

# Run QEMU with KVM, 4 Processors (SMP), and VirtIO Networking
runsmp: $(TARGET_FD) $(TARGET_ROOTFS)
	qemu-system-x86_64 \
	    -enable-kvm \
	    -machine q35 \
	    -cpu host \
	    -smp 4 \
	    -m 2G \
	    -bios $(TARGET_FD) \
	    -kernel /boot/vmlinuz-linux \
	    -initrd $(TARGET_ROOTFS) \
	    -device virtio-net-pci,netdev=net0 \
	    -netdev user,id=net0 \
	    -append "console=ttyS0 earlycon=uart8250,io,0x3f8 quiet loglevel=3 tsc=reliable clocksource=tsc no_timer_check lpj=3187195 audit=0 mitigations=off" \
	    -serial stdio \
	    -display none \
	    -no-reboot

# Run Automated Benchmark vs OVMF
benchmark: $(TARGET_FD) $(TARGET_ROOTFS)
	$(PYTHON) scripts/benchmark.py

clean:
	@echo "[CLEAN] Removing build directory and generated firmware"
	@$(call RM,$(BUILD_DIR))
	@$(call DEL,$(TARGET_FD))
