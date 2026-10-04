#!/usr/bin/env python3
"""Patch a copy of the 1.1 Release FIREFGHT.EXE so it runs on NT-family Windows (Windows 11 too).

The original bug: Eem::init (1ee_main.cpp) calls Kbd::init and Mouse::init before it sets
Eem::thread, so both SetWindowsHookEx calls get thread 0 and no module. That asks for a
system-wide hook, which Windows 95 accepted and NT refuses: the game stops with "unable to hook
keyboard [Kbd::init]".

The patch gives both hooks the current thread. In Kbd::init and Mouse::init, the 5-byte
`mov eax,[Eem::thread]` before `push eax` becomes a call to a 12-byte stub placed in int3 padding:
`call [GetCurrentThreadId]; mov [Eem::thread],eax; ret`. Eem::init stores the same value a few
lines later, so nothing else changes. 22 bytes in all. The exe isn't marked for ASLR, so it always
loads at its image base and the stub's absolute addresses hold.

It patches the file in place, so give it a copy, never the archive's exe (it refuses those).

  patch_hooks.py EXE

docs/original-archive.md ("Running 1.1") has the rest of the setup.
"""
import argparse
import hashlib
import pathlib
import struct
import sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from ffarchive import archive_root  # noqa: E402

ORIGINAL = 'db4b0bd7bf2c540b44d670714fb5a185508c040e006b6a86773f377f2452da9d'  # FF/C/GAME/RELEASE
PATCHED = 'd05dff7e16dcccbbb2ff4e2eb4edde4db96beeb21af47466b2f6f1ceeef7621a'

THREAD = 0x4b5f80         # Eem::thread
GET_THREAD_ID = 0x54b394  # import address of GetCurrentThreadId
SITES = (0x47d9a1, 0x47fe15)  # mov eax,[Eem::thread] in Kbd::init, Mouse::init
STUB = 0x403872           # inside 15 bytes of int3 padding at 0x403871


def file_offset(exe, va):
    """File offset of a virtual address, from the PE section table."""
    pe = struct.unpack_from('<I', exe, 0x3c)[0]
    sections = struct.unpack_from('<H', exe, pe + 6)[0]
    base = struct.unpack_from('<I', exe, pe + 0x34)[0]
    table = pe + 24 + struct.unpack_from('<H', exe, pe + 20)[0]
    for i in range(sections):
        vsize, vaddr, rsize, raw = struct.unpack_from('<4I', exe, table + 40 * i + 8)
        if vaddr <= va - base < vaddr + max(vsize, rsize):
            return va - base - vaddr + raw
    raise ValueError(f'{va:#x} is in no section')


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('exe', help='a copy of FF/C/GAME/RELEASE/FIREFGHT.EXE')
    path = pathlib.Path(ap.parse_args().exe).resolve()

    archive = archive_root(None)
    if archive and archive.resolve() in path.parents:
        sys.exit(f'{path} is inside the archive; copy it out first')
    exe = bytearray(path.read_bytes())
    digest = hashlib.sha256(exe).hexdigest()
    if digest == PATCHED:
        print(f'{path}: already patched')
        return
    if digest != ORIGINAL:
        sys.exit(f'{path}: not the 1.1 Release FIREFGHT.EXE (sha256 {digest})')

    stub = (b'\xff\x15' + struct.pack('<I', GET_THREAD_ID) +  # call [GetCurrentThreadId]
            b'\xa3' + struct.pack('<I', THREAD) +             # mov [Eem::thread],eax
            b'\xc3')                                          # ret
    at = file_offset(exe, STUB)
    exe[at:at + len(stub)] = stub
    for site in SITES:
        at = file_offset(exe, site)
        exe[at:at + 5] = b'\xe8' + struct.pack('<i', STUB - (site + 5))  # call stub
    if hashlib.sha256(exe).hexdigest() != PATCHED:
        sys.exit('patch result differs from the expected file; nothing written')
    path.write_bytes(exe)
    print(f'{path}: patched (hooks get the current thread)')


if __name__ == '__main__':
    main()
