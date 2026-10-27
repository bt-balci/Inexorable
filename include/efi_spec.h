#pragma once

#ifndef EFIAPI
#if defined(_MSC_VER)
#define EFIAPI __cdecl
#else
#define EFIAPI __attribute__((ms_abi))
#endif
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifndef _UEFI_BASE_TYPES_
#define _UEFI_BASE_TYPES_

#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif
#ifndef OPTIONAL
#define OPTIONAL
#endif
#ifndef CONST
#define CONST const
#endif

typedef uint8_t   UINT8;
typedef uint16_t  UINT16;
typedef uint32_t  UINT32;
typedef uint64_t  UINT64;
typedef int8_t    INT8;
typedef int16_t   INT16;
typedef int32_t   INT32;
typedef int64_t   INT64;
typedef uint64_t  UINTN;
typedef int64_t   INTN;
typedef char      CHAR8;
typedef uint16_t  CHAR16;
typedef uint8_t   BOOLEAN;
#ifndef TRUE
#define TRUE  ((BOOLEAN)1)
#define FALSE ((BOOLEAN)0)
#endif
#ifndef VOID
#define VOID void
#endif

typedef void*     EFI_HANDLE;
typedef void*     EFI_EVENT;
typedef UINT64    EFI_STATUS;
typedef UINT64    EFI_TPL;
typedef UINT64    EFI_PHYSICAL_ADDRESS;
typedef UINT64    EFI_VIRTUAL_ADDRESS;

#ifndef EFI_ERROR
#define EFI_ERROR(status) (((INT64)(status)) < 0)
#endif

#define EFI_SUCCESS                 0x0000000000000000ULL
#define EFI_LOAD_ERROR              0x8000000000000001ULL
#define EFI_INVALID_PARAMETER       0x8000000000000002ULL
#define EFI_UNSUPPORTED             0x8000000000000003ULL
#define EFI_BAD_BUFFER_SIZE         0x8000000000000004ULL
#define EFI_BUFFER_TOO_SMALL        0x8000000000000005ULL
#define EFI_NOT_READY               0x8000000000000006ULL
#define EFI_DEVICE_ERROR            0x8000000000000007ULL
#define EFI_WRITE_PROTECTED         0x8000000000000008ULL
#define EFI_OUT_OF_RESOURCES        0x8000000000000009ULL
#define EFI_NOT_FOUND               0x800000000000000EULL
#define EFI_ACCESS_DENIED           0x800000000000000FULL
#define EFI_NO_MAPPING              0x8000000000000011ULL
#define EFI_TIMEOUT                 0x8000000000000012ULL
#define EFI_NOT_STARTED             0x8000000000000013ULL
#define EFI_ALREADY_STARTED         0x8000000000000014ULL
#define EFI_ABORTED                 0x8000000000000015ULL
#define EFI_NOT_AVAILABLE_YET       0x800000000000001CULL

#ifndef _EFI_GUID_DEFINED_
#define _EFI_GUID_DEFINED_
typedef struct {
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8  Data4[8];
} EFI_GUID;
#endif

typedef enum {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
    MaxAllocateType
} EFI_ALLOCATE_TYPE;

typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef struct {
    UINT32                Type;
    EFI_PHYSICAL_ADDRESS  PhysicalStart;
    EFI_VIRTUAL_ADDRESS   VirtualStart;
    UINT64                NumberOfPages;
    UINT64                Attribute;
} EFI_MEMORY_DESCRIPTOR;

#define EFI_MEMORY_UC   0x0000000000000001ULL
#define EFI_MEMORY_WC   0x0000000000000002ULL
#define EFI_MEMORY_WT   0x0000000000000004ULL
#define EFI_MEMORY_WB   0x0000000000000008ULL
#define EFI_MEMORY_UCE  0x0000000000000010ULL
#define EFI_MEMORY_WP   0x0000000000001000ULL
#define EFI_MEMORY_RP   0x0000000000002000ULL
#define EFI_MEMORY_XP   0x0000000000004000ULL
#define EFI_MEMORY_NV   0x0000000000008000ULL
#define EFI_MEMORY_MORE_RELIABLE 0x0000000000010000ULL
#define EFI_MEMORY_RO   0x0000000000020000ULL
#define EFI_MEMORY_RUNTIME 0x8000000000000000ULL

#define EFI_PAGE_SIZE   4096ULL
#define EFI_PAGE_MASK   0xFFFULL
#define EFI_PAGE_SHIFT  12
#define EFI_SIZE_TO_PAGES(Size)  (((Size) + EFI_PAGE_MASK) >> EFI_PAGE_SHIFT)

typedef struct {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;

#define EFI_SYSTEM_TABLE_SIGNATURE      0x5453595320494249ULL
#define EFI_SYSTEM_TABLE_REVISION       ((2<<16) | (80))
#define EFI_BOOT_SERVICES_SIGNATURE     0x56524553544f4f42ULL
#define EFI_BOOT_SERVICES_REVISION      EFI_SYSTEM_TABLE_REVISION
#define EFI_RUNTIME_SERVICES_SIGNATURE  0x56524553544e5552ULL
#define EFI_RUNTIME_SERVICES_REVISION   EFI_SYSTEM_TABLE_REVISION

typedef struct {
    EFI_GUID VendorGuid;
    VOID     *VendorTable;
} EFI_CONFIGURATION_TABLE;

#endif // _UEFI_BASE_TYPES_

// Specific Standard GUIDs
#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    { 0x5B1B31A1, 0x9562, 0x11D2, { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } }

#define EFI_LOAD_FILE2_PROTOCOL_GUID \
    { 0x4006C0C1, 0xFCB3, 0x403E, { 0x99, 0x6D, 0x4A, 0x6C, 0x87, 0x24, 0xE0, 0x6D } }

#define LINUX_EFI_INITRD_MEDIA_GUID \
    { 0x5568E427, 0x68FC, 0x4F3D, { 0xAC, 0x74, 0xCA, 0x55, 0x52, 0x31, 0xCC, 0x68 } }

#define EFI_RNG_PROTOCOL_GUID \
    { 0x3152BCA5, 0xEADE, 0x433D, { 0x86, 0x2E, 0xC0, 0x1C, 0xDC, 0x29, 0x1F, 0x44 } }

#define EFI_RNG_ALGORITHM_RAW \
    { 0xE43176D7, 0xB6E8, 0x4827, { 0xB7, 0x84, 0x7F, 0xFD, 0xC4, 0xB6, 0x85, 0x61 } }

#define EFI_ACPI_20_TABLE_GUID \
    { 0x8868E871, 0xE4F1, 0x11D3, { 0xBC, 0x22, 0x00, 0x80, 0xC7, 0x3C, 0x88, 0x81 } }

#define EFI_RT_PROPERTIES_TABLE_GUID \
    { 0xEB66918A, 0x7EEF, 0x402A, { 0x84, 0x2E, 0x93, 0x1D, 0x21, 0xC3, 0x8A, 0xE9 } }

#pragma pack(push, 1)
typedef struct {
    UINT16 Version;
    UINT16 Length;
    UINT32 RuntimeServicesSupported;
} EFI_RT_PROPERTIES_TABLE;

typedef struct {
    UINT8  Type;
    UINT8  SubType;
    UINT16 Length;
} EFI_DEVICE_PATH_PROTOCOL;

typedef struct {
    struct {
        UINT8    Type;
        UINT8    SubType;
        UINT16   Length;
        EFI_GUID Guid;
    } Vendor;
    struct {
        UINT8    Type;
        UINT8    SubType;
        UINT16   Length;
    } End;
} IX_INITRD_DEVICE_PATH;
#pragma pack(pop)

// Forward declarations
struct _EFI_SYSTEM_TABLE;
struct _EFI_BOOT_SERVICES;
struct _EFI_RUNTIME_SERVICES;

typedef EFI_STATUS (EFIAPI *EFI_IMAGE_ENTRY_POINT)(
    EFI_HANDLE                ImageHandle,
    struct _EFI_SYSTEM_TABLE  *SystemTable
);

typedef EFI_STATUS (EFIAPI *EFI_IMAGE_UNLOAD)(
    EFI_HANDLE  ImageHandle
);

typedef struct {
    UINT32            Revision;
    EFI_HANDLE        ParentHandle;
    struct _EFI_SYSTEM_TABLE *SystemTable;
    EFI_HANDLE        DeviceHandle;
    VOID              *FilePath;
    VOID              *Reserved;
    UINT32            LoadOptionsSize;
    VOID              *LoadOptions;
    VOID              *ImageBase;
    UINT64            ImageSize;
    EFI_MEMORY_TYPE   ImageCodeType;
    EFI_MEMORY_TYPE   ImageDataType;
    EFI_IMAGE_UNLOAD  Unload;
} EFI_LOADED_IMAGE_PROTOCOL;

typedef struct _EFI_LOAD_FILE2_PROTOCOL EFI_LOAD_FILE2_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_LOAD_FILE2)(
    EFI_LOAD_FILE2_PROTOCOL   *This,
    EFI_DEVICE_PATH_PROTOCOL  *FilePath,
    BOOLEAN                   BootPolicy,
    UINTN                     *BufferSize,
    VOID                      *Buffer
);

struct _EFI_LOAD_FILE2_PROTOCOL {
    EFI_LOAD_FILE2 LoadFile;
};

typedef struct _EFI_RNG_PROTOCOL EFI_RNG_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_RNG_GET_INFO)(
    EFI_RNG_PROTOCOL *This,
    UINTN            *RNGAlgorithmListSize,
    EFI_GUID         *RNGAlgorithmList
);

typedef EFI_STATUS (EFIAPI *EFI_RNG_GET_RNG)(
    EFI_RNG_PROTOCOL *This,
    EFI_GUID         *RNGAlgorithm,
    UINTN            RNGValueLength,
    UINT8            *RNGValue
);

struct _EFI_RNG_PROTOCOL {
    EFI_RNG_GET_INFO GetInfo;
    EFI_RNG_GET_RNG  GetRNG;
};

// Boot Services function pointer types
typedef EFI_TPL (EFIAPI *EFI_RAISE_TPL)(EFI_TPL NewTpl);
typedef VOID (EFIAPI *EFI_RESTORE_TPL)(EFI_TPL OldTpl);

typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_PAGES)(
    EFI_ALLOCATE_TYPE     Type,
    EFI_MEMORY_TYPE       MemoryType,
    UINTN                 Pages,
    EFI_PHYSICAL_ADDRESS  *Memory
);

typedef EFI_STATUS (EFIAPI *EFI_FREE_PAGES)(
    EFI_PHYSICAL_ADDRESS  Memory,
    UINTN                 Pages
);

typedef EFI_STATUS (EFIAPI *EFI_GET_MEMORY_MAP)(
    UINTN                  *MemoryMapSize,
    EFI_MEMORY_DESCRIPTOR  *MemoryMap,
    UINTN                  *MapKey,
    UINTN                  *DescriptorSize,
    UINT32                 *DescriptorVersion
);

typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_POOL)(
    EFI_MEMORY_TYPE  PoolType,
    UINTN            Size,
    VOID             **Buffer
);

typedef EFI_STATUS (EFIAPI *EFI_FREE_POOL)(
    VOID  *Buffer
);

typedef EFI_STATUS (EFIAPI *EFI_EXIT_BOOT_SERVICES)(
    EFI_HANDLE  ImageHandle,
    UINTN       MapKey
);

typedef EFI_STATUS (EFIAPI *EFI_INSTALL_CONFIGURATION_TABLE)(
    EFI_GUID  *Guid,
    VOID      *Table
);

typedef EFI_STATUS (EFIAPI *EFI_LOCATE_HANDLE)(
    INT32     SearchType,
    EFI_GUID  *Protocol,
    VOID      *SearchKey,
    UINTN     *BufferSize,
    EFI_HANDLE *Buffer
);

typedef EFI_STATUS (EFIAPI *EFI_LOCATE_PROTOCOL)(
    EFI_GUID  *Protocol,
    VOID      *Registration,
    VOID      **Interface
);

typedef EFI_STATUS (EFIAPI *EFI_HANDLE_PROTOCOL)(
    EFI_HANDLE  Handle,
    EFI_GUID    *Protocol,
    VOID        **Interface
);

typedef EFI_STATUS (EFIAPI *EFI_STALL)(
    UINTN  Microseconds
);

typedef EFI_STATUS (EFIAPI *EFI_SET_WATCHDOG_TIMER)(
    UINTN   Timeout,
    UINT64  WatchdogCode,
    UINTN   DataSize,
    CHAR16  *WatchdogData
);

typedef VOID (EFIAPI *EFI_COPY_MEM)(
    VOID   *Destination,
    VOID   *Source,
    UINTN  Length
);

typedef VOID (EFIAPI *EFI_SET_MEM)(
    VOID   *Buffer,
    UINTN  Size,
    UINT8  Value
);

typedef EFI_STATUS (EFIAPI *EFI_CALCULATE_CRC32)(
    VOID    *Data,
    UINTN   DataSize,
    UINT32  *Crc32
);

typedef struct _EFI_BOOT_SERVICES {
    EFI_TABLE_HEADER               Hdr;
    EFI_RAISE_TPL                  RaiseTPL;
    EFI_RESTORE_TPL                RestoreTPL;
    EFI_ALLOCATE_PAGES             AllocatePages;
    EFI_FREE_PAGES                 FreePages;
    EFI_GET_MEMORY_MAP             GetMemoryMap;
    EFI_ALLOCATE_POOL              AllocatePool;
    EFI_FREE_POOL                  FreePool;
    VOID                           *CreateEvent;
    VOID                           *SetTimer;
    VOID                           *WaitForEvent;
    VOID                           *SignalEvent;
    VOID                           *CloseEvent;
    VOID                           *CheckEvent;
    VOID                           *InstallProtocolInterface;
    VOID                           *ReinstallProtocolInterface;
    VOID                           *UninstallProtocolInterface;
    EFI_HANDLE_PROTOCOL            HandleProtocol;
    VOID                           *Reserved;
    VOID                           *RegisterProtocolNotify;
    EFI_LOCATE_HANDLE              LocateHandle;
    VOID                           *LocateDevicePath;
    EFI_INSTALL_CONFIGURATION_TABLE InstallConfigurationTable;
    VOID                           *LoadImage;
    VOID                           *StartImage;
    VOID                           *Exit;
    VOID                           *UnloadImage;
    EFI_EXIT_BOOT_SERVICES         ExitBootServices;
    VOID                           *GetNextMonotonicCount;
    EFI_STALL                      Stall;
    EFI_SET_WATCHDOG_TIMER         SetWatchdogTimer;
    VOID                           *ConnectController;
    VOID                           *DisconnectController;
    VOID                           *OpenProtocol;
    VOID                           *CloseProtocol;
    VOID                           *OpenProtocolInformation;
    VOID                           *ProtocolsPerHandle;
    VOID                           *LocateHandleBuffer;
    EFI_LOCATE_PROTOCOL            LocateProtocol;
    VOID                           *InstallMultipleProtocolInterfaces;
    VOID                           *UninstallMultipleProtocolInterfaces;
    EFI_CALCULATE_CRC32            CalculateCrc32;
    EFI_COPY_MEM                   CopyMem;
    EFI_SET_MEM                    SetMem;
    VOID                           *CreateEventEx;
} EFI_BOOT_SERVICES;

// Runtime Services function pointer types
typedef EFI_STATUS (EFIAPI *EFI_GET_VARIABLE)(
    CHAR16    *VariableName,
    EFI_GUID  *VendorGuid,
    UINT32    *Attributes,
    UINTN     *DataSize,
    VOID      *Data
);

typedef EFI_STATUS (EFIAPI *EFI_SET_VARIABLE)(
    CHAR16    *VariableName,
    EFI_GUID  *VendorGuid,
    UINT32    Attributes,
    UINTN     DataSize,
    VOID      *Data
);

typedef VOID (EFIAPI *EFI_RESET_SYSTEM)(
    INT32       ResetType,
    EFI_STATUS  ResetStatus,
    UINTN       DataSize,
    VOID        *ResetData
);

typedef struct _EFI_RUNTIME_SERVICES {
    EFI_TABLE_HEADER    Hdr;
    VOID                *GetTime;
    VOID                *SetTime;
    VOID                *GetWakeupTime;
    VOID                *SetWakeupTime;
    VOID                *SetVirtualAddressMap;
    VOID                *ConvertPointer;
    EFI_GET_VARIABLE    GetVariable;
    VOID                *GetNextVariableName;
    EFI_SET_VARIABLE    SetVariable;
    VOID                *GetNextHighMonotonicCount;
    EFI_RESET_SYSTEM    ResetSystem;
    VOID                *UpdateCapsule;
    VOID                *QueryCapsuleCapabilities;
    VOID                *QueryVariableInfo;
} EFI_RUNTIME_SERVICES;

typedef struct _EFI_SYSTEM_TABLE {
    EFI_TABLE_HEADER          Hdr;
    CHAR16                    *FirmwareVendor;
    UINT32                    FirmwareRevision;
    EFI_HANDLE                ConsoleInHandle;
    VOID                      *ConIn;
    EFI_HANDLE                ConsoleOutHandle;
    VOID                      *ConOut;
    EFI_HANDLE                StandardErrorHandle;
    VOID                      *StdErr;
    EFI_RUNTIME_SERVICES      *RuntimeServices;
    EFI_BOOT_SERVICES         *BootServices;
    UINTN                     NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE   *ConfigurationTable;
} EFI_SYSTEM_TABLE;
