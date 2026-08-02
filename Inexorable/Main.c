#include <Uefi.h>
#include <Base.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>

#include "Efi.h"
#include "Util.h"
#include "Variable.h"
#include "Time.h"
#include "Misc.h"
#include "VirtualMemory.h"
#include "Memory.h"

EFI_SYSTEM_TABLE		gST = { 0 };
EFI_BOOT_SERVICES	    gBS = { 0 };
EFI_RUNTIME_SERVICES	gRT = { 0 };

VOID
EFIAPI
UefiMain(
	VOID
) {
	DebugPrint0402("Inexorable - As if it was always there. (It never was)\n");

	//
	// Setup system table.
	//
	gST.Hdr.HeaderSize	= sizeof(gST);
	gST.Hdr.Revision	= EFI_SYSTEM_TABLE_REVISION;
	gST.Hdr.Signature	= EFI_SYSTEM_TABLE_SIGNATURE;

	gST.Hdr.CRC32 = CalculateCrc32(&gST.Hdr, gST.Hdr.HeaderSize);

	gST.BootServices                = &gBS;
	gST.RuntimeServices             = &gRT;

	//
	// Setup runtime services.
	//
	gRT.Hdr.HeaderSize	= sizeof(gRT);
	gRT.Hdr.Revision	= EFI_RUNTIME_SERVICES_REVISION;
	gRT.Hdr.Signature	= EFI_RUNTIME_SERVICES_SIGNATURE;

	gRT.Hdr.CRC32 = CalculateCrc32(&gRT.Hdr, gRT.Hdr.HeaderSize);

	gRT.GetVariable                 = IxGetVariable;
	gRT.SetVariable                 = IxSetVariable;
	gRT.QueryVariableInfo           = IxQueryVariableInfo;
	gRT.GetNextVariableName         = IxGetNextVariableName;

	gRT.SetTime                     = IxSetTime;
	gRT.GetTime                     = IxGetTime;
	gRT.GetWakeupTime               = IxGetWakeupTime;
	gRT.SetWakeupTime               = IxSetWakeupTime;

	gRT.GetNextHighMonotonicCount   = IxGetNextHighMonotonicCount;
	gRT.ResetSystem                 = IxResetSystem;

	gRT.SetVirtualAddressMap		= IxSetVirtualAddressMap;
	gRT.ConvertPointer				= IxConvertPointer;

	//
	// Setup boot services.
	//
	gBS.Hdr.HeaderSize	= sizeof(gBS);
	gBS.Hdr.Revision	= EFI_BOOT_SERVICES_REVISION;
	gBS.Hdr.Signature	= EFI_BOOT_SERVICES_SIGNATURE;

	gBS.Hdr.CRC32 = CalculateCrc32(&gBS.Hdr, gBS.Hdr.HeaderSize);

	gBS.ExitBootServices			= IxExitBootServices;

	gBS.AllocatePages				= IxAllocatePages;
	gBS.FreePages					= IxFreePages;

	gBS.AllocatePool				= IxAllocatePool;
	gBS.FreePool					= IxFreePool;

	gBS.GetMemoryMap				= IxGetMemoryMap;

	gBS.CalculateCrc32				= IxCalculateCrc32;

	gBS.CopyMem						= IxCopyMem;
	gBS.SetMem						= IxSetMem;

	//
	// Who uses this dude?
	// 
	//gBS.GetNextMonotonicCount		= IxGetNextHighMonotonicCount;
}