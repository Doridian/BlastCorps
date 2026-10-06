#!/usr/bin/env python3
"""Compare two builds of the port run for run (docs/PORT.md, "Comparing builds").

  build_cmp.py rdram DIR_A DIR_B N...
      rdram_N.bin (PORT_DUMP=N,...) of two runs, word by word, leaving out
      words that are addresses in the port's image or on the fibers' host
      stacks in both (they differ between builds by construction); prints
      the first differing words of each dump.

  build_cmp.py rdram --moved MOVED.txt EXE DIR_A DIR_B N...
      the same between a build with the N64's layout and an LP64 one (EXE,
      whose gen/port_rdram_moved.txt is MOVED.txt): the N64 places of the
      variables the LP64 build moved, and pointers to them, are left out.

  build_cmp.py rdram --native DIR_BE DIR_NATIVE N...
      the same between the big-endian build and a native-endian one: a
      word of the native dump matches if it is the big-endian word in one
      of the orders its fields can have (a u32 or two u16s or bytes, in any
      mix: the word, its bytes reversed, either half's bytes reversed);
      64-bit scalars are two words, high first, in both.  Where the native
      run is a PORT_ACCESS_PROFILE build's, its dumps come with the width
      every byte was last written or converted at (rdram_N.widths), and the
      comparison is byte by byte at those widths, which is exact.

  build_cmp.py trace A.bin B.bin EXE...
      two PORT_TRACE=FILE call traces (a PORT_TRACE_CALLS build): the first
      call where they part, with the calls around it, named from EXE's
      symbols.

  build_cmp.py itrace A.bin B.bin
      two PORT_ITRACE=FILE instruction traces (a PORT_TRACE_ASM build of
      each): the first instruction where a register holds another value in
      the two, other than the same bytes in another order (a word copy of
      data that isn't a word, which native-endian memory holds in its own
      order), with the registers that differed so before it.

Runs to be compared need the same inputs and a timing that doesn't depend
on the code: --deterministic and the same --save (or none).
"""

import os
import subprocess
import sys

import numpy as np


def image(v):
    return (v >= 0x80400000) & (v < 0x81000000)


def stack(v):
    return (v >= 0x90000000) & (v < 0x91000000)


# rdram --moved: the variables the LP64 build put elsewhere (gen_ld.py's
# port_rdram_moved.txt, next to the build's gen/): (N64 address, N64 size,
# the LP64 build's address, its size)
MOVED = []


def load_moved(path, exe):
    host = {}
    for line in subprocess.run(["nm", exe], capture_output=True, text=True, check=True).stdout.splitlines():
        p = line.split()
        if len(p) == 3:
            host[p[2]] = int(p[0], 16)
    for line in open(path):
        name, addr, hsize, nsize = line.split()
        if name in host:
            MOVED.append((int(addr, 16), int(nsize, 16), host[name], int(hsize, 16)))


def moved_ok(x, y):
    """words that differ only because of the moved variables: their N64
    places (empty in the LP64 build), and pointers to them"""
    ok = np.zeros(len(x), bool)
    for lo, size, hlo, hsize in MOVED:
        w0, w1 = (lo - 0x80000000) // 4, (lo - 0x80000000 + size + 3) // 4
        ok[w0:w1] = True
        ok |= (x >= lo) & (x < lo + max(size, 1)) & (y >= hlo) & (y < hlo + max(hsize, 1))
    return ok


def rdram(a, b, ns, order=">u4"):
    for n in ns:
        x = np.fromfile(f"{a}/rdram_{n}.bin", order)
        y = np.fromfile(f"{b}/rdram_{n}.bin", order)
        bad = x != y
        if MOVED:
            bad &= ~moved_ok(x.astype(np.int64), y.astype(np.int64))
        d = np.nonzero(bad)[0]
        real = [w for w in d if not (image(x[w]) and image(y[w])) and not (stack(x[w]) and stack(y[w]))]
        print(f"rdram_{n}: {len(real)} words differ",
              " ".join(f"{0x80000000 + 4 * w:08X}:{x[w]:08X}/{y[w]:08X}" for w in real[:8]))


def rdram_typed(a, b, n):
    """byte by byte, the big-endian dump put into host order at the widths
    the native build's profiler recorded (rdram_N.widths); bytes of unknown
    width fall back to the any-order word check.  A byte left of a unit
    that was partly overwritten later (a u16 stored into a word that held a
    pointer: gzip's huft union, a record built over a stale one) holds a
    different byte of the old value in each order, so it isn't compared:
    it can only be read at another width than it was written at, which the
    profiler's table reports."""
    x = np.fromfile(f"{a}/rdram_{n}.bin", "u1")
    y = np.fromfile(f"{b}/rdram_{n}.bin", "u1")
    w = np.fromfile(f"{b}/rdram_{n}.widths", "u1")
    width, start = w & 0xF, (w & 0x10) != 0
    pos = w >> 5
    exp = x.copy()
    known = width == 1
    cut = np.zeros(len(x), bool)
    for u in (2, 4, 8):
        idx = np.nonzero(start & (width == u))[0]
        idx = idx[idx + u <= len(x)]
        whole = np.ones(len(idx), bool)
        for k in range(1, u):
            whole &= (width[idx + k] == u) & (pos[idx + k] == k)
        # a u64 is two host-order words, the high one first
        for k in range(u):
            src = k // 4 * 4 + 3 - k % 4 if u == 8 else u - 1 - k
            exp[idx[whole] + k] = x[idx[whole] + src]
            known[idx[whole] + k] = True
    cut = (width > 1) & ~known
    bad = np.nonzero(known & (exp != y))[0]
    # the words with bytes of unknown width: any order
    xw, yw = x.view(">u4"), y.view(">u4")
    unk = np.nonzero((width == 0).reshape(-1, 4).any(axis=1))[0]
    bs16 = lambda v: ((v & 0x00FF00FF) << 8) | ((v >> 8) & 0x00FF00FF)
    xu, yu = xw[unk], yw[unk]
    ok = (xu == yu) | (xu.byteswap() == yu) | (bs16(xu) == yu) | \
         (((xu & 0xFFFF) | (bs16(xu) & 0xFFFF0000)) == yu) | (((xu & 0xFFFF0000) | (bs16(xu) & 0xFFFF)) == yu)
    ok |= image(xu) & image(yu.byteswap()) | stack(xu) & stack(yu.byteswap())
    badw = unk[~ok]
    # addresses in the port's image or on the host stacks in both builds
    # differ by construction
    ptrs = set(np.nonzero(image(xw) & image(yw.byteswap()) | stack(xw) & stack(yw.byteswap()))[0].tolist())
    known_bad = sorted(set(int(i) // 4 for i in bad) - set(int(i) for i in unk) - ptrs)
    print(f"rdram_{n}: {len(known_bad)} words differ at their widths, {len(badw)} of unknown width in every order"
          f" ({int(cut.sum())} bytes of cut units not compared)")
    for i in known_bad[:10]:
        print(f"   {0x80000000 + 4 * i:08X}: be {x[4*i:4*i+4].tobytes().hex()} native {y[4*i:4*i+4].tobytes().hex()}"
              f" widths {[int(v) for v in width[4*i:4*i+4]]}")
    for i in badw[:6]:
        print(f"   {0x80000000 + 4 * int(i):08X}: be {x[4*i:4*i+4].tobytes().hex()} native {y[4*i:4*i+4].tobytes().hex()} (unknown)")


def rdram_native(a, b, ns, widths=None):
    for n in ns:
        if os.path.exists(f"{b}/rdram_{n}.widths"):
            rdram_typed(a, b, n)
            continue
        x = np.fromfile(f"{a}/rdram_{n}.bin", ">u4")
        yb = np.fromfile(f"{b}/rdram_{n}.bin", "u1")
        y = yb.view(">u4")              # the native dump's bytes, read big-endian
        bs16 = lambda v: ((v & 0x00FF00FF) << 8) | ((v >> 8) & 0x00FF00FF)
        rev = x.byteswap()              # a u32
        both = bs16(x)                  # two u16s
        hi = (x & 0x0000FFFF) | (bs16(x) & 0xFFFF0000)
        lo = (x & 0xFFFF0000) | (bs16(x) & 0x0000FFFF)
        ok = (x == y) | (rev == y) | (both == y) | (hi == y) | (lo == y)
        yn = y.byteswap()               # the native word as a value
        ok |= image(x) & image(yn) | stack(x) & stack(yn)
        d = np.nonzero(~ok)[0]
        print(f"rdram_{n}: {len(d)} words differ in every order",
              " ".join(f"{0x80000000 + 4 * w:08X}:{x[w]:08X}/{y[w]:08X}" for w in d[:8]))


def fnv(name):
    h = 2166136261
    for c in name.encode():
        h = ((h ^ c) * 16777619) & 0xFFFFFFFF
    return h


# the port's own functions one build has and the other doesn't: left out
# of the traces (the native-endian build's boot-time fixups)
PORT_ONLY = ["port_native_fixups"]


def trace(a, b, exes):
    names = {0: "-- controller read --"}
    for exe in exes:
        for line in subprocess.run(["nm", exe], capture_output=True, text=True).stdout.splitlines():
            p = line.split()
            if len(p) == 3 and p[1] in "Tt":
                names[fnv(p[2])] = p[2]
    x = np.fromfile(a, "<u4")
    y = np.fromfile(b, "<u4")
    drop = np.array([fnv(n) for n in PORT_ONLY], dtype="<u4")
    x = x[~np.isin(x, drop)]
    y = y[~np.isin(y, drop)]
    n = min(len(x), len(y))
    d = np.nonzero(x[:n] != y[:n])[0]
    if not len(d):
        print(f"identical for {n} calls ({len(x)} and {len(y)} in all)")
        return
    i = int(d[0])
    print(f"first difference at call {i}, after {int((x[:i] == 0).sum())} controller reads of the trace")
    for k in range(max(0, i - 12), min(n, i + 8)):
        print(f"{k:9d} {'>' if k == i else ' '} {names.get(int(x[k]), hex(x[k])):32s} {names.get(int(y[k]), hex(y[k]))}")


REG = ["pc", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
       "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra", "lo"]


def itrace(a, b):
    x = np.fromfile(a, "<u4").reshape(-1, 33)
    y = np.fromfile(b, "<u4").reshape(-1, 33)
    n = min(len(x), len(y))
    x, y = x[:n], y[:n]
    # the word's bytes as native memory holds them for each way its four
    # bytes can be fields (a word; two halves; a half and two bytes; two
    # bytes and a half; bytes), read as a word
    b = [(x >> (24 - 8 * k)) & 0xFF for k in range(4)]     # big-endian bytes
    le = lambda n: n[0] | n[1] << 8 | n[2] << 16 | n[3] << 24
    same = np.zeros(x.shape, bool)
    for n in ([b[3], b[2], b[1], b[0]], [b[1], b[0], b[3], b[2]], [b[1], b[0], b[2], b[3]],
              [b[0], b[1], b[3], b[2]], b):
        same |= le(n) == y
    perm = ~(x == y) & same
    bad = np.nonzero(~same.all(axis=1))[0]
    if not len(bad):
        print(f"identical for {n} instructions, but for byte order ({len(x)} and {len(y)} in all)")
        return
    i = int(bad[0])
    print(f"first difference at instruction {i} (pc {x[i, 0]:08X} / {y[i, 0]:08X}), before it:")
    for k in range(max(0, i - 24), i + 1):
        diff = [f"{REG[r]} {x[k, r]:08X}/{y[k, r]:08X}{'' if not perm[k, r] else ' (order)'}"
                for r in range(1, 33) if x[k, r] != y[k, r]]
        print(f"{k:9d} {'>' if k == i else ' '} {x[k, 0]:08X} {'  '.join(diff)}")


if __name__ == "__main__":
    if len(sys.argv) > 5 and sys.argv[1:3] == ["rdram", "--native"]:
        rdram_native(sys.argv[3], sys.argv[4], sys.argv[5:])
    elif len(sys.argv) > 6 and sys.argv[1:3] == ["rdram", "--moved"]:
        load_moved(sys.argv[3], sys.argv[4])
        rdram(sys.argv[5], sys.argv[6], sys.argv[7:], "<u4")    # both native-endian
    elif len(sys.argv) > 4 and sys.argv[1] == "rdram":
        rdram(sys.argv[2], sys.argv[3], sys.argv[4:])
    elif len(sys.argv) == 4 and sys.argv[1] == "itrace":
        itrace(sys.argv[2], sys.argv[3])
    elif len(sys.argv) > 4 and sys.argv[1] == "trace":
        trace(sys.argv[2], sys.argv[3], sys.argv[4:])
    else:
        sys.exit(__doc__)
