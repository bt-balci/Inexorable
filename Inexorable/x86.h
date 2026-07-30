#pragma once
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

__attribute__((naked)) inline UINT64 __vmx_vmcall(UINT64 function, UINT64 arg1, UINT64 arg2, UINT64 arg3, UINT64 arg4) {
    __asm__ __volatile__(
        "vmcall\n\t"
        "ret"
    );
}