#pragma once
#include <Uefi.h>
#include "x86.h"

/*
	Structure representing each virtual processor.
*/
typedef struct {
	INTEL_VMX_REGION VmxOn;
	INTEL_VMX_REGION Vmcs;
	X86_CONTEXT_AREA Context;
	UINT32 Index;
} __attribute__((aligned(4096))) CATALYST_VIRTUAL_CPU;