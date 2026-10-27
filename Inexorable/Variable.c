#include "Variable.h"
#include "Memory.h"
#include "Serial.h"

#define MAX_VARS 32

typedef struct {
    CHAR16   Name[64];
    EFI_GUID Guid;
    UINT32   Attributes;
    UINTN    DataSize;
    VOID     *Data;
    BOOLEAN  InUse;
} VAR_ENTRY;

static VAR_ENTRY g_Variables[MAX_VARS];

static BOOLEAN GuidEquals(const EFI_GUID *a, const EFI_GUID *b) {
    if (!a || !b) return FALSE;
    const UINT64 *p1 = (const UINT64 *)a;
    const UINT64 *p2 = (const UINT64 *)b;
    return (p1[0] == p2[0] && p1[1] == p2[1]);
}

static int StrCmp16(const CHAR16 *s1, const CHAR16 *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (int)(*s1 - *s2);
}

static void StrCpy16(CHAR16 *dst, const CHAR16 *src, UINTN max_chars) {
    UINTN i = 0;
    while (src[i] && i + 1 < max_chars) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

void VariableInit(void) {
    for (UINTN i = 0; i < MAX_VARS; i++) {
        g_Variables[i].InUse = FALSE;
        g_Variables[i].Data = NULL;
    }
}

EFI_STATUS EFIAPI IxGetVariable(
    IN     CHAR16   *VariableName,
    IN     EFI_GUID *VendorGuid,
    OUT    UINT32   *Attributes OPTIONAL,
    IN OUT UINTN    *DataSize,
    OUT    VOID     *Data       OPTIONAL
) {
    if (VariableName == NULL || VendorGuid == NULL || DataSize == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    for (UINTN i = 0; i < MAX_VARS; i++) {
        if (g_Variables[i].InUse &&
            GuidEquals(&g_Variables[i].Guid, VendorGuid) &&
            StrCmp16(g_Variables[i].Name, VariableName) == 0) {

            if (Attributes) *Attributes = g_Variables[i].Attributes;

            if (Data == NULL || *DataSize < g_Variables[i].DataSize) {
                *DataSize = g_Variables[i].DataSize;
                return EFI_BUFFER_TOO_SMALL;
            }

            // Copy data
            const UINT8 *src = (const UINT8 *)g_Variables[i].Data;
            UINT8 *dst = (UINT8 *)Data;
            for (UINTN j = 0; j < g_Variables[i].DataSize; j++) {
                dst[j] = src[j];
            }
            *DataSize = g_Variables[i].DataSize;
            return EFI_SUCCESS;
        }
    }

    return EFI_NOT_FOUND;
}

EFI_STATUS EFIAPI IxSetVariable(
    IN CHAR16   *VariableName,
    IN EFI_GUID *VendorGuid,
    IN UINT32   Attributes,
    IN UINTN    DataSize,
    IN VOID     *Data
) {
    if (VariableName == NULL || VendorGuid == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    // Check if variable exists
    for (UINTN i = 0; i < MAX_VARS; i++) {
        if (g_Variables[i].InUse &&
            GuidEquals(&g_Variables[i].Guid, VendorGuid) &&
            StrCmp16(g_Variables[i].Name, VariableName) == 0) {

            if (DataSize == 0) {
                // Delete variable
                if (g_Variables[i].Data) {
                    IxFreePool(g_Variables[i].Data);
                    g_Variables[i].Data = NULL;
                }
                g_Variables[i].InUse = FALSE;
                return EFI_SUCCESS;
            }

            // Update variable
            if (g_Variables[i].Data) {
                IxFreePool(g_Variables[i].Data);
            }
            VOID *NewBuf = NULL;
            EFI_STATUS Status = IxAllocatePool(EfiRuntimeServicesData, DataSize, &NewBuf);
            if (EFI_ERROR(Status)) return Status;

            const UINT8 *src = (const UINT8 *)Data;
            UINT8 *dst = (UINT8 *)NewBuf;
            for (UINTN j = 0; j < DataSize; j++) dst[j] = src[j];

            g_Variables[i].Data = NewBuf;
            g_Variables[i].DataSize = DataSize;
            g_Variables[i].Attributes = Attributes;
            return EFI_SUCCESS;
        }
    }

    if (DataSize == 0) {
        return EFI_NOT_FOUND;
    }

    // Create new variable
    for (UINTN i = 0; i < MAX_VARS; i++) {
        if (!g_Variables[i].InUse) {
            VOID *NewBuf = NULL;
            EFI_STATUS Status = IxAllocatePool(EfiRuntimeServicesData, DataSize, &NewBuf);
            if (EFI_ERROR(Status)) return Status;

            const UINT8 *src = (const UINT8 *)Data;
            UINT8 *dst = (UINT8 *)NewBuf;
            for (UINTN j = 0; j < DataSize; j++) dst[j] = src[j];

            StrCpy16(g_Variables[i].Name, VariableName, 64);
            g_Variables[i].Guid = *VendorGuid;
            g_Variables[i].Data = NewBuf;
            g_Variables[i].DataSize = DataSize;
            g_Variables[i].Attributes = Attributes;
            g_Variables[i].InUse = TRUE;
            return EFI_SUCCESS;
        }
    }

    return EFI_OUT_OF_RESOURCES;
}

EFI_STATUS EFIAPI IxGetNextVariableName(
    IN OUT UINTN    *VariableNameSize,
    IN OUT CHAR16   *VariableName,
    IN OUT EFI_GUID *VendorGuid
) {
    (void)VariableNameSize;
    (void)VariableName;
    (void)VendorGuid;
    return EFI_NOT_FOUND;
}

EFI_STATUS EFIAPI IxQueryVariableInfo(
    IN  UINT32 Attributes,
    OUT UINT64 *MaximumVariableStorageSize,
    OUT UINT64 *RemainingVariableStorageSize,
    OUT UINT64 *MaximumVariableSize
) {
    (void)Attributes;
    if (MaximumVariableStorageSize)   *MaximumVariableStorageSize = 65536;
    if (RemainingVariableStorageSize) *RemainingVariableStorageSize = 32768;
    if (MaximumVariableSize)          *MaximumVariableSize = 4096;
    return EFI_SUCCESS;
}
