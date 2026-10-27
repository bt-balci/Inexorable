#pragma once

#include "efi_spec.h"

#define MAX_HANDLES     64
#define MAX_PROTOCOLS   128
#define MAX_CONFIG_TABLES 16

void ProtocolInit(void);

EFI_STATUS EFIAPI IxHandleProtocol(
    IN  EFI_HANDLE Handle,
    IN  EFI_GUID   *Protocol,
    OUT VOID       **Interface
);

EFI_STATUS EFIAPI IxOpenProtocol(
    IN  EFI_HANDLE Handle,
    IN  EFI_GUID   *Protocol,
    OUT VOID       **Interface OPTIONAL,
    IN  EFI_HANDLE AgentHandle,
    IN  EFI_HANDLE ControllerHandle,
    IN  UINT32     Attributes
);

EFI_STATUS EFIAPI IxCloseProtocol(
    IN EFI_HANDLE Handle,
    IN EFI_GUID   *Protocol,
    IN EFI_HANDLE AgentHandle,
    IN EFI_HANDLE ControllerHandle
);

EFI_STATUS EFIAPI IxLocateProtocol(
    IN  EFI_GUID *Protocol,
    IN  VOID     *Registration OPTIONAL,
    OUT VOID     **Interface
);

EFI_STATUS EFIAPI IxLocateHandle(
    IN     INT32      SearchType,
    IN     EFI_GUID   *Protocol,
    IN     VOID       *SearchKey,
    IN OUT UINTN      *BufferSize,
    OUT    EFI_HANDLE *Buffer
);

EFI_STATUS EFIAPI IxLocateHandleBuffer(
    IN     INT32      SearchType,
    IN     EFI_GUID   *Protocol,
    IN     VOID       *SearchKey,
    OUT    UINTN      *NoHandles,
    OUT    EFI_HANDLE **Buffer
);

EFI_STATUS EFIAPI IxLocateDevicePath(
    IN     EFI_GUID                       *Protocol,
    IN OUT EFI_DEVICE_PATH_PROTOCOL       **DevicePath,
    OUT    EFI_HANDLE                     *Device
);

EFI_STATUS EFIAPI IxInstallProtocolInterface(
    IN OUT EFI_HANDLE *Handle,
    IN     EFI_GUID   *Protocol,
    IN     INT32      InterfaceType,
    IN     VOID       *Interface
);

EFI_STATUS EFIAPI IxUninstallProtocolInterface(
    IN EFI_HANDLE Handle,
    IN EFI_GUID   *Protocol,
    IN VOID       *Interface
);

EFI_STATUS EFIAPI IxInstallConfigurationTable(
    IN EFI_GUID *Guid,
    IN VOID     *Table
);
