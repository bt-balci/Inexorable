#ifndef _X86_H_
#define _X86_H_

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

#define PIC1_COMMAND		0x20
#define PIC2_COMMAND		0xA0
#define PIC_EOI				0x20

#define PIC_IRQ_OFFSET		0x20
#define PIC_IRQ_END			0x2F
#define PIC_SLAVE_OFFSET	0x28

#define RFLAGS_IF			0x200

#define QEMU_DEBUG_PORT		0x402

///////////////////////////////////////////////
////				Enums				   ////
///////////////////////////////////////////////

typedef enum {
	ApicDeliveryModeFixed = 0,
	ApicDeliveryModeLowest = 1,
	ApicDeliveryModeSMI = 2,
	ApicDeliveryModeNMI = 4,
	ApicDeliveryModeINIT = 5,
	ApicDeliveryModeStartup = 6
} ApicDeliveryMode;

typedef enum {
	ApicDestinationModePhysical = 0,
	ApicDestinationModeLogical = 1
} ApicDestinationMode;

typedef enum {
	ApicDestinationShorthandNone = 0,
	ApicDestinationShorthandAll = 1,
	ApicDestinationShorthandAllIncludingSelf = 2,
	ApicDestinationShorthandAllExcludingSelf = 3
} ApicDestinationShorthand;

typedef enum {
	ApicTriggerModeEdge = 0,
	ApicTriggerModeLevel = 1
} ApicTriggerMode;

typedef enum {
	ApicLevelAssert = 0,
	ApicLevelDeAssert = 1
} ApicLevel;

///////////////////////////////////////////////
////				Structs				   ////
///////////////////////////////////////////////

/*
	General-purpose VMX area that is aligned to maximum 0x1000 bytes boundary.
*/
typedef struct {
	UINT32 RevisionIdentifier;
	UINT8 Reserved[4092];
} __attribute__((aligned(4096))) INTEL_VMX_REGION;

typedef union {
	UINT64 All;
	struct {
		UINT64 Vector : 8;
		UINT64 DeliveryMode : 3;
		UINT64 DestinationMode : 1;
		UINT64 Reserved : 2;
		UINT64 Level : 1;
		UINT64 TriggerMode : 1;
		UINT64 Reserved2 : 2;
		UINT64 DestinationShort : 2;
		UINT64 Reserved3 : 12;
		UINT64 ApicId : 32;
	};
} INTERRUPT_COMMAND_REGISTER;

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

typedef union {
	UINT64 All;
	struct {
		UINT64 MemoryType : 3;
		UINT64 PageWalkLength : 3;
		UINT64 DirtyAndAccessEnabled : 1;
		UINT64 Reserved1 : 5;
		UINT64 Pml4Address : 36;
		UINT64 Reserved2 : 16;
	};
} EPTP;

typedef union {
	UINT64 All;
	struct {
		UINT64 Read : 1;
		UINT64 Write : 1;
		UINT64 Execute : 1;
		UINT64 Reserved1 : 5;
		UINT64 Accessed : 1;
		UINT64 Present : 1;
		UINT64 ExecuteUser : 1;
		UINT64 Ignored2 : 1;
		UINT64 PhysicalAddress : 36;
		UINT64 Reserved2 : 4;
		UINT64 Ignored3 : 12;
	};
} EPT_PML4;

typedef union {
	UINT64 All;
	struct {
		UINT64 Read : 1;
		UINT64 Write : 1;
		UINT64 Execute : 1;
		UINT64 Reserved1 : 5;
		UINT64 Accessed : 1;
		UINT64 Present : 1;
		UINT64 ExecuteUser : 1;
		UINT64 Ignored2 : 1;
		UINT64 PhysicalAddress : 36;
		UINT64 Reserved2 : 4;
		UINT64 Ignored3 : 12;
	};
} EPT_PDPTE;

typedef union {
	UINT64 All;
	struct {
		UINT64 Read : 1;
		UINT64 Write : 1;
		UINT64 Execute : 1;
		UINT64 MemoryType : 3;
		UINT64 IgnorePAT : 1;
		UINT64 Large : 1;
		UINT64 Accessed : 1;
		UINT64 Dirty : 1;
		UINT64 ExecuteUser : 1;
		UINT64 Present : 1;
		UINT64 Reserved1 : 18;
		UINT64 PhysicalAddress : 18;
		UINT64 Reserved2 : 4;
		UINT64 Ignored3 : 11;
		UINT64 SuppressVE : 1;
	};
} EPT_LARGE_PDPTE;

typedef union {
	UINT64 All;
	struct {
		UINT64 Read : 1;
		UINT64 Write : 1;
		UINT64 Execute : 1;
		UINT64 Reserved1 : 4;
		UINT64 MustBeZero : 1;
		UINT64 Accessed : 1;
		UINT64 Present : 1;
		UINT64 ExecuteUser : 1;
		UINT64 Ignore21 : 1;
		UINT64 PhysicalAddress : 36;
		UINT64 Reserved3 : 4;
		UINT64 Ignored3 : 12;
	};
} EPT_PDE;

typedef union {
	UINT64 All;
	struct {
		UINT64 Read : 1;
		UINT64 Write : 1;
		UINT64 Execute : 1;
		UINT64 MemoryType : 3;
		UINT64 IgnorePAT : 1;
		UINT64 Large : 1;
		UINT64 Accessed : 1;
		UINT64 Dirty : 1;
		UINT64 ExecuteUser : 1;
		UINT64 Present : 1;
		UINT64 Reserved2 : 9;
		UINT64 PhysicalAddress : 27;
		UINT64 Reserved3 : 4;
		UINT64 Ignored3 : 11;
		UINT64 SuppressVE : 1;
	};
} EPT_LARGE_PDE;

typedef union {
	UINT64 All;
	struct {
		UINT64 Read : 1;
		UINT64 Write : 1;
		UINT64 Execute : 1;
		UINT64 EptMemoryType : 3;
		UINT64 IgnorePAT : 1;
		UINT64 Present : 1;
		UINT64 Accessed : 1;
		UINT64 Dirty : 1;
		UINT64 ExecuteUser : 1;
		UINT64 Ignored2 : 1;
		UINT64 PhysicalAddress : 36;
		UINT64 Reserved2 : 4;
		UINT64 Ignored3 : 11;
		UINT64 SuppressVE : 1;
	};
} EPT_PTE;

typedef union {
	UINT64 All;
	struct {
		UINT64 Present : 1;
		UINT64 Write : 1;
		UINT64 User : 1;
		UINT64 WriteThrough : 1;
		UINT64 CacheDisabled : 1;
		UINT64 Accessed : 1;
		UINT64 Ignored1 : 1;
		UINT64 Reserved1 : 1;
		UINT64 Ignored2 : 4;
		UINT64 PhysicalAddress : 40;
		UINT64 Ignored3 : 11;
		UINT64 NoExecute : 1;
	};
} PML4;

typedef union {
	UINT64 All;
	struct {
		UINT64 Present : 1;
		UINT64 Write : 1;
		UINT64 User : 1;
		UINT64 WriteThrough : 1;
		UINT64 CacheDisabled : 1;
		UINT64 Accessed : 1;
		UINT64 Dirty : 1;
		UINT64 Large : 1;
		UINT64 Global : 1;
		UINT64 Ignored2 : 3;
		UINT64 PAT : 1;
		UINT64 Reserved : 17;
		UINT64 PhysicalAddress : 22;
		UINT64 Ignored3 : 7;
		UINT64 PKE : 4;
		UINT64 NoExecute : 1;
	};
} PDPE_LARGE;

typedef union {
	UINT64 All;
	struct {
		UINT64 Present : 1;
		UINT64 Write : 1;
		UINT64 User : 1;
		UINT64 WriteThrough : 1;
		UINT64 CacheDisabled : 1;
		UINT64 Accessed : 1;
		UINT64 Ignored1 : 1;
		UINT64 Large : 1;
		UINT64 Ignored2 : 4;
		UINT64 PhysicalAddress : 40;
		UINT64 Ignored3 : 11;
		UINT64 NoExecute : 1;
	};
} PDPTE;

typedef union {
	UINT64 All;
	struct {
		UINT64 Present : 1;
		UINT64 Write : 1;
		UINT64 User : 1;
		UINT64 WriteThrough : 1;
		UINT64 CacheDisabled : 1;
		UINT64 Accessed : 1;
		UINT64 Dirty : 1;
		UINT64 Large : 1;
		UINT64 Global : 1;
		UINT64 Ignored1 : 3;
		UINT64 PAT : 1;
		UINT64 Reserved : 8;
		UINT64 PhysicalAddress : 31;
		UINT64 Ignored2 : 7;
		UINT64 PKE : 4; 
		UINT64 NoExecute : 1;
	};
} PDE_LARGE;

typedef union {
	UINT64 All;
	struct {
		UINT64 Present : 1;
		UINT64 Write : 1;
		UINT64 User : 1;
		UINT64 WriteThrough : 1;
		UINT64 CacheDisabled : 1;
		UINT64 Accessed : 1;
		UINT64 Ignored1 : 1;
		UINT64 Large : 1;
		UINT64 Ignored2 : 4;
		UINT64 PhysicalAddress : 40;
		UINT64 Ignored3 : 11;
		UINT64 NoExecute : 1;
	};
} PDE;

typedef union {
	UINT64 All;
	struct {
		UINT64 Present : 1;
		UINT64 Write : 1;
		UINT64 User : 1;
		UINT64 WriteThrough : 1;
		UINT64 CacheDisabled : 1;
		UINT64 Accessed : 1;
		UINT64 Dirty : 1;
		UINT64 PAT : 1;
		UINT64 Global : 1;
		UINT64 Ignored1 : 3;
		UINT64 PhysicalAddress : 40;
		UINT64 Ignored2 : 7;
		UINT64 PKE : 4;
		UINT64 NoExecute : 1;
	};
} PTE;

typedef union {
	UINT64 All;
	struct {
		UINT64 Reserved1 : 3;
		UINT64 PWT : 1;
		UINT64 PCD : 1;
		UINT64 Reserved2 : 7;
		UINT64 PhysicalAddress : 40;
		UINT64 Reserved3 : 12;
	};
} CR3;

typedef union {
	UINT32 All;
	struct {
		UINT32 Present : 1;
		UINT32 Write : 1;
		UINT32 User : 1;
		UINT32 ReservedBitsSet : 1;
		UINT32 Execute : 1;
	};
} PAGE_FAULT_ERROR_CODE;

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


#define IA32_EFER 0xC0000080


#define IA32_APIC_BASE 0x1B
#define APIC_BASE_ENABLE_X2APIC     (1ULL << 10)
#define APIC_BASE_APIC_GLOBAL_EN    (1ULL << 11)

#define IA32_X2APIC_ICR 0x830

///////////////////////////////////////////////
////				Enums				   ////
///////////////////////////////////////////////

typedef enum {
	ExceptionInterruptTypeDivideByZero = 0,
	ExceptionInterruptTypeDebug = 1,
	ExceptionInterruptTypeNMI = 2,
	ExceptionInterruptTypeBreakpoint = 3,
	ExceptionInterruptTypeInvalidOpcode = 6,
	ExceptionInterruptTypeDeviceNotAvailable = 7,
	ExceptionInterruptTypeDoubleFault = 8,
	ExceptionInterruptTypeGeneralProtectionFault = 13,
	ExceptionInterruptTypePageFault = 14,
	ExceptionInterruptTypeMachineCheck = 18,
	ExceptionInterruptTypeMaskableInterrupt = 32
} ExceptionInterruptType;

///////////////////////////////////////////////
////				Methods				   ////
///////////////////////////////////////////////

BOOLEAN
EFIAPI
IsVmxEnabled(
	VOID
);

BOOLEAN
EFIAPI
IsVmxSupported(
	VOID
);

X86_IDT_ENTRY
EFIAPI
GenerateIdtEntry(
	IN UINT64	Offset,
	IN UINT16	Selector,
	IN UINT8	GateType,
	IN UINT8	Dpl
);

X86_GDT_ENTRY
EFIAPI
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

VOID
EFIAPI
SendInterrupt(
	IN UINT8					Vector,
	IN ApicDestinationShorthand Shorthand		OPTIONAL,
	IN UINT32					DestinationId	OPTIONAL
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

inline UINT8 __vmx_on(UINT64 phys_addr) {
	UINT8 error;
	__asm__ __volatile__(
		"vmxon %1\n\t"
		"jc 1f\n\t"
		"jz 2f\n\t"
		"movb $0, %0\n\t"
		"jmp 3f\n"
		"1:\n\t"
		"movb $2, %0\n\t"
		"jmp 3f\n"
		"2:\n\t"
		"movb $1, %0\n"
		"3:"
		: "=q" (error)
		: "m" (phys_addr)
		: "memory", "cc"
	);
	return error; // 0 = Success, 1 = VMfailValid, 2 = VMfailInvalid
}

inline UINT8 __vmx_vmclear(UINT64 vmcs_physical_address) {
	UINT8 error;
	__asm__ __volatile__(
		"vmclear %[vmcs]\n\t"
		"setna %[err]"
		: [err] "=rm" (error)
		: [vmcs] "m" (vmcs_physical_address)
		: "cc", "memory"
	);
	return error;
}

inline UINT8 __vmx_vmptrld(UINT64 vmcs_physical_address) {
	UINT8 error;
	__asm__ __volatile__(
		"vmptrld %[vmcs]\n\t"
		"setna %[err]"
		: [err] "=rm" (error)
		: [vmcs] "m" (vmcs_physical_address)
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
	return GdtBase + (Selector & ~7ULL);
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
	if (Selector == 0)
		return 0x10000;

	UINT64 Result;
	UINT64 Zero = 0;
	__asm__ __volatile__(
		"lar %1, %0\n\t"
		"cmovnz %2, %0"
		: "=&r" (Result)
		: "r" (Selector), "r" (Zero)
		: "cc"
	);
	return (Result >> 8) & 0xF0FF;
}

inline VOID __outb(UINT16 port, UINT8 value) {
	__asm__ __volatile__(
		"outb %0, %1"
		:
	: "a"(value), "Nd"(port)
		);
}

#endif