BITS 32

section .multiboot
align 8
header_start:
    dd 0xE85250D6
    dd 0
    dd header_end - header_start
    dd 0x100000000 - (0xE85250D6 + 0 + (header_end - header_start))

    dw 0
    dw 0
    dd 8
header_end:

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .rodata
align 8
gdt64:
    dq 0x0000000000000000
    dq 0x00209A0000000000
    dq 0x0020920000000000
gdt64_end:
gdt64_ptr:
    dw gdt64_end - gdt64 - 1
    dd gdt64

section .text
global start
extern kernel_main

start:
    cli
    mov esp, stack_top
    mov edi, eax
    mov esi, ebx

    lgdt [gdt64_ptr]

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    xor eax, eax
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
	
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    jmp 0x08:.long_mode

BITS 64
.long_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rsp, stack_top

    call kernel_main

.hang:
    hlt
    jmp .hang
