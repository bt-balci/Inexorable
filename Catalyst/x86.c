#include <Uefi.h>
#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
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
	UINT64 Feature = AsmReadMsr64(IA32_FEATURE_CONTROL_MSR);
	DebugPrint(DEBUG_VERBOSE, "[CATALYST] IA32_FEATURE_CONTROL_MSR = %p\n", Feature);

	if ((Feature & FEATURE_CONTROL_LOCK_BIT) != 0) {
		// BIOS hasn't configured it yet.
		return FALSE;
	}

	if ((Feature & FEATURE_CONTROL_VMXON_OUTSIDE_SMX) != 0) {
		return FALSE;
	}

	return TRUE;
}