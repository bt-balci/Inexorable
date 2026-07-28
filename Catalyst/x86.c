#include <Uefi.h>
#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>

#include "x86.h"

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
) {
	X86_GDT_ENTRY Entry = { 0 };

	//
	// Base splitting.
	//
	Entry.BaseLow = (UINT16)(Base & 0xFFFF);
	Entry.BaseMid = (UINT8)((Base >> 16) & 0xFF);
	Entry.BaseHigh = (UINT8)((Base >> 24) & 0xFF);

	//
	// Limit splitting.
	//
	Entry.LimitLow = (UINT16)(Limit & 0xFFFF);
	Entry.FlagsAndLimit.LimitHigh = (UINT8)((Limit >> 16) & 0x0F);

	//
	// Access byte.
	// At this point I'm just pulling random constants from internet.
	//
	Entry.Access.Accessed = 0;
	Entry.Access.WriteAllowed = WriteAllowed;
	Entry.Access.Direction = 0;
	Entry.Access.Executable = Executable;
	Entry.Access.DescriptorType = 1;
	Entry.Access.Dpl = Dpl & 0x03;
	Entry.Access.Present = 1;

	//
	// Flags.
	//
	Entry.FlagsAndLimit.Reserved = 0;
	Entry.FlagsAndLimit.LongMode = IsLongMode;
	Entry.FlagsAndLimit.Size = IsLongMode ? 0 : Is32Bit;
	Entry.FlagsAndLimit.Granularity = Granularity; // 0 = Byte, 1 = 4KiB page

	return Entry;
}

X86_GDT_SYSTEM_ENTRY
EFIAPI
CreateGdtSystemEntry(
	IN UINT64	Base,
	IN UINT32	Limit,
	IN UINT8	SystemType,
	IN UINT8	Dpl
) {
	X86_GDT_SYSTEM_ENTRY SysEntry = { 0 };

	//
	// Fill the lower 32-bit descriptor portion.
	//
	SysEntry.BaseDescriptor = CreateGdtEntry(
		(UINT32)(Base & 0xFFFFFFFF),
		Limit,
		0,
		0,
		Dpl,
		0,
		0,
		0
	);

	//
	// Override System Descriptor specific fields.
	//
	SysEntry.BaseDescriptor.Access.DescriptorType = 0;				// System segment
	SysEntry.BaseDescriptor.Access.Value |= (SystemType & 0x0F);	// Type (e.g., 0x9 for 64-bit TSS)

	//
	// Fill the upper 64-bit extension base.
	//
	SysEntry.BaseUpper = (UINT32)(Base >> 32);
	SysEntry.Reserved = 0;

	return SysEntry;
}

X86_IDT_ENTRY
EFIAPI
GenerateIdtEntry(
	IN UINT64	Offset,
	IN UINT16	Selector,
	IN UINT8	GateType,
	IN UINT8	Dpl
) {
	X86_IDT_ENTRY Entry = { 0 };

	//
	// Splitting 64-bit offset.
	//
	Entry.OffsetLow = (UINT16)(Offset & 0xFFFF);
	Entry.OffsetMid = (UINT16)((Offset >> 16) & 0xFFFF);
	Entry.OffsetHigh = (UINT32)(Offset >> 32);

	Entry.Selector = Selector;

	//
	// We do not care about IST, yet...
	//
	Entry.Ist = 0;

	//
	// Set attributes.
	//
	Entry.TypeAttributes.GateType = GateType & 0x0F;
	Entry.TypeAttributes.StoragePresent = 0;
	Entry.TypeAttributes.Dpl = Dpl & 0x03;
	Entry.TypeAttributes.Present = 1;

	return Entry;
}

BOOLEAN
EFIAPI
IsVmxSupported(
	VOID
) {
	UINT32 Ecx = 0;
	AsmCpuid(1, NULL, NULL, &Ecx, NULL);

	DebugPrint(DEBUG_VERBOSE, "[CATALYST] Leaf 1, ECX = %p\n", Ecx);

	return (Ecx & (VMX_SUPPORT_BIT)) != 0;
}

BOOLEAN
EFIAPI
IsVmxEnabled(
	VOID
) {
	//
	// For some reason, AsmReadMsr64 cannot reference __readmsr... Gross.
	//
	UINT64 Feature = __readmsr(IA32_FEATURE_CONTROL);
	DebugPrint(DEBUG_VERBOSE, "[CATALYST] IA32_FEATURE_CONTROL_MSR = %p\n", Feature);

	if ((Feature & FEATURE_CONTROL_LOCK_BIT) != 0) {
		DebugPrint(DEBUG_ERROR, "[CATALYST] IA32_FEATURE_CONTROL_MSR.FEATURE_CONTROL_LOCK_BIT = 0\n", Feature);
		return FALSE;
	}

	if ((Feature & FEATURE_CONTROL_VMXON_OUTSIDE_SMX) != 0) {
		DebugPrint(DEBUG_ERROR, "[CATALYST] IA32_FEATURE_CONTROL_MSR.FEATURE_CONTROL_VMXON_OUTSIDE_SMX = 0\n", Feature);
		return FALSE;
	}

	return TRUE;
}

VOID
EFIAPI
SendInterrupt(
	IN UINT8					Vector,
	IN ApicDestinationShorthand Shorthand		OPTIONAL,
	IN UINT32					DestinationId	OPTIONAL
) {
	INTERRUPT_COMMAND_REGISTER Register = {
		.Vector = Vector,
		.DeliveryMode = ApicDeliveryModeFixed,
		.DestinationMode = ApicDestinationModeLogical,
		.TriggerMode = ApicTriggerModeEdge,
		.DestinationShort = (UINT64)Shorthand,
		.Level = ApicLevelAssert,
		.ApicId = (Shorthand == ApicDestinationShorthandNone) ? DestinationId : 0
	};

	__writemsr(IA32_X2APIC_ICR, Register.All);
}