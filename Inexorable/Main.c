#include <Uefi.h>
#include <Base.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Protocol/GraphicsOutput.h>

#include "Efi.h"
#include "Util.h"
#include "Variable.h"
#include "Time.h"
#include "Misc.h"
#include "VirtualMemory.h"
#include "Memory.h"
#include "Protocol.h"

EFI_SYSTEM_TABLE		gST = { 0 };
EFI_BOOT_SERVICES	    gBS = { 0 };
EFI_RUNTIME_SERVICES	gRT = { 0 };

VOID
EFIAPI
TestMain(
	VOID
) {
	EFI_GRAPHICS_OUTPUT_PROTOCOL* Gop = NULL;

	EFI_STATUS Status = gBS.LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&Gop);
	if (EFI_ERROR(Status)) {
		DebugPrint0402("[GUEST] Failed to locate Graphics Output Protocol: %r\n", Status);
		return;
	}

	DebugPrint0402("[GUEST] Graphics Output Protocol at: 0x%p\n", Gop);

	DebugPrint0402("[GUEST] Graphics Output Protocol Mode.HResolution: %d\n", Gop->Mode->Info->HorizontalResolution);
	DebugPrint0402("[GUEST] Graphics Output Protocol Mode.VResolution: %d\n", Gop->Mode->Info->VerticalResolution);

	DebugPrint0402("[GUEST] Graphics Output Protocol Mode.FrameBufferBase: 0x%p\n", Gop->Mode->FrameBufferBase);
	DebugPrint0402("[GUEST] Graphics Output Protocol Mode.FrameBufferSize: 0x%p\n", Gop->Mode->FrameBufferSize);

	//
	// Blue
	//
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color = { 0, 0, 255, 0 };

	if (EFI_ERROR(Gop->Blt(Gop, &Color, EfiBltVideoFill, 0, 0, 0, 0, Gop->Mode->Info->HorizontalResolution, Gop->Mode->Info->VerticalResolution, 0))) {
		DebugPrint0402("[GUEST] Failed to call Blt: %r\n", Status);
	}
}

VOID
EFIAPI
UefiMain(
	VOID
) {
	DebugPrint0402("[GUEST] Inexorable - As if it was always there. (It never was)\n");

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

	gBS.OpenProtocol				= IxOpenProtocol;
	gBS.LocateProtocol				= IxLocateProtocol;
	gBS.HandleProtocol				= IxHandleProtocol;

	//
	// Who uses this dude?
	// 
	//gBS.GetNextMonotonicCount		= IxGetNextHighMonotonicCount;

	TestMain();
}