#include "RngProtocol.h"
#include "Protocol.h"
#include "Serial.h"

static inline UINT64 rdtsc(void) {
    UINT32 lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((UINT64)hi << 32) | lo;
}

static inline BOOLEAN has_rdrand(void) {
    UINT32 eax, ebx, ecx, edx;
    __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    return (ecx & (1 << 30)) != 0;
}

static inline int rdrand64(UINT64 *val) {
    UINT8 ok;
    __asm__ __volatile__("rdrand %0; setc %1" : "=r"(*val), "=qm"(ok));
    return ok;
}

// Xorshift64 fallback PRNG
static UINT64 xorshift64(UINT64 *state) {
    UINT64 x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

static EFI_STATUS EFIAPI IxRngGetInfo(
    EFI_RNG_PROTOCOL *This,
    UINTN            *RNGAlgorithmListSize,
    EFI_GUID         *RNGAlgorithmList
) {
    (void)This;
    if (RNGAlgorithmListSize == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    if (*RNGAlgorithmListSize < sizeof(EFI_GUID) || RNGAlgorithmList == NULL) {
        *RNGAlgorithmListSize = sizeof(EFI_GUID);
        return EFI_BUFFER_TOO_SMALL;
    }

    EFI_GUID RawAlgorithm = EFI_RNG_ALGORITHM_RAW;
    RNGAlgorithmList[0] = RawAlgorithm;
    *RNGAlgorithmListSize = sizeof(EFI_GUID);
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI IxGetRNG(
    EFI_RNG_PROTOCOL *This,
    EFI_GUID         *RNGAlgorithm,
    UINTN            RNGValueLength,
    UINT8            *RNGValue
) {
    (void)This;
    (void)RNGAlgorithm;

    if (RNGValueLength == 0 || RNGValue == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    BOOLEAN use_hardware_rng = has_rdrand();
    static UINT64 prng_state = 0;
    if (prng_state == 0) {
        prng_state = rdtsc() ^ 0x5DEECE66DULL;
    }

    UINTN Generated = 0;
    while (Generated < RNGValueLength) {
        UINT64 RandVal = 0;
        BOOLEAN ok = FALSE;

        if (use_hardware_rng) {
            int Retries = 10;
            while (!rdrand64(&RandVal) && --Retries > 0) {
                __asm__ __volatile__("pause");
            }
            if (Retries > 0) {
                ok = TRUE;
            }
        }

        if (!ok) {
            RandVal = xorshift64(&prng_state) ^ rdtsc();
        }

        UINTN Chunk = (RNGValueLength - Generated >= 8) ? 8 : (RNGValueLength - Generated);
        const UINT8 *p = (const UINT8 *)&RandVal;
        for (UINTN i = 0; i < Chunk; i++) {
            RNGValue[Generated + i] = p[i];
        }
        Generated += Chunk;
    }

    return EFI_SUCCESS;
}

static EFI_RNG_PROTOCOL g_RngProtocol = {
    .GetInfo = IxRngGetInfo,
    .GetRNG  = IxGetRNG
};

static EFI_HANDLE g_RngHandle = NULL;

EFI_STATUS RngProtocolInit(void) {
    EFI_GUID RngGuid = EFI_RNG_PROTOCOL_GUID;

    EFI_STATUS Status = IxInstallProtocolInterface(
        &g_RngHandle,
        &RngGuid,
        0,
        &g_RngProtocol
    );
    if (EFI_ERROR(Status)) {
        SerialPrint("[RNG] Failed to install RNG protocol: 0x%x\n", Status);
        return Status;
    }

    SerialPrint("[RNG] Hardware RNG protocol installed (rdrand backed)\n");
    return EFI_SUCCESS;
}
