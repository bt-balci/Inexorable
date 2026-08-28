#!/bin/bash

qemu-system-x86_64 \
	-machine q35 \
	-cpu host,vmx=on \
	-accel kvm \
	-m 4G \
	-smp 4 \
	-d int,cpu_reset -no-reboot \
	-drive if=pflash,format=raw,readonly=on,file=OVMF_CODE.fd \
	-drive if=pflash,format=raw,file=OVMF_VARS.fd \
	-drive format=raw,file=fat:rw:build \
	-net none \
	-s -S \
	-boot menu=on,splash-time=5000 \
    -chardev stdio,id=sev-isa-debugcon \
    -device isa-debugcon,iobase=0x402,chardev=sev-isa-debugcon