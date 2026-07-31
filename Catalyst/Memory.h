#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <Uefi.h>

#include "Vm.h"

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