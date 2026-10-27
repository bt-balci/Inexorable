#!/usr/bin/env python3
"""
INEXORABLE: Firmware Flash Device (.fd) Builder
Generates a 2MB standalone QEMU ROM image containing:
- 64-bit Inexorable firmware payload at offset 0x000000 (linear 0xFFE00000)
- Reset vector trampoline & GDT at offset 0x1FF000 (linear 0xFFFFF000)
- Hardware reset vector instruction at offset 0x1FFFF0 (linear 0xFFFFFFF0)
"""

import sys
import os
import argparse

FD_SIZE = 2 * 1024 * 1024        # 2 MB (0x200000)
RESET_VECTOR_SIZE = 4096          # 4 KB (0x1000)
PAYLOAD_REGION_SIZE = 512 * 1024  # 512 KB firmware image space

def map_pe_to_memory(pe_bytes, max_size=PAYLOAD_REGION_SIZE):
    if pe_bytes[:2] != b"MZ":
        print("[INEXORABLE-FD] Payload is raw binary (not PE). Using directly.")
        return pe_bytes[:max_size]

    e_lfanew = int.from_bytes(pe_bytes[0x3C:0x40], 'little')
    if pe_bytes[e_lfanew:e_lfanew + 4] != b"PE\0\0":
        return pe_bytes[:max_size]

    num_sections = int.from_bytes(pe_bytes[e_lfanew + 0x6 : e_lfanew + 0x8], 'little')
    opt_size = int.from_bytes(pe_bytes[e_lfanew + 0x14 : e_lfanew + 0x16], 'little')
    size_of_headers = int.from_bytes(pe_bytes[e_lfanew + 0x54 : e_lfanew + 0x58], 'little')
    size_of_image = int.from_bytes(pe_bytes[e_lfanew + 0x50 : e_lfanew + 0x54], 'little')
    entry_rva = int.from_bytes(pe_bytes[e_lfanew + 0x28 : e_lfanew + 0x2C], 'little')

    print(f"[INEXORABLE-FD] Pre-mapping PE image: SizeOfImage=0x{size_of_image:X}, EntryPointRVA=0x{entry_rva:X}")

    mem = bytearray(max(size_of_image, max_size))

    # Copy headers
    mem[0:size_of_headers] = pe_bytes[0:size_of_headers]

    # Map each section to its VirtualAddress
    sec_offset = e_lfanew + 0x18 + opt_size
    for i in range(num_sections):
        sec = pe_bytes[sec_offset + i*40 : sec_offset + (i+1)*40]
        name = sec[0:8].decode('latin1', 'ignore').strip('\x00')
        vsize = int.from_bytes(sec[8:12], 'little')
        vaddr = int.from_bytes(sec[12:16], 'little')
        raw_size = int.from_bytes(sec[16:20], 'little')
        raw_ptr = int.from_bytes(sec[20:24], 'little')

        copy_len = min(vsize, raw_size)
        if copy_len > 0 and raw_ptr > 0:
            mem[vaddr : vaddr + copy_len] = pe_bytes[raw_ptr : raw_ptr + copy_len]
            print(f"  Mapped section {name:8s} to 0x{vaddr:05X} ({copy_len} bytes)")

    return bytes(mem[:max_size])

def build_fd(payload_path, reset_vector_path, output_path):
    print(f"[INEXORABLE-FD] Creating 2MB firmware image: {output_path}")
    image = bytearray([0xFF] * FD_SIZE)

    # 1. Place Payload if available
    if payload_path and os.path.exists(payload_path):
        with open(payload_path, "rb") as f:
            raw_payload = f.read()

        payload = map_pe_to_memory(raw_payload, PAYLOAD_REGION_SIZE)
        print(f"[INEXORABLE-FD] Embedding mapped payload: {len(payload)} bytes at offset 0x000000")
        image[0:len(payload)] = payload
    else:
        print(f"[INEXORABLE-FD] Notice: No payload specified or file missing. Using minimal HLT payload at 0x000000.")
        image[0:4] = b"\xFA\xF4\xEB\xFC"

    # 2. Place Reset Vector
    if not os.path.exists(reset_vector_path):
        raise FileNotFoundError(f"Reset vector binary not found at: {reset_vector_path}")

    with open(reset_vector_path, "rb") as f:
        reset_vec = f.read()

    if len(reset_vec) != RESET_VECTOR_SIZE:
        raise ValueError(f"Reset vector size must be exactly {RESET_VECTOR_SIZE} bytes (got {len(reset_vec)})")

    reset_vec_offset = FD_SIZE - RESET_VECTOR_SIZE
    image[reset_vec_offset:FD_SIZE] = reset_vec
    print(f"[INEXORABLE-FD] Embedded reset vector at offset 0x{reset_vec_offset:06X} ({len(reset_vec)} bytes)")

    # 3. Verify Hardware Reset Vector Opcode at 0x1FFFF0
    reset_vector_inst_offset = FD_SIZE - 16
    first_byte = image[reset_vector_inst_offset]
    print(f"[INEXORABLE-FD] Verifying reset vector at offset 0x{reset_vector_inst_offset:06X}: Opcode 0x{first_byte:02X}")
    if first_byte not in (0xE9, 0xEB, 0xEA):
        raise RuntimeError(f"Invalid reset vector opcode at 0x{reset_vector_inst_offset:06X}: 0x{first_byte:02X} (expected JMP)")

    # 4. Write image
    with open(output_path, "wb") as f:
        f.write(image)

    print(f"[INEXORABLE-FD] Successfully generated {output_path} ({len(image)} bytes).")

if __name__ == "__main__":
    candidates = [
        "Inexorable/x64/Release/Inexorable.efi",
        "x64/Release/Inexorable.efi",
        "Inexorable/x64/Debug/Inexorable.efi",
        "x64/Debug/Inexorable.efi"
    ]
    default_payload = None
    for cand in candidates:
        if os.path.exists(cand):
            default_payload = cand
            break

    parser = argparse.ArgumentParser(description="Inexorable Firmware FD Builder")
    parser.add_argument("--payload", default=default_payload, help="Path to inexorable 64-bit binary payload")
    parser.add_argument("--reset-vector", default="qemu_target/reset_vector.bin", help="Path to reset_vector.bin")
    parser.add_argument("--output", default="inexorable.fd", help="Path to output .fd image")
    args = parser.parse_args()

    build_fd(args.payload, args.reset_vector, args.output)
