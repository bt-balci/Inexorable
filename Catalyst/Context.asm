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

	vmresume

AsmVmExit endp

end