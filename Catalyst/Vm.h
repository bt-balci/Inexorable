#ifndef _VM_H_
#define _VM_H_

#include <Uefi.h>
#include "x86.h"

///////////////////////////////////////////////
////			  Definitions			   ////
///////////////////////////////////////////////

#define FIRMWARE_IMAGE_BASE		0x1000
// In page counts. (E.g. 4 = 0x4000 bytes)
#define BSP_INITIAL_STACK_SIZE	4
#define HOST_STACK_SIZE			0x4000
#define CONFIG_CPU_COUNT		4

///////////////////////////////////////////////
////				VMCALLs 			   ////
///////////////////////////////////////////////

#define CATALYST_VMCALL_SET_VARIABLE			0x0
#define CATALYST_VMCALL_GET_VARIABLE			0x1
#define CATALYST_VMCALL_QUERY_VARIABLE_INFO		0x2
#define CATALYST_VMCALL_GET_NEXT_VARIABLE_NAME	0x3


///////////////////////////////////////////////
////				Methods				   ////
///////////////////////////////////////////////

VOID
EFIAPI
PrepareProcessor(
	IN OUT VOID*	Argument
);

VOID
EFIAPI
AsmVmExit(
	VOID
);

VOID
EFIAPI
StartVm(
	VOID
);

BOOLEAN
EFIAPI
CopyToGuest(
	IN CR3      GuestCr3,
	IN VOID*	HostBuffer,
	IN UINT64   GuestVirtualAddress,
	IN UINTN    Size
);

BOOLEAN
EFIAPI
CopyFromGuest(
	IN CR3      GuestCr3,
	IN VOID*	HostBuffer,
	IN UINT64   GuestVirtualAddress,
	IN UINTN    Size
);

///////////////////////////////////////////////
////				Structs				   ////
///////////////////////////////////////////////

/*
	Structure representing each virtual processor.
*/
typedef struct {
	struct {
		INTEL_VMX_REGION VmxOn;
		INTEL_VMX_REGION Vmcs;
	} Vmx;

	X86_CONTEXT_AREA Context;

	struct {
		CR3 Cr3;
	} ExtendedContext;

	struct {
		UINT8 MsrBitmap[0x1000];
		UINT8 HostStack[HOST_STACK_SIZE];
	} __attribute__((aligned(4096))) Data;

	UINT32 Index;
} __attribute__((aligned(4096))) CATALYST_VIRTUAL_CPU;

typedef struct {
	//
	// Pray it works.
	//
	CHAR16 VariableName[256];
	EFI_GUID VendorGuid;
	UINT32 Attributes;
	UINTN DataSize;
	VOID* Data;
} NVRAM_NODE;

typedef struct {
	struct {
		__attribute__((aligned(4096))) X86_IDT_ENTRY	Idt[256];
		__attribute__((aligned(4096))) X86_GDT_ENTRY	Gdt[8192];
		__attribute__((aligned(4096))) UINT8			Tss[104];

		VOID* IdtBase;
		VOID* GdtBase;
		VOID* TssBase;

		UINT16 TrSelector;
		UINT16 TrAccessRights;
		UINT16 TrLimit;
	} SharedState;

	struct {
		__attribute__((aligned(4096))) CR3              IdentityCr3;

		//
		// Clang says its better padding this way.
		//
		UINT32 EpdptCount;
		UINT32 EpdeCount;
		UINT32 EpteCount;

		__attribute__((aligned(4096))) PML4         IdentityPml4[512];
		__attribute__((aligned(4096))) PDPE_LARGE         IdentityPdpe[512];

		__attribute__((aligned(4096))) EPTP				Eptp;
		__attribute__((aligned(4096))) EPT_PML4			Epml4[512];
		__attribute__((aligned(4096))) EPT_PDPTE        Epdpt[64][512];
		__attribute__((aligned(4096))) EPT_LARGE_PDE    Epde[512][512];
		__attribute__((aligned(4096))) EPT_PTE			Epte[16384][512];
	} Memory;

	struct {
		struct {
			NVRAM_NODE* Variables;
			UINTN VariablesLength;
		} Variables;


	} Firmware;
} __attribute__((aligned(4096))) CATALYST_VIRTUAL_MACHINE;

typedef union {
	UINT32 All;
	struct {
		UINT32 Vector : 8;
		UINT32 Type : 3;
		UINT32 DeliverErrorCode : 1;
		UINT32 Reserved : 19;
		UINT32 Valid : 1;
	};
} VMX_INTERRUPTION_INFORMATION;

typedef union {
	UINT32 All;
	struct {
		UINT32 Present : 1;
		UINT32 Write : 1;
		UINT32 User : 1;
		UINT32 ReservedBitSet : 1;
		UINT32 NxViolation : 1;
		UINT32 ProtectionKeyViolation : 1;
		UINT32 Reserved : 26;
	};
} VMX_PAGE_FAULT_ERROR_CODE;

typedef union {
	UINT32 All;
	struct {
		UINT32 Read : 1;
		UINT32 Write : 1;
		UINT32 Execute : 1;
		UINT32 IsReadable : 1;
		UINT32 IsWritable : 1;
		UINT32 IsExecutable : 1;
		UINT32 UserExecute : 1;
		UINT32 GuestLinearAddressValid : 1;
		UINT32 TranslationBork : 1;
	};
} VMX_EPT_VIOLATION_QUAILIFICATION;

///////////////////////////////////////////////
////				Globals				   ////
///////////////////////////////////////////////

__attribute__((aligned(4096))) extern CATALYST_VIRTUAL_MACHINE g_VmState;

///////////////////////////////////////////////
////				Enums				   ////
///////////////////////////////////////////////

typedef enum {
	VmEntryLoadDebugControls = 1 << 2,
	VmEntryIa32eModeGuest = 1 << 9,
	VmEntryEntryToSmm = 1 << 10,
	VmEntryDeactivateDualMonitor = 1 << 11,
	VmEntryLoadIa32PerfGlobalCtrl = 1 << 13,
	VmEntryLoadIa32Pat = 1 << 14,
	VmEntryLoadIa32Efer = 1 << 15,
	VmEntryLoadIa32Bndcfgs = 1 << 16,
	VmEntryConcealVmxFromPt = 1 << 17,
	VmEntryLoadIa32RtitCtl = 1 << 18,
} VmxVmEntryControls;

typedef enum {
	VmExitSaveDebugControls = 1 << 2,
	VmExitHostAddressSpaceSize = 1 << 9,
	VmExitLoadIa32PerfGlobalCtrl = 1 << 12,
	VmExitAckInterruptOnExit = 1 << 15,
	VmExitSaveIa32Pat = 1 << 18,
	VmExitLoadIa32Pat = 1 << 19,
	VmExitSaveIa32Efer = 1 << 20,
	VmExitLoadIa32Efer = 1 << 21,
	VmExitSaveVmxPreemptionTimer = 1 << 22,
	VmExitClearIa32Bndcfgs = 1 << 23,
	VmExitConcealVmxFromPt = 1 << 24,
	VmExitClearIa32RtitCtl = 1 << 25,
} VmxVmExitControls;

typedef enum {
	ProcessorBased2VirtualizeApic = 1 << 0,
	ProcessorBased2EnableEpt = 1 << 1,
	ProcessorBased2DtableExiting = 1 << 2,
	ProcessorBased2EnableRdtscp = 1 << 3,
	ProcessorBased2VirtualizeX2apic = 1 << 4,
	ProcessorBased2EnableVpid = 1 << 5,
	ProcessorBased2WbinvdExiting = 1 << 6,
	ProcessorBased2UnrestrictedGuest = 1 << 7,
	ProcessorBased2VirtualizeApicRegister = 1 << 8,
	ProcessorBased2VirtualInterruptDelivery = 1 << 9,
	ProcessorBased2PauseLoopExiting = 1 << 10,
	ProcessorBased2RdrandExiting = 1 << 11,
	ProcessorBased2EnableInvpcid = 1 << 12,
	ProcessorBased2EnableVmFunctions = 1 << 13,
	ProcessorBased2VmcsShadowing = 1 << 14,
	ProcessorBased2EnclsExiting = 1 << 15,
	ProcessorBased2RdseedExiting = 1 << 16,
	ProcessorBased2EnablePml = 1 << 17,
	ProcessorBased2EptViolationVe = 1 << 18,
	ProcessorBased2ConcealVmxFromPt = 1 << 19,
	ProcessorBased2EnableXsavesXrstors = 1 << 20,
	ProcessorBased2ModeBasedEpt = 1 << 22,
	ProcessorBased2SubPageEpt = 1 << 23,
	ProcessorBased2IntelPtGuestPhysical = 1 << 24,
	ProcessorBased2UseTscScaling = 1 << 25,
	ProcessorBased2EnableUserWaitPause = 1 << 26,
	ProcessorBased2EnclvExiting = 1 << 28,
} VmxProcessorBased2Controls;

typedef enum {
	ProcessorBasedInterruptWindowExiting = 1 << 2,
	ProcessorBasedUseTscOffsetting = 1 << 3,
	ProcessorBasedHltExiting = 1 << 7,
	ProcessorBasedInvlpgExiting = 1 << 9,
	ProcessorBasedMwaitExiting = 1 << 10,
	ProcessorBasedRdpmcExiting = 1 << 11,
	ProcessorBasedRdtscExiting = 1 << 12,
	ProcessorBasedCr3LoadExiting = 1 << 15,
	ProcessorBasedCr3StoreExiting = 1 << 16,
	ProcessorBasedCr8LoadExiting = 1 << 19,
	ProcessorBasedCr8StoreExiting = 1 << 20,
	ProcessorBasedUseTprShadow = 1 << 21,
	ProcessorBasedNmiWindowExiting = 1 << 22,
	ProcessorBasedMovDrExiting = 1 << 23,
	ProcessorBasedUncondIoExiting = 1 << 24,
	ProcessorBasedUseIoBitmaps = 1 << 25,
	ProcessorBasedMonitorTrapFlag = 1 << 27,
	ProcessorBasedUseMsrBitmaps = 1 << 28,
	ProcessorBasedMonitorExiting = 1 << 29,
	ProcessorBasedPauseExiting = 1 << 30,
	ProcessorBasedSecondaryControls = 1 << 31,
} VmxProcessorBasedControls;

typedef enum {
	PinBasedExternalInterruptExiting = 1 << 0,
	PinBasedNmiExiting = 1 << 3,
	PinBasedVirtualNmis = 1 << 5,
	PinBasedVmxPreemptionTimer = 1 << 6,
	PinBasedPostedInterrupts = 1 << 7,
} VmxPinBasedControls;

typedef enum {
	InterruptionTypeExternalInterrupt = 0,
	InterruptionTypeReserved = 1,
	InterruptionTypeNonMaskableInterrupt = 2,
	InterruptionTypeHardwareException = 3,
	InterruptionTypeSoftwareInterrupt = 4,
	InterruptionTypePrivilegedSoftwareException = 5,
	InterruptionTypeSoftwareException = 6,
	InterruptionTypeOther = 7
} VmxInterruptionType;

///////////////////////////////////////////////
////				VMCS				   ////
///////////////////////////////////////////////


#define VPID 0x0
#define POSTED_INTERRUPT_NOTIFICATION_VECTOR 0x2
#define EPTP_INDEX 0x4

#define IO_BITMAP_A_ADDR_FULL 0x2000
#define IO_BITMAP_A_ADDR_HIGH 0x2001
#define IO_BITMAP_B_ADDR_FULL 0x2002
#define IO_BITMAP_B_ADDR_HIGH 0x2003
#define MSR_BITMAPS_ADDR_FULL 0x2004
#define MSR_BITMAPS_ADDR_HIGH 0x2005
#define VMEXIT_MSR_STORE_ADDR_FULL 0x2006
#define VMEXIT_MSR_STORE_ADDR_HIGH 0x2007
#define VMEXIT_MSR_LOAD_ADDR_FULL 0x2008
#define VMEXIT_MSR_LOAD_ADDR_HIGH 0x2009
#define VMENTRY_MSR_LOAD_ADDR_FULL 0x200A
#define VMENTRY_MSR_LOAD_ADDR_HIGH 0x200B
#define EXECUTIVE_VMCS_PTR_FULL 0x200C
#define EXECUTIVE_VMCS_PTR_HIGH 0x200D
#define PML_ADDR_FULL 0x200E
#define PML_ADDR_HIGH 0x200F
#define TSC_OFFSET_FULL 0x2010
#define TSC_OFFSET_HIGH 0x2011
#define VIRT_APIC_ADDR_FULL 0x2012
#define VIRT_APIC_ADDR_HIGH 0x2013
#define APIC_ACCESS_ADDR_FULL 0x2014
#define APIC_ACCESS_ADDR_HIGH 0x2015
#define POSTED_INTERRUPT_DESC_ADDR_FULL 0x2016
#define POSTED_INTERRUPT_DESC_ADDR_HIGH 0x2017
#define VM_FUNCTION_CONTROLS_FULL 0x2018
#define VM_FUNCTION_CONTROLS_HIGH 0x2019
#define EPTP_FULL 0x201A
#define EPTP_HIGH 0x201B
#define EOI_EXIT0_FULL 0x201C
#define EOI_EXIT0_HIGH 0x201D
#define EOI_EXIT1_FULL 0x201E
#define EOI_EXIT1_HIGH 0x201F
#define EOI_EXIT2_FULL 0x2020
#define EOI_EXIT2_HIGH 0x2021
#define EOI_EXIT3_FULL 0x2022
#define EOI_EXIT3_HIGH 0x2023
#define EPTP_LIST_ADDR_FULL 0x2024
#define EPTP_LIST_ADDR_HIGH 0x2025
#define VMREAD_BITMAP_ADDR_FULL 0x2026
#define VMREAD_BITMAP_ADDR_HIGH 0x2027
#define VMWRITE_BITMAP_ADDR_FULL 0x2028
#define VMWRITE_BITMAP_ADDR_HIGH 0x2029
#define VIRT_EXCEPTION_INFO_ADDR_FULL 0x202A
#define VIRT_EXCEPTION_INFO_ADDR_HIGH 0x202B
#define XSS_EXITING_BITMAP_FULL 0x202C
#define XSS_EXITING_BITMAP_HIGH 0x202D
#define ENCLS_EXITING_BITMAP_FULL 0x202E
#define ENCLS_EXITING_BITMAP_HIGH 0x202F
#define SUBPAGE_PERM_TABLE_PTR_FULL 0x2030
#define SUBPAGE_PERM_TABLE_PTR_HIGH 0x2031
#define TSC_MULTIPLIER_FULL 0x2032
#define TSC_MULTIPLIER_HIGH 0x2033

#define PINBASED_EXEC_CONTROLS 0x4000
#define PRIMARY_PROCBASED_EXEC_CONTROLS 0x4002
#define EXCEPTION_BITMAP 0x4004
#define PAGE_FAULT_ERR_CODE_MASK 0x4006
#define PAGE_FAULT_ERR_CODE_MATCH 0x4008
#define CR3_TARGET_COUNT 0x400A
#define VMEXIT_CONTROLS 0x400C
#define VMEXIT_MSR_STORE_COUNT 0x400E
#define VMEXIT_MSR_LOAD_COUNT 0x4010
#define VMENTRY_CONTROLS 0x4012
#define VMENTRY_MSR_LOAD_COUNT 0x4014
#define VMENTRY_INTERRUPTION_INFO_FIELD 0x4016
#define VMENTRY_EXCEPTION_ERR_CODE 0x4018
#define VMENTRY_INSTRUCTION_LEN 0x401A
#define TPR_THRESHOLD 0x401C
#define SECONDARY_PROCBASED_EXEC_CONTROLS 0x401E
#define PLE_GAP 0x4020
#define PLE_WINDOW 0x4022

#define CR0_GUEST_HOST_MASK 0x6000
#define CR4_GUEST_HOST_MASK 0x6002
#define CR0_READ_SHADOW 0x6004
#define CR4_READ_SHADOW 0x6006
#define CR3_TARGET_VALUE0 0x6008
#define CR3_TARGET_VALUE1 0x600A
#define CR3_TARGET_VALUE2 0x600C
#define CR3_TARGET_VALUE3 0x600E




#define HOST_ES_SELECTOR 0xC00
#define HOST_CS_SELECTOR 0xC02
#define HOST_SS_SELECTOR 0xC04
#define HOST_DS_SELECTOR 0xC06
#define HOST_FS_SELECTOR 0xC08
#define HOST_GS_SELECTOR 0xC0A
#define HOST_TR_SELECTOR 0xC0C

#define HOST_IA32_PAT_FULL 0x2C00
#define HOST_IA32_PAT_HIGH 0x2C01
#define HOST_IA32_EFER_FULL 0x2C02
#define HOST_IA32_EFER_HIGH 0x2C03
#define HOST_IA32_PERF_GLOBAL_CTRL_FULL 0x2C04
#define HOST_IA32_PERF_GLOBAL_CTRL_HIGH 0x2C05

#define HOST_IA32_SYSENTER_CS 0x4C00

#define HOST_CR0 0x6C00
#define HOST_CR3 0x6C02
#define HOST_CR4 0x6C04
#define HOST_FS_BASE 0x6C06
#define HOST_GS_BASE 0x6C08
#define HOST_TR_BASE 0x6C0A
#define HOST_GDTR_BASE 0x6C0C
#define HOST_IDTR_BASE 0x6C0E
#define HOST_IA32_SYSENTER_ESP 0x6C10
#define HOST_IA32_SYSENTER_EIP 0x6C12
#define HOST_RSP 0x6C14
#define HOST_RIP 0x6C16


#define READONLY_GUEST_PHYSICAL_ADDR_FULL 0x2400
#define READONLY_GUEST_PHYSICAL_ADDR_HIGH 0x2401

#define READONLY_VM_INSTRUCTION_ERROR 0x4400
#define READONLY_EXIT_REASON 0x4402
#define READONLY_VMEXIT_INTERRUPTION_INFO 0x4404
#define READONLY_VMEXIT_INTERRUPTION_ERR_CODE 0x4406
#define READONLY_IDT_VECTORING_INFO 0x4408
#define READONLY_IDT_VECTORING_ERR_CODE 0x440A
#define READONLY_VMEXIT_INSTRUCTION_LEN 0x440C
#define READONLY_VMEXIT_INSTRUCTION_INFO 0x440E

#define READONLY_EXIT_QUALIFICATION 0x6400
#define READONLY_IO_RCX 0x6402
#define READONLY_IO_RSI 0x6404
#define READONLY_IO_RDI 0x6406
#define READONLY_IO_RIP 0x6408
#define READONLY_GUEST_LINEAR_ADDR 0x640A

#define GUEST_ES_SELECTOR 0x800
#define GUEST_CS_SELECTOR 0x802
#define GUEST_SS_SELECTOR 0x804
#define GUEST_DS_SELECTOR 0x806
#define GUEST_FS_SELECTOR 0x808
#define GUEST_GS_SELECTOR 0x80A
#define GUEST_LDTR_SELECTOR 0x80C
#define GUEST_TR_SELECTOR 0x80E
#define GUEST_INTERRUPT_STATUS 0x810
#define GUEST_PML_INDEX 0x812

#define GUEST_LINK_PTR_FULL 0x2800
#define GUEST_LINK_PTR_HIGH 0x2801
#define GUEST_IA32_DEBUGCTL_FULL 0x2802
#define GUEST_IA32_DEBUGCTL_HIGH 0x2803
#define GUEST_IA32_PAT_FULL 0x2804
#define GUEST_IA32_PAT_HIGH 0x2805
#define GUEST_IA32_EFER_FULL 0x2806
#define GUEST_IA32_EFER_HIGH 0x2807
#define GUEST_IA32_PERF_GLOBAL_CTRL_FULL 0x2808
#define GUEST_IA32_PERF_GLOBAL_CTRL_HIGH 0x2809
#define GUEST_PDPTE0_FULL 0x280A
#define GUEST_PDPTE0_HIGH 0x280B
#define GUEST_PDPTE1_FULL 0x280C
#define GUEST_PDPTE1_HIGH 0x280D
#define GUEST_PDPTE2_FULL 0x280E
#define GUEST_PDPTE2_HIGH 0x280F
#define GUEST_PDPTE3_FULL 0x2810
#define GUEST_PDPTE3_HIGH 0x2811
#define GUEST_IA32_BNDCFGS_FULL 0x2812
#define GUEST_IA32_BNDCFGS_HIGH 0x2813
#define GUEST_IA32_RTIT_CTL_FULL 0x2814
#define GUEST_IA32_RTIT_CTL_HIGH 0x2815

#define GUEST_ES_LIMIT 0x4800
#define GUEST_CS_LIMIT 0x4802
#define GUEST_SS_LIMIT 0x4804
#define GUEST_DS_LIMIT 0x4806
#define GUEST_FS_LIMIT 0x4808
#define GUEST_GS_LIMIT 0x480A
#define GUEST_LDTR_LIMIT 0x480C
#define GUEST_TR_LIMIT 0x480E
#define GUEST_GDTR_LIMIT 0x4810
#define GUEST_IDTR_LIMIT 0x4812
#define GUEST_ES_ACCESS_RIGHTS 0x4814
#define GUEST_CS_ACCESS_RIGHTS 0x4816
#define GUEST_SS_ACCESS_RIGHTS 0x4818
#define GUEST_DS_ACCESS_RIGHTS 0x481A
#define GUEST_FS_ACCESS_RIGHTS 0x481C
#define GUEST_GS_ACCESS_RIGHTS 0x481E
#define GUEST_LDTR_ACCESS_RIGHTS 0x4820
#define GUEST_TR_ACCESS_RIGHTS 0x4822
#define GUEST_INTERRUPTIBILITY_STATE 0x4824
#define GUEST_ACTIVITY_STATE 0x4826
#define GUEST_SMBASE 0x4828
#define GUEST_IA32_SYSENTER_CS 0x482A
#define GUEST_VMX_PREEMPTION_TIMER_VALUE 0x482E

#define GUEST_CR0 0x6800
#define GUEST_CR3 0x6802
#define GUEST_CR4 0x6804
#define GUEST_ES_BASE 0x6806
#define GUEST_CS_BASE 0x6808
#define GUEST_SS_BASE 0x680A
#define GUEST_DS_BASE 0x680C
#define GUEST_FS_BASE 0x680E
#define GUEST_GS_BASE 0x6810
#define GUEST_LDTR_BASE 0x6812
#define GUEST_TR_BASE 0x6814
#define GUEST_GDTR_BASE 0x6816
#define GUEST_IDTR_BASE 0x6818
#define GUEST_DR7 0x681A
#define GUEST_RSP 0x681C
#define GUEST_RIP 0x681E
#define GUEST_RFLAGS 0x6820
#define GUEST_PENDING_DBG_EXCEPTIONS 0x6822
#define GUEST_IA32_SYSENTER_ESP 0x6824
#define GUEST_IA32_SYSENTER_EIP 0x6826



#define VMEXIT_REASON_EXCEPTION_OR_NMI 0
#define VMEXIT_REASON_EXT_INT 1
#define VMEXIT_REASON_TRIPLE_FAULT 2
#define VMEXIT_REASON_INIT_SIGNAL 3
#define VMEXIT_REASON_SIPI 4
#define VMEXIT_REASON_SMI 5
#define VMEXIT_REASON_OTHER_SMI 6
#define VMEXIT_REASON_INTERRUPT_WINDOW 7
#define VMEXIT_REASON_NMI_WINDOW 8
#define VMEXIT_REASON_TASK_SWITCH 9
#define VMEXIT_REASON_CPUID 10
#define VMEXIT_REASON_GETSEC 11
#define VMEXIT_REASON_HLT 12
#define VMEXIT_REASON_INVD 13
#define VMEXIT_REASON_INVLPG 14
#define VMEXIT_REASON_RDPMC 15
#define VMEXIT_REASON_RDTSC 16
#define VMEXIT_REASON_RSM 17
#define VMEXIT_REASON_VMCALL 18
#define VMEXIT_REASON_VMCLEAR 19
#define VMEXIT_REASON_VMLAUNCH 20
#define VMEXIT_REASON_VMPTRLD 21
#define VMEXIT_REASON_VMPTRST 22
#define VMEXIT_REASON_VMREAD 23
#define VMEXIT_REASON_VMRESUME 24
#define VMEXIT_REASON_VMWRITE 25
#define VMEXIT_REASON_VMXOFF 26
#define VMEXIT_REASON_VMXON 27
#define VMEXIT_REASON_CONTROL_REGISTER_ACCESS 28
#define VMEXIT_REASON_MOV_DR 29
#define VMEXIT_REASON_IO_INSTRUCTION 30
#define VMEXIT_REASON_RDMSR 31
#define VMEXIT_REASON_WRMSR 32
#define VMEXIT_REASON_VMENTRY_FAILURE_DUE_TO_GUEST_STATE 33
#define VMEXIT_REASON_VMENTRY_FAILURE_DUE_TO_MSR_LOADING 34
#define VMEXIT_REASON_MWAIT 36
#define VMEXIT_REASON_MONITOR_TRAP_FLAG 37
#define VMEXIT_REASON_MONITOR 39
#define VMEXIT_REASON_PAUSE 40
#define VMEXIT_REASON_VMENTRY_FAILURE_DUE_TO_MACHINE_CHECK_EVENT 41
#define VMEXIT_REASON_TPR_BELOW_THRESHOLD 43
#define VMEXIT_REASON_APIC_ACCESS 44
#define VMEXIT_REASON_VIRTUALIZED_EOI 45
#define VMEXIT_REASON_ACCESS_TO_GDTR_OR_IDTR 46
#define VMEXIT_REASON_ACCESS_TO_LDTR_OR_TR 47
#define VMEXIT_REASON_EPT_VIOLATION 48
#define VMEXIT_REASON_EPT_MISCONFIGURATION 49
#define VMEXIT_REASON_INVEPT 50
#define VMEXIT_REASON_RDTSCP 51
#define VMEXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED 52
#define VMEXIT_REASON_INVVPID 53
#define VMEXIT_REASON_WBINVD 54
#define VMEXIT_REASON_XSETBV 55
#define VMEXIT_REASON_APIC_WRITE 56
#define VMEXIT_REASON_RDRAND 57
#define VMEXIT_REASON_INVPCID 58
#define VMEXIT_REASON_VMFUNC 59
#define VMEXIT_REASON_ENCLS 60
#define VMEXIT_REASON_RDSEED 61
#define VMEXIT_REASON_PML_FULL 62
#define VMEXIT_REASON_XSAVES 63
#define VMEXIT_REASON_XRSTORS 64

#endif