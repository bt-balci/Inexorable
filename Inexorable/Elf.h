#pragma once
#include <Uefi.h>

#define EI_NIDENT 16

enum {
    ElfIdentMagic0 = 0,
    ElfIdentMagic1 = 1,
    ElfIdentMagic2 = 2,
    ElfIdentMagic3 = 3,
    ElfIdentClass = 4,
    ElfIdentData = 5,
    ElfIdentVersion = 6,
    ElfIdentOsAbi = 7,
    ElfIdentAbiVersion = 8,
    ElfIdentPad = 9
} ElfIdent;

enum {
    ElfTypeNone = 0,
    ElfTypeRelocatable = 1,
    ElfTypeExecutable = 2
} ElfType;

#define PT_LOAD 1

#define EM_X86_64   0x3E
#define EV_CURRENT	1

#define ELF_MAG0 0x7f
#define ELF_MAG1 'E'
#define ELF_MAG2 'L'
#define ELF_MAG3 'F'

typedef struct {
    UINT8   Ident[EI_NIDENT];
    UINT16  Type;
    UINT16  Machine;
    UINT32  Version;
    UINT64  Entry;
    UINT64  Phoff;
    UINT64  Shoff;
    UINT32  Flags;
    UINT16  Ehsize;
    UINT16  Phentsize;
    UINT16  Phnum;
    UINT16  Shentsize;
    UINT16  Shnum;
    UINT16  Shstrndx;
} ELF64_HEADER;

typedef struct {
    UINT32   Type;
    UINT32   Flags;
    UINT64   Offset;
    UINT64   Vaddr;
    UINT64   Paddr;
    UINT64   FileSize;
    UINT64   MemSize;
    UINT64   Align;
} ELF64_PHDR;

typedef struct {
    UINT32  Name;
    UINT32  Type;
    UINT64  Flags;
    UINT64  Addr;
    UINT64  Offset;
    UINT64  Size;
    UINT32  Link;
    UINT32  Info;
    UINT64  AddrAlign;
    UINT64  EntSize;
} ELF64_SHDR;