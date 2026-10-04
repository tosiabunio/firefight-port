#!/usr/bin/env python3
"""Test vectors for the MSVC 4 CRT functions the simulation depends on, from the original exe.

Runs qsort and rand of the 1.1 FIREFGHT.EXE (Release build, FF/C/GAME/RELEASE) in an x86
emulator and writes what they return to tests/golden/crt_qsort.txt and tests/golden/crt_rand.txt.
They define the clones phase 5 needs (docs/porting-plan.md) on every platform. The script also
checks the qsort transcription in docs/original-archive.md (qsort_reference below) against
every vector.

  pip install pefile unicorn
  crt_vectors.py [--archive ROOT]

The archive is found as ffarchive.py finds it.
"""
import argparse
import pathlib
import random
import struct
import sys

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from ffarchive import ROOT, archive_root  # noqa: E402

QSORT, RAND, HOLDRAND = 0x48a340, 0x488d50, 0x4b8e40
SIGNATURES = {QSORT: '8b442408 81ecf8000000', RAND: 'a1408e4b00 8d0c80'}

CODE, KEYS, BUF, STOP = 0x00100000, 0x00101000, 0x00102000, 0x00100f00
STACK_TOP = 0x002ff000
# int comp(const void *a, const void *b) { return keys[*(uchar*)a] - keys[*(uchar*)b]; }
COMPARATOR = bytes.fromhex(
    '8b442404'    # mov eax, [esp+4]
    '8b4c2408'    # mov ecx, [esp+8]
    '0fb600'      # movzx eax, byte [eax]
    '0fb609'      # movzx ecx, byte [ecx]
    '0fb68000101000'  # movzx eax, byte [eax+KEYS]
    '0fb68900101000'  # movzx ecx, byte [ecx+KEYS]
    '2bc1'        # sub eax, ecx
    'c3')         # ret


class Exe:
    def __init__(self, path):
        pe = pefile.PE(str(path))
        base = pe.OPTIONAL_HEADER.ImageBase
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        size = (pe.OPTIONAL_HEADER.SizeOfImage + 0xfff) & ~0xfff
        self.uc.mem_map(base, size)
        self.uc.mem_write(base, pe.header)
        for s in pe.sections:
            self.uc.mem_write(base + s.VirtualAddress, s.get_data())
        for addr, sig in SIGNATURES.items():
            if self.uc.mem_read(addr, len(bytes.fromhex(sig))) != bytes.fromhex(sig):
                sys.exit(f'{path}: unexpected code at {addr:#x}; not the 1.1 Release FIREFGHT.EXE')
        self.uc.mem_map(CODE, 0x00200000 - CODE)
        self.uc.mem_map(0x00200000, STACK_TOP + 0x1000 - 0x00200000)
        self.uc.mem_write(CODE, COMPARATOR)

    def call(self, addr, *args):
        sp = STACK_TOP - 4 * (len(args) + 1)
        self.uc.mem_write(sp, struct.pack(f'<{len(args) + 1}I', STOP, *args))
        self.uc.reg_write(UC_X86_REG_ESP, sp)
        self.uc.emu_start(addr, STOP)
        return self.uc.reg_read(UC_X86_REG_EAX)

    def qsort(self, keys, width):
        n = len(keys)
        self.uc.mem_write(KEYS, bytes(keys) + bytes(256 - n))
        self.uc.mem_write(BUF, b''.join(bytes([i]) + bytes(width - 1) for i in range(n)))
        self.call(QSORT, BUF, n, width, CODE)
        out = self.uc.mem_read(BUF, n * width)
        return [out[i * width] for i in range(n)]

    def rand(self, seed, count):
        self.uc.mem_write(HOLDRAND, struct.pack('<I', seed))  # srand(seed)
        return [self.call(RAND) for _ in range(count)]


def qsort_reference(keys, width):
    """The algorithm as docs/original-archive.md gives it, on (id, padding) elements."""
    buf = bytearray(b''.join(bytes([i]) + bytes(width - 1) for i in range(len(keys))))
    comp = lambda a, b: keys[buf[a]] - keys[buf[b]]
    def swap(a, b):
        if a != b:
            buf[a:a + width], buf[b:b + width] = buf[b:b + width], buf[a:a + width]
    def shortsort(lo, hi):
        while hi > lo:
            mx = lo
            for p in range(lo + width, hi + 1, width):
                if comp(p, mx) > 0:
                    mx = p
            swap(mx, hi)
            hi -= width
    if len(keys) >= 2:
        stack = []
        lo, hi = 0, width * (len(keys) - 1)
        while True:
            size = (hi - lo) // width + 1
            if size <= 8:
                shortsort(lo, hi)
            else:
                swap(lo + (size // 2) * width, lo)
                loguy, higuy = lo, hi + width
                while True:
                    loguy += width
                    while loguy <= hi and comp(loguy, lo) <= 0:
                        loguy += width
                    higuy -= width
                    while higuy > lo and comp(higuy, lo) >= 0:
                        higuy -= width
                    if higuy < loguy:
                        break
                    swap(loguy, higuy)
                swap(lo, higuy)
                if higuy - 1 - lo >= hi - loguy:
                    if lo + width < higuy:
                        stack.append((lo, higuy - width))
                    if loguy < hi:
                        lo = loguy
                        continue
                else:
                    if loguy < hi:
                        stack.append((loguy, hi))
                    if lo + width < higuy:
                        hi = higuy - width
                        continue
            if not stack:
                break
            lo, hi = stack.pop()
    return [buf[i * width] for i in range(len(keys))]


def vectors():
    rng = random.Random(1996)
    patterns = {
        'zero': lambda n: [0] * n,
        'alternate': lambda n: [i & 1 for i in range(n)],
        'two': lambda n: [rng.randrange(2) for _ in range(n)],
        'four': lambda n: [rng.randrange(4) for _ in range(n)],
        'any': lambda n: [rng.randrange(256) for _ in range(n)],
    }
    for width in (1, 2, 4):
        for n in list(range(21)) + [24, 31, 32, 33, 47, 64, 100, 128, 200, 255]:
            for name, make in patterns.items():
                if n > 20 and name == 'alternate':
                    continue
                yield width, make(n)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--archive')
    a = ap.parse_args()
    exe = Exe(archive_root(a.archive) / 'FF' / 'C' / 'GAME' / 'RELEASE' / 'FIREFGHT.EXE')

    lines, mismatches = [], 0
    for width, keys in vectors():
        order = exe.qsort(keys, width)
        if sorted(keys[i] for i in order) != sorted(keys) or \
                any(keys[order[i]] > keys[order[i + 1]] for i in range(len(order) - 1)):
            sys.exit(f'emulated qsort did not sort {keys}')
        if qsort_reference(keys, width) != order:
            mismatches += 1
            print(f'reference differs: width {width}, keys {bytes(keys).hex()}')
        lines.append(f'{width} {bytes(keys).hex() or "-"} {bytes(order).hex() or "-"}')
    out = ROOT / 'tests' / 'golden' / 'crt_qsort.txt'
    out.write_text(
        '# MSVC 4 CRT qsort as linked into the 1.1 FIREFGHT.EXE (Release, 0x48a340), run in an emulator\n'
        '# by tools/archive/crt_vectors.py. Each line: <width> <keys> <order>. Element i (0-based) is\n'
        '# <width> bytes, the first one i, the rest 0. The comparator returns keys[a]-keys[b], read from\n'
        '# the first byte of each element. <order> lists the first bytes after the sort. Hex bytes; - if\n'
        '# empty.\n' + '\n'.join(lines) + '\n')
    print(f'{len(lines)} qsort vectors written to {out}; the documented algorithm differs on {mismatches}')

    values = exe.rand(0, 1024)
    out = ROOT / 'tests' / 'golden' / 'crt_rand.txt'
    out.write_text(
        '# MSVC 4 CRT rand as linked into the 1.1 FIREFGHT.EXE (Release, 0x488d50), run in an emulator\n'
        '# by tools/archive/crt_vectors.py: srand(0), then 1024 calls, which is how Rand::init fills its\n'
        '# table (gobj.cpp). 16 values per line.\n' +
        '\n'.join(' '.join(str(v) for v in values[i:i + 16]) for i in range(0, len(values), 16)) + '\n')
    lcg, state = [], 0
    for _ in range(1024):
        state = (state * 214013 + 2531011) & 0xffffffff
        lcg.append((state >> 16) & 0x7fff)
    print(f'1024 rand values written to {out}; the documented LCG ' +
          ('matches' if lcg == values else 'DIFFERS'))
    return 1 if mismatches or lcg != values else 0


if __name__ == '__main__':
    sys.exit(main())
