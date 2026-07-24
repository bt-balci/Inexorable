#pragma once
#include <Uefi.h>

///////////////////////////////////////////////
////				Methods				   ////
///////////////////////////////////////////////

BOOLEAN
IsVmxEnabled(
	VOID
);

BOOLEAN
IsVmxSupported(
	VOID
);

///////////////////////////////////////////////
////				Structs				   ////
///////////////////////////////////////////////

/*
	General-purpose VMX area that is aligned to maximum 0x1000 bytes boundary.
*/
typedef struct {
	union {
		UINT32 RevisionIdentifier : 31;
	};
} __attribute__((aligned(4096))) INTEL_VMX_REGION;

/*
	All x86 general purpose registers + Xmm0-5 because they are volatile in Microsoft x64 Calling Convention
*/
typedef struct {
	UINT64 Rax;
	UINT64 Rbx;
	UINT64 Rcx;
	UINT64 Rdx;
	UINT64 Rsi;
	UINT64 Rdi;

	UINT64 R8;
	UINT64 R9;
	UINT64 R10;
	UINT64 R11;
	UINT64 R12;
	UINT64 R13;
	UINT64 R14;
	UINT64 R15;

	UINT64 Rbp;
	UINT64 Rsp;
	UINT64 Rip;
	UINT64 RFlags;

	__uint128_t Xmm0;
	__uint128_t Xmm1;
	__uint128_t Xmm2;
	__uint128_t Xmm3;
	__uint128_t Xmm4;
	__uint128_t Xmm5;
} __attribute__((aligned(16))) X86_CONTEXT_AREA;

///////////////////////////////////////////////
////				CPUID				   ////
///////////////////////////////////////////////

/*
	Leaf 1, ECX
*/
#define VMX_SUPPORT_BIT						(1 << 5)

///////////////////////////////////////////////
////				MSRs				   ////
///////////////////////////////////////////////


#define IA32_FEATURE_CONTROL_MSR			0x3A
#define FEATURE_CONTROL_LOCK_BIT            (1ULL << 0)
#define FEATURE_CONTROL_VMXON_OUTSIDE_SMX   (1ULL << 2)