#ifndef _VM_C_
#define _VM_C_

#include <Uefi.h>
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include "x86.h"
#include "Vm.h"
#include "Efi.h"
#include "Memory.h"

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

#undef DUMP_VMCS_FIELD
}

VOID
VmExitHandler(
	
) {
	//
	// Get arguments.
	//
	CATALYST_VIRTUAL_CPU* Cpu = (CATALYST_VIRTUAL_CPU*)_readgsbase_u64();

	UINT64 ExitQualification	= 0;
	UINT64 ExitReason			= 0;
	UINT64 ExitInstructionLen	= 0;

	__vmx_vmread(READONLY_EXIT_QUALIFICATION,		&ExitQualification);
	__vmx_vmread(READONLY_EXIT_REASON,				&ExitReason);
	__vmx_vmread(READONLY_VMEXIT_INSTRUCTION_LEN,	&ExitInstructionLen);

	__vmx_vmread(GUEST_RIP,	&Cpu->Context.Rip);
	__vmx_vmread(GUEST_RSP,	&Cpu->Context.Rsp);

	//
	// Get basic exit reason.
	//
	ExitReason &= 0xFFFF;

	DebugPrint0402("VMEXIT: Reason = %d, Qualification = %d, ExitInstructionLen = %d, GuestRip = %x, GuestRsp = %x\n",
		ExitReason, ExitQualification, ExitInstructionLen, Cpu->Context.Rip, Cpu->Context.Rsp);

	if (ExitReason == VMEXIT_REASON_VMENTRY_FAILURE_DUE_TO_GUEST_STATE) {
		DebugPrint0402("VMCS is borked\n");
		goto Bork;
	}
	else if (ExitReason == VMEXIT_REASON_TRIPLE_FAULT) {
		DebugPrint0402("Guest triple-faulted\n");
		goto Bork;
	}
	else if (ExitReason == VMEXIT_REASON_EXCEPTION_OR_NMI) {
		VMX_INTERRUPTION_INFORMATION InterruptionInfo = { 0 };
		__vmx_vmread(READONLY_VMEXIT_INTERRUPTION_INFO, (UINT64*)&InterruptionInfo.All);
		DebugPrint0402("Guest exception occured. Vector = %d, Type = %d, Deliver = %d, Valid = %d\n",
			InterruptionInfo.Vector, InterruptionInfo.Type, InterruptionInfo.DeliverErrorCode, InterruptionInfo.Valid);

		if (InterruptionInfo.Vector == 14) {
			VMX_PAGE_FAULT_ERROR_CODE PageFaultErrorCode = { 0 };
			__vmx_vmread(PAGE_FAULT_ERR_CODE_MASK, (UINT64*)&PageFaultErrorCode.All);

			DebugPrint0402("Page Fault occured. Present = %d, Write = %d, User = %d, ReservedBitSet = %d, NxViolation = %d, ProtectionKeyInvalid = %d\n",
				PageFaultErrorCode.Present, PageFaultErrorCode.Write, PageFaultErrorCode.User,
				PageFaultErrorCode.ReservedBitSet, PageFaultErrorCode.NxViolation, PageFaultErrorCode.ProtectionKeyViolation);

			goto Bork;
		}
	}
	else if (ExitReason == VMEXIT_REASON_EPT_VIOLATION) {
		VMX_EPT_VIOLATION_QUAILIFICATION EptQualification = { .All = (UINT32)ExitQualification };
		DebugPrint0402("EPT Violation. Read = %d, Write = %d, Execute = %d, IsReadable = %d, IsWritable = %d, IsExecutable = %d, UserExecute = %d, GuestLinearAddressValid = %d, TranslationBork = %d\n",
			EptQualification.Read, EptQualification.Write, EptQualification.Execute, EptQualification.IsReadable, EptQualification.IsWritable, EptQualification.IsExecutable,
			EptQualification.UserExecute, EptQualification.GuestLinearAddressValid, EptQualification.TranslationBork);

		goto Bork;
	}

	goto End;

Bork:
	DumpVmcs();
	__asm__ __volatile__("hlt");
End:
	return;
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
	} /*else if (Control == TERTIARY_PROCBASED_EXEC_CONTROLS) {
		UINT64 Allowed1 = RzAsmReadMsr(IA32_VMX_PROCBASED_CTLS3);
		UINT64 EffectiveValue = RequestedValue & Allowed1;

		//ASSERT((EffectiveValue | RequestedValue) == EffectiveValue);

		return EffectiveValue;
	}*/

	UINT64 Capabilities	= __readmsr(CapabilityMsr);
	UINT32 Allowed0		= (UINT32)Capabilities;
	UINT32 Allowed1		= (UINT32)(Capabilities >> 32);

	//ASSERT((EffectiveValue | RequestedValue) == EffectiveValue);

	return (RequestedValue | Allowed0) & Allowed1;
}

VOID
EFIAPI
SetupVmcs(
	VOID
) {
	CATALYST_VIRTUAL_CPU* Cpu = (CATALYST_VIRTUAL_CPU*)_readgsbase_u64();

	DebugPrint0402("Virtualizing processor %d...\n", Cpu->Index);

	//
	// Initialize controls.
	//
	__vmx_vmwrite(VMEXIT_CONTROLS, AdjustVmcsControl(VMEXIT_CONTROLS, VmExitHostAddressSpaceSize));
	__vmx_vmwrite(VMENTRY_CONTROLS, AdjustVmcsControl(VMENTRY_CONTROLS, VmEntryIa32eModeGuest));
	__vmx_vmwrite(PINBASED_EXEC_CONTROLS, AdjustVmcsControl(PINBASED_EXEC_CONTROLS, 0));

	__vmx_vmwrite(PRIMARY_PROCBASED_EXEC_CONTROLS, AdjustVmcsControl(PRIMARY_PROCBASED_EXEC_CONTROLS,
		(UINT64)(ProcessorBasedSecondaryControls | ProcessorBasedUseMsrBitmaps | ProcessorBasedHltExiting)));
	__vmx_vmwrite(SECONDARY_PROCBASED_EXEC_CONTROLS, AdjustVmcsControl(SECONDARY_PROCBASED_EXEC_CONTROLS,
		(UINT64)(ProcessorBased2EnableRdtscp | ProcessorBased2EnableUserWaitPause | ProcessorBased2EnableXsavesXrstors | ProcessorBased2EnableInvpcid | ProcessorBased2EnableEpt)));

	__vmx_vmwrite(EPTP_FULL, g_Eptp.All);
	__vmx_vmwrite(MSR_BITMAPS_ADDR_FULL, (UINT64)&Cpu->Data.MsrBitmap);

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
	//
	// Because for some reason, original identity page tables stay in EfiConventionalMemory.
	//
	__vmx_vmwrite(HOST_CR3, g_IdentityCr3.All);
	__vmx_vmwrite(HOST_CR4, __readcr4());

	__vmx_vmwrite(HOST_FS_BASE, 0);
	__vmx_vmwrite(HOST_GS_BASE, (UINT64)Cpu);

	__vmx_vmwrite(HOST_TR_BASE, (UINT64)Cpu->DescriptorTables.TssBase);
	__vmx_vmwrite(HOST_GDTR_BASE, (UINT64)Cpu->DescriptorTables.GdtBase);
	__vmx_vmwrite(HOST_IDTR_BASE, (UINT64)Cpu->DescriptorTables.IdtBase);

	__vmx_vmwrite(HOST_RIP, (UINT64)AsmVmExit);
	__vmx_vmwrite(HOST_RSP, (UINT64)&Cpu->Data.HostStack);

	//
	// Initialize guest.
	//
	__vmx_vmwrite(GUEST_ES_SELECTOR, AsmReadEs());
	__vmx_vmwrite(GUEST_SS_SELECTOR, AsmReadSs());
	__vmx_vmwrite(GUEST_DS_SELECTOR, AsmReadDs());
	__vmx_vmwrite(GUEST_LDTR_SELECTOR, AsmReadLdtr());

	__vmx_vmwrite(GUEST_TR_SELECTOR, AsmReadTr());
	__vmx_vmwrite(GUEST_FS_SELECTOR, AsmReadFs());
	__vmx_vmwrite(GUEST_GS_SELECTOR, AsmReadGs());
	__vmx_vmwrite(GUEST_CS_SELECTOR, AsmReadCs());

	__vmx_vmwrite(GUEST_ES_LIMIT, AsmLoadSegmentLimit(AsmReadEs()));
	__vmx_vmwrite(GUEST_SS_LIMIT, AsmLoadSegmentLimit(AsmReadSs()));
	__vmx_vmwrite(GUEST_DS_LIMIT, AsmLoadSegmentLimit(AsmReadDs()));
	__vmx_vmwrite(GUEST_CS_LIMIT, AsmLoadSegmentLimit(AsmReadCs()));

	__vmx_vmwrite(GUEST_FS_LIMIT, AsmLoadSegmentLimit(AsmReadFs()));
	__vmx_vmwrite(GUEST_GS_LIMIT, AsmLoadSegmentLimit(AsmReadGs()));
	__vmx_vmwrite(GUEST_TR_LIMIT, AsmLoadSegmentLimit(AsmReadTr()));

	__vmx_vmwrite(GUEST_ES_ACCESS_RIGHTS, AsmLoadSegmentAccessRights(AsmReadEs()));
	__vmx_vmwrite(GUEST_CS_ACCESS_RIGHTS, AsmLoadSegmentAccessRights(AsmReadCs()));
	__vmx_vmwrite(GUEST_SS_ACCESS_RIGHTS, AsmLoadSegmentAccessRights(AsmReadSs()));
	__vmx_vmwrite(GUEST_DS_ACCESS_RIGHTS, AsmLoadSegmentAccessRights(AsmReadDs()));
	__vmx_vmwrite(GUEST_FS_ACCESS_RIGHTS, AsmLoadSegmentAccessRights(AsmReadFs()));
	__vmx_vmwrite(GUEST_GS_ACCESS_RIGHTS, AsmLoadSegmentAccessRights(AsmReadGs()));
	__vmx_vmwrite(GUEST_TR_ACCESS_RIGHTS, AsmLoadSegmentAccessRights(AsmReadTr()));
	__vmx_vmwrite(GUEST_LDTR_ACCESS_RIGHTS, AsmLoadSegmentAccessRights(0));

	__vmx_vmwrite(GUEST_FS_BASE, (UINT64)Cpu);
	__vmx_vmwrite(GUEST_GS_BASE, 0);

	IA32_DESCRIPTOR Descriptor = { 0 };
	AsmReadIdtr(&Descriptor);

	__vmx_vmwrite(GUEST_IDTR_BASE, (UINT64)Cpu->DescriptorTables.IdtBase);
	__vmx_vmwrite(GUEST_IDTR_LIMIT, Descriptor.Limit);

	AsmReadGdtr(&Descriptor);

	__vmx_vmwrite(GUEST_GDTR_BASE, (UINT64)Cpu->DescriptorTables.GdtBase);
	__vmx_vmwrite(GUEST_GDTR_LIMIT, Descriptor.Limit);

	__vmx_vmwrite(GUEST_TR_BASE, (UINT64)Cpu->DescriptorTables.TssBase);

	//
	// Intel's propaganda to force everyone to use sysenter again.
	//
	__vmx_vmwrite(GUEST_IA32_SYSENTER_CS,	__readmsr(IA32_SYSENTER_CS));
	__vmx_vmwrite(GUEST_IA32_SYSENTER_EIP,	__readmsr(IA32_SYSENTER_EIP));
	__vmx_vmwrite(GUEST_IA32_SYSENTER_ESP,	__readmsr(IA32_SYSENTER_ESP));

	__vmx_vmwrite(GUEST_LINK_PTR_FULL, MAX_UINT64);

	__vmx_vmwrite(GUEST_CR0, __readcr0());
	__vmx_vmwrite(GUEST_CR3, g_IdentityCr3.All);
	__vmx_vmwrite(GUEST_CR4, __readcr4());

	__vmx_vmwrite(GUEST_RFLAGS, AsmReadEflags());

	//
	// Test
	//
	UINT8* GuestAddress = TranslateGuestToHostAddress(0x1000);
	GuestAddress[0] = 0xF4; // HLT
	__vmx_vmwrite(GUEST_RIP, 0x1000);
	__vmx_vmwrite(GUEST_RSP, 0x2000);

	DebugPrint0402("GUEST_RIP = 0x%x, GUEST_RSP = 0x%x\n", GuestAddress, 0x2000);

	UINT64 Error = __vmx_vmlaunch();
	UINT64 ErrorExtended = 0;
	__vmx_vmread(READONLY_VM_INSTRUCTION_ERROR, &ErrorExtended);
	DebugPrint0402("Error vmlaunch: %d, Extended = 0x%p\n", Error, ErrorExtended);
}

VOID
EFIAPI
PrepareProcessor(
	IN OUT VOID*	Argument
) {
	CATALYST_VIRTUAL_CPU* Cpu = (CATALYST_VIRTUAL_CPU*)Argument;

	DebugPrint0402("Preparing processor %d...\n", Cpu->Index);

	DebugPrint0402("Preparing CR registers...\n");

	//
	// Read CR0 and CR4.
	//
	UINT64 Cr0 = __readcr0();
	DebugPrint0402("CR0 = %p\n", Cr0);

	UINT64 Cr4 = __readcr4();

	//
	// Appearently CR4.FSGSBASE is not enabled. Let's fix that.
	//
	Cr4 |= (1 << 16);

	DebugPrint0402("CR4 = %p\n", Cr4);

	//
	// Read required bitmask values for Cr0 and Cr4.
	//
	UINT64 Cr0OrMask	= __readmsr(IA32_VMX_CR0_FIXED0);
	UINT64 Cr0AndMask	= __readmsr(IA32_VMX_CR0_FIXED1);

	DebugPrint0402("CR0_FIXED0 = %p\n", Cr0OrMask);
	DebugPrint0402("CR0_FIXED1 = %p\n", Cr0AndMask);

	UINT64 Cr4OrMask	= __readmsr(IA32_VMX_CR4_FIXED0);
	UINT64 Cr4AndMask	= __readmsr(IA32_VMX_CR4_FIXED1);

	DebugPrint0402("CR4_FIXED0 = %p\n", Cr4OrMask);
	DebugPrint0402("CR4_FIXED1 = %p\n", Cr4AndMask);

	Cr0 = (Cr0 & Cr0AndMask) | Cr0OrMask;
	Cr4 = (Cr4 & Cr4AndMask) | Cr4OrMask;

	DebugPrint0402("CR0_FINAL = %p\n", Cr0);
	DebugPrint0402("CR4_FINAL = %p\n", Cr4);

	//
	// Write values back.
	//
	__writecr0(Cr0);
	__writecr4(Cr4);

	//
	// Enable x2APIC. (Death to APIC and xAPIC)
	//
	UINT64 ApicBase = __readmsr(IA32_APIC_BASE);
	ApicBase |= APIC_BASE_ENABLE_X2APIC;
	ApicBase |= APIC_BASE_APIC_GLOBAL_EN;

	__writemsr(IA32_APIC_BASE, ApicBase);

	//
	// Set the GS base address to current CPU's control pointer.
	//
	_writegsbase_u64((unsigned long long)Cpu);

	//
	// 1:1 map descriptor structures. Read the disclaimer at Main.c for the other screw up.
	// Warning: This will INEVITABLY BREAK if the Catalyst is not mapped over the 4GB limit and guest has more than 4GB memory. 
	//
	IA32_DESCRIPTOR Descriptor = { 0 };
	AsmReadIdtr(&Descriptor);

	Cpu->DescriptorTables.IdtBase = (VOID*)Descriptor.Base;
	MapSinglePage((UINT64)Cpu->DescriptorTables.IdtBase, (UINT64)Cpu->DescriptorTables.IdtBase, FALSE);

	AsmReadGdtr(&Descriptor);

	Cpu->DescriptorTables.GdtBase = (VOID*)Descriptor.Base;
	MapSinglePage((UINT64)Cpu->DescriptorTables.GdtBase, (UINT64)Cpu->DescriptorTables.GdtBase, FALSE);

	Cpu->DescriptorTables.TssBase = (VOID*)GET_GDT_TSS_BASE((X86_GDT_SYSTEM_ENTRY*)AsmLoadSegmentDescriptor(Descriptor.Base, AsmReadTr()));
	MapSinglePage((UINT64)Cpu->DescriptorTables.TssBase, (UINT64)Cpu->DescriptorTables.TssBase, FALSE);

	//
	// Set the interrupt handler for sexy ACPI shenanigans.
	//
	((X86_IDT_ENTRY*)Descriptor.Base)[209] = GenerateIdtEntry((UINT64)0, AsmReadCs(), X86_GATE_TYPE_INTERRUPT, 0);

	//
	// Read VMX revision ID.
	//
	UINT32 Revision = __readmsr(IA32_VMX_BASIC);

	DebugPrint0402("VMX Revision (IA32_VMX_BASIC) = %d\n", Revision);

	//
	// Set revision field of each VMX structure.
	//
	Cpu->Vmx.Vmcs.RevisionIdentifier	= Revision;
	Cpu->Vmx.VmxOn.RevisionIdentifier	= Revision;

	DebugPrint0402("VMXON = %p\n", &Cpu->Vmx.VmxOn);
	DebugPrint0402("VMCS = %p\n", &Cpu->Vmx.Vmcs);

	//
	// Set VMXON region.
	// We also need better error handling. The caller has no idea we fucked up.
	//
	UINT8 Error = __vmx_on((UINT64)&Cpu->Vmx.VmxOn);
	if (Error != 0) {
		DebugPrint0402("Failed to vmxon: %d\n", Error);
		return;
	}

	Error = __vmx_vmclear((UINT64)&Cpu->Vmx.Vmcs);
	if (Error != 0) {
		DebugPrint0402("Failed to vmclear: %d\n", Error);
		return;
	}

	Error = __vmx_vmptrld((UINT64)&Cpu->Vmx.Vmcs);
	if (Error != 0) {
		DebugPrint0402("Failed to vmptrld: %d\n", Error);
		return;
	}

	DebugPrint0402("Processor %d prepared\n", Cpu->Index);

	//
	// Fill VMCS and virtualize processor.
	//
	SetupVmcs();
}

#endif