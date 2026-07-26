#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <Uefi.h>

#include "Vm.h"

///////////////////////////////////////////////
////				Globals				   ////
///////////////////////////////////////////////

__attribute__((aligned(4096))) extern CR3               g_IdentityCr3;
__attribute__((aligned(4096))) extern PML4_1GB          g_IdentityPml4[512];
__attribute__((aligned(4096))) extern PDPE_1GB          g_IdentityPdpe[512];

__attribute__((aligned(4096))) extern EPTP				g_Eptp;
__attribute__((aligned(4096))) extern EPT_PML4			g_Epml4[512];
__attribute__((aligned(4096))) extern EPT_PDPTE         g_Epdpt[64][512];
__attribute__((aligned(4096))) extern EPT_LARGE_PDE     g_Epde[512][512];
__attribute__((aligned(4096))) extern EPT_PTE		    g_Epte[16384][512];

///////////////////////////////////////////////
////				Methods				   ////
///////////////////////////////////////////////


BOOLEAN
EFIAPI
MapSinglePage(
    IN UINT64   GuestPhysical,
    IN UINT64   HostPhysical,
    IN BOOLEAN  Big
);

BOOLEAN
EFIAPI
MapPagesLinear(
    IN UINT64       PhysicalStart,
    IN UINT64       NumberOfPages,
    IN OUT UINT64*  CurrentGuestPhysical
);

VOID*
EFIAPI
TranslateGuestToHostAddress(
    IN UINT64 GuestPhysicalAddress
);

#endif