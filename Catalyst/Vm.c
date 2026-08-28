#include <Uefi.h>
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include "x86.h"
#include "Vm.h"
#include "Efi.h"
#include "Memory.h"
#include "Firmware.h"

__attribute__((aligned(4096))) CATALYST_VIRTUAL_MACHINE g_VmState = { 0 };

STATIC
VOID
EFIAPI
DumpVmcs(
	VOID
) {
#define DUMP_VMCS_FIELD(Field, Name) \
	do { \
		UINT64 _val = 0; \
		if (__vmx_vmread(Field, &_val) == 0) \
			DebugPrint0402("%a = 0x%p\n", Name, _val); \
		else \
			DebugPrint0402("%a = [READ ERROR]\n", Name); \
	} while (0)


	DebugPrint0402("--- BEGIN VMCS DUMP ---\n");

	//
	// Controls
	//
	DUMP_VMCS_FIELD(VMEXIT_CONTROLS, "VMEXIT_CONTROLS");
	DUMP_VMCS_FIELD(VMENTRY_CONTROLS, "VMENTRY_CONTROLS");

	DUMP_VMCS_FIELD(PRIMARY_PROCBASED_EXEC_CONTROLS, "PRIMARY_PROCBASED_EXEC_CONTROLS");
	DUMP_VMCS_FIELD(SECONDARY_PROCBASED_EXEC_CONTROLS, "SECONDARY_PROCBASED_EXEC_CONTROLS");
	DUMP_VMCS_FIELD(PINBASED_EXEC_CONTROLS, "PINBASED_EXEC_CONTROLS");

	DUMP_VMCS_FIELD(EPTP_FULL, "EPTP_FULL");
	DUMP_VMCS_FIELD(MSR_BITMAPS_ADDR_FULL, "MSR_BITMAPS_ADDR_FULL");

	//
	// Host
	//
	DUMP_VMCS_FIELD(HOST_ES_SELECTOR, "HOST_ES_SELECTOR");
	DUMP_VMCS_FIELD(HOST_CS_SELECTOR, "HOST_CS_SELECTOR");
	DUMP_VMCS_FIELD(HOST_SS_SELECTOR, "HOST_SS_SELECTOR");
	DUMP_VMCS_FIELD(HOST_DS_SELECTOR, "HOST_DS_SELECTOR");
	DUMP_VMCS_FIELD(HOST_FS_SELECTOR, "HOST_FS_SELECTOR");
	DUMP_VMCS_FIELD(HOST_GS_SELECTOR, "HOST_GS_SELECTOR");
	DUMP_VMCS_FIELD(HOST_TR_SELECTOR, "HOST_TR_SELECTOR");

	DUMP_VMCS_FIELD(HOST_CR0, "HOST_CR0");
	DUMP_VMCS_FIELD(HOST_CR3, "HOST_CR3");
	DUMP_VMCS_FIELD(HOST_CR4, "HOST_CR4");

	DUMP_VMCS_FIELD(HOST_FS_BASE, "HOST_FS_BASE");
	DUMP_VMCS_FIELD(HOST_GS_BASE, "HOST_GS_BASE");
	DUMP_VMCS_FIELD(HOST_TR_BASE, "HOST_TR_BASE");
	DUMP_VMCS_FIELD(HOST_GDTR_BASE, "HOST_GDTR_BASE");
	DUMP_VMCS_FIELD(HOST_IDTR_BASE, "HOST_IDTR_BASE");

	DUMP_VMCS_FIELD(HOST_RIP, "HOST_RIP");
	DUMP_VMCS_FIELD(HOST_RSP, "HOST_RSP");

	//
	// Guest
	//
	DUMP_VMCS_FIELD(GUEST_ES_SELECTOR, "GUEST_ES_SELECTOR");
	DUMP_VMCS_FIELD(GUEST_SS_SELECTOR, "GUEST_SS_SELECTOR");
	DUMP_VMCS_FIELD(GUEST_DS_SELECTOR, "GUEST_DS_SELECTOR");
	DUMP_VMCS_FIELD(GUEST_LDTR_SELECTOR, "GUEST_LDTR_SELECTOR");

	DUMP_VMCS_FIELD(GUEST_TR_SELECTOR, "GUEST_TR_SELECTOR");
	DUMP_VMCS_FIELD(GUEST_FS_SELECTOR, "GUEST_FS_SELECTOR");
	DUMP_VMCS_FIELD(GUEST_GS_SELECTOR, "GUEST_GS_SELECTOR");
	DUMP_VMCS_FIELD(GUEST_CS_SELECTOR, "GUEST_CS_SELECTOR");

	DUMP_VMCS_FIELD(GUEST_ES_LIMIT, "GUEST_ES_LIMIT");
	DUMP_VMCS_FIELD(GUEST_SS_LIMIT, "GUEST_SS_LIMIT");
	DUMP_VMCS_FIELD(GUEST_DS_LIMIT, "GUEST_DS_LIMIT");
	DUMP_VMCS_FIELD(GUEST_CS_LIMIT, "GUEST_CS_LIMIT");

	DUMP_VMCS_FIELD(GUEST_FS_LIMIT, "GUEST_FS_LIMIT");
	DUMP_VMCS_FIELD(GUEST_GS_LIMIT, "GUEST_GS_LIMIT");
	DUMP_VMCS_FIELD(GUEST_TR_LIMIT, "GUEST_TR_LIMIT");

	DUMP_VMCS_FIELD(GUEST_ES_ACCESS_RIGHTS, "GUEST_ES_ACCESS_RIGHTS");
	DUMP_VMCS_FIELD(GUEST_CS_ACCESS_RIGHTS, "GUEST_CS_ACCESS_RIGHTS");
	DUMP_VMCS_FIELD(GUEST_SS_ACCESS_RIGHTS, "GUEST_SS_ACCESS_RIGHTS");
	DUMP_VMCS_FIELD(GUEST_DS_ACCESS_RIGHTS, "GUEST_DS_ACCESS_RIGHTS");
	DUMP_VMCS_FIELD(GUEST_FS_ACCESS_RIGHTS, "GUEST_FS_ACCESS_RIGHTS");
	DUMP_VMCS_FIELD(GUEST_GS_ACCESS_RIGHTS, "GUEST_GS_ACCESS_RIGHTS");
	DUMP_VMCS_FIELD(GUEST_TR_ACCESS_RIGHTS, "GUEST_TR_ACCESS_RIGHTS");
	DUMP_VMCS_FIELD(GUEST_LDTR_ACCESS_RIGHTS, "GUEST_LDTR_ACCESS_RIGHTS");

	DUMP_VMCS_FIELD(GUEST_FS_BASE, "GUEST_FS_BASE");
	DUMP_VMCS_FIELD(GUEST_GS_BASE, "GUEST_GS_BASE");

	DUMP_VMCS_FIELD(GUEST_IDTR_BASE, "GUEST_IDTR_BASE");
	DUMP_VMCS_FIELD(GUEST_IDTR_LIMIT, "GUEST_IDTR_LIMIT");

	DUMP_VMCS_FIELD(GUEST_TR_BASE, "GUEST_TR_BASE");

	DUMP_VMCS_FIELD(GUEST_GDTR_BASE, "GUEST_GDTR_BASE");
	DUMP_VMCS_FIELD(GUEST_GDTR_LIMIT, "GUEST_GDTR_LIMIT");

	DUMP_VMCS_FIELD(GUEST_IA32_SYSENTER_CS, "GUEST_IA32_SYSTENTER_CS");
	DUMP_VMCS_FIELD(GUEST_IA32_SYSENTER_EIP, "GUEST_IA32_SYSENTER_EIP");
	DUMP_VMCS_FIELD(GUEST_IA32_SYSENTER_ESP, "GUEST_IA32_SYSENTER_ESP");

	DUMP_VMCS_FIELD(GUEST_IA32_EFER_FULL, "GUEST_IA32_EFER_FULL");

	DUMP_VMCS_FIELD(GUEST_LINK_PTR_FULL, "GUEST_LINK_PTR_FULL");

	DUMP_VMCS_FIELD(GUEST_CR0, "GUEST_CR0");
	DUMP_VMCS_FIELD(GUEST_CR3, "GUEST_CR3");
	DUMP_VMCS_FIELD(GUEST_CR4, "GUEST_CR4");

	DUMP_VMCS_FIELD(GUEST_RFLAGS, "GUEST_RFLAGS");

	DUMP_VMCS_FIELD(READONLY_GUEST_PHYSICAL_ADDR_FULL, "READONLY_GUEST_PHYSICAL_ADDR_FULL");
	DUMP_VMCS_FIELD(GUEST_PDPTE0_FULL, "GUEST_PDPTE0_FULL");
	DUMP_VMCS_FIELD(GUEST_PDPTE1_FULL, "GUEST_PDPTE1_FULL");
	DUMP_VMCS_FIELD(GUEST_PDPTE2_FULL, "GUEST_PDPTE2_FULL");
	DUMP_VMCS_FIELD(GUEST_PDPTE3_FULL, "GUEST_PDPTE3_FULL");

	DebugPrint0402("--- END VMCS DUMP ---\n");
	DebugPrint0402("--- BEGIN CONTEXT DUMP ---\n");

	CATALYST_VIRTUAL_CPU* Cpu = (CATALYST_VIRTUAL_CPU*)_readgsbase_u64();

	DebugPrint0402(
		"RIP = 0x%p\n"
		"RSP = 0x%p\n"
		"RAX = 0x%p\n"
		"RBX = 0x%p\n"
		"RCX = 0x%p\n"
		"RDX = 0x%p\n"
		"RSI = 0x%p\n"
		"RDI = 0x%p\n"
		"RBP = 0x%p\n"
		"R8  = 0x%p\n"
		"R9  = 0x%p\n"
		"R10 = 0x%p\n"
		"R11 = 0x%p\n"
		"R12 = 0x%p\n"
		"R13 = 0x%p\n"
		"R14 = 0x%p\n"
		"R15 = 0x%p\n",
		Cpu->Context.Rip,
		Cpu->Context.Rsp,
		Cpu->Context.Rax,
		Cpu->Context.Rbx,
		Cpu->Context.Rcx,
		Cpu->Context.Rdx,
		Cpu->Context.Rsi,
		Cpu->Context.Rdi,
		Cpu->Context.Rbp,
		Cpu->Context.R8,
		Cpu->Context.R9,
		Cpu->Context.R10,
		Cpu->Context.R11,
		Cpu->Context.R12,
		Cpu->Context.R13,
		Cpu->Context.R14,
		Cpu->Context.R15
	);

	DebugPrint0402("--- END CONTEXT DUMP ---\n");

#undef DUMP_VMCS_FIELD
}

VOID
EFIAPI
InjectEvent(
	IN UINT32					ErrorCode	OPTIONAL,
	IN BOOLEAN					DeliverErrorCode,
	IN ExceptionInterruptType	Vector,
	IN VmxInterruptionType		Interruption,
	IN BOOLEAN					Valid
) {
	VMX_INTERRUPTION_INFORMATION Event = {
		.Valid = Valid,
		.Type = Interruption,
		.Vector = Vector,
		.DeliverErrorCode = DeliverErrorCode
	};

	if (DeliverErrorCode)
		__vmx_vmwrite(VMENTRY_EXCEPTION_ERR_CODE, ErrorCode);

	__vmx_vmwrite(VMENTRY_INTERRUPTION_INFO_FIELD, Event.All);
}

VOID
VmExitHandler(

) {
	//
	// Retrieve per-CPU structure via GS base.
	//
	CATALYST_VIRTUAL_CPU* Cpu = (CATALYST_VIRTUAL_CPU*)_readgsbase_u64();

	UINT64 ExitQualification = 0;
	UINT64 ExitReason = 0;
	UINT64 ExitInstructionLen = 0;

	UINT64 HpaRip = 0;
	UINT64 HpaRsp = 0;

	__vmx_vmread(READONLY_EXIT_QUALIFICATION, &ExitQualification);
	__vmx_vmread(READONLY_EXIT_REASON, &ExitReason);
	__vmx_vmread(READONLY_VMEXIT_INSTRUCTION_LEN, &ExitInstructionLen);

	__vmx_vmread(GUEST_RIP, &Cpu->Context.Rip);
	__vmx_vmread(GUEST_RSP, &Cpu->Context.Rsp);
	//
	// I guess
	//
	__vmx_vmread(GUEST_RFLAGS, &Cpu->Context.RFlags);

	__vmx_vmread(GUEST_CR3, &Cpu->ExtendedContext.Cr3.All);

	//
	// Only the lower 16 bits contain the basic exit reason.
	//
	ExitReason &= 0xFFFF;

	if (ExitReason == VMEXIT_REASON_IO_INSTRUCTION) {
		//
		// Completely skip.
		//
		__vmx_vmwrite(GUEST_RIP, Cpu->Context.Rip + ExitInstructionLen);
		return;
	}

	DebugPrint0402("VMEXIT[%d]: Reason = %d (0x%x), Qualification = 0x%p, ExitInstructionLen = %d, GuestRip = 0x%p, GuestRsp = 0x%p, GuestCr3 = 0x%p\n",
		Cpu->Index, ExitReason, ExitReason, ExitQualification, ExitInstructionLen,
		Cpu->Context.Rip, Cpu->Context.Rsp, Cpu->ExtendedContext.Cr3.All);

	BOOLEAN Borked = FALSE;

	switch (ExitReason) {
	case VMEXIT_REASON_CPUID:
		DebugPrint0402("Guest CPUID\n");
		AsmCpuid(Cpu->Context.Rax, (UINT32*)&Cpu->Context.Rax, (UINT32*)&Cpu->Context.Rbx, (UINT32*)&Cpu->Context.Rcx, (UINT32*)&Cpu->Context.Rdx);
		break;
	case VMEXIT_REASON_VMENTRY_FAILURE_DUE_TO_GUEST_STATE:
		DebugPrint0402("VMCS is borked\n");
		Borked = TRUE;
		break;

	case VMEXIT_REASON_TRIPLE_FAULT:
		DebugPrint0402("Guest triple-faulted\n");
		Borked = TRUE;
		break;

	case VMEXIT_REASON_EXCEPTION_OR_NMI: {
		VMX_INTERRUPTION_INFORMATION InterruptionInfo = { 0 };
		__vmx_vmread(READONLY_VMEXIT_INTERRUPTION_INFO, (UINT64*)&InterruptionInfo.All);

		DebugPrint0402("Guest exception occured. Vector = %d, Type = %d, Deliver = %d, Valid = %d\n",
			InterruptionInfo.Vector, InterruptionInfo.Type,
			InterruptionInfo.DeliverErrorCode, InterruptionInfo.Valid);

		//
		// Page fault.
		//
		if (InterruptionInfo.Vector == 14) {
			VMX_PAGE_FAULT_ERROR_CODE PageFaultErrorCode = { 0 };
			__vmx_vmread(PAGE_FAULT_ERR_CODE_MASK, (UINT64*)&PageFaultErrorCode.All);

			DebugPrint0402("Page Fault occured. Present = %d, Write = %d, User = %d, "
				"ReservedBitSet = %d, NxViolation = %d, ProtectionKeyInvalid = %d\n",
				PageFaultErrorCode.Present, PageFaultErrorCode.Write,
				PageFaultErrorCode.User, PageFaultErrorCode.ReservedBitSet,
				PageFaultErrorCode.NxViolation, PageFaultErrorCode.ProtectionKeyViolation);
		}

		//
		// For now, all exceptions are considered fatal.
		//
		Borked = TRUE;
		break;
	}

	case VMEXIT_REASON_EPT_VIOLATION: {
		VMX_EPT_VIOLATION_QUAILIFICATION EptQualification = { .All = (UINT32)ExitQualification };
		UINT64 Gpa = 0;
		__vmx_vmread(READONLY_GUEST_PHYSICAL_ADDR_FULL, &Gpa);

		DebugPrint0402("EPT Violation (0x%p). Read = %d, Write = %d, Execute = %d, "
			"IsReadable = %d, IsWritable = %d, IsExecutable = %d, "
			"UserExecute = %d, GuestLinearAddressValid = %d, TranslationBork = %d\n",
			Gpa,
			EptQualification.Read, EptQualification.Write, EptQualification.Execute,
			EptQualification.IsReadable, EptQualification.IsWritable, EptQualification.IsExecutable,
			EptQualification.UserExecute, EptQualification.GuestLinearAddressValid,
			EptQualification.TranslationBork);

		EPT_PTE* Pte = NULL;
		if (!GetGuestEptEntry(Gpa, NULL, NULL, NULL, &Pte, NULL)) {
			DebugPrint0402("EPT entry does not exist\n");
		}
		else {
			DebugPrint0402("EPT PTE: Present = %d, Read = %d, Write = %d, Execute = %d, ExecuteUser = %d\n",
				Pte->Present, Pte->Read, Pte->Write, Pte->Execute, Pte->ExecuteUser);
		}

		Borked = TRUE;
		break;
	}

	case VMEXIT_REASON_WRMSR: {
		UINT64 MsrValue = ((Cpu->Context.Rdx & 0xFFFFFFFF) << 32) | (Cpu->Context.Rax & 0xFFFFFFFF);
		UINT64 Msr = Cpu->Context.Rcx;

		DebugPrint0402("Guest writes 0x%p to MSR 0x%p\n", MsrValue, Msr);

		if (Msr == IA32_EFER) {
			DebugPrint0402("Passthrough EFER...\n");
			__vmx_vmwrite(GUEST_IA32_EFER_FULL, MsrValue);
		}
		else if (Msr == IA32_GS_BASE) {
			__vmx_vmwrite(GUEST_GS_BASE, MsrValue);
		}
		else if (Msr == IA32_FS_BASE) {
			__vmx_vmwrite(GUEST_FS_BASE, MsrValue);
		}
		else if (Msr == IA32_KERNEL_GS_BASE) {
			DebugPrint0402("UH oh\n", Msr, MsrValue);
		}
		else {
			__writemsr(Msr, MsrValue);
		}
		break;
	}
	case VMEXIT_REASON_RDMSR: {
		UINT64 Msr = Cpu->Context.Rcx;
		UINT64 MsrValue = 0;

		if (Msr == IA32_EFER) {
			__vmx_vmread(GUEST_IA32_EFER_FULL, &MsrValue);
		}
		else if (Msr == IA32_GS_BASE) {
			__vmx_vmread(GUEST_GS_BASE, &MsrValue);
		}
		else if (Msr == IA32_FS_BASE) {
			__vmx_vmread(GUEST_FS_BASE, &MsrValue);
		}
		else if (Msr == IA32_KERNEL_GS_BASE) {
			DebugPrint0402("UH oh\n", Msr, MsrValue);
		}
		else {
			MsrValue = __readmsr(Msr);
		}

		DebugPrint0402("Guest reads MSR 0x%p as 0x%p\n", Msr, MsrValue);

		Cpu->Context.Rdx = (UINT32)(MsrValue >> 32);
		Cpu->Context.Rax = (UINT32)MsrValue;
		break;
	}


	case VMEXIT_REASON_VMCALL:
		DebugPrint0402("VMCALL[%d]: Fn=0x%x RCX=0x%p RDX=0x%p R8=0x%p R9=0x%p R10=0x%p R11=0x%p\n",
			Cpu->Index,
			(UINT32)Cpu->Context.Rax, Cpu->Context.Rcx, Cpu->Context.Rdx,
			Cpu->Context.R8, Cpu->Context.R9, Cpu->Context.R10, Cpu->Context.R11);

		//
		// What a beautiful, little switch case.... It's so hard to resist making it into a jump table....
		//
		switch (Cpu->Context.Rax) {
		case CATALYST_VMCALL_GET_VARIABLE:
			DebugPrint0402("  VMCALL -> GetVariable\n");
			FirmwareGetVariable(Cpu);
			break;
		case CATALYST_VMCALL_SET_VARIABLE:
			DebugPrint0402("  VMCALL -> SetVariable\n");
			FirmwareSetVariable(Cpu);
			break;
		case CATALYST_VMCALL_GET_NEXT_VARIABLE_NAME:
			DebugPrint0402("  VMCALL -> GetNextVariableName\n");
			FirmwareGetNextVariableName(Cpu);
			break;
		case CATALYST_VMCALL_QUERY_VARIABLE_INFO:
			DebugPrint0402("  VMCALL -> QueryVariableInfo\n");
			FirmwareQueryVariableInfo(Cpu);
			break;
		case CATALYST_VMCALL_GET_TIME:
			DebugPrint0402("  VMCALL -> GetTime\n");
			FirmwareGetTime(Cpu);
			break;
		case CATALYST_VMCALL_SET_TIME:
			DebugPrint0402("  VMCALL -> SetTime\n");
			FirmwareSetTime(Cpu);
			break;
		case CATALYST_VMCALL_GET_WAKEUP_TIME:
			DebugPrint0402("  VMCALL -> GetWakeupTime\n");
			FirmwareGetWakeupTime(Cpu);
			break;
		case CATALYST_VMCALL_SET_WAKEUP_TIME:
			DebugPrint0402("  VMCALL -> SetWakeupTime\n");
			FirmwareSetWakeupTime(Cpu);
			break;
		case CATALYST_VMCALL_GET_NEXT_HIGH_MONO_COUNT:
			DebugPrint0402("  VMCALL -> GetNextHighMonotonicCount\n");
			FirmwareGetNextHighMonotonicCount(Cpu);
			break;
		case CATALYST_VMCALL_RESET_SYSTEM:
			DebugPrint0402("  VMCALL -> ResetSystem - HALTING\n");
			__asm__ __volatile__("hlt");
			break;

		case CATALYST_VMCALL_EXIT_BOOT_SERVICES:
			DebugPrint0402("  VMCALL -> ExitBootServices\n");
			FirmwareExitBootServices(Cpu);
			break;
		case CATALYST_VMCALL_FREE_POOL:
			DebugPrint0402("  VMCALL -> FreePool\n");
			FirmwareFreePool(Cpu);
			break;
		case CATALYST_VMCALL_FREE_PAGES:
			DebugPrint0402("  VMCALL -> FreePages\n");
			FirmwareFreePages(Cpu);
			break;
		case CATALYST_VMCALL_ALLOCATE_POOL:
			DebugPrint0402("  VMCALL -> AllocatePool\n");
			FirmwareAllocatePool(Cpu);
			break;
		case CATALYST_VMCALL_ALLOCATE_PAGES:
			DebugPrint0402("  VMCALL -> AllocatePages\n");
			FirmwareAllocatePages(Cpu);
			break;
		case CATALYST_VMCALL_GET_MEMORY_MAP:
			DebugPrint0402("  VMCALL -> GetMemoryMap\n");
			FirmwareGetMemoryMap(Cpu);
			break;
		case CATALYST_VMCALL_HANDLE_PROTOCOL:
			DebugPrint0402("  VMCALL -> HandleProtocol\n");
			FirmwareHandleProtocol(Cpu);
			break;
		case CATALYST_VMCALL_LOCATE_PROTOCOL:
			DebugPrint0402("  VMCALL -> LocateProtocol\n");
			FirmwareLocateProtocol(Cpu);
			break;
		case CATALYST_VMCALL_EXECUTE_PROTOCOL:
			DebugPrint0402("  VMCALL -> ExecuteProtocol\n");
			FirmwareExecuteProtocol(Cpu);
			break;
		default:
			DebugPrint0402("  VMCALL -> UNKNOWN 0x%x, returning EFI_NOT_FOUND\n", (UINT32)Cpu->Context.Rax);
			Cpu->Context.Rax = EFI_NOT_FOUND;
		}

		DebugPrint0402("VMCALL[%d] returned RAX=0x%p (%r)\n", Cpu->Index, Cpu->Context.Rax, Cpu->Context.Rax);

		//
		// Fall through to advance RIP
		//
		break;
	case VMEXIT_REASON_IO_INSTRUCTION: {
		//
		// Ok.
		//

		VMX_EXIT_QUALIFICATION_IO IoQualification = { .All = ExitQualification };

		if (IoQualification.Direction == 0 && IoQualification.PortNumber == UART_COM1_PORT) {
			__outb(UART_COM1_PORT, (UINT8)Cpu->Context.Rax);
			break;
		}

		if (IoQualification.Direction == 0) {
			DebugPrint0402("Guest wrote %d to port %d\n", Cpu->Context.Rax, IoQualification.PortNumber);
		} else {
			DebugPrint0402("Guest read from port %d\n", IoQualification.PortNumber);
		}


		break;
	}
	case VMEXIT_REASON_EXT_INT: {
		//
		// Not our problem.
		//
		VMX_INTERRUPTION_INFORMATION InterruptionInfo = { 0 };
		__vmx_vmread(READONLY_VMEXIT_INTERRUPTION_INFO, (UINT64*)&InterruptionInfo.All);

		DebugPrint0402("Guest received external interrupt. Vector = %d, Type = %d, Deliver = %d, Valid = %d\n",
			InterruptionInfo.Vector, InterruptionInfo.Type,
			InterruptionInfo.DeliverErrorCode, InterruptionInfo.Valid);

		if (InterruptionInfo.Valid) {
			//
			// Retarded PIC.
			//
			if (InterruptionInfo.Vector == 32) {
				volatile UINT32* LApicEoi = (volatile UINT32*)0xFEE000B0;
				*LApicEoi = 0;
			}

			//
			// EOI the pic.
			//
			if (InterruptionInfo.Vector >= PIC_IRQ_OFFSET && InterruptionInfo.Vector <= PIC_IRQ_END) {
				__outb(PIC1_COMMAND, PIC_EOI);
				if (InterruptionInfo.Vector >= PIC_SLAVE_OFFSET) {
					__outb(PIC2_COMMAND, PIC_EOI);
				}
			}

			if ((Cpu->Context.RFlags & RFLAGS_IF) != 0) {
				DebugPrint0402("External interrupt has been injected\n");
				InjectEvent(0, FALSE, InterruptionInfo.Vector, InterruptionTypeExternalInterrupt, InterruptionInfo.Valid);
			}
			else {
				DebugPrint0402("Guest has RFLAGS.IF disabled. Enabling external interrupt exiting.\n");
				//
				// Retarded guest.
				//
				Cpu->Hardware.HasPendingInterrupt = TRUE;
				Cpu->Hardware.PendingInterrupt = InterruptionInfo;

				UINT64 PrimaryControl = 0;
				__vmx_vmread(PRIMARY_PROCBASED_EXEC_CONTROLS, &PrimaryControl);
				PrimaryControl |= ProcessorBasedInterruptWindowExiting;
				__vmx_vmwrite(PRIMARY_PROCBASED_EXEC_CONTROLS, PrimaryControl);
			}

		}

		//
		// Stay where you are.
		//
		ExitInstructionLen = 0;
		break;
	}
	case VMEXIT_REASON_INTERRUPT_WINDOW: {
		DebugPrint0402("Guest enabled RFLAGS.IF. Injecting pending interrupt...\n");

		UINT64 PrimaryControl = 0;
		__vmx_vmread(PRIMARY_PROCBASED_EXEC_CONTROLS, &PrimaryControl);

		PrimaryControl &= ~ProcessorBasedInterruptWindowExiting;
		__vmx_vmwrite(PRIMARY_PROCBASED_EXEC_CONTROLS, PrimaryControl);

		if (Cpu->Hardware.HasPendingInterrupt) {
			InjectEvent(0, FALSE, Cpu->Hardware.PendingInterrupt.Vector, InterruptionTypeExternalInterrupt, Cpu->Hardware.PendingInterrupt.Valid);
			Cpu->Hardware.HasPendingInterrupt = FALSE;
		}
		break;
	}
	default:
		DebugPrint0402("VMEXIT[%d]: UNKNOWN EXIT REASON %d (0x%x) — BORKED\n",
			Cpu->Index, ExitReason, ExitReason);
		//
		// Unknown exit reason. Fatal.
		//
		Borked = TRUE;
		break;
	}

	if (Borked) {
		DebugPrint0402("VMEXIT[%d]: BORKED — dumping VMCS and halting\n", Cpu->Index);
		DumpVmcs();
		__asm__ __volatile__("hlt");
	}
	else {
		//
		// Advance the guest RIP.
		//
		__vmx_vmwrite(GUEST_RIP, Cpu->Context.Rip + ExitInstructionLen);
		DebugPrint0402("VMEXIT[%d]: advancing RIP from 0x%p to 0x%p (+%d)\n",
			Cpu->Index, Cpu->Context.Rip, Cpu->Context.Rip + ExitInstructionLen, ExitInstructionLen);
	}
}

STATIC
UINT64
AdjustVmcsControl(
	IN UINT64	Control,
	IN UINT64	RequestedValue
) {
	UINT64 VmxBasic = __readmsr(IA32_VMX_BASIC);

	BOOLEAN TrueCapabilitySupported = (VmxBasic & (1ULL << 55)) != 0;
	UINT32 CapabilityMsr = 0;

	if (Control == PINBASED_EXEC_CONTROLS && TrueCapabilitySupported) {
		CapabilityMsr = IA32_VMX_TRUE_PINBASED_CTLS;
	}
	else if (Control == PINBASED_EXEC_CONTROLS && !TrueCapabilitySupported) {
		CapabilityMsr = IA32_VMX_PINBASED_CTLS;
	}
	else if (Control == PRIMARY_PROCBASED_EXEC_CONTROLS && TrueCapabilitySupported) {
		CapabilityMsr = IA32_VMX_TRUE_PROCBASED_CTLS;
	}
	else if (Control == PRIMARY_PROCBASED_EXEC_CONTROLS && !TrueCapabilitySupported) {
		CapabilityMsr = IA32_VMX_PROCBASED_CTLS;
	}
	else if (Control == VMEXIT_CONTROLS && TrueCapabilitySupported) {
		CapabilityMsr = IA32_VMX_TRUE_EXIT_CTLS;
	}
	else if (Control == VMEXIT_CONTROLS && !TrueCapabilitySupported) {
		CapabilityMsr = IA32_VMX_EXIT_CTLS;
	}
	else if (Control == VMENTRY_CONTROLS && TrueCapabilitySupported) {
		CapabilityMsr = IA32_VMX_TRUE_ENTRY_CTLS;
	}
	else if (Control == VMENTRY_CONTROLS && !TrueCapabilitySupported) {
		CapabilityMsr = IA32_VMX_ENTRY_CTLS;
	}
	else if (Control == SECONDARY_PROCBASED_EXEC_CONTROLS) {
		CapabilityMsr = IA32_VMX_PROCBASED_CTLS2;
	}

	UINT64 Capabilities = __readmsr(CapabilityMsr);
	UINT32 Allowed0 = (UINT32)Capabilities;
	UINT32 Allowed1 = (UINT32)(Capabilities >> 32);

	UINT64 EffectiveValue = (RequestedValue | Allowed0) & Allowed1;
	UINT64 IgnoredMask = (~Allowed1) | Allowed0;
	UINT64 IgnoredRequestBits = RequestedValue & IgnoredMask;

	if (EffectiveValue != RequestedValue) {
		DebugPrint0402("AdjustVmcsControl: WARNING - Requested Value = 0x%p and Effective Value 0x%p is NOT same. Ignored Request Bits = 0x%p\n",
			RequestedValue, EffectiveValue, IgnoredRequestBits);
	}

	return EffectiveValue;
}

VOID
EFIAPI
StartVm(
	VOID
) {
	UINT64 Error = __vmx_vmlaunch();
	UINT64 ErrorExtended = 0;
	__vmx_vmread(READONLY_VM_INSTRUCTION_ERROR, &ErrorExtended);
	DebugPrint0402("Error vmlaunch: %d, Extended = 0x%p\n", Error, ErrorExtended);
	DumpVmcs();
}

VOID
EFIAPI
SetupVmcs(
	VOID
) {
	CATALYST_VIRTUAL_CPU* Cpu = (CATALYST_VIRTUAL_CPU*)_readgsbase_u64();

	DebugPrint0402("SetupVmcs[%d]: Virtualizing processor...\n", Cpu->Index);

	//
	// Initialize controls.
	//
	__vmx_vmwrite(VMEXIT_CONTROLS, AdjustVmcsControl(VMEXIT_CONTROLS, VmExitHostAddressSpaceSize | VmExitSaveIa32Efer | VmExitAckInterruptOnExit));
	__vmx_vmwrite(VMENTRY_CONTROLS, AdjustVmcsControl(VMENTRY_CONTROLS, VmEntryIa32eModeGuest | VmEntryLoadIa32Efer));
	__vmx_vmwrite(PINBASED_EXEC_CONTROLS, AdjustVmcsControl(PINBASED_EXEC_CONTROLS, PinBasedExternalInterruptExiting));

	__vmx_vmwrite(PRIMARY_PROCBASED_EXEC_CONTROLS, AdjustVmcsControl(PRIMARY_PROCBASED_EXEC_CONTROLS,
		(UINT64)(ProcessorBasedSecondaryControls | ProcessorBasedUseIoBitmaps | ProcessorBasedHltExiting)));
	__vmx_vmwrite(SECONDARY_PROCBASED_EXEC_CONTROLS, AdjustVmcsControl(SECONDARY_PROCBASED_EXEC_CONTROLS,
		(UINT64)(ProcessorBased2EnableRdtscp | ProcessorBased2EnableUserWaitPause | ProcessorBased2EnableXsavesXrstors | ProcessorBased2EnableInvpcid | ProcessorBased2EnableEpt
			| ProcessorBased2UnrestrictedGuest)));

	__vmx_vmwrite(EPTP_FULL, g_VmState.AddressSpace.Eptp.All);
	__vmx_vmwrite(MSR_BITMAPS_ADDR_FULL, (UINT64)&Cpu->Data.MsrBitmap);
	__vmx_vmwrite(EXCEPTION_BITMAP, MAX_UINT64);
	DebugPrint0402("SetupVmcs[%d]: EPTP=0x%p, MsrBitmap=0x%p, ExceptionBitmap=0x%p\n",
		Cpu->Index, g_VmState.AddressSpace.Eptp.All, (UINT64)&Cpu->Data.MsrBitmap, (UINT64)MAX_UINT64);

	__vmx_vmwrite(IO_BITMAP_A_ADDR_FULL, (UINT64)&Cpu->Data.IoBitmapA);
	__vmx_vmwrite(IO_BITMAP_B_ADDR_FULL, (UINT64)&Cpu->Data.IoBitmapB);

	SET_IO_PORT_BIT(Cpu->Data.IoBitmapA, PIC1_COMMAND);
	SET_IO_PORT_BIT(Cpu->Data.IoBitmapA, 0x21);
	SET_IO_PORT_BIT(Cpu->Data.IoBitmapA, PIC2_COMMAND);
	SET_IO_PORT_BIT(Cpu->Data.IoBitmapA, 0xA1);

	SET_IO_PORT_BIT(Cpu->Data.IoBitmapA, UART_COM1_PORT);

	//
	// Initialize host.
	//
	__vmx_vmwrite(HOST_ES_SELECTOR, AsmReadEs());
	__vmx_vmwrite(HOST_CS_SELECTOR, AsmReadCs());
	__vmx_vmwrite(HOST_SS_SELECTOR, AsmReadSs());
	__vmx_vmwrite(HOST_DS_SELECTOR, AsmReadDs());
	__vmx_vmwrite(HOST_FS_SELECTOR, AsmReadFs());
	__vmx_vmwrite(HOST_GS_SELECTOR, AsmReadGs());
	__vmx_vmwrite(HOST_TR_SELECTOR, AsmReadTr());

	__vmx_vmwrite(HOST_CR0, __readcr0());
	__vmx_vmwrite(HOST_CR3, g_VmState.AddressSpace.HostCr3.All);
	__vmx_vmwrite(HOST_CR4, __readcr4());

	__vmx_vmwrite(HOST_FS_BASE, 0);
	__vmx_vmwrite(HOST_GS_BASE, (UINT64)Cpu);

	__vmx_vmwrite(HOST_TR_BASE, GET_GDT_TSS_BASE((X86_GDT_SYSTEM_ENTRY*)&g_VmState.SharedState.HostGdt[AsmReadTr() >> 3]));
	__vmx_vmwrite(HOST_GDTR_BASE, (UINT64)g_VmState.SharedState.HostGdtBase);
	__vmx_vmwrite(HOST_IDTR_BASE, (UINT64)g_VmState.SharedState.HostIdtBase);

	__vmx_vmwrite(HOST_RIP, (UINT64)AsmVmExit);
	__vmx_vmwrite(HOST_RSP, (UINT64)&Cpu->Data.HostStack);

	//
	// Initialize guest.
	// Guest GDT layout: 0=null, 6=data(0x30), 7=code(0x38), 8=TSS(0x40)
	//
	__vmx_vmwrite(GUEST_ES_SELECTOR, 0x30);
	__vmx_vmwrite(GUEST_SS_SELECTOR, 0x30);
	__vmx_vmwrite(GUEST_DS_SELECTOR, 0x30);
	__vmx_vmwrite(GUEST_LDTR_SELECTOR, 0);

	__vmx_vmwrite(GUEST_TR_SELECTOR, g_VmState.SharedState.TrSelector);
	__vmx_vmwrite(GUEST_FS_SELECTOR, 0x30);
	__vmx_vmwrite(GUEST_GS_SELECTOR, 0x30);
	__vmx_vmwrite(GUEST_CS_SELECTOR, 0x38);

	//
	// Guest segment limits — flat 4 GB model (G=1, limit field = 0xFFFFF).
	//
	__vmx_vmwrite(GUEST_ES_LIMIT, 0xFFFFFFFF);
	__vmx_vmwrite(GUEST_SS_LIMIT, 0xFFFFFFFF);
	__vmx_vmwrite(GUEST_DS_LIMIT, 0xFFFFFFFF);
	__vmx_vmwrite(GUEST_CS_LIMIT, 0xFFFFFFFF);

	__vmx_vmwrite(GUEST_FS_LIMIT, 0xFFFFFFFF);
	__vmx_vmwrite(GUEST_GS_LIMIT, 0xFFFFFFFF);
	__vmx_vmwrite(GUEST_TR_LIMIT, g_VmState.SharedState.TrLimit);

	//
	// Guest segment access rights.
	// 0xC092 = present, DPL0, data read/write, 32-bit, G=1
	// 0xA09A = present, DPL0, code execute/read, long mode, G=1
	//
	__vmx_vmwrite(GUEST_ES_ACCESS_RIGHTS, 0xC093);
	__vmx_vmwrite(GUEST_CS_ACCESS_RIGHTS, 0xA09B);
	__vmx_vmwrite(GUEST_SS_ACCESS_RIGHTS, 0xC093);
	__vmx_vmwrite(GUEST_DS_ACCESS_RIGHTS, 0xC093);
	__vmx_vmwrite(GUEST_FS_ACCESS_RIGHTS, 0xC093);
	__vmx_vmwrite(GUEST_GS_ACCESS_RIGHTS, 0xC093);
	__vmx_vmwrite(GUEST_TR_ACCESS_RIGHTS, g_VmState.SharedState.TrAccessRights);
	__vmx_vmwrite(GUEST_LDTR_ACCESS_RIGHTS, 0x10000);   // Unusable

	__vmx_vmwrite(GUEST_FS_BASE, 0);
	__vmx_vmwrite(GUEST_GS_BASE, 0);

	__vmx_vmwrite(GUEST_IDTR_BASE, (UINT64)g_VmState.SharedState.GuestIdtBase);
	__vmx_vmwrite(GUEST_IDTR_LIMIT, g_VmState.SharedState.GuestIdtLimit);

	__vmx_vmwrite(GUEST_GDTR_BASE, (UINT64)g_VmState.SharedState.GuestGdtBase);
	__vmx_vmwrite(GUEST_GDTR_LIMIT, g_VmState.SharedState.GuestGdtLimit);

	__vmx_vmwrite(GUEST_TR_BASE, (UINT64)g_VmState.SharedState.GuestTssBase);

	//
	// SYSENTER_CS must not be 0 when IA-32e mode guest is active (SDM 26.3.1.5).
	// Use our guest code selector.  The guest won't use SYSENTER (UEFI is
	// long-mode SYSCALL), so EIP/ESP = 0 is harmless.
	//
	__vmx_vmwrite(GUEST_IA32_SYSENTER_CS, 0x38);
	__vmx_vmwrite(GUEST_IA32_SYSENTER_EIP, 0);
	__vmx_vmwrite(GUEST_IA32_SYSENTER_ESP, 0);

	__vmx_vmwrite(GUEST_IA32_EFER_FULL, __readmsr(IA32_EFER));

	__vmx_vmwrite(GUEST_LINK_PTR_FULL, MAX_UINT64);

	__vmx_vmwrite(GUEST_CR0, __readcr0());
	__vmx_vmwrite(GUEST_CR3, g_VmState.AddressSpace.GuestCr3);
	__vmx_vmwrite(GUEST_CR4, __readcr4());

	__vmx_vmwrite(GUEST_RFLAGS, AsmReadEflags());

	__vmx_vmwrite(GUEST_RIP, Cpu->Context.Rip);
	__vmx_vmwrite(GUEST_RSP, Cpu->Context.Rsp);

	DebugPrint0402("SetupVmcs[%d]: GUEST_RIP = 0x%p, GUEST_RSP = 0x%p, GUEST_CR3 = 0x%p\n", Cpu->Index, Cpu->Context.Rip, Cpu->Context.Rsp, g_VmState.AddressSpace.GuestCr3);
}

VOID
EFIAPI
PrepareProcessor(
	IN OUT VOID* Argument
) {
	CATALYST_VIRTUAL_CPU* Cpu = (CATALYST_VIRTUAL_CPU*)Argument;

	DebugPrint0402("PrepareProcessor: Preparing processor %d...\n", Cpu->Index);

	DebugPrint0402("PrepareProcessor: Preparing CR registers...\n");

	//
	// Read CR0 and CR4.
	//
	UINT64 Cr0 = __readcr0();
	DebugPrint0402("PrepareProcessor: CR0 = %p\n", Cr0);

	UINT64 Cr4 = __readcr4();

	//
	// Appearently CR4.FSGSBASE is not enabled. Let's fix that.
	//
	Cr4 |= (1 << 16);

	DebugPrint0402("PrepareProcessor: CR4 = %p\n", Cr4);

	//
	// Read required bitmask values for Cr0 and Cr4.
	//
	UINT64 Cr0OrMask = __readmsr(IA32_VMX_CR0_FIXED0);
	UINT64 Cr0AndMask = __readmsr(IA32_VMX_CR0_FIXED1);

	DebugPrint0402("PrepareProcessor: CR0_FIXED0 = %p\n", Cr0OrMask);
	DebugPrint0402("PrepareProcessor: CR0_FIXED1 = %p\n", Cr0AndMask);

	UINT64 Cr4OrMask = __readmsr(IA32_VMX_CR4_FIXED0);
	UINT64 Cr4AndMask = __readmsr(IA32_VMX_CR4_FIXED1);

	DebugPrint0402("PrepareProcessor: CR4_FIXED0 = %p\n", Cr4OrMask);
	DebugPrint0402("PrepareProcessor: CR4_FIXED1 = %p\n", Cr4AndMask);

	Cr0 = (Cr0 & Cr0AndMask) | Cr0OrMask;
	Cr4 = (Cr4 & Cr4AndMask) | Cr4OrMask;

	DebugPrint0402("PrepareProcessor: CR0_FINAL = %p\n", Cr0);
	DebugPrint0402("PrepareProcessor: CR4_FINAL = %p\n", Cr4);

	//
	// Write values back.
	//
	__writecr0(Cr0);
	__writecr4(Cr4);

	//
	// Enable x2APIC. (Death to APIC and xAPIC. Imagine doing MMIO lol)
	//
	UINT64 ApicBase = __readmsr(IA32_APIC_BASE);
	ApicBase |= APIC_BASE_ENABLE_X2APIC;
	ApicBase |= APIC_BASE_APIC_GLOBAL_EN;

	__writemsr(IA32_APIC_BASE, ApicBase);

	//
	// Set the GS base address to current CPU's control pointer.
	//
	_writegsbase_u64((UINT64)Cpu);

	//
	// Read VMX revision ID.
	//
	UINT32 Revision = __readmsr(IA32_VMX_BASIC);

	DebugPrint0402("PrepareProcessor: VMX Revision (IA32_VMX_BASIC) = %d\n", Revision);

	//
	// Set revision field of each VMX structure.
	//
	Cpu->Vmx.Vmcs.RevisionIdentifier = Revision;
	Cpu->Vmx.VmxOn.RevisionIdentifier = Revision;

	DebugPrint0402("PrepareProcessor: VMXON = %p\n", &Cpu->Vmx.VmxOn);
	DebugPrint0402("PrepareProcessor: VMCS = %p\n", &Cpu->Vmx.Vmcs);

	//
	// Set VMXON region.
	// We also need better error handling. The caller has no idea we fucked up.
	//
	UINT8 Error = __vmx_on((UINT64)&Cpu->Vmx.VmxOn);
	if (Error != 0) {
		DebugPrint0402("PrepareProcessor: FAIL: Failed to vmxon: %d\n", Error);
		return;
	}

	Error = __vmx_vmclear((UINT64)&Cpu->Vmx.Vmcs);
	if (Error != 0) {
		DebugPrint0402("PrepareProcessor: FAIL: Failed to vmclear: %d\n", Error);
		return;
	}

	Error = __vmx_vmptrld((UINT64)&Cpu->Vmx.Vmcs);
	if (Error != 0) {
		DebugPrint0402("PrepareProcessor: FAIL: Failed to vmptrld: %d\n", Error);
		return;
	}

	DebugPrint0402("PrepareProcessor: FAIL: Processor %d prepared\n", Cpu->Index);

	//
	// Fill VMCS and virtualize processor.
	//
	SetupVmcs();
}

