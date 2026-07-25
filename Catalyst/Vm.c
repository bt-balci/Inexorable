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
UINT64
AdjustVmcsControl(
	IN UINT64	Control,
	IN UINT64	RequestedValue
) {
	UINT64 VmxBasic = __readmsr(IA32_VMX_BASIC);

	BOOLEAN TrueCapabilitySupported = (VmxBasic & (1ULL << 55)) == 1;
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

	DebugPrint0402("[CATALYST] Virtualizing processor %d...\n", Cpu->Index);

	//
	// Initialize controls.
	//

	__vmx_vmwrite(VMEXIT_CONTROLS, AdjustVmcsControl(VMEXIT_CONTROLS, VmExitHostAddressSpaceSize));
	__vmx_vmwrite(VMENTRY_CONTROLS, AdjustVmcsControl(VMENTRY_CONTROLS, VmEntryIa32eModeGuest));
	__vmx_vmwrite(PINBASED_EXEC_CONTROLS, AdjustVmcsControl(PINBASED_EXEC_CONTROLS, 0));

	__vmx_vmwrite(PRIMARY_PROCBASED_EXEC_CONTROLS, AdjustVmcsControl(PRIMARY_PROCBASED_EXEC_CONTROLS,
		(UINT64)(ProcessorBasedSecondaryControls | ProcessorBasedUseMsrBitmaps)));
	__vmx_vmwrite(SECONDARY_PROCBASED_EXEC_CONTROLS, AdjustVmcsControl(SECONDARY_PROCBASED_EXEC_CONTROLS,
		(UINT64)(ProcessorBased2EnableRdtscp | ProcessorBased2EnableUserWaitPause | ProcessorBased2EnableXsavesXrstors | ProcessorBased2EnableInvpcid | ProcessorBased2EnableEpt)));

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
	__vmx_vmwrite(HOST_CR3, __readcr3());
	__vmx_vmwrite(HOST_CR4, __readcr4());

	__vmx_vmwrite(HOST_FS_BASE, (UINT64)Cpu);
	__vmx_vmwrite(HOST_GS_BASE, 0);

	IA32_DESCRIPTOR Descriptor = { 0 };
	AsmReadGdtr(&Descriptor);

	__vmx_vmwrite(HOST_TR_BASE, GET_GDT_TSS_BASE((X86_GDT_SYSTEM_ENTRY*)AsmLoadSegmentDescriptor(Descriptor.Base, AsmReadTr())));
	__vmx_vmwrite(HOST_GDTR_BASE, Descriptor.Base);

	AsmReadIdtr(&Descriptor);

	__vmx_vmwrite(HOST_IDTR_BASE, Descriptor.Base);

	//
	// Initialize guest.
	//

	__vmx_vmwrite(GUEST_ES_SELECTOR, AsmReadEs());
	__vmx_vmwrite(GUEST_SS_SELECTOR, AsmReadSs());
	__vmx_vmwrite(GUEST_DS_SELECTOR, AsmReadDs());
	__vmx_vmwrite(GUEST_LDTR_SELECTOR, 0);

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
	__vmx_vmwrite(GUEST_LDTR_ACCESS_RIGHTS, 0);

	__vmx_vmwrite(GUEST_FS_BASE, (UINT64)Cpu);
	__vmx_vmwrite(GUEST_GS_BASE, 0);

	AsmReadIdtr(&Descriptor);

	__vmx_vmwrite(GUEST_IDTR_BASE, Descriptor.Base);
	__vmx_vmwrite(GUEST_IDTR_LIMIT, Descriptor.Limit);

	AsmReadGdtr(&Descriptor);

	__vmx_vmwrite(GUEST_TR_BASE, GET_GDT_TSS_BASE((X86_GDT_SYSTEM_ENTRY*)AsmLoadSegmentDescriptor(Descriptor.Base, AsmReadTr())));

	__vmx_vmwrite(GUEST_GDTR_BASE, Descriptor.Base);
	__vmx_vmwrite(GUEST_GDTR_LIMIT, Descriptor.Limit);

	__vmx_vmwrite(EPTP_FULL, g_Eptp.All);
	__vmx_vmwrite(MSR_BITMAPS_ADDR_FULL, (UINT64)&Cpu->Data.MsrBitmap);

	//
	// Intel's propaganda to force everyone to use sysenter again.
	//
	__vmx_vmwrite(GUEST_IA32_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS));
	__vmx_vmwrite(GUEST_IA32_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP));
	__vmx_vmwrite(GUEST_IA32_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP));

	__vmx_vmwrite(GUEST_LINK_PTR_FULL, ~0ULL);

	__vmx_vmwrite(GUEST_CR0, __readcr0());
	__vmx_vmwrite(GUEST_CR3, __readcr3());
	__vmx_vmwrite(GUEST_CR4, __readcr4());

	__vmx_vmwrite(GUEST_RFLAGS, AsmReadEflags());
}	

VOID
EFIAPI
PrepareProcessor(
	IN OUT VOID*	Argument
) {
	CATALYST_VIRTUAL_CPU* Cpu = (CATALYST_VIRTUAL_CPU*)Argument;

	DebugPrint0402("[CATALYST] Preparing processor %d...\n", Cpu->Index);

	DebugPrint0402("[CATALYST] Preparing CR registers...\n");

	//
	// Read CR0 and CR4.
	//
	UINT64 Cr0 = __readcr0();
	DebugPrint0402("[CATALYST] CR0 = %p\n", Cr0);

	UINT64 Cr4 = __readcr4();

	//
	// Appearently CR4.FSGSBASE is not enabled. Let's fix that.
	//
	Cr4 |= (1 << 16);

	DebugPrint0402("[CATALYST] CR4 = %p\n", Cr4);

	//
	// Read required bitmask values for Cr0 and Cr4.
	//
	UINT64 Cr0OrMask	= __readmsr(IA32_VMX_CR0_FIXED0);
	UINT64 Cr0AndMask	= __readmsr(IA32_VMX_CR0_FIXED1);

	DebugPrint0402("[CATALYST] CR0_FIXED0 = %p\n", Cr0OrMask);
	DebugPrint0402("[CATALYST] CR0_FIXED1 = %p\n", Cr0AndMask);

	UINT64 Cr4OrMask	= __readmsr(IA32_VMX_CR4_FIXED0);
	UINT64 Cr4AndMask	= __readmsr(IA32_VMX_CR4_FIXED1);

	DebugPrint0402("[CATALYST] CR4_FIXED0 = %p\n", Cr4OrMask);
	DebugPrint0402("[CATALYST] CR4_FIXED1 = %p\n", Cr4AndMask);

	Cr0 = (Cr0 & Cr0AndMask) | Cr0OrMask;
	Cr4 = (Cr4 & Cr4AndMask) | Cr4OrMask;

	DebugPrint0402("[CATALYST] CR0_FINAL = %p\n", Cr0);
	DebugPrint0402("[CATALYST] CR4_FINAL = %p\n", Cr4);

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
	// Setup IDT.
	//
	IA32_DESCRIPTOR Temp = { 0 };
	AsmReadIdtr(&Temp);

	DebugPrint0402("[CATALYST] IDT.Base = %p, IDT.Limit = %d\n", Temp.Base, Temp.Limit);

	//
	// Copy the default IDT.
	//
	CopyMem(&Cpu->DescriptorTables.Idt, (VOID*)Temp.Base, Temp.Limit + 1);

	//
	// Set the interrupt handler for sexy ACPI shenanigans.
	// Or don't.
	// Or do, I guess.
	//
	Cpu->DescriptorTables.Idt[209] = GenerateIdtEntry((UINT64)0, AsmReadCs(), X86_GATE_TYPE_INTERRUPT, 0);

	//
	// Load IDT.
	//
	Temp.Base = (UINTN)&Cpu->DescriptorTables.Idt;
	Temp.Limit = (UINT16)(sizeof(Cpu->DescriptorTables.Idt) - 1);

	DebugPrint0402("[CATALYST] New IDT.Base = %p, New IDT.Limit = %d\n", Temp.Base, Temp.Limit);

	AsmWriteIdtr(&Temp);

	//
	// Setup GDT.
	// Or don't.
	//

	/*
	Cpu->DescriptorTables.Gdt[GDT_INDEX_NULL] = (X86_GDT_ENTRY){0};
	Cpu->DescriptorTables.Gdt[GDT_INDEX_KERNEL_CODE]	= CreateGdtEntry(0, 0xFFFFF, TRUE,	TRUE, 0, TRUE,	 FALSE, TRUE);
	Cpu->DescriptorTables.Gdt[GDT_INDEX_KERNEL_DATA]	= CreateGdtEntry(0, 0xFFFFF, FALSE, TRUE, 0, FALSE,	 TRUE,  TRUE);

	//
	// Load GDT.
	//
	Temp.Base = (UINTN)&Cpu->DescriptorTables.Gdt;
	Temp.Limit = (UINT16)(sizeof(Cpu->DescriptorTables.Gdt) - 1);

	DebugPrint0402("[CATALYST] New GDT.Base = %p, New GDT.Limit = %d\n", Temp.Base, Temp.Limit);

	AsmWriteGdtr(&Temp);

	DebugPrint0402("[CATALYST] Ds = %d, Ss = %d, Cs = %d\n", KERNEL_DS_SELECTOR, KERNEL_DS_SELECTOR, KERNEL_CS_SELECTOR);
	*/

	//
	// Read VMX revision ID.
	//
	UINT32 Revision = __readmsr(IA32_VMX_BASIC);

	DebugPrint0402("[CATALYST] VMX Revision (IA32_VMX_BASIC) = %d\n", Revision);

	//
	// Set revision field of each VMX structure.
	//
	Cpu->Vmx.Vmcs.RevisionIdentifier	= Revision;
	Cpu->Vmx.VmxOn.RevisionIdentifier	= Revision;

	DebugPrint0402("[CATALYST] VMXON = %p\n", &Cpu->Vmx.VmxOn);
	DebugPrint0402("[CATALYST] VMCS = %p\n", &Cpu->Vmx.Vmcs);

	//
	// Set VMXON region.
	// We also need better error handling. The caller has no idea we fucked up.
	//
	UINT8 Error = __vmx_on((UINT64)&Cpu->Vmx.VmxOn);
	if (Error != 0) {
		DebugPrint0402("[CATALYST] Failed to vmxon: %d\n", Error);
		return;
	}

	Error = __vmx_vmclear((UINT64)&Cpu->Vmx.Vmcs);
	if (Error != 0) {
		DebugPrint0402("[CATALYST] Failed to vmclear: %d\n", Error);
		return;
	}

	Error = __vmx_vmptrld((UINT64)&Cpu->Vmx.Vmcs);
	if (Error != 0) {
		DebugPrint0402("[CATALYST] Failed to vmptrld: %d\n", Error);
		return;
	}

	DebugPrint0402("[CATALYST] Processor %d prepared\n", Cpu->Index);

	//
	// Fill VMCS and virtualize processor.
	//
	SetupVmcs();
}

#endif