#!/usr/bin/env python3
"""Inspect the original Fire Fight archive (not part of this repository).

See docs/original-archive.md for what the archive holds and what these checks found.

  ffarchive.py bin2iso BIN ISO          data track of the retail CD image (MODE1/2352) to an ISO
  ffarchive.py unpack VOL [OUTDIR]      list a .vol volume, or extract it into OUTDIR
  ffarchive.py cache FILE...            header and phases of built sprite caches (.sph/.spl/.spc)
  ffarchive.py compare-cd CDDIR         every file in CDDIR/*.vol against data/ or FF/WORK.RTL
  ffarchive.py provenance [--write F]   every file in data/ and music/ against its original; F gets
                                        their SHA-256 list (tests/golden/data_files.sha256)
  ffarchive.py sprites DIR              a sprite_dump=1 run of the port (DIR: its preferences) vs.
                                        the shipped caches: phase bounds, pixel data, palette tables

The archive root is --archive, else $FF_ARCHIVE, else this repository's parent directory if it
holds FF/WORK.RTL, else ../FireFight next to the repository. Its file names are mixed-case 8.3
names, so lookups ignore case.
"""
import argparse
import hashlib
import os
import pathlib
import re
import struct
import sys
import zlib

ROOT = pathlib.Path(__file__).resolve().parents[2]
DATA = ROOT / 'data'

VOLUME_ID = 0x43024202           # compressed volume (1io.h); uncompressed ones were unsupported
FILE_INFO_SIZE = 572             # File_Info on 32-bit: 260+260+32 + size,date,offset + attrib + next
CACHE_ID = b'CWE sprite\0'       # 1sp_ldsa.cpp
CACHE_FILE_LEVEL = 12            # spr_file_level in 1sp_hdrs.h


def archive_root(arg):
    for cand in (arg, os.environ.get('FF_ARCHIVE'), ROOT.parent, ROOT.parent / 'FireFight'):
        if cand and (pathlib.Path(cand) / 'FF' / 'WORK.RTL').is_dir():
            return pathlib.Path(cand)
    sys.exit('archive not found: pass --archive or set FF_ARCHIVE')


def find_ci(base, rel):
    """Resolve a DOS path (any case, '\\' or '/') below base; None if missing."""
    p = pathlib.Path(base)
    for part in re.split(r'[\\/]', rel):
        if not part:
            continue
        try:
            names = {n.lower(): n for n in os.listdir(p)}
        except (FileNotFoundError, NotADirectoryError):
            return None
        if part.lower() not in names:
            return None
        p = p / names[part.lower()]
    return p


# ----- volumes ---------------------------------------------------------------------------------

def xio_decompress(src):
    """Xio::decompress (1io_cmpr.cpp, an LZRW1-A variant)."""
    if src[0] == 1:                                  # FLAG_COPY: stored
        return bytes(src[4:])
    out = bytearray()
    i, n, control = 4, len(src), 1
    while i != n:
        if control == 1:
            control = 0x10000 | src[i] | (src[i + 1] << 8)
            i += 2
        for _ in range(16 if i <= n - 32 else 1):
            if control & 1:
                length = src[i]
                start = len(out) - (((length & 0xF0) << 4) | src[i + 1])
                i += 2
                for k in range((length & 0xF) + 3):
                    out.append(out[start + k])
            else:
                out.append(src[i])
                i += 1
            control >>= 1
            if i == n:
                break
    return bytes(out)


def read_volume(path):
    """Yield (logical name, DOS path, flags, data) for each file in a .vol."""
    raw = pathlib.Path(path).read_bytes()
    _total, file_size, files, dir_offset, vid = struct.unpack_from('<5I', raw, len(raw) - 20)
    if vid != VOLUME_ID:
        raise ValueError(f'{path}: not a compressed volume ({vid:#x})')
    mem = bytearray()
    p = 0
    while p < file_size:                             # blocks of 0x4000 bytes, each compressed
        (block,) = struct.unpack_from('<H', raw, p)
        mem += xio_decompress(raw[p + 2:p + 2 + block])
        p += 2 + block
    text = lambda b: b.split(b'\0')[0].decode('cp852')
    for k in range(files):
        e = dir_offset + k * FILE_INFO_SIZE
        size, _date, offset = struct.unpack_from('<3I', mem, e + 552)
        yield text(mem[e:e + 260]), text(mem[e + 260:e + 520]), text(mem[e + 520:e + 552]), \
            bytes(mem[offset:offset + size])


# ----- sprite caches ---------------------------------------------------------------------------

def read_cache(path):
    """A built sprite cache: header fields and per-phase (ox, oy, sx, sy)."""
    b = pathlib.Path(path).read_bytes()
    if not b.startswith(CACHE_ID) or struct.unpack_from('<i', b, 11)[0] != 36:
        raise ValueError(f'{path}: not a sprite cache')
    level, _date, mirrors, onecolor, scale, _o0, _o1, size, phases = struct.unpack_from('<9i', b, 15)
    p = 15 + 36 + size
    ph = [struct.unpack_from('<4h', b, p + 16 * i + 8) for i in range(phases)]
    return {'level': level, 'scale': scale, 'mirrors': mirrors, 'onecolor': onecolor,
            'rle_bytes': size, 'phases': ph, 'rle': b[15 + 36:15 + 36 + size]}


def read_palette_tables(path):
    """A built palette cache (.spp): the 10 palettes, the 64^3 closest-colour and 256x256 tables."""
    b = pathlib.Path(path).read_bytes()
    return {'palettes': b[8:8 + 10240], 'closest': b[8 + 10240:8 + 10240 + 262144],
            'tsp': b[8 + 10240 + 262144:]}


def parse_manifest(path):
    """[(area path, {key: value})] for every area in a .dir manifest."""
    out, stack = [], []
    for raw in pathlib.Path(path).read_bytes().decode('cp852').splitlines():
        line = raw.split(';')[0].strip()
        if not line:
            continue
        m = re.match(r'area\s+(\S+)', line, re.I)
        if m:
            stack.append((m.group(1), {}))
        elif re.match(r'endarea', line, re.I):
            name, d = stack.pop()
            out.append(('/'.join([s[0] for s in stack] + [name]), d))
        elif stack and (m := re.match(r'(\S+)\s*=\s*(.*)', line)):
            stack[-1][1][m.group(1).lower()] = m.group(2).strip()
    return out


def union_bounds(h, l, c):
    """Phase bounds as Sprite::load computes them (1sp_lmai.cpp): hires /2, lores, collis *2."""
    tdiv = lambda a, b: int(a / b)                   # C division truncates toward zero
    out = []
    for i in range(max(len(h), len(l), len(c))):
        parts = []
        for ph, f in ((h, lambda v: tdiv(v, 2)), (l, lambda v: v), (c, lambda v: v * 2)):
            if i < len(ph):
                ox, oy, sx, sy = ph[i]
                parts.append((f(-ox), f(-ox + sx), f(-oy), f(-oy + sy)))
            else:
                parts.append((0, 0, 0, 0))           # phase memset to 0
        out.append((min(p[0] for p in parts), max(p[1] for p in parts),
                    min(p[2] for p in parts), max(p[3] for p in parts)))
    return out


# ----- commands --------------------------------------------------------------------------------

def cmd_bin2iso(a):
    cue = pathlib.Path(a.bin).with_suffix('.cue')
    sectors = None
    if cue.exists():                                 # data track ends where track 2 starts
        m = re.search(r'TRACK 02.*?INDEX 0[01] (\d+):(\d+):(\d+)', cue.read_text(), re.S)
        if m:
            mm, ss, ff = map(int, m.groups())
            sectors = (mm * 60 + ss) * 75 + ff
    with open(a.bin, 'rb') as src, open(a.iso, 'wb') as dst:
        n = 0
        while sectors is None or n < sectors:
            s = src.read(2352)
            if len(s) < 2352:
                break
            dst.write(s[16:16 + 2048])
            n += 1
    print(f'{n} sectors written; list or extract the ISO with any ISO 9660 tool (7-Zip)')


def cmd_unpack(a):
    for name, dos, flags, data in read_volume(a.vol):
        print(f'{len(data):9d}  {name:24s} {dos} {flags}')
        if a.outdir:
            dst = pathlib.Path(a.outdir) / dos.replace('\\', '/').lower()
            dst.parent.mkdir(parents=True, exist_ok=True)
            dst.write_bytes(data)


def cmd_cache(a):
    for f in a.files:
        c = read_cache(f)
        print(f"{f}: level {c['level']}, scale {c['scale']}, mirrors {c['mirrors']}, "
              f"onecolor {c['onecolor']}, {c['rle_bytes']} RLE bytes, {len(c['phases'])} phases")
        for i, ph in enumerate(c['phases']):
            print(f'  {i:3d}  ox {ph[0]:4d}  oy {ph[1]:4d}  sx {ph[2]:4d}  sy {ph[3]:4d}')


def cmd_compare_cd(a):
    rtl = archive_root(a.archive) / 'FF' / 'WORK.RTL'
    counts, bad = {}, []
    for vol in sorted(pathlib.Path(a.cddir).glob('*.[Vv][Oo][Ll]')):
        for _name, dos, _flags, data in read_volume(vol):
            rel = dos.replace('\\', '/').lower()
            where, path = ('data/', DATA / rel) if (DATA / rel).exists() else ('WORK.RTL', find_ci(rtl, rel))
            if path is None:
                key = 'missing'
            else:
                key = f"{where} {'same' if path.read_bytes() == data else 'DIFFERENT'}"
            counts[key] = counts.get(key, 0) + 1
            if not key.endswith('same'):
                bad.append(f'{key}: {vol.name} {dos}')
    for k, v in sorted(counts.items()):
        print(f'{v:6d}  {k}')
    print('\n'.join(bad))
    return 1 if bad else 0


def parse_bounds_line(line):
    """'<target> <phases> l,r,u,d[*n] ...' (sprite_bounds.txt) -> (target, [(l, r, u, d)])."""
    target, phases, *runs = line.split()
    out = []
    for run in runs:
        b, _, n = run.partition('*')
        out += [tuple(int(v) for v in b.split(','))] * int(n or 1)
    if len(out) != int(phases):
        raise ValueError(f'bad line: {line}')
    return target, out


def cmd_provenance(a):
    root = archive_root(a.archive)
    origins = {'data': root / 'FF' / 'WORK.RTL', 'music': root / 'CDAudio'}
    lines, bad = [], 0
    for top, origin in origins.items():
        for path in sorted((ROOT / top).rglob('*')):
            if not path.is_file():
                continue
            rel = path.relative_to(ROOT).as_posix()
            data = path.read_bytes()
            original = find_ci(origin, path.relative_to(ROOT / top).as_posix())
            if original is None or original.read_bytes() != data:
                print(f"{rel}: {'no original' if original is None else 'differs from'} in {origin}")
                bad += 1
            lines.append(f'{hashlib.sha256(data).hexdigest()}  {rel}')
    if a.write and not bad:
        pathlib.Path(a.write).write_text('\n'.join(sorted(lines, key=lambda l: l[66:])) + '\n')
    print(f'{len(lines)} files checked, {bad} differ from the archive' +
          (f'; list written to {a.write}' if a.write and not bad else ''))
    return 1 if bad else 0


def shipped_sprites(rtl):
    """first target -> [(bounds, data line)] for every sprite area in the original manifests, and
    palette target -> data line, as sprite_bounds.txt and sprite_data.txt describe them."""
    caches = {}
    def cache(v):
        if not v:
            return None
        path = find_ci(rtl, v.split()[0])
        if path not in caches:
            caches[path] = read_cache(path)
        return caches[path]
    crc = lambda b: f'{len(b)} {zlib.crc32(b):08x}'
    sprites, palettes = {}, {}
    for man in sorted(rtl.glob('*.DIR')):
        for area, d in parse_manifest(man):
            if 'target' in d:
                key = d['target'].split()[0].replace('\\', '/').lower()
                tables = read_palette_tables(find_ci(rtl, key))
                palettes[key] = f'{key} ' + ' '.join(f'{k} {crc(v)}' for k, v in tables.items())
                continue
            key = next((d[k] for k in ('hires', 'collis', 'lores') if k in d), None)
            if not key:
                continue
            key = key.split()[0].replace('\\', '/').lower()
            h, l, c = cache(d.get('hires')), cache(d.get('lores')), cache(d.get('collis'))
            ph = lambda x: x['phases'] if x else []
            bounds = union_bounds(ph(h), ph(l), ph(c))
            data = key + (f" hires {crc(h['rle'])}" if h else '') + (f" collis {crc(c['rle'])}" if c else '')
            sprites.setdefault(key, []).append((bounds, data))
    return sprites, palettes


def cmd_sprites(a):
    rtl = archive_root(a.archive) / 'FF' / 'WORK.RTL'
    sprites, palettes = shipped_sprites(rtl)
    dump = pathlib.Path(a.dir)
    bad = 0
    bounds_lines = sorted(set(dump.joinpath('sprite_bounds.txt').read_text().splitlines()) - {''})
    for line in bounds_lines:
        target, bounds = parse_bounds_line(line)
        expected = [b for b, _ in sprites.get(target, [])]
        if not expected:
            print(f'{target}: not in the original manifests')
            bad += 1
        elif bounds not in expected:
            exp = expected[0]
            i = next((i for i, (x, y) in enumerate(zip(bounds, exp)) if x != y), min(len(bounds), len(exp)))
            print(f'{target}: phase {i} bounds {bounds[i] if i < len(bounds) else None}, '
                  f'shipped {exp[i] if i < len(exp) else None} ({len(bounds)} vs {len(exp)} phases)')
            bad += 1
    data_lines = sorted(set(dump.joinpath('sprite_data.txt').read_text().splitlines()) - {''})
    for line in data_lines:
        target = line.split()[0]
        expected = [palettes[target]] if target in palettes else [d for _, d in sprites.get(target, [])]
        if line not in expected:
            print(f'{line}\n{"shipped:":>{len(target)}}{expected[0][len(target):] if expected else " nothing"}')
            bad += 1
    print(f'{len(bounds_lines)} sprite bounds and {len(data_lines)} pixel data/palette lines checked, '
          f'{bad} differ from the shipped caches')
    return 1 if bad else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--archive', help='archive root (the directory holding FF/, LIB/, BIN/)')
    sub = ap.add_subparsers(dest='cmd', required=True)
    p = sub.add_parser('bin2iso'); p.add_argument('bin'); p.add_argument('iso'); p.set_defaults(f=cmd_bin2iso)
    p = sub.add_parser('unpack'); p.add_argument('vol'); p.add_argument('outdir', nargs='?'); p.set_defaults(f=cmd_unpack)
    p = sub.add_parser('cache'); p.add_argument('files', nargs='+'); p.set_defaults(f=cmd_cache)
    p = sub.add_parser('compare-cd'); p.add_argument('cddir'); p.set_defaults(f=cmd_compare_cd)
    p = sub.add_parser('sprites'); p.add_argument('dir'); p.set_defaults(f=cmd_sprites)
    p = sub.add_parser('provenance'); p.add_argument('--write', metavar='FILE'); p.set_defaults(f=cmd_provenance)
    a = ap.parse_args()
    sys.exit(a.f(a) or 0)


if __name__ == '__main__':
    main()
