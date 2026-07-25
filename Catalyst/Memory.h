#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <Uefi.h>


#include "Vm.h"

//
// Maybe move to x86.h?
//

///////////////////////////////////////////////
////				Globals				   ////
///////////////////////////////////////////////

__attribute__((aligned(4096))) extern EPTP				g_Eptp;
__attribute__((aligned(4096))) extern EPT_PML4			g_Epml4[512];
__attribute__((aligned(4096))) extern EPT_PDPTE         g_Epdpt[64][512];
__attribute__((aligned(4096))) extern EPT_LARGE_PDE     g_Epde[512][512];
__attribute__((aligned(4096))) extern EPT_PTE		    g_Epte[16384][512];

///////////////////////////////////////////////
////				Methods				   ////
///////////////////////////////////////////////

VOID
EFIAPI
MapPagesLinear(
    IN UINT64                       PhysicalStart,
    IN UINT64                       NumberOfPages,
    IN OUT UINT64*                  CurrentGuestPhysical
);

#endif