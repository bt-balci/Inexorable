#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/IoLib.h>

#include "x86.h"
#include "Vm.h"
#include "Memory.h"

UINTN
EFIAPI
DebugPrint0402(
    IN CONST CHAR8* Format,
    ...
) {
    VA_LIST Marker;
    CHAR8   Buffer[1024];
    UINTN   CharCount;
    UINTN   Index;

    if (!Format) {
        return 0;
    }

    VA_START(Marker, Format);
    CharCount = AsciiVSPrint(Buffer, sizeof(Buffer), Format, Marker);
    VA_END(Marker);

    for (Index = 0; Index < CharCount; Index++) {
        __outb(QEMU_DEBUG_PORT, (UINT8)Buffer[Index]);
    }

    return CharCount;
}