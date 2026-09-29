#!/usr/bin/env python3
"""Which C functions don't match a version, compiled for it.

Run from the repo root after `make VERSION=<v> -C blastcorps` (the link may
fail; the objects are what's compared).  Every function of every C object
built for the version is compared with the version's binary where the
function's name puts it (symbol_addrs, else tools/vermap.py), with the
fields its relocations fill in masked.  A layout problem shifts everything
after it; this looks at each function on its own, so it names the functions
that need a version's own code (`#if`, GLOBAL_ASM) before the sha1 can.

Also listed per object: functions the version has that the C doesn't
(`only in <v>`: a start after a return, or a call target, that no compiled
function is at), and .text/.data/.rodata/.bss sizes that differ from the
version's blocks (from its config).

With --func, prints that function side by side instead: the compiled
words and the version's, `*` where they differ.

Usage:
  tools/vdiff.py <version> [--module m] [-v]
  tools/vdiff.py <version> --module m --func <name>
"""
import argparse
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import libmatch  # noqa: E402
import modmap  # noqa: E402
import vermap  # noqa: E402

CODE = HERE.parent / "blastcorps"


def symbols(mod, ver):
    out = {}
    p = CODE / f"symbol_addrs.{mod}.{ver}.txt"
    for line in p.read_text().splitlines() if p.exists() else []:
        m = re.match(r"^\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+)", line)
        if m:
            out.setdefault(m.group(1), int(m.group(2), 16))
    return out


def disasm(words, addr):
    with tempfile.NamedTemporaryFile(suffix=".bin") as f:
        f.write(b"".join(w.to_bytes(4, "big") for w in words))
        f.flush()
        out = subprocess.run(["mips-linux-gnu-objdump", "-D", "-b", "binary", "-m", "mips:4300", "-EB",
                              "--adjust-vma", hex(addr), f.name], capture_output=True, text=True).stdout
    lines = [re.sub(r"\s+", " ", line.split("\t", 2)[-1]) for line in out.splitlines()
             if re.match(r"\s*[0-9a-f]+:\s+[0-9a-f]{8}\s", line)]
    return lines + [""] * (len(words) - len(lines))


def show(sig, tw, addr):
    a, b = disasm(sig.words, addr), disasm(tw, addr)
    for i, (x, y, mk) in enumerate(zip(sig.words, tw, sig.masks)):
        mark = "*" if (x ^ y) & mk else " "
        print(f"{addr + 4 * i:08X} {mark} {x:08X} {a[i]:<34} {y:08X} {b[i]}")


def starts(words, vram, lo, hi):
    """Function starts in [lo, hi) (module offsets): after each return and
    its padding that no branch spans, and every call target."""
    out = {lo}
    spans = []
    for i in range(lo // 4, hi // 4):
        w = words[i]
        op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
        if op in (4, 5, 6, 7, 0x14, 0x15, 0x16, 0x17) or \
                (op == 1 and rt in (0, 1, 2, 3, 0x10, 0x11, 0x12, 0x13)) or (op == 0x11 and rs == 8):
            off = w & 0xFFFF
            spans.append(sorted((i, i + 1 + (off - 0x10000 if off & 0x8000 else off))))
    for i, w in enumerate(words):
        if w >> 26 == 3:
            t = (((vram & 0xF0000000) | ((w & 0x3FFFFFF) << 2)) - vram)
            if lo <= t < hi:
                out.add(t)
        if lo <= 4 * i < hi and w in (0x03E00008, 0x42000018):
            j = i + 2
            while j < len(words) and words[j] == 0:
                j += 1
            if 4 * j < hi and not any(a < j <= b for a, b in spans):
                out.add(4 * j)
    return out


def placed(o, sig, tw, addr):
    """{symbol: address} that the version's copy of a function uses where
    the compiled one has a %hi/%lo relocation (with no addend)."""
    out, hi = {}, {}
    for i, typ, symidx in sig.relocs:
        name, sec = o.sym_name(symidx)
        if name is None:
            continue
        if typ == libmatch.R_MIPS_HI16:
            hi[symidx] = (tw[i] & 0xFFFF, sig.words[i] & 0xFFFF)
        elif typ == libmatch.R_MIPS_LO16 and symidx in hi:
            vhi, ohi = hi[symidx]
            v = ((vhi << 16) + libmatch.sext16(tw[i] & 0xFFFF)) & 0xFFFFFFFF
            addend = ((ohi << 16) + libmatch.sext16(sig.words[i] & 0xFFFF)) & 0xFFFFFFFF
            out.setdefault(name, (v - addend) & 0xFFFFFFFF)
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("version")
    ap.add_argument("--module", action="append")
    ap.add_argument("-v", "--verbose", action="store_true", help="list matching objects too")
    ap.add_argument("--func", help="show this function's words next to the version's")
    args = ap.parse_args()
    ver = args.version
    vm = vermap.load(ver)
    mods = modmap.modules(ver, CODE)
    bad_total = 0
    seen_syms = set()
    for name in args.module or libmatch.MODULES:
        mod = mods[name]
        blob = (CODE / f"{name}.{ver}.bin").read_bytes()
        subs = [x for x in mod.subsegments if isinstance(x, list)]
        text_end = next(x[0] for x in subs if len(x) >= 2 and x[1] in ("linker", "data", ".data"))
        words = struct.unpack(f">{text_end // 4}I", blob[:text_end])
        known = symbols(name, ver)
        order = sorted((x for x in subs if len(x) >= 3), key=lambda x: x[0])
        blocks = {}  # (kind, obj) -> size
        for k, x in enumerate(order):
            nxt = order[k + 1][0] if k + 1 < len(order) else mod.size
            if x[0] < text_end:
                nxt = min(nxt, text_end)
            kind = "text" if x[1] in ("c", "asm") else x[1].lstrip(".")
            blocks[(kind, x[2])] = (x[0], nxt - x[0])
        for k, (start, kind, obj) in enumerate(mod.bss):
            end = mod.bss[k + 1][0] if k + 1 < len(mod.bss) else mod.bss_end
            blocks[("bss", obj)] = (start, end - start)
        for x in order:
            if x[1] != "c":
                continue
            ofile = CODE / "build" / "src" / f"{x[2]}.c.o"
            if not ofile.exists():
                print(f"{x[2]}: not built")
                continue
            o = libmatch.Obj(ofile)
            bad = []
            at = {}  # module offset -> name of the compiled functions
            differ = set()
            for fname, s, e, is_global in o.functions():
                addr = known.get(fname)
                if addr is None:
                    addr = vm.original(fname)
                if addr is None:
                    # a static, or a name no symbol file places (libaudio's
                    # init_lpfilter): nothing to compare it with
                    if is_global and vermap.NAME.match(fname):
                        bad.append(f"{fname} (not in {ver})")
                    continue
                at[addr - mod.vram] = fname
                sig = libmatch.Signature(o, fname, s, e, True)
                off = addr - mod.vram
                data = blob[off:off + 4 * len(sig)]
                if len(data) < 4 * len(sig):
                    bad.append(f"{fname} (past the end)")
                    continue
                tw = struct.unpack(f">{len(sig)}I", data)
                if args.func == fname:
                    show(sig, tw, addr)
                    return
                diff = sum(1 for a, b, mk in zip(sig.words, tw, sig.masks) if (a ^ b) & mk)
                if diff:
                    bad.append(f"{fname} ({diff} of {len(sig)} words)")
                    differ.add(fname)
                else:
                    # the code is the version's: where it puts each symbol
                    # is where the symbol is, whatever the map says
                    for sym, where in placed(o, sig, tw, addr).items():
                        a = known.get(sym)
                        if a is None:
                            a = vm.original(sym)
                        if a != where and sym not in seen_syms and vermap.NAME.match(sym):
                            seen_syms.add(sym)
                            now = f"0x{a:08X}" if a is not None else "nowhere"
                            bad.append(f"{sym}: the code has it at 0x{where:08X}, the names {now}"
                                       f" (vermap --pair {vermap.NAME.match(sym).group(2)}:{where:08X})")
            lo, size = blocks[("text", x[2])]
            # a function the version has longer (or shorter) than the C:
            # the next one in the file isn't where it ends
            addr_of = {n: a for a, n in at.items()}
            order = o.functions()
            for (n, s, e, _), nxt in zip(order, order[1:]):
                if n in differ or n not in addr_of or nxt[0] not in addr_of:
                    continue
                have = addr_of[nxt[0]] - addr_of[n]
                if have != nxt[1] - s:
                    bad.append(f"{n}: 0x{nxt[1] - s:X} bytes, 0x{have:X} in {ver}")
            text = next((sec["size"] for sec in o.secs if sec["name"] == ".text"), 0)
            if text != size:
                bad.append(f".text: 0x{text:X} bytes, the version's is 0x{size:X}")
                extra = sorted(starts(words, mod.vram, lo, lo + size) - set(at))
                for s in extra if at else []:
                    prev = max((a for a in at if a < s), default=None)
                    where = f" (after {at[prev]})" if prev is not None else " (first)"
                    bad.append(f"only in {ver}: {vm.name('func', mod.vram + s, True)}{where}")
            sizes = {sec["name"]: sec["size"] for sec in o.secs}
            for sec, kind in ((".data", "data"), (".rodata", "rodata"), (".bss", "bss")):
                have = (sizes.get(sec, 0) + 15) & ~15
                want = blocks.get((kind, x[2]))
                if want is None:
                    if have:
                        bad.append(f"{sec}: 0x{have:X} bytes, the config has no block")
                elif (want[1] + 15) & ~15 != have:
                    bad.append(f"{sec}: 0x{sizes.get(sec, 0):X} bytes, the version's block is 0x{want[1]:X}")
            if bad:
                bad_total += len(bad)
                print(f"{x[2]}:")
                for b in bad:
                    print(f"    {b}")
            elif args.verbose:
                print(f"{x[2]}: ok")
    if args.func:
        sys.exit(f"{args.func}: not in a C object of {ver}")
    print(f"{bad_total} differences")


if __name__ == "__main__":
    main()
