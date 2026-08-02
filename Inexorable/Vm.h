#ifndef _VM_H_
#define _VM_H_

///////////////////////////////////////////////
////				VMCALLs 			   ////
///////////////////////////////////////////////

#define CATALYST_VMCALL_SET_VARIABLE				0x0
#define CATALYST_VMCALL_GET_VARIABLE				0x1
#define CATALYST_VMCALL_QUERY_VARIABLE_INFO			0x2
#define CATALYST_VMCALL_GET_NEXT_VARIABLE_NAME		0x3

#define CATALYST_VMCALL_GET_TIME					0x4
#define CATALYST_VMCALL_SET_TIME					0x5
#define CATALYST_VMCALL_GET_WAKEUP_TIME				0x6
#define CATALYST_VMCALL_SET_WAKEUP_TIME				0x7

#define CATALYST_VMCALL_GET_NEXT_HIGH_MONO_COUNT	0x8
#define CATALYST_VMCALL_RESET_SYSTEM				0x9


#define CATALYST_VMCALL_EXIT_BOOT_SERVICES			0x10

#endif