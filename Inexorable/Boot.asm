BITS 64

SECTION .data

CODE_SEL    equ 08h
DATA_SEL    equ 10h

align 4
gdt_start:
	dq 0000000000000000h            ; 00h: Null

	; 08h: 32‑bit Code (D=1, L=0, P=1, DPL=0, Read/Exec)
	dw 0FFFFh
	dw 00000h
	db 00h
	db 10011010b
	db 11001111b
	db 00h

	; 10h: 32‑bit Data (B=1, P=1, DPL=0, Read/Write)
	dw 0FFFFh
	dw 00000h
	db 00h
	db 10010010b
	db 11001111b
	db 00h
gdt_end:

gdt_descriptor:
	dw gdt_end - gdt_start - 1
	dq gdt_start

temp_gdtr:
	dw 0
	dq 0

far_ptr:
	dd Compat32
	dw CODE_SEL

SECTION .text


global Multiboot2Boot

; void Multiboot2Boot(UINT64 EntryPoint [RCX], UINT64 MultibootInfo [RDX])
Multiboot2Boot:
	mov rbp, rcx	; Save entry point
	mov rsi, rdx	; Save multiboot

	cli

	lgdt [gdt_descriptor]

	; Hypervisor shenanigans
	sgdt [temp_gdtr]
	mov rbx, [temp_gdtr + 2]

	; Limit=0xFFFF, Base=0, Access=0x9A, Flags=0xCF
	mov rax, 0x00CF9A000000FFFF
	; Why? Apparently, I dont even know.
	mov [rbx + 8], rax

	; Far jump to 32‑bit compatibility mode
	jmp dword far [far_ptr]

BITS 32
Compat32:
	mov ax, DATA_SEL
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	; Disable Paging (CR0.PG = 0)
	mov eax, cr0
	and eax, 7FFFFFFFh
	mov cr0, eax

	; Disable Long Mode (EFER.LME = 0)
	mov ecx, 0C0000080h
	rdmsr
	btr eax, 8
	wrmsr

	; Multiboot2 magic number
	mov eax, 36D76289h

	; multiboot info (saved in RSI, but we only need lower 32 bits)
	mov ebx, esi

	; jump to the entry point (now in EBP because we saved RCX in RBP)
	jmp ebp