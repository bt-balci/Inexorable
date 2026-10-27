;=============================================================================
; INEXORABLE: Reset Vector & Direct-Long-Mode Transition Engine
; Target Architecture: x86_64
; Purpose: Bare-metal power-on / reset entry point for 2MB firmware image.
; Sits at linear 0xFFFFF000 - 0xFFFFFFFF (ROM offset 0x1FF000 - 0x1FFFFF).
; Hardware Reset Vector executes at linear 0xFFFFFFF0 (ROM offset 0x1FFFF0).
; Transitions from 16-bit Real Mode -> 32-bit Protected Mode -> 64-bit Long Mode
; and transfers 128KB payload from ROM (0xFFE00000) to RAM (0x40000000).
;=============================================================================

BITS 16
ORG 0xF000                  ; Offset within CS=0xF000 at linear 0xFFFFF000

code_base:

;-----------------------------------------------------------------------------
; 16-bit Reset Entry
;-----------------------------------------------------------------------------
ResetEntry16:
    cli                     ; Disable interrupts
    cld                     ; Clear direction flag

    ; Initialize segment registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7000          ; Temporary real-mode stack

    ; Load 32-bit/64-bit GDT descriptor
    ; CS base is 0xFFFF0000, so CS:gdtr references linear 0xFFFFF000 + (gdtr - code_base)
    o32 lgdt [cs:gdtr]

    ; Enable Protected Mode (CR0.PE = 1)
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; 32-bit far jump to Protected Mode entry point
    ; Selector 0x08 = 32-bit Code Segment
    jmp dword 0x08:(0xFFFFF000 + (ProtMode32 - code_base))

;-----------------------------------------------------------------------------
; Global Descriptor Table (GDT)
;-----------------------------------------------------------------------------
align 16
gdt_start:
    ; 0x00: Null Descriptor
    dq 0x0000000000000000

    ; 0x08: 32-bit Flat Code Segment (Base=0, Limit=4GB, DPL=0, Read/Exec, Pre-Accessed)
    ; Access=0x9B (P=1, DPL=0, S=1, Code, Non-conforming, Readable, Accessed=1)
    ; Pre-setting Accessed=1 prevents CPU from writing to ROM during GDT load in VMX/KVM!
    dw 0xFFFF               ; Limit 15:0
    dw 0x0000               ; Base 15:0
    db 0x00                 ; Base 23:16
    db 0x9B                 ; Access (Pre-Accessed)
    db 0xCF                 ; Flags / Limit 19:16
    db 0x00                 ; Base 31:24

    ; 0x10: 32-bit Flat Data Segment (Base=0, Limit=4GB, DPL=0, Read/Write, Pre-Accessed)
    ; Access=0x93 (P=1, DPL=0, S=1, Data, Expand-up, Writable, Accessed=1)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x93                 ; Access (Pre-Accessed)
    db 0xCF
    db 0x00

    ; 0x18: 64-bit Flat Code Segment (Base=0, Limit=4GB, DPL=0, Read/Exec, Pre-Accessed)
    ; Access=0x9B, Flags=0xAF (G=1, D=0, L=1)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9B                 ; Access (Pre-Accessed)
    db 0xAF
    db 0x00

    ; 0x20: 64-bit Flat Data Segment (Base=0, Limit=4GB, DPL=0, Read/Write, Pre-Accessed)
    ; Access=0x93, Flags=0xCF
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x93                 ; Access (Pre-Accessed)
    db 0xCF
    db 0x00
gdt_end:

align 16
gdtr:
    dw (gdt_end - gdt_start - 1)
    dd (0xFFFFF000 + (gdt_start - code_base))

;-----------------------------------------------------------------------------
; 32-bit Protected Mode Entry Point
;-----------------------------------------------------------------------------
BITS 32
align 16
ProtMode32:
    ; Reload all data segments with flat 32-bit selector 0x10
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    mov esp, 0x70000        ; 32-bit temporary stack in low RAM

    ; Zero out 8KB of memory at 0x1000 for PML4 (0x1000) and PDPT (0x2000)
    mov edi, 0x1000
    xor eax, eax
    mov ecx, 2048
    rep stosd

    ; Construct 4-level identity page tables mapping 0 to 4GB
    ; PML4[0] at 0x1000 -> points to PDPT at 0x2000 (Present | Writable = 0x03)
    mov dword [0x1000], 0x00002003
    mov dword [0x1004], 0x00000000

    ; PDPT at 0x2000: 4 x 1GB Large Pages (Present | Writable | 1GB Page = 0x83)
    ; 0x00000000 - 0x3FFFFFFF (1GB Guest RAM)
    mov dword [0x2000], 0x00000083
    mov dword [0x2004], 0x00000000

    ; 0x40000000 - 0x7FFFFFFF (1GB to 2GB: Inexorable Firmware & ACPI/Staging)
    mov dword [0x2008], 0x40000083
    mov dword [0x200C], 0x00000000

    ; 0x80000000 - 0xBFFFFFFF (2GB to 3GB)
    mov dword [0x2010], 0x80000083
    mov dword [0x2014], 0x00000000

    ; 0xC0000000 - 0xFFFFFFFF (3GB to 4GB: Maps PCI/MMIO & ROM at 0xFFE00000)
    mov dword [0x2018], 0xC0000083
    mov dword [0x201C], 0x00000000

    ; Enable PAE (bit 5), OSFXSR (bit 9), OSXMMEXCPT (bit 10) in CR4
    mov eax, cr4
    or eax, (1 << 5) | (1 << 9) | (1 << 10)
    mov cr4, eax

    ; Enable Long Mode in IA32_EFER MSR (0xC0000080, bit 8 = LME)
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)
    wrmsr

    ; Load CR3 with PML4 base address (0x1000)
    mov eax, 0x1000
    mov cr3, eax

    ; Enable Paging (PG, bit 31), Protected Mode (PE, bit 0), Monitor Coprocessor (MP, bit 1)
    ; Clear Emulation (EM, bit 2) and Task Switched (TS, bit 3)
    mov eax, cr0
    and eax, ~((1 << 2) | (1 << 3))
    or eax, 0x80000003
    mov cr0, eax

    ; Far jump to 64-bit Long Mode code segment (Selector 0x18)
    jmp 0x18:(0xFFFFF000 + (LongMode64 - code_base))

;-----------------------------------------------------------------------------
; 64-bit Long Mode Entry Point
;-----------------------------------------------------------------------------
BITS 64
align 16
LongMode64:
    ; Reload all 64-bit segment registers
    mov ax, 0x20
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; Setup 64-bit BSP stack in low conventional RAM (safe from firmware image at 0x40000000)
    mov rsp, 0x00080000

    ; Send marker to QEMU debugcon port 0xE9 / 0x402
    mov dx, 0x3F8           ; COM1 Base
    ; Initialize COM1 for early telemetry
    mov dx, 0x3F9
    xor al, al
    out dx, al              ; Disable all interrupts
    mov dx, 0x3FB
    mov al, 0x80
    out dx, al              ; Enable DLAB
    mov dx, 0x3F8
    mov al, 0x01
    out dx, al              ; Set baud rate divisor to 1 (115200 baud)
    mov dx, 0x3F9
    xor al, al
    out dx, al
    mov dx, 0x3FB
    mov al, 0x03
    out dx, al              ; 8 bits, no parity, one stop bit
    mov dx, 0x3FA
    mov al, 0xC7
    out dx, al              ; Enable FIFO, clear them, 14-byte threshold
    mov dx, 0x3FC
    mov al, 0x0B
    out dx, al              ; IRQs enabled, RTS/DSR set

    ; Print early banner '[INEXORABLE] Reset Vector -> Long Mode OK\r\n'
    lea rsi, [rel msg_longmode]
.print_loop:
    lodsb
    test al, al
    jz .copy_payload
    mov dx, 0x3F8
    out dx, al              ; Send to COM1 (0x3F8 THR)
    out 0xE9, al            ; Also mirror to QEMU debugcon (0xE9)
    jmp .print_loop

.copy_payload:
    ; Copy 512KB firmware payload from ROM (0xFFE00000) to RAM (0x40000000)
    cld
    mov rsi, 0xFFE00000
    mov rdi, 0x40000000
    mov rcx, (512 * 1024) / 8    ; 65536 qwords = 524288 bytes
    rep movsq

    ; Read PE entry point RVA from 0x40000000 and jump to it
    mov rbx, 0x40000000
    mov edx, [rbx + 0x3C]               ; e_lfanew offset
    mov eax, [rbx + rdx + 0x28]         ; AddressOfEntryPoint RVA
    add rax, rbx                        ; Linear GPA of InexorableMain
    push 0                              ; Push 8-byte dummy return address (MS x64 ABI stack alignment)
    jmp rax

align 16
msg_longmode:
    db "[INEXORABLE] Reset Vector -> Long Mode OK", 0x0D, 0x0A, 0

;-----------------------------------------------------------------------------
; Hardware Reset Vector (Linear 0xFFFFFFF0, ROM offset 0x1FFFF0)
; The CPU reset sequence starts execution here (CS:IP = F000:FFF0).
; Must be located exactly 16 bytes before the end of the 4KB block.
;-----------------------------------------------------------------------------
TIMES (0x1000 - 16) - ($ - code_base) db 0x90

BITS 16
ResetVector:
    jmp ResetEntry16
    align 16
