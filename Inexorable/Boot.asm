.data
CODE_SEL EQU 08h
DATA_SEL EQU 10h

ALIGN 4
GDT_Start LABEL Byte
	DQ 0000000000000000h        ; 00h: Null

	; 08h: 32-bit Code (D=1, L=0, P=1, DPL=0, Read/Exec)
	DW 0FFFFh, 00000h
	DB 00h, 10011010b, 11001111b, 00h 

	; 10h: 32-bit Data (B=1, P=1, DPL=0, Read/Write)
	DW 0FFFFh, 00000h
	DB 00h, 10010010b, 11001111b, 00h
GDT_End LABEL Byte

GDT_Descriptor DW (GDT_End - GDT_Start - 1)
			   DQ OFFSET GDT_Start

.code

; void Multiboot2Boot(UINT64 EntryPoint [RCX], UINT64 MultibootInfo [RDX])
Multiboot2Boot PROC
	; Save RCX here because it will be clobbered due to wrmsr/rdmsr nonsense
	mov rbp, rcx
	
	cli

	lgdt fword ptr [GDT_Descriptor]

	; Jump to compatibility mode
	push CODE_SEL                   ; Push 32-bit CS selector
	lea rax, Compat32               ; Load target address
	push rax                        ; Push RIP
	retfq

; I hate masm
Compat32:
	mov ax, DATA_SEL
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	; Disable Paging (CR0.PG = 0)
	mov rax, cr0
	and eax, 07FFFFFFFh             ; Clear bit 31 (PG)
	mov cr0, rax
	DB 0Fh, 22h, 0C0h

	; Disable Long Mode (EFER.LME = 0)
	mov ecx, 0C0000080h             ; IA32_EFER MSR
	rdmsr
	btr eax, 8                      ; Clear bit 8 (LME)
	wrmsr

	; EAX must contain the Multiboot2 Magic Number
	mov eax, 36D76289h              
	
	; Since our C code passed it in RDX, the lower 32-bits are in EDX.
	mov ebx, edx                 

	; Yay!
	jmp rbp

Multiboot2Boot ENDP

end