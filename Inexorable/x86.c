#include <Uefi.h>

__attribute__((naked)) UINT64 EFIAPI __vmx_vmcall(
	UINT64 function, UINT64 arg1, UINT64 arg2, UINT64 arg3, UINT64 arg4, UINT64 arg5, UINT64 arg6) {
	__asm__ __volatile__(
		".intel_syntax noprefix\n\t"
		//
		// Save all incoming to stack.
		//
		"push rcx\n\t"
		"push rdx\n\t"
		"push r8\n\t"
		"push r9\n\t"
		"push r10\n\t"
		"push r11\n\t"

		//
		// Load for our ABI.
		//
		"mov rax, [rsp + 40]\n\t"
		"mov rcx, [rsp + 32]\n\t"
		"mov rdx, [rsp + 24]\n\t"
		"mov r8,  [rsp + 16]\n\t"
		"mov r9,  [rsp + 88]\n\t"
		"mov r10, [rsp + 96]\n\t"
		"mov r11, [rsp + 104]\n\t"

		//
		// Vroom.
		//
		"vmcall\n\t"

		//
		// Clean-up our mess.
		//
		"add rsp, 48\n\t"  /* 6 pushes * 8 bytes = 48 */
		"ret"
	);
}