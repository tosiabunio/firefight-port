#!/usr/bin/env python3
"""Generates source/game/stale_gen.cpp: what each game object left in its FastAlloc slot at the
offset where the original read ACannon::global_time uninitialised (see source/game/stale.h).

The original was a 32-bit MSVC build. Clang lays out records exactly as MSVC does for the target
i386-pc-windows-msvc, so its record layouts give the offsets the original used. For every class
allocated from FastAlloc (the FASTALLOC_FITS list in source/game/gobj.cpp) this script finds what
covers each byte of the word at ACannon::global_time's offset and writes C++ that reads the same
bytes from the port's object:

  - an integer or char member: its bytes (both are little endian);
  - a pointer, reference, vftable or vbtable pointer: a non-null address (0x00400000, the image
    base) or 0;
  - padding or nothing (the object ends before the word): the bytes are left as they were.

  stale_memory.py [--clang CLANG] [--sysroot DIR]

Needs a clang that can target i386-pc-windows-msvc (Apple clang and LLVM clang can). It parses
the game headers with the host's C headers, which it doesn't need to compile: only the layouts
are used.
"""
import argparse
import pathlib
import re
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
SRC = ROOT / 'source'
OUT = SRC / 'game' / 'stale_gen.cpp'
ADDRESS = 0x00400000
MODULES = ['1ba', '1cw', '1ee', '1io', '1lg', '1mm', '1rg', '1sp', '1ss']
SCALARS = {'int': 4, 'unsigned int': 4, 'unsigned': 4, 'long': 4, 'unsigned long': 4,
           'short': 2, 'unsigned short': 2, 'char': 1, 'unsigned char': 1, 'signed char': 1,
           'bool': 1, '_Bool': 1, 'float': 4, 'DWORD': 4, 'BOOL': 4, 'WORD': 2, 'BYTE': 1}


def fastalloc_classes():
    text = (SRC / 'game' / 'gobj.cpp').read_text()
    return [c for c in re.findall(r'FASTALLOC_FITS\((\w+)\)', text) if c != 'type']


def dump_layouts(classes, clang, sysroot):
    with tempfile.TemporaryDirectory() as tmp:
        tmp = pathlib.Path(tmp)
        stubs = tmp / 'stubs'
        stubs.mkdir()
        for h in ['conio.h', 'crtdbg.h', 'direct.h', 'io.h', 'malloc.h', 'process.h', 'share.h']:
            (stubs / h).write_text('')
        probe = tmp / 'probe.cpp'
        probe.write_text('#include "headers.h"\nint probe_sizes[] = {\n' +
                         ''.join(f'  sizeof({c}),\n' for c in classes) + '0};\n')
        inc = [f'-I{SRC / "game"}', f'-I{SRC / "regdata"}', f'-I{SRC / "engine" / "common"}', f'-I{SRC}']
        inc += [f'-I{SRC / "engine" / m}' for m in MODULES]
        cmd = [clang, '-target', 'i386-pc-windows-msvc', '-fsyntax-only', '-std=c++17',
               '-fms-extensions', '-fms-compatibility', '-ferror-limit=0',
               '-DEXCLUDE_LIBS', '-DUNPROTECT', '-D_MAX_PATH=260', '-D_MAX_DRIVE=3',
               '-D_MAX_DIR=256', '-D_MAX_FNAME=256', '-D_MAX_EXT=256',
               '-isystem', str(stubs), '-isystem', str(pathlib.Path(sysroot) / 'usr' / 'include'),
               *inc, '-Xclang', '-fdump-record-layouts', str(probe)]
        out = subprocess.run(cmd, capture_output=True, text=True).stdout
    layouts = {}
    for block in out.split('*** Dumping AST Record Layout'):
        lines = [l for l in block.split('\n') if '|' in l]
        if lines:
            m = re.match(r'\s*0 \| (?:class|struct) (\S+)$', lines[0].rstrip())
            if m and m.group(1) not in layouts:
                layouts[m.group(1)] = lines
    return layouts


def parse(lines):
    """[(offset, indent, description)] and the size of a dumped record."""
    entries = []
    for l in lines[1:]:
        m = re.match(r'\s*(\d+)(:\d+-\d+)? \|(\s+)(.*)', l)
        if m:
            entries.append((int(m.group(1)), len(m.group(3)), m.group(4).strip(), bool(m.group(2))))
    size = int(re.search(r'sizeof=(\d+)', ' '.join(lines)).group(1))
    return entries, size


def element_layout(layouts, owner, name):
    for key in (f'{owner}::{name}', name):
        if key in layouts:
            return parse(layouts[key])
    sys.exit(f'no layout for {name} (in {owner})')


def leaves(layouts, cls):
    """Every scalar, pointer or hidden pointer in cls: (offset, size, kind, owner, access path)."""
    entries, _ = parse(layouts[cls])
    out = []
    stack = [(-1, cls, '')]  # (indent, class that owns the members, path prefix)
    for k, (off, ind, desc, bitfield) in enumerate(entries):
        nested = k + 1 < len(entries) and entries[k + 1][1] > ind
        while stack[-1][0] >= ind:
            stack.pop()
        _, owner, prefix = stack[-1]
        m = re.match(r'(?:class|struct) (\S+) \((?:primary )?(?:virtual )?base\)', desc)
        if m:
            stack.append((ind, m.group(1), ''))
            continue
        if re.match(r'\((\S+ )?(vftable|vbtable) pointer\)', desc):
            out.append((off, 4, 'address', owner, None))
            continue
        if desc.startswith('(vtordisp'):
            out.append((off, 4, 'zero', owner, None))
            continue
        if bitfield:
            out.append((off, 4, 'bitfield', owner, prefix + desc))
            continue
        m = re.match(r'(.+?) (\w+)$', desc)
        if not m:
            continue
        ftype, fname = m.group(1).strip(), prefix + m.group(2)
        arr = re.match(r'(?:struct |class )?(.+?)\[(\d+)\]$', ftype)
        if ftype.endswith('*') or ftype.endswith('&'):
            out.append((off, 4, 'pointer', owner, fname))
        elif arr:
            etype, count = arr.group(1).strip(), int(arr.group(2))
            if etype.endswith('*'):
                for i in range(count):
                    out.append((off + i * 4, 4, 'pointer', owner, f'{fname}[{i}]'))
            elif etype in SCALARS:
                esize = SCALARS[etype]
                for i in range(count):
                    out.append((off + i * esize, esize, 'scalar', owner, f'{fname}[{i}]'))
            else:
                eentries, esize = element_layout(layouts, owner, etype.split('::')[-1])
                for i in range(count):
                    for eoff, _, edesc, ebit in eentries:
                        em = re.match(r'(.+?) (\w+)$', edesc)
                        if not em or re.match(r'(class|struct) \S+ \(', edesc):
                            continue
                        et = em.group(1).strip()
                        if ebit or et not in SCALARS:
                            sys.exit(f'{cls}: unsupported element field {edesc}')
                        out.append((off + i * esize + eoff, SCALARS[et], 'scalar', owner,
                                    f'{fname}[{i}].{em.group(2)}'))
        elif nested:
            stack.append((ind, owner, fname + '.'))  # a struct member: its fields follow
        else:
            out.append((off, SCALARS.get(ftype, 4), 'scalar', owner, fname))  # else an enum
    return out


def word_sources(layouts, cls, offset):
    """For bytes offset..offset+3: (dest byte, kind, owner, path, source byte, count)."""
    _, size = parse(layouts[cls])
    pieces = []
    for leaf_off, leaf_size, kind, owner, path in leaves(layouts, cls):
        if kind == 'struct':
            continue
        lo, hi = max(leaf_off, offset), min(leaf_off + leaf_size, offset + 4)
        if lo >= hi:
            continue
        if kind == 'bitfield':
            sys.exit(f'{cls}: a bit field covers the word ({path})')
        pieces.append((lo - offset, kind, owner, path, lo - leaf_off, hi - lo))
    return sorted(pieces), size


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--clang', default='clang++')
    ap.add_argument('--sysroot', default=None, help='C headers root (default: xcrun --show-sdk-path, else /)')
    a = ap.parse_args()
    sysroot = a.sysroot
    if not sysroot:
        try:
            sysroot = subprocess.run(['xcrun', '--show-sdk-path'], capture_output=True, text=True,
                                     check=True).stdout.strip()
        except (OSError, subprocess.CalledProcessError):
            sysroot = '/'

    classes = fastalloc_classes()
    layouts = dump_layouts(classes + ['ACannon'], a.clang, sysroot)
    missing = [c for c in classes if c not in layouts]
    if missing:
        sys.exit(f'clang dumped no layout for {missing}')
    target = [l for l in leaves(layouts, 'ACannon') if l[4] == 'global_time']
    if len(target) != 1:
        sys.exit('ACannon::global_time not found')
    offset = target[0][0]

    body, owners = [], set()
    for cls in classes:
        pieces, size = word_sources(layouts, cls, offset)
        if not pieces:
            continue
        stmts, notes = [], []
        for dest, kind, owner, path, src, n in pieces:
            if kind == 'scalar':
                owners.add(owner)
                stmts.append(f'copy(w,{dest},&dynamic_cast<{owner}*>(o)->{path},{src},{n});')
                notes.append(f'{owner}::{path}')
            elif kind == 'pointer':
                owners.add(owner)
                stmts.append(f'address(w,{dest},dynamic_cast<{owner}*>(o)->{path}!=NULL,{src},{n});')
                notes.append(f'{owner}::{path} (pointer)')
            elif kind == 'address':
                stmts.append(f'address(w,{dest},1,{src},{n});')
                notes.append(f'{owner} vftable/vbtable pointer')
            else:
                stmts.append(f'address(w,{dest},0,{src},{n});')
                notes.append(f'{owner} vtordisp')
        covered = sum(p[5] for p in pieces)
        note = ', '.join(notes) + ('' if covered == 4 else '; the other bytes are padding')
        keyword = 'else if' if body else 'if'
        body.append(f'  {keyword} (type==typeid({cls}))  // {note}\n  {{\n    ' +
                    '\n    '.join(stmts) + '\n  }')

    last = offset + 3
    text = f'''// Generated by tools/layout/stale_memory.py from clang's 32-bit MSVC record layouts. Do not edit.
//
// For each game object class that reaches offset {offset} (ACannon::global_time in the original's
// layout), the bytes it holds at offsets {offset}-{last}, read from the port's object. Classes
// that end before that offset leave the word alone. See stale.h.

#include <typeinfo>

#include "headers.h"

const int Stale_memory::offset={offset};

void Stale_memory::copy_word (Object *o, unsigned char *w)
{{
  const std::type_info &type=typeid(*o);
{chr(10).join(body)}
}}
'''
    OUT.write_text(text)
    print(f'{OUT}: offset {offset}, {len(body)} classes; members read from: {", ".join(sorted(owners))}')


if __name__ == '__main__':
    main()
