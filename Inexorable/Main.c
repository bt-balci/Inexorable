#include <Uefi.h>
#include <Base.h>
#include <Library/PrintLib.h>

#include "Efi.h"
#include "Util.h"

VOID
EFIAPI
UefiMain(
) {
    DebugPrint0402("Coming from guest!\n");
    DebugPrint0402("VMCALL result = 0x%p\n", __vmx_vmcall(0, 0, 0, 0, 0));
}