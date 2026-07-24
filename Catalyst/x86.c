#include <Uefi.h>
#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>

#include <intrin.h>

#include "x86.h"

BOOLEAN
IsVmxSupported(
	VOID
) {
	UINT32 Ecx = 0;
	AsmCpuid(1, NULL, NULL, &Ecx, NULL);

	DebugPrint(DEBUG_VERBOSE, "[CATALYST] Leaf 1, ECX = %p\n", Ecx);

	return (Ecx & (VMX_SUPPORT_BIT)) != 0;
}


BOOLEAN
IsVmxEnabled(
	VOID
) {
	//
	// For some reason, AsmReadMsr64 cannot reference __readmsr... Gross.
	//
	UINT64 Feature = __readmsr(IA32_FEATURE_CONTROL_MSR);
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