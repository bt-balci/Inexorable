#include <Uefi.h>
#include "Vm.h"
#include "x86.h"

EFI_STATUS
EFIAPI
IxFreePool(
	IN VOID* Buffer
) {
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_FREE_POOL, (UINT64)Buffer, 0, 0, 0, 0, 0);
}

EFI_STATUS
EFIAPI
IxFreePages(
	IN EFI_PHYSICAL_ADDRESS Memory,
	IN UINTN				Pages
) {
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_FREE_PAGES, Memory, Pages, 0, 0, 0, 0);
}

EFI_STATUS
EFIAPI
IxAllocatePool(
	IN EFI_MEMORY_TYPE	PoolType,
	IN UINTN			Size,
	OUT VOID**			Buffer
) {
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_ALLOCATE_POOL, PoolType, Size, (UINT64)Buffer, 0, 0, 0);
}

EFI_STATUS
EFIAPI
IxAllocatePages(
	IN EFI_ALLOCATE_TYPE			Type,
	IN EFI_MEMORY_TYPE				MemoryType,
	IN UINTN						Pages,
	IN OUT EFI_PHYSICAL_ADDRESS*	Memory
) {
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_ALLOCATE_PAGES, Type, MemoryType, Pages, (UINT64)Memory, 0, 0);
}

EFI_STATUS
EFIAPI
IxGetMemoryMap(
	IN OUT UINTN* MemoryMapSize,
	OUT EFI_MEMORY_DESCRIPTOR* MemoryMap,
	OUT UINTN* MapKey,
	OUT UINTN* DescriptorSize,
	OUT UINT32* DescriptorVersion
) {
	return (EFI_STATUS)__vmx_vmcall(CATALYST_VMCALL_GET_MEMORY_MAP, (UINT64)MemoryMapSize, (UINT64)MemoryMap, (UINT64)MapKey, (UINT64)DescriptorSize, (UINT64)DescriptorVersion, 0);
}