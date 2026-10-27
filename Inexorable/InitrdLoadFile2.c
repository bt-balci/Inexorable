#include "InitrdLoadFile2.h"
#include "inexorable_layout.h"
#include "Protocol.h"
#include "Serial.h"

static IX_INITRD_DEVICE_PATH g_InitrdDevicePath = {
    .Vendor = {
        .Type = 0x04,       // MEDIA_DEVICE_PATH
        .SubType = 0x03,    // MEDIA_VENDOR_DP
        .Length = sizeof(g_InitrdDevicePath.Vendor),
        .Guid = LINUX_EFI_INITRD_MEDIA_GUID
    },
    .End = {
        .Type = 0x7F,       // END_DEVICE_PATH_TYPE
        .SubType = 0xFF,    // END_ENTIRE_DEVICE_PATH_SUBTYPE
        .Length = sizeof(g_InitrdDevicePath.End)
    }
};

static EFI_STATUS EFIAPI IxInitrdLoadFile2(
    EFI_LOAD_FILE2_PROTOCOL  *This,
    EFI_DEVICE_PATH_PROTOCOL *FilePath,
    BOOLEAN                  BootPolicy,
    UINTN                    *BufferSize,
    VOID                     *Buffer
) {
    (void)This;
    (void)FilePath;

    if (BootPolicy) {
        return EFI_UNSUPPORTED;
    }

    if (BufferSize == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    UINTN ActualInitrdSize = (UINTN)g_HostBootInfo->InitrdSize;
    if (ActualInitrdSize == 0) {
        SerialPrint("[INITRD] No initrd registered (InitrdSize = 0)\n");
        return EFI_NOT_FOUND;
    }

    if (Buffer == NULL || *BufferSize < ActualInitrdSize) {
        *BufferSize = ActualInitrdSize;
        return EFI_BUFFER_TOO_SMALL;
    }

    SerialPrint("[INITRD] Streaming initrd (%u bytes) from GPA 0x%p to buffer 0x%p...\n",
        ActualInitrdSize, GUEST_INITRD_STAGE_BASE, Buffer);

    const UINT8 *src = (const UINT8 *)GUEST_INITRD_STAGE_BASE;
    UINT8 *dst = (UINT8 *)Buffer;
    for (UINTN i = 0; i < ActualInitrdSize; i++) {
        dst[i] = src[i];
    }

    *BufferSize = ActualInitrdSize;
    SerialPrint("[INITRD] Initrd streamed successfully!\n");
    return EFI_SUCCESS;
}

static EFI_LOAD_FILE2_PROTOCOL g_InitrdLoadFile2Protocol = {
    .LoadFile = IxInitrdLoadFile2
};

static EFI_HANDLE g_InitrdHandle = NULL;

EFI_STATUS InitrdProtocolInit(void) {
    EFI_GUID DevicePathGuid = { 0x09576E91, 0x6D3F, 0x11D2, { 0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };
    EFI_GUID LoadFile2Guid  = EFI_LOAD_FILE2_PROTOCOL_GUID;

    EFI_STATUS Status = IxInstallProtocolInterface(
        &g_InitrdHandle,
        &DevicePathGuid,
        0,
        &g_InitrdDevicePath
    );
    if (EFI_ERROR(Status)) {
        SerialPrint("[INITRD] Failed to install DevicePath protocol: 0x%x\n", Status);
        return Status;
    }

    Status = IxInstallProtocolInterface(
        &g_InitrdHandle,
        &LoadFile2Guid,
        0,
        &g_InitrdLoadFile2Protocol
    );
    if (EFI_ERROR(Status)) {
        SerialPrint("[INITRD] Failed to install LoadFile2 protocol: 0x%x\n", Status);
        return Status;
    }

    SerialPrint("[INITRD] LOAD_FILE2 protocol installed on handle 0x%p (LINUX_EFI_INITRD_MEDIA_GUID)\n", g_InitrdHandle);
    return EFI_SUCCESS;
}
