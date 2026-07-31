extern VmExitHandler:proc

.code
AsmVmExit proc

	push rax				; Save RAX
	rdgsbase rax

	add rax, 8192			; Skip Intel VM regions

	;mov [rsp],			rax
	mov [rax + 8],		rbx
	mov [rax + 16],		rcx
	mov [rax + 24],		rdx
	mov [rax + 32],		rsi
	mov [rax + 40],		rdi

	mov [rax + 48],		r8
	mov [rax + 56],		r9
	mov [rax + 64],		r10
	mov [rax + 72],		r11
	mov [rax + 80],		r12
	mov [rax + 88],		r13
	mov [rax + 96],		r14
	mov [rax + 104],	r15

	mov [rax + 112],	rbp
	;mov [rax + 120],	0
	;mov [rax + 128],	0	; Cant be bothered to use vmread in asm

	pushfq
	pop [rax + 136]			; RFlags

	movups [rax + 144], xmm0
	movups [rax + 160], xmm1
	movups [rax + 176], xmm2
	movups [rax + 192], xmm3
	movups [rax + 208], xmm4
	movups [rax + 224], xmm5

	;mov rcx, rax			; Set as first argument
	pop [rax]				; Restore original rax

	sub rsp, 40
	call VmExitHandler
	add rsp, 40

	rdgsbase rax
	add rax, 8192			; Skip Intel VM regions

	mov rbx,	[rax + 8]
	mov rcx,	[rax + 16]
	mov rdx,	[rax + 24]
	mov rsi,	[rax + 32]
	mov rdi,	[rax + 40]

	mov r8,		[rax + 48]
	mov r9,		[rax + 56]
	mov r10,	[rax + 64]
	mov r11,	[rax + 72]
	mov r12,	[rax + 80]
	mov r13,	[rax + 88]
	mov r14,	[rax + 96]
	mov r15,	[rax + 104]

	mov rbp,	[rax + 112]
	
	push [rax + 136]		; RFlags
	popfq

	movups xmm0, [rax + 144]
	movups xmm0, [rax + 160]
	movups xmm0, [rax + 176]
	movups xmm0, [rax + 192]
	movups xmm0, [rax + 208]
	movups xmm0, [rax + 224]

	mov rax, [rax]

	vmresume				; What happened? Where did these 3000 cycles come from? Mehhh.. Whatever.

AsmVmExit endp

end