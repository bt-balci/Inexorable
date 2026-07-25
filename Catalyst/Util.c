#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/IoLib.h>

#include "x86.h"

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

    if (Format == NULL) {
        return 0;
    }

    // 1. Format the string into an ASCII buffer using EDK2's PrintLib
    VA_START(Marker, Format);
    CharCount = AsciiVSPrint(Buffer, sizeof(Buffer), Format, Marker);
    VA_END(Marker);

    // 2. Blast the buffer byte-by-byte out to I/O port 0x402
    for (Index = 0; Index < CharCount; Index++) {
        __outb(0x402, (UINT8)Buffer[Index]);
    }

    return CharCount;
}