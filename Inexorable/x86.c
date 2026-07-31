#include <Uefi.h>

__attribute__((naked)) UINT64 EFIAPI __vmx_vmcall(
    UINT64 function, UINT64 arg1, UINT64 arg2, UINT64 arg3, UINT64 arg4, UINT64 arg5, UINT64 arg6) {
    __asm__ __volatile__(
        "vmcall\n\t"
        "ret"
    );
}