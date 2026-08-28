AS       := nasm
CC       := gcc
LD       := ld

ASFLAGS  := -f elf64
CFLAGS   := -m64 -ffreestanding -fno-builtin -fno-pie -fno-pic \
            -fno-stack-protector -mno-red-zone -Wall -Wextra
LDFLAGS  := -m elf_x86_64 -nostdlib -T linker.ld

TARGET   := kernel.bin

all: $(TARGET)

boot.o: boot.asm
	$(AS) $(ASFLAGS) $< -o $@

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): boot.o kernel.o linker.ld
	$(LD) $(LDFLAGS) -o $@ boot.o kernel.o

run: $(TARGET)
	qemu-system-x86_64 -kernel $(TARGET) -serial stdio -display none

clean:
	rm -f boot.o kernel.o $(TARGET)

.PHONY: all run clean
