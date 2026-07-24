#pragma once
#include <Uefi.h>

///////////////////////////////////////////////
////			   Definitions			   ////
///////////////////////////////////////////////

#define X86_GATE_TYPE_INTERRUPT		0xE
#define X86_GATE_TYPE_TRAP			0xF

#define X86_MAKE_SELECTOR(Index, Ti, Rpl) \
	((UINT32)(((Index) << 3) | (((Ti) & 1) << 2) | ((Rpl) & 3)))

#define GDT_NULL_SELECTOR \
	X86_MAKE_SELECTOR(0, 0, 0)

#define GDT_KERNEL_SELECTOR(Index) \
	X86_MAKE_SELECTOR((Index), 0, 0)

#define GDT_USER_SELECTOR(Index) \
	X86_MAKE_SELECTOR((Index), 0, 3)

#define GDT_INDEX_NULL        0
#define GDT_INDEX_KERNEL_CODE 1
#define GDT_INDEX_KERNEL_DATA 2
#define GDT_INDEX_USER_DATA   3
#define GDT_INDEX_USER_CODE   4
#define GDT_INDEX_TSS         5

#define KERNEL_CS_SELECTOR   GDT_KERNEL_SELECTOR(GDT_INDEX_KERNEL_CODE) // 0x08
#define KERNEL_DS_SELECTOR   GDT_KERNEL_SELECTOR(GDT_INDEX_KERNEL_DATA) // 0x10
#define USER_DS_SELECTOR     GDT_USER_SELECTOR(GDT_INDEX_USER_DATA)     // 0x1B (0x18 | RPL 3)
#define USER_CS_SELECTOR     GDT_USER_SELECTOR(GDT_INDEX_USER_CODE)     // 0x23 (0x20 | RPL 3)

#define GET_GDT_BASE(entry) ( \
	((UINTN)(entry)->BaseLow) | \
	((UINTN)(entry)->BaseMid << 16) | \
	((UINTN)(entry)->BaseHigh << 24) \
)

#define GET_GDT_TSS_BASE(entry) ( \
	((UINT64)(entry)->BaseDescriptor.BaseLow) | \
	((UINT64)(entry)->BaseDescriptor.BaseMid << 16) | \
	((UINT64)(entry)->BaseDescriptor.BaseHigh << 24) | \
	((UINT64)(entry)->BaseUpper << 32) \
)

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

typedef struct {
	UINT16 LimitLow;
	UINT16 BaseLow;
	UINT8 BaseMid;
	union {
		UINT8 Value;
		struct {
			UINT8 Accessed : 1;
			UINT8 WriteAllowed : 1;
			UINT8 Direction : 1;
			UINT8 Executable : 1;
			UINT8 DescriptorType : 1;
			UINT8 Dpl : 2;
			UINT8 Present : 1;
		};
	} Access;
	union {
		UINT8 Value;
		struct {
			UINT8 LimitHigh : 4;
			UINT8 Reserved : 1;
			UINT8 LongMode : 1;
			UINT8 Size : 1;
			UINT8 Granularity : 1;
		};
	} FlagsAndLimit;
	UINT8 BaseHigh;
} __attribute__((packed)) X86_GDT_ENTRY;

typedef struct {
	X86_GDT_ENTRY BaseDescriptor;
	UINT32 BaseUpper;
	UINT32 Reserved;
} __attribute__((packed)) X86_GDT_SYSTEM_ENTRY;

typedef struct {
	UINT16 OffsetLow;
	UINT16 Selector;
	UINT8 Ist;
	union {
		UINT8 Value;
		struct {
			UINT8 GateType : 4;
			UINT8 StoragePresent : 1;
			UINT8 Dpl : 2;
			UINT8 Present : 1;
		};
	} TypeAttributes;
	UINT16 OffsetMid;
	UINT32 OffsetHigh;
	UINT32 Zero;
} __attribute__((packed)) X86_IDT_ENTRY;

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


#define IA32_FEATURE_CONTROL				0x3A
#define FEATURE_CONTROL_LOCK_BIT            (1ULL << 0)
#define FEATURE_CONTROL_VMXON_OUTSIDE_SMX   (1ULL << 2)

#define IA32_VMX_BASIC			0x480
#define VMX_BASIC_REVISION_MASK (0x7FFFFFFF)

#define IA32_VMX_CR0_FIXED0     0x486


#define IA32_VMX_CR0_FIXED1     0x487


#define IA32_VMX_CR4_FIXED0     0x488


#define IA32_VMX_CR4_FIXED1     0x489


#define IA32_VMX_PINBASED_CTLS 0x481


#define IA32_VMX_PROCBASED_CTLS 0x482


#define IA32_VMX_EXIT_CTLS 0x483


#define IA32_VMX_ENTRY_CTLS 0x484


#define IA32_VMX_MISC 0x485


#define IA32_VMX_PROCBASED_CTLS2 0x48b


#define IA32_VMX_TRUE_PINBASED_CTLS 0x48d


#define IA32_VMX_TRUE_PROCBASED_CTLS 0x48e


#define IA32_VMX_TRUE_EXIT_CTLS 0x48f


#define IA32_VMX_TRUE_ENTRY_CTLS 0x490


#define IA32_VMX_PROCBASED_CTLS3 0x492


#define IA32_SYSENTER_CS 0x174


#define IA32_SYSENTER_ESP 0x175


#define IA32_SYSENTER_EIP 0x176

#define IA32_APIC_BASE 0x1B


#define IA32_X2APIC_ICR 0x830



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

X86_IDT_ENTRY
GenerateIdtEntry(
	IN UINT64	Offset,
	IN UINT16	Selector,
	IN UINT8	GateType,
	IN UINT8	Dpl
);

X86_GDT_ENTRY
CreateGdtEntry(
	IN UINT32	Base,
	IN UINT32	Limit,
	IN UINT8	Executable,
	IN UINT8	WriteAllowed,
	IN UINT8	Dpl,
	IN UINT8	IsLongMode,
	IN UINT8	Is32Bit,
	IN UINT8	Granularity
);

inline UINT64 __readcr0(VOID) {
	UINT64 cr0;
	__asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
	return cr0;
}

inline VOID __writecr0(UINT64 val) {
	__asm__ __volatile__("mov %0, %%cr0" :: "r"(val) : "memory");
}

inline UINT64 __readcr3(VOID) {
	UINT64 cr3;
	__asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
	return cr3;
}

inline UINT64 __readcr4(VOID) {
	UINT64 cr4;
	__asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));
	return cr4;
}

inline VOID __writecr4(UINT64 val) {
	__asm__ __volatile__("mov %0, %%cr4" :: "r"(val) : "memory");
}

__attribute__((target("fsgsbase")))
inline UINT64 _readgsbase_u64(VOID) {
	UINT64 val;
	__asm__ __volatile__("rdgsbase %0" : "=r"(val));
	return val;
}

__attribute__((target("fsgsbase")))
inline VOID _writegsbase_u64(UINT64 val) {
	__asm__ __volatile__("wrgsbase %0" :: "r"(val));
}

inline UINT64 __readmsr(UINT32 msr) {
	UINT32 low, high;
	__asm__ __volatile__(
		"rdmsr"
		: "=a"(low), "=d"(high)
		: "c"(msr)
	);
	return ((UINT64)high << 32) | low;
}

inline VOID __writemsr(UINT32 msr, UINT64 val) {
	UINT32 low = (UINT32)val;
	UINT32 high = (UINT32)(val >> 32);
	__asm__ __volatile__(
		"wrmsr"
		:: "c"(msr), "a"(low), "d"(high)
		: "memory"
	);
}

inline UINT8 __vmx_on(UINT64* phys_addr) {
	UINT8 cf;
	__asm__ __volatile__(
		"vmxon %1; setc %0"
		: "=q"(cf)
		: "m"(*phys_addr)
		: "memory", "cc"
	);
	return cf;
}

inline UINT8 __vmx_vmclear(UINT64* vmcs_physical_address) {
	UINT8 error;
	__asm__ __volatile__(
		"vmclear %[vmcs]\n\t"
		"setna %[err]"
		: [err] "=rm" (error)
		: [vmcs] "m" (*vmcs_physical_address)
		: "cc", "memory"
	);
	return error;
}

inline UINT8 __vmx_vmptrld(UINT64* vmcs_physical_address) {
	UINT8 error;
	__asm__ __volatile__(
		"vmptrld %[vmcs]\n\t"
		"setna %[err]"
		: [err] "=rm" (error)
		: [vmcs] "m" (*vmcs_physical_address)
		: "cc", "memory"
	);
	return error;
}

inline UINT8 __vmx_vmread(UINT64 field, UINT64* value) {
	UINT8 error;
	UINT64 val;
	__asm__ __volatile__(
		"vmread %2, %0\n\t"
		"setbe %1"
		: "=r"(val), "=q"(error)
		: "r"(field)
		: "cc"
	);
	*value = val;
	return error;
}

inline UINT8 __vmx_vmwrite(UINT64 field, UINT64 value) {
	UINT8 error;
	__asm__ __volatile__(
		"vmwrite %1, %2\n\t"
		"setbe %0"
		: "=q"(error)
		: "r"(value), "r"(field)
		: "cc"
	);
	return error;
}

inline UINT8 __vmx_vmlaunch(VOID) {
	UINT8 error;
	__asm__ __volatile__(
		"vmlaunch\n\t"
		"setbe %0"
		: "=q"(error)
		:: "memory", "cc"
	);
	return error;
}

inline UINT8 __vmx_vmresume(VOID) {
	UINT8 error;
	__asm__ __volatile__(
		"vmresume\n\t"
		"setbe %0"
		: "=q"(error)
		:: "memory", "cc"
	);
	return error;
}

inline UINT64 AsmLoadSegmentDescriptor(UINT64 GdtBase, UINT32 Selector) {
	UINT64 Result;
	__asm__ __volatile__(
		"shrl $3, %k2\n\t"
		"lea (%1, %q2, 8), %0"
		: "=r" (Result)
		: "r" (GdtBase), "r" ((UINT64)Selector)
		: "cc"
	);
	return Result;
}

inline UINT64 AsmLoadSegmentLimit(UINT64 Selector) {
	UINT64 Result;
	UINT64 Zero = 0;
	__asm__ __volatile__(
		"lsl %1, %0\n\t"
		"cmovnz %2, %0"
		: "=&r" (Result)
		: "r" (Selector), "r" (Zero)
		: "cc"
	);
	return Result;
}

inline UINT64 AsmLoadSegmentAccessRights(UINT64 Selector) {
	UINT64 Result;
	UINT64 Zero = 0;
	__asm__ __volatile__(
		"lar %1, %0\n\t"
		"cmovnz %2, %0"
		: "=&r" (Result)
		: "r" (Selector), "r" (Zero)
		: "cc"
	);
	return Result;
}