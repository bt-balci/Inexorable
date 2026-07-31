#include <Uefi.h>
#include <Base.h>
#include <Library/PrintLib.h>

#include "Efi.h"
#include "Util.h"
#include "Variable.h"

EFI_SYSTEM_TABLE		gST = { 0 };
EFI_BOOT_SERVICES	    gBS = { 0 };
EFI_RUNTIME_SERVICES	gRT = { 0 };

VOID
EFIAPI
UefiMain(
    VOID
) {
    DebugPrint0402("Inexorable - As if it was always there. (It never was)\n");

    //
    // Setup system table.
    //
    gST.BootServices        = &gBS;
    gST.RuntimeServices     = &gRT;

    gRT.GetVariable         = IxGetVariable;
    gRT.SetVariable         = IxSetVariable;
    gRT.QueryVariableInfo   = IxQueryVariableInfo;
    gRT.GetNextVariableName = IxGetNextVariableName;
}