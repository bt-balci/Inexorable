#ifndef _X86_H_
#define _X86_H_

#include <Uefi.h>

///////////////////////////////////////////////
////				Methods				   ////
///////////////////////////////////////////////

inline VOID __outb(UINT16 port, UINT8 value) {
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
        );
}

__attribute__((naked)) UINT64 EFIAPI __vmx_vmcall(UINT64 function, UINT64 arg1, UINT64 arg2, UINT64 arg3, UINT64 arg4, UINT64 arg5, UINT64 arg6);

#endif