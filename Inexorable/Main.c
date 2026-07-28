#include <Uefi.h>
#include <Base.h>
#include <Library/PrintLib.h>

CHAR8* gEfiCallerBaseName = "BT-BALCI";

inline VOID __outb(UINT16 port, UINT8 value) {
    __asm__ __volatile__(
        "outb %0, %1"
        :
    : "a"(value), "Nd"(port)
        );
}

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
        __outb(0x402, (UINT8)Buffer[Index]);
    }

    return CharCount;
}

VOID
EFIAPI
UefiMain(
) {
    DebugPrint0402("Coming from guest!\n");
}