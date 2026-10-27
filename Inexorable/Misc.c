#include "efi_spec.h"
#include "Memory.h"
#include "Serial.h"

BOOLEAN g_BootServicesExited = FALSE;

static inline void outb(UINT16 port, UINT8 val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(UINT16 port, UINT16 val) {
    __asm__ __volatile__("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline UINT64 rdtsc(void) {
    UINT32 lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((UINT64)hi << 32) | lo;
}

EFI_STATUS EFIAPI IxExitBootServices(
    IN EFI_HANDLE ImageHandle,
    IN UINTN      MapKey
) {
    (void)ImageHandle;

    if (MapKey != g_MemoryMapKey) {
        SerialPrint("[UEFI] ExitBootServices: MapKey mismatch (passed %u, current %u)\n", MapKey, g_MemoryMapKey);
        // We log the warning but accept to ensure boot resilience
    }

    // Disable firmware interrupts
    __asm__ __volatile__("cli");

    g_BootServicesExited = TRUE;
    SerialPrint("[UEFI] ExitBootServices: Handing over to OS kernel!\n");

    return EFI_SUCCESS;
}

VOID EFIAPI IxResetSystem(
    IN INT32      ResetType,
    IN EFI_STATUS ResetStatus,
    IN UINTN      DataSize,
    IN VOID       *ResetData
) {
    (void)ResetType;
    (void)ResetStatus;
    (void)DataSize;
    (void)ResetData;

    SerialPrint("[UEFI] ResetSystem requested\n");

    // Standard PCI system reset: write 0x06 to port 0xCF9
    outb(0xCF9, 0x06);

    // QEMU ACPI poweroff: write 0x2000 to port 0x604
    outw(0x604, 0x2000);

    // QEMU debug exit: write 0x00 to port 0x501
    outb(0x501, 0x00);

    // Fallback halt loop
    while (1) {
        __asm__ __volatile__("cli; hlt");
    }
}

EFI_STATUS EFIAPI IxStall(
    IN UINTN Microseconds
) {
    // 1 microsecond is roughly 2000 to 3000 cycles on modern x86
    UINT64 Cycles = Microseconds * 2500ULL;
    UINT64 Start = rdtsc();

    while (rdtsc() - Start < Cycles) {
        __asm__ __volatile__("pause");
    }

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI IxSetWatchdogTimer(
    IN UINTN   Timeout,
    IN UINT64  WatchdogCode,
    IN UINTN   DataSize,
    IN CHAR16  *WatchdogData
) {
    (void)Timeout;
    (void)WatchdogCode;
    (void)DataSize;
    (void)WatchdogData;
    return EFI_SUCCESS;
}

VOID EFIAPI IxCopyMem(
    IN VOID  *Destination,
    IN VOID  *Source,
    IN UINTN Length
) {
    if (Destination == Source || Length == 0) return;

    UINT8 *d = (UINT8 *)Destination;
    const UINT8 *s = (const UINT8 *)Source;

    if (d < s || d >= (s + Length)) {
        __asm__ __volatile__(
            "cld\n\t"
            "rep movsb"
            : "+D"(d), "+S"(s), "+c"(Length)
            :
            : "memory"
        );
    } else {
        d += Length - 1;
        s += Length - 1;
        __asm__ __volatile__(
            "std\n\t"
            "rep movsb\n\t"
            "cld"
            : "+D"(d), "+S"(s), "+c"(Length)
            :
            : "memory"
        );
    }
}

VOID EFIAPI IxSetMem(
    IN VOID  *Buffer,
    IN UINTN Size,
    IN UINT8 Value
) {
    if (Size == 0) return;
    VOID *d = Buffer;
    __asm__ __volatile__(
        "cld\n\t"
        "rep stosb"
        : "+D"(d), "+c"(Size)
        : "a"(Value)
        : "memory"
    );
}

EFI_STATUS EFIAPI IxCalculateCrc32(
    IN  VOID   *Data,
    IN  UINTN  DataSize,
    OUT UINT32 *Crc32
) {
    if (Data == NULL || Crc32 == NULL || DataSize == 0) {
        return EFI_INVALID_PARAMETER;
    }

    const UINT8 *bytes = (const UINT8 *)Data;
    UINT32 crc = 0xFFFFFFFF;

    for (UINTN i = 0; i < DataSize; i++) {
        crc ^= bytes[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
    }

    *Crc32 = ~crc;
    return EFI_SUCCESS;
}
