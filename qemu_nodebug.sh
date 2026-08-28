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
	-device usb-ehci,id=usb \
	-drive id=fatdisk,format=raw,if=none,file=fat:rw:build \
	-device usb-storage,bus=usb.0,drive=fatdisk \
	-net none \
	-boot menu=on,splash-time=5000 \
	-chardev stdio,id=sev-isa-debugcon \
	-device isa-debugcon,iobase=0x402,chardev=sev-isa-debugcon \
	-chardev file,id=serial0,path=serial_output.txt \
	-device isa-serial,chardev=serial0