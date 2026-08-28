OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(start)

BASE = 0x100000;

SECTIONS
{
    . = BASE;

    .multiboot : ALIGN(8) {
        KEEP(*(.multiboot))
    }

    .text : ALIGN(4) {
        *(.text*)
    }

    .rodata : ALIGN(4) {
        *(.rodata*)
    }

    .data : ALIGN(4) {
        *(.data*)
    }

    .bss : ALIGN(16) {
        *(COMMON)
        *(.bss*)
    }

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note*)
        *(.comment*)
    }
}
