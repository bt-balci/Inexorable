#include "Protocol.h"
#include "Memory.h"
#include "Serial.h"
#include "Efi.h"

typedef struct {
    EFI_HANDLE Handle;
    EFI_GUID   Guid;
    VOID       *Interface;
    BOOLEAN    InUse;
} PROTOCOL_ENTRY;

static PROTOCOL_ENTRY g_Protocols[MAX_PROTOCOLS];
static EFI_CONFIGURATION_TABLE g_ConfigTables[MAX_CONFIG_TABLES];
static UINTN g_NextHandleId = 0x1000;

static BOOLEAN GuidEquals(const EFI_GUID *a, const EFI_GUID *b) {
    if (!a || !b) return FALSE;
    const UINT64 *p1 = (const UINT64 *)a;
    const UINT64 *p2 = (const UINT64 *)b;
    return (p1[0] == p2[0] && p1[1] == p2[1]);
}

void ProtocolInit(void) {
    for (UINTN i = 0; i < MAX_PROTOCOLS; i++) {
        g_Protocols[i].InUse = FALSE;
    }
    for (UINTN i = 0; i < MAX_CONFIG_TABLES; i++) {
        g_ConfigTables[i].VendorTable = NULL;
    }
    gST.ConfigurationTable = g_ConfigTables;
    gST.NumberOfTableEntries = 0;
}

EFI_STATUS EFIAPI IxHandleProtocol(
    IN  EFI_HANDLE Handle,
    IN  EFI_GUID   *Protocol,
    OUT VOID       **Interface
) {
    if (Handle == NULL || Protocol == NULL || Interface == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    for (UINTN i = 0; i < MAX_PROTOCOLS; i++) {
        if (g_Protocols[i].InUse &&
            g_Protocols[i].Handle == Handle &&
            GuidEquals(&g_Protocols[i].Guid, Protocol)) {
            *Interface = g_Protocols[i].Interface;
            return EFI_SUCCESS;
        }
    }

    SerialPrint("[UEFI] HandleProtocol: Handle=0x%p, GUID 0x%08x not found\n", Handle, Protocol->Data1);
    return EFI_NOT_FOUND;
}

EFI_STATUS EFIAPI IxOpenProtocol(
    IN  EFI_HANDLE Handle,
    IN  EFI_GUID   *Protocol,
    OUT VOID       **Interface OPTIONAL,
    IN  EFI_HANDLE AgentHandle,
    IN  EFI_HANDLE ControllerHandle,
    IN  UINT32     Attributes
) {
    (void)AgentHandle;
    (void)ControllerHandle;
    (void)Attributes;

    if (Interface == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    return IxHandleProtocol(Handle, Protocol, Interface);
}

EFI_STATUS EFIAPI IxCloseProtocol(
    IN EFI_HANDLE Handle,
    IN EFI_GUID   *Protocol,
    IN EFI_HANDLE AgentHandle,
    IN EFI_HANDLE ControllerHandle
) {
    (void)Handle;
    (void)Protocol;
    (void)AgentHandle;
    (void)ControllerHandle;
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI IxLocateProtocol(
    IN  EFI_GUID *Protocol,
    IN  VOID     *Registration OPTIONAL,
    OUT VOID     **Interface
) {
    (void)Registration;

    if (Protocol == NULL || Interface == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    for (UINTN i = 0; i < MAX_PROTOCOLS; i++) {
        if (g_Protocols[i].InUse && GuidEquals(&g_Protocols[i].Guid, Protocol)) {
            *Interface = g_Protocols[i].Interface;
            return EFI_SUCCESS;
        }
    }

    SerialPrint("[UEFI] LocateProtocol: GUID 0x%08x not found\n", Protocol->Data1);
    return EFI_NOT_FOUND;
}

EFI_STATUS EFIAPI IxLocateHandle(
    IN     INT32      SearchType,
    IN     EFI_GUID   *Protocol,
    IN     VOID       *SearchKey,
    IN OUT UINTN      *BufferSize,
    OUT    EFI_HANDLE *Buffer
) {
    (void)SearchType;
    (void)SearchKey;

    if (BufferSize == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    UINTN MatchingCount = 0;
    EFI_HANDLE MatchingHandles[MAX_HANDLES];

    for (UINTN i = 0; i < MAX_PROTOCOLS; i++) {
        if (g_Protocols[i].InUse && GuidEquals(&g_Protocols[i].Guid, Protocol)) {
            // Check if already in list
            BOOLEAN Exists = FALSE;
            for (UINTN j = 0; j < MatchingCount; j++) {
                if (MatchingHandles[j] == g_Protocols[i].Handle) {
                    Exists = TRUE;
                    break;
                }
            }
            if (!Exists && MatchingCount < MAX_HANDLES) {
                MatchingHandles[MatchingCount++] = g_Protocols[i].Handle;
            }
        }
    }

    if (MatchingCount == 0) {
        return EFI_NOT_FOUND;
    }

    UINTN RequiredBytes = MatchingCount * sizeof(EFI_HANDLE);
    if (*BufferSize < RequiredBytes || Buffer == NULL) {
        *BufferSize = RequiredBytes;
        return EFI_BUFFER_TOO_SMALL;
    }

    for (UINTN i = 0; i < MatchingCount; i++) {
        Buffer[i] = MatchingHandles[i];
    }
    *BufferSize = RequiredBytes;

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI IxLocateHandleBuffer(
    IN     INT32      SearchType,
    IN     EFI_GUID   *Protocol,
    IN     VOID       *SearchKey,
    OUT    UINTN      *NoHandles,
    OUT    EFI_HANDLE **Buffer
) {
    if (NoHandles == NULL || Buffer == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    UINTN BufferSize = 0;
    EFI_STATUS Status = IxLocateHandle(SearchType, Protocol, SearchKey, &BufferSize, NULL);
    if (Status != EFI_BUFFER_TOO_SMALL) {
        *NoHandles = 0;
        *Buffer = NULL;
        return Status;
    }

    Status = IxAllocatePool(EfiBootServicesData, BufferSize, (VOID **)Buffer);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = IxLocateHandle(SearchType, Protocol, SearchKey, &BufferSize, *Buffer);
    if (EFI_ERROR(Status)) {
        IxFreePool(*Buffer);
        *Buffer = NULL;
        return Status;
    }

    *NoHandles = BufferSize / sizeof(EFI_HANDLE);
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI IxLocateDevicePath(
    IN     EFI_GUID                       *Protocol,
    IN OUT EFI_DEVICE_PATH_PROTOCOL       **DevicePath,
    OUT    EFI_HANDLE                     *Device
) {
    if (Protocol == NULL || DevicePath == NULL || Device == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    SerialPrint("[UEFI] LocateDevicePath: Protocol={%x-%x-%x}, *DevicePath=%p\n",
        Protocol->Data1, (UINT32)Protocol->Data2, (UINT32)Protocol->Data3, *DevicePath);

    for (UINTN i = 0; i < MAX_PROTOCOLS; i++) {
        if (g_Protocols[i].InUse && GuidEquals(&g_Protocols[i].Guid, Protocol)) {
            EFI_HANDLE h = g_Protocols[i].Handle;
            EFI_GUID DpGuid = { 0x09576E91, 0x6D3F, 0x11D2, { 0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };
            VOID *Dp = NULL;
            if (IxHandleProtocol(h, &DpGuid, &Dp) == EFI_SUCCESS) {
                *Device = h;
                SerialPrint("[UEFI] LocateDevicePath: found matching handle 0x%p\n", h);
                return EFI_SUCCESS;
            }
        }
    }

    SerialPrint("[UEFI] LocateDevicePath: Protocol {%x} not found on any handle with DevicePath\n", Protocol->Data1);
    return EFI_NOT_FOUND;
}

EFI_STATUS EFIAPI IxInstallProtocolInterface(
    IN OUT EFI_HANDLE *Handle,
    IN     EFI_GUID   *Protocol,
    IN     INT32      InterfaceType,
    IN     VOID       *Interface
) {
    (void)InterfaceType;

    if (Handle == NULL || Protocol == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    if (*Handle == NULL) {
        *Handle = (EFI_HANDLE)(g_NextHandleId++);
    }

    // Find free slot
    for (UINTN i = 0; i < MAX_PROTOCOLS; i++) {
        if (!g_Protocols[i].InUse) {
            g_Protocols[i].Handle = *Handle;
            g_Protocols[i].Guid = *Protocol;
            g_Protocols[i].Interface = Interface;
            g_Protocols[i].InUse = TRUE;
            return EFI_SUCCESS;
        }
    }

    return EFI_OUT_OF_RESOURCES;
}

EFI_STATUS EFIAPI IxUninstallProtocolInterface(
    IN EFI_HANDLE Handle,
    IN EFI_GUID   *Protocol,
    IN VOID       *Interface
) {
    if (Handle == NULL || Protocol == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    for (UINTN i = 0; i < MAX_PROTOCOLS; i++) {
        if (g_Protocols[i].InUse &&
            g_Protocols[i].Handle == Handle &&
            GuidEquals(&g_Protocols[i].Guid, Protocol) &&
            g_Protocols[i].Interface == Interface) {
            g_Protocols[i].InUse = FALSE;
            return EFI_SUCCESS;
        }
    }

    return EFI_NOT_FOUND;
}

EFI_STATUS EFIAPI IxInstallConfigurationTable(
    IN EFI_GUID *Guid,
    IN VOID     *Table
) {
    if (Guid == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    // Check if table already exists -> update
    for (UINTN i = 0; i < gST.NumberOfTableEntries; i++) {
        if (GuidEquals(&g_ConfigTables[i].VendorGuid, Guid)) {
            if (Table == NULL) {
                // Delete entry by shifting
                for (UINTN j = i; j + 1 < gST.NumberOfTableEntries; j++) {
                    g_ConfigTables[j] = g_ConfigTables[j + 1];
                }
                gST.NumberOfTableEntries--;
            } else {
                g_ConfigTables[i].VendorTable = Table;
            }
            return EFI_SUCCESS;
        }
    }

    if (Table == NULL) {
        return EFI_NOT_FOUND;
    }

    if (gST.NumberOfTableEntries >= MAX_CONFIG_TABLES) {
        return EFI_OUT_OF_RESOURCES;
    }

    UINTN Idx = gST.NumberOfTableEntries++;
    g_ConfigTables[Idx].VendorGuid = *Guid;
    g_ConfigTables[Idx].VendorTable = Table;

    return EFI_SUCCESS;
}
