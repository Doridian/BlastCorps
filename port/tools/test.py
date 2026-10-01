#!/usr/bin/env python3
"""The port's test suite (docs/PORT.md, "Testing the port").

    port/tools/test.py quick BUILD [BUILD...] [--against BUILD] [--update]
    port/tools/test.py tas BUILD [BUILD...] [--polls FILE]
    port/tools/test.py recomp [--trials N]
    port/tools/test.py variants [--version V] [--no-build] [--tas] [--only NAME,...]
                                [--emsdk DIR]

quick: deterministic headless runs (PORT_COUNT_PER_OP=0 --deterministic,
the software renderer), a few thousand frames each; the save, the --wav and
a screenshot every 250 frames are hashed and compared with the committed
references (port/tools/test_refs.json, by version), with --against another
build's results, and within the build (the pthread backend against
ucontext, --widescreen, --interpolate with either renderer against
the plain run: the save and the sound must not change).  A build that takes
its data from the ROM (PORT_ROM_DATA, the movable ones by default) is also
searched for the ROM's data (rom_scan.py), and must carry none.  --update writes
the build's hashes as the references for its version.

tas: the TAS replay (docs/PORT.md, "The TAS"), several builds at once:
57 platinum (tas_check.py), and the replay's report: every one of the log's
reads matched, none skipped, no mode forced, unless the references list
the variant's current report as a known drift (then that report, exactly,
passes as a known failure and anything else fails).

recomp: the translated engine's differential test (make -C tools/recomp
test, for blastcorps/'s version).

variants: configures and builds the standard variants into build/test-*/
(from the stage 2 blastcorps/ holds) and runs quick on each; every variant
must give the references' hashes, which makes them all equal to each
other.  --tas runs the TAS on all of them too.  The WebAssembly build
(wasm, under node) is one of them where emsdk is there (--emsdk, $EMSDK,
or emcmake on the PATH), and it must equal mn32 in every hash, the
layout-dependent screenshots included.

Exit status: 0 when everything passed (known drifts included), 1 on a
failure, 77 when nothing could run (no ROM, no log, no venv: CTest's skip).
"""
import argparse
import concurrent.futures as cf
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import threading
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
REFS = os.path.join(ROOT, "port", "tools", "test_refs.json")
SKIP = 77

# name: (cmake options) -- the standard variant set, in build/test-<name>
VARIANTS = {
    "32": [],
    "64": ["-DPORT_64BIT=ON"],
    "n64": ["-DPORT_64BIT=ON", "-DPORT_NATIVE_ENDIAN=ON"],
    "lp64": ["-DPORT_LP64=ON"],
    "m64": ["-DPORT_64BIT=ON", "-DPORT_MOVABLE=ON"],
    "mlp64": ["-DPORT_LP64=ON", "-DPORT_MOVABLE=ON"],
    "mn32": ["-DPORT_NATIVE_ENDIAN=ON", "-DPORT_MOVABLE=ON"],
    # the movable 32-bit native-endian build for wasm32, under node (emsdk)
    "wasm": ["-DPORT_WASM_TARGET=node"],
}
# builds whose layouts are the same, so that they must be identical in
# every hash, the layout-dependent screenshots included (docs/PORT.md,
# "WebAssembly": the escaping locals' frames are port-arena's)
TWINS = {frozenset(("wasm", "mn32"))}
CMAKE = ["-G", "Ninja", "-DCMAKE_C_COMPILER=clang", "-DCMAKE_CXX_COMPILER=clang++",
         "-DCMAKE_ASM_COMPILER=clang"]

SHOT_EVERY = 250
# name: (PORT_AUTOSTART, frames, extra args, extra env, base scenario, what must equal base's)
#   base None: compared with the references; otherwise with the base run
#   of the same build ("all": every hash, "game": the save and the sound)
SCENARIOS = {
    "attract": ("0", 4000, [], {}, None, None),
    "auto1": ("1", 3000, [], {}, None, None),
    "auto2": ("2", 2000, [], {}, None, None),       # (it stays at the hint box after 1750)
    "auto3": ("3", 3000, [], {}, None, None),
    "auto3.pthread": ("3", 3000, [], {"PORT_THREADS": "pthread"}, "auto3", "all"),
    "auto3.ucontext": ("3", 3000, [], {"PORT_THREADS": "ucontext"}, "auto3", "all"),
    "auto3.wide": ("3", 3000, ["--widescreen"], {}, "auto3", "game"),
    "auto3.interp": ("3", 3000, ["--interpolate", "--widescreen"], {}, "auto3", "game"),
    "auto3.gl": ("3", 3000, ["--renderer", "gl", "--scale", "1"], {}, "auto3", "game"),
    "auto3.gl.interp": ("3", 3000, ["--renderer", "gl", "--scale", "1", "--interpolate", "--widescreen"],
                        {}, "auto3", "game"),
    # --hd-text (docs/FONTS.md) draws the text from a font: render-side only
    # (auto2 ends at a hint box, auto3 has the level's numbers)
    "auto2.gl.hdtext": ("2", 2000, ["--renderer", "gl", "--scale", "2", "--hd-text"], {}, "auto2", "game"),
    "auto3.gl.hdtext": ("3", 3000, ["--renderer", "gl", "--scale", "2", "--hd-text", "--interpolate"],
                        {}, "auto3", "game"),
    # the attract mode's story and several of its demo levels (about 70s):
    # the coverage of a version the movie isn't
    "attract.long": ("0", 12000, [], {}, None, None),
}
# scenarios only some versions run (the others run all of SCENARIOS)
SCENARIO_VERSIONS = {"attract.long": ("us.v11", "jp")}


def scenarios_for(version):
    return [n for n in SCENARIOS if version in SCENARIO_VERSIONS.get(n, (version,))]

REPORT = re.compile(r"replay: (\d+) reads, (-?\d+) of the log's (\d+) matched \((\d+) skipped\), "
                    r"(\d+) without a match, (\d+) retraces given anyway, .* (\d+) save commands let go "
                    r"early, (\d+) modes forced")

COLOR = sys.stdout.isatty()


def tag(s):
    if not COLOR:
        return s
    c = {"PASS": "32", "FAIL": "31", "SKIP": "33", "XFAIL": "36", "NOTE": "33"}.get(s.strip(), "0")
    return f"\033[{c}m{s}\033[0m"


_tl = threading.local()


def emit(s):
    """print, or collect into this thread's buffer (variants runs builds at once)"""
    buf = getattr(_tl, "buf", None)
    if buf is not None:
        buf.append(s)
    else:
        print(s, flush=True)


def say(status, what, detail=""):
    emit(f"{tag(f'{status:5}')} {what}" + (f": {detail}" if detail else ""))


def sha(path):
    h = hashlib.sha1()
    with open(path, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()[:16]


def load_refs():
    with open(REFS) as f:
        return json.load(f)


def save_refs(refs):
    with open(REFS, "w") as f:
        json.dump(refs, f, indent=1, sort_keys=True)
        f.write("\n")


class Build:
    def __init__(self, path):
        self.path = os.path.abspath(path)
        self.exe = os.path.join(self.path, "blastcorps")
        cache = {}
        try:
            with open(os.path.join(self.path, "CMakeCache.txt")) as f:
                for line in f:
                    m = re.match(r"([A-Za-z0-9_]+):[A-Z]+=(.*)", line.strip())
                    if m:
                        cache[m.group(1)] = m.group(2)
        except OSError:
            raise SystemExit(f"{path}: not a configured port build (no CMakeCache.txt)")
        on = lambda k: cache.get(k, "OFF").upper() in ("ON", "1", "TRUE", "YES")
        self.version = cache.get("PORT_VERSION", "us.v11")
        self.lp64, self.native, self.bits64, self.movable = (
            on("PORT_LP64"), on("PORT_NATIVE_ENDIAN"), on("PORT_64BIT"), on("PORT_MOVABLE"))
        self.rom_data = on("PORT_ROM_DATA")
        self.gl = cache.get("EPOXY_FOUND", "") == "1"
        self.threads = cache.get("PORT_THREADS", "ucontext")
        self.wasm = cache.get("EMSCRIPTEN", "") == "1"
        self.variant = ("m" if self.movable else "") + (
            "lp64" if self.lp64 else ("n" if self.native else "") + ("64" if self.bits64 else "32"))
        self.node = None
        if self.wasm:       # (the movable native-endian 32-bit build, for wasm32)
            self.variant = "wasm"
            self.exe = os.path.join(self.path, "blastcorps.js")
            self.node = cache.get("CMAKE_CROSSCOMPILING_EMULATOR") or shutil.which("node")
        self.name = os.path.relpath(self.path, ROOT) if self.path.startswith(ROOT) else self.path

    def rom(self):
        return os.path.join(ROOT, f"baserom.{self.version}.z64")

    def copy_exe(self, d):
        """a copy of the executable in d (wasm: the .js and its .wasm); its path"""
        os.makedirs(d, exist_ok=True)
        shutil.copy2(self.exe, d)
        if self.wasm:
            shutil.copy2(os.path.splitext(self.exe)[0] + ".wasm", d)
        return os.path.join(d, os.path.basename(self.exe))

    def command(self, exe=None):
        return ([self.node] if self.wasm else []) + [exe or self.exe]

    def runnable(self):
        return os.access(self.exe, os.X_OK) if not self.wasm else (
            os.path.exists(self.exe) and bool(self.node) and os.access(self.node, os.X_OK))


def twins(a, b):
    return frozenset((a.variant, b.variant)) in TWINS


def run_port(build, outdir, frames, autostart, args=(), env=(), shots=True, exe=None):
    """one deterministic headless run in a fresh outdir; returns (rc, seconds)"""
    if os.path.isdir(outdir):
        shutil.rmtree(outdir)
    os.makedirs(outdir)
    e = dict(os.environ)
    for k in list(e):
        if k.startswith("PORT_"):
            del e[k]
    e.update(PORT_COUNT_PER_OP="0", PORT_AUTOSTART=autostart, SDL_VIDEODRIVER="offscreen",
             SDL_AUDIODRIVER="dummy")
    if shots:
        e["PORT_SHOT_EVERY"] = str(SHOT_EVERY)
    e.update(env)
    cmd = build.command(exe) + ["--headless", "--deterministic", "--frames", str(frames),
           "--save", "save.eep", "--wav", "audio.wav"]
    if shots:
        cmd += ["--screenshot", "shot"]
    if "--renderer" not in args:
        cmd += ["--renderer", "sw"]
    cmd += list(args) + [build.rom()]
    t = time.time()
    with open(os.path.join(outdir, "log.txt"), "w") as log:
        rc = subprocess.call(cmd, cwd=outdir, env=e, stdout=log, stderr=subprocess.STDOUT)
    return rc, time.time() - t


def hashes(outdir):
    h = {"shots": {}}
    for f in sorted(os.listdir(outdir)):
        p = os.path.join(outdir, f)
        m = re.match(r"shot(\d+)\.bmp$", f)
        if m:
            h["shots"][str(int(m.group(1)))] = sha(p)
        elif f == "save.eep":
            h["save"] = sha(p)
        elif f == "audio.wav":
            h["wav"] = sha(p)
    return h


def val(h, k):
    """a hash set's hash for key k: 'save', 'wav' or a screenshot's frame"""
    return h.get(k) if k in ("save", "wav") else h.get("shots", {}).get(k)


def diff_keys(a, b, what="all", layout=()):
    """the keys whose hashes differ; what: 'all' or 'game' (save, sound)"""
    keys = ["save", "wav"]
    if what == "all":
        keys += sorted((set(a["shots"]) | set(b["shots"])) - set(layout), key=int)
    return [k for k in keys if val(a, k) != val(b, k)]


def describe(keys, a, b):
    return "; ".join(f"{k if k in ('save', 'wav') else 'shot ' + k} {val(a, k)} != {val(b, k)}" for k in keys)


def compare(a, b, what, layout=()):
    """the differences between two hash sets, as text"""
    k = diff_keys(a, b, what, layout)
    return [describe(k, a, b)] if k else []


def known_for(refs, build, name):
    """the known failures' hashes for this build's variant in one scenario"""
    out, notes = {"shots": {}}, []
    for k in refs.get("known", {}).get(build.version, []):
        if build.variant in k["variants"] and name in k["scenarios"]:
            s = k["scenarios"][name]
            out.update({x: s[x] for x in ("save", "wav") if x in s})
            out["shots"].update(s.get("shots", {}))
            notes.append(k["note"])
    return out, notes


def add_known(refs, build, name, h, keys, note):
    """records h's hashes at keys as a known failure of the build's variant"""
    s = {k: h[k] for k in keys if k in ("save", "wav")}
    shots = {k: val(h, k) for k in keys if k not in ("save", "wav")}
    if shots:
        s["shots"] = shots
    lst = refs.setdefault("known", {}).setdefault(build.version, [])
    for k in lst:     # the same failure in another variant, or more of this one's
        if k["note"] == note and (k["scenarios"].get(name) == s or k["variants"] == [build.variant]):
            k["scenarios"][name] = s
            if build.variant not in k["variants"]:
                k["variants"].append(build.variant)
            return
    lst.append({"note": note, "variants": [build.variant], "scenarios": {name: s}})


# ---- quick -------------------------------------------------------------------

def quick_run(build, jobs, scenarios):
    """runs the scenarios; returns {name: hashes or None}, logging failures"""
    out = os.path.join(build.path, "test", "quick")
    # a copy of the executable, so a rebuild during the run doesn't mix two
    exe = build.copy_exe(os.path.join(build.path, "test"))
    res, times = {}, {}

    def one(name):
        autostart, frames, args, env, base, what = SCENARIOS[name]
        rc, t = run_port(build, os.path.join(out, name), frames, autostart, args, env,
                         shots=True, exe=exe)
        return name, rc, t

    with cf.ThreadPoolExecutor(jobs) as ex:
        # the longest first, so the others run beside it
        for name, rc, t in ex.map(one, sorted(scenarios, key=lambda n: -SCENARIOS[n][1])):
            d = os.path.join(out, name)
            times[name] = t
            if rc != 0:
                tail = open(os.path.join(d, "log.txt"), errors="replace").read().splitlines()[-3:]
                res[name] = None
                res[name + ".error"] = f"exit {rc} ({' | '.join(tail)})"
            else:
                res[name] = hashes(d)
    with open(os.path.join(build.path, "test", "quick.json"), "w") as f:
        json.dump({"version": build.version, "variant": build.variant,
                   "results": {k: v for k, v in res.items() if not k.endswith(".error")}},
                  f, indent=1, sort_keys=True)
    return res, times


def quick(build, refs, jobs, against=None, update=False, known=None):
    """one build's quick tier; returns the number of failures (None: skipped)"""
    emit(f"== quick: {build.name} ({build.version}, {build.variant}"
         f"{', no OpenGL' if not build.gl else ''})")
    if not os.path.exists(build.rom()):
        say("SKIP", build.name, f"no {os.path.basename(build.rom())} in the repo's root")
        return None
    if not build.runnable():
        say("FAIL", build.name, "not built" + (" (or no node)" if build.wasm else ""))
        return 1
    names = [n for n in scenarios_for(build.version) if build.gl or "gl" not in n.split(".")]
    if build.threads == "pthread":      # the default is the base run then
        names.remove("auto3.pthread")
    elif build.threads == "ucontext":
        names.remove("auto3.ucontext")
    else:                               # (asyncify: the one backend there)
        names.remove("auto3.pthread")
        names.remove("auto3.ucontext")
    t0 = time.time()
    res, times = quick_run(build, jobs, names)
    vref = refs.get("quick", {}).get(build.version, {})
    layout = refs.get("layout", {}).get(build.version, {})
    fails = 0
    for name in names:
        autostart, frames, args, env, base, what = SCENARIOS[name]
        h = res[name]
        label = f"{build.name} {name} ({frames} frames, {times[name]:.0f}s)"
        if h is None:
            say("FAIL", label, res[name + ".error"])
            fails += 1
            continue
        if base is not None:
            if res.get(base) is None:
                say("FAIL", label, f"no {base} run to compare with")
                fails += 1
                continue
            d = compare(h, res[base], what)
            kind = "every hash" if what == "all" else "save and sound"
            if d:
                say("FAIL", label, f"{kind} should equal {base}'s: " + "; ".join(d))
                fails += 1
            else:
                say("PASS", label, f"{kind} as {base}'s")
            continue
        n = len(h["shots"])
        if update:
            vref[name] = h
            say("NOTE", label, "reference updated")
        elif name not in vref:
            say("SKIP", label, f"no reference for {build.version} (--update writes one)")
        else:
            lay = layout.get(name, [])
            keys = diff_keys(h, vref[name], "all", lay)
            kn, notes = known_for(refs, build, name)
            real = [k for k in keys if val(kn, k) != val(h, k)]
            what = f"save, wav and {n - len(lay)} of {n} screenshots"
            what += f" (layout-dependent, not compared: {', '.join(lay)})" if lay else ""
            if real and known is not None:
                add_known(refs, build, name, h, real, known)
                say("NOTE", label, f"recorded as known: {describe(real, h, vref[name])}")
            elif real:
                say("FAIL", label, "differs from the reference: " + describe(real, h, vref[name]))
                fails += 1
            elif keys:
                say("XFAIL", label, f"{', '.join(k if k in ('save', 'wav') else 'shot ' + k for k in keys)} "
                    f"as known: {'; '.join(notes)}")
            else:
                say("PASS", label, f"{what} as the reference"
                    + (f"; the known failure is gone, take it out of test_refs.json: {'; '.join(notes)}"
                       if notes else ""))
    if against is not None:
        try:
            other = json.load(open(os.path.join(against.path, "test", "quick.json")))
        except OSError:
            say("FAIL", f"{build.name} against {against.name}", "no results there (run quick on it first)")
            fails += 1
        else:
            exact = twins(build, against)
            for name in names:
                if res[name] is None or name not in other["results"]:
                    continue
                lay = [] if exact else layout.get(SCENARIOS[name][4] or name, [])
                # the OpenGL screenshots are the host's GL's, but the same on
                # one machine
                d = compare(res[name], other["results"][name], "all", lay)
                label = f"{build.name} {name} against {against.name}"
                if d:
                    say("FAIL", label, "; ".join(d))
                    fails += 1
                else:
                    say("PASS", label, "identical" + (", the layout-dependent screenshots too" if exact else ""))
    if build.rom_data:
        fails += rom_scan(build)
    if update:
        refs.setdefault("quick", {})[build.version] = vref
    if update or known is not None:
        save_refs(refs)
    emit(f"   {build.name}: {fails} failed, {time.time() - t0:.0f}s")
    return fails


def rom_scan(build):
    """a PORT_ROM_DATA build carries none of the ROM (rom_scan.py: no
    stretch of 24 bytes of it or of its inflated modules); the failures"""
    files = [build.exe] + ([os.path.splitext(build.exe)[0] + ".wasm"] if build.wasm else [])
    t = time.time()
    r = subprocess.run([sys.executable, os.path.join(ROOT, "port", "tools", "rom_scan.py"), "--version",
                        build.version, "--max-bytes", "0", "--top", "5"] + files, capture_output=True, text=True)
    label = f"{build.name} rom_scan ({time.time() - t:.0f}s)"
    if r.returncode == 0:
        say("PASS", label, "none of the ROM's data in " + ", ".join(os.path.basename(f) for f in files))
        return 0
    if r.returncode == SKIP:
        say("SKIP", label, r.stderr.strip())
        return 0
    say("FAIL", label, "the ROM's data in the build:\n" + r.stdout + r.stderr)
    return 1


# ---- tas ---------------------------------------------------------------------

def tas_one(build, polls, again=False):
    out = os.path.join(build.path, "test", "tas")
    if again:           # the last run's results, checked again
        try:
            rc, t = map(float, open(os.path.join(out, "exit.txt")).read().split())
        except (OSError, ValueError):
            rc, t = 0, 0
        return out, int(rc), t
    if os.path.isdir(out):
        shutil.rmtree(out)
    os.makedirs(out)
    exe = build.copy_exe(out)
    e = {k: v for k, v in os.environ.items() if not k.startswith("PORT_")}
    e.update(SDL_VIDEODRIVER="offscreen", SDL_AUDIODRIVER="dummy")
    t = time.time()
    with open(os.path.join(out, "log.txt"), "w") as log:
        rc = subprocess.call(build.command(exe) + ["--headless", "--replay", polls, "--save", "save.eep", build.rom()],
                             cwd=out, env=e, stdout=log, stderr=subprocess.STDOUT)
    t = time.time() - t
    with open(os.path.join(out, "exit.txt"), "w") as f:
        f.write(f"{rc} {t:.0f}\n")
    return out, rc, t


def tas(builds, refs, polls, jobs, again=False):
    polls = os.path.abspath(polls)
    if not again and not os.path.exists(polls):
        say("SKIP", "tas", f"no {polls} (port/tools/tas.sh makes it)")
        return None
    fails, ran = 0, 0
    todo = []
    for b in builds:
        if b.version != "us.v10":
            say("SKIP", f"{b.name} tas", f"the movie is us.v10's, this build is {b.version}")
        elif not os.path.exists(b.rom()):
            say("SKIP", f"{b.name} tas", f"no {os.path.basename(b.rom())}")
        elif again and not os.path.exists(os.path.join(b.path, "test", "tas", "log.txt")):
            say("SKIP", f"{b.name} tas", "no replay to check again")
        else:
            todo.append(b)
    print(f"== tas{' (the last runs, checked again)' if again else ''}: {', '.join(b.name for b in todo)} ({min(jobs, len(todo))} at a time, "
          f"about 10-20 minutes each)", flush=True)
    with cf.ThreadPoolExecutor(max(1, jobs)) as ex:
        futs = {ex.submit(tas_one, b, polls, again): b for b in todo}
        for fut in cf.as_completed(futs):
            b = futs[fut]
            out, rc, t = fut.result()
            ran += 1
            fails += tas_check(b, refs, out, rc, t)
    return fails if ran else None


def tas_check(b, refs, out, rc, t):
    label = f"{b.name} tas ({t / 60:.1f} min)"
    log = open(os.path.join(out, "log.txt"), errors="replace").read()
    m = None
    for m in REPORT.finditer(log):
        pass
    chk = subprocess.run([sys.executable, os.path.join(ROOT, "port", "tools", "tas_check.py"),
                          os.path.join(out, "save.eep"), "--platinum", "57"],
                         capture_output=True, text=True)
    problems = []
    if rc != 0:
        problems.append(f"exit {rc} ({' | '.join(log.splitlines()[-2:])})")
    if chk.returncode != 0:
        problems.append((chk.stderr or chk.stdout).strip().splitlines()[-1])
    if m is None:
        problems.append("no replay report in the log")
        say("FAIL", label, "; ".join(problems))
        return 1
    reads, matched, total, skipped, unmatched, given, early, forced = map(int, m.groups())
    savep = os.path.join(out, "save.eep")
    got = {"matched": matched, "total": total, "skipped": skipped, "forced": forced,
           "save": sha(savep) if os.path.exists(savep) else None}
    summary = (f"{matched} of the log's {total} matched ({skipped} skipped), {forced} modes forced; "
               f"{given} retraces given anyway, {early} save commands let go early")
    with open(os.path.join(out, "summary.json"), "w") as f:
        json.dump(dict(got, given=given, early=early), f)
    tref = refs.get("tas", {}).get(b.version, {})
    exact = matched == total and skipped == 0 and forced == 0
    known = next((k for k in tref.get("known", []) if b.variant in k["variants"]), None)
    if problems:
        say("FAIL", label, "; ".join(problems) + f" [{summary}]")
        return 1
    if exact:
        if tref.get("save") and got["save"] != tref["save"]:
            say("FAIL", label, f"the replay matched, but the save is {got['save']}, not {tref['save']}")
            return 1
        note = " (the known drift is gone: take it out of test_refs.json)" if known else ""
        say("PASS", label, f"57 platinum, the save as the reference, {summary}{note}")
        return 0
    if known and all(known.get(k) == got[k] for k in got):
        say("XFAIL", label, f"57 platinum, as known ({known['note']}): {summary}")
        return 0
    say("FAIL", label, f"57 platinum, but the replay drifts: {summary}, save {got['save']}"
        + (f" (known: {', '.join(f'{k} {known[k]}' for k in got)})" if known else ""))
    return 1


# ---- recomp ------------------------------------------------------------------

def venv_python():
    for p in (os.path.join(ROOT, ".env", "bin", "python"), sys.executable):
        if os.path.exists(p) and subprocess.run([p, "-c", "import numpy, unicorn"],
                                                capture_output=True).returncode == 0:
            return p
    return None


def recomp(trials):
    try:
        version = open(os.path.join(ROOT, "blastcorps", ".version")).read().strip()
    except OSError:
        say("SKIP", "recomp difftest", "no stage 2 extracted (blastcorps/.version)")
        return None
    py = venv_python()
    if py is None:
        say("SKIP", "recomp difftest", "no Python with numpy and unicorn (.env)")
        return None
    print(f"== recomp: difftest, {version}, {trials} trials a function", flush=True)
    t = time.time()
    log = os.path.join(ROOT, "blastcorps", "build", "recomp", "difftest.log")
    os.makedirs(os.path.dirname(log), exist_ok=True)
    with open(log, "w") as f:
        rc = subprocess.call(["make", "-C", os.path.join(ROOT, "tools", "recomp"), "test",
                              f"VERSION={version}", f"PYTHON={py}", f"TRIALS={trials}"],
                             stdout=f, stderr=subprocess.STDOUT)
    lines = [l for l in open(log, errors="replace").read().splitlines()
             if re.search(r"\d+/\d+ functions|pass|fail", l, re.I)]
    detail = " | ".join(lines[-2:]) if lines else ""
    say("PASS" if rc == 0 else "FAIL", f"recomp difftest ({(time.time() - t) / 60:.1f} min)",
        detail + f" (log: {os.path.relpath(log, ROOT)})")
    return 0 if rc == 0 else 1


# ---- variants ----------------------------------------------------------------

def emsdk_cmake(given):
    """how to configure a WebAssembly build: the cmake command's start, or None"""
    root = given or os.environ.get("EMSDK")
    if root:
        root = os.path.abspath(root)
        tc = os.path.join(root, "upstream", "emscripten", "cmake", "Modules", "Platform", "Emscripten.cmake")
        if not os.path.exists(tc):
            return None
        nodes = sorted(d for d in (os.listdir(os.path.join(root, "node"))
                                   if os.path.isdir(os.path.join(root, "node")) else [])
                       if os.path.exists(os.path.join(root, "node", d, "bin", "node")))
        node = os.path.join(root, "node", nodes[-1], "bin", "node") if nodes else shutil.which("node")
        if not node:
            return None
        return ["cmake", f"-DCMAKE_TOOLCHAIN_FILE={tc}", f"-DCMAKE_CROSSCOMPILING_EMULATOR={node}"]
    if shutil.which("emcmake"):
        return ["emcmake", "cmake"]
    return None


def variants(args, refs):
    version = args.version
    try:
        extracted = open(os.path.join(ROOT, "blastcorps", ".version")).read().strip()
    except OSError:
        extracted = None
    names = args.only.split(",") if args.only else list(VARIANTS)
    builds = []
    t0 = time.time()
    emcmake = emsdk_cmake(args.emsdk)
    for n in list(names):
        d = os.path.join(ROOT, "build", f"test-{n}")
        if n == "wasm" and (not os.path.exists(os.path.join(d, "CMakeCache.txt")) if args.no_build
                            else emcmake is None):
            say("SKIP", f"build/test-{n}", "no emsdk (--emsdk, $EMSDK or emcmake on the PATH)"
                if not args.no_build else "not configured")
            names.remove(n)
            continue
        if not args.no_build:
            if extracted != version:
                raise SystemExit(f"blastcorps/ holds {extracted}'s stage 2, not {version}'s")
            t = time.time()
            log = d + ".build.log"
            with open(log, "w") as f:
                start, flags = (emcmake, ["-G", "Ninja"]) if n == "wasm" else (["cmake"], CMAKE)
                rc = subprocess.call(start + ["-S", os.path.join(ROOT, "port"), "-B", d] + flags
                                     + [f"-DPORT_VERSION={version}"] + VARIANTS[n],
                                     stdout=f, stderr=subprocess.STDOUT)
                rc = rc or subprocess.call(["cmake", "--build", d], stdout=f, stderr=subprocess.STDOUT)
            if rc:
                say("FAIL", f"build/test-{n}", f"build failed (log: {os.path.relpath(log, ROOT)})")
                continue
            say("PASS", f"build/test-{n}", f"built in {time.time() - t:.0f}s")
        builds.append(Build(d))
    tb = time.time()
    fails = len(names) - len(builds)
    # the builds' quick tiers in parallel, each with its own share of the
    # cores (every run is one process)
    jobs = max(1, (os.cpu_count() or 2) // 2)
    per = max(1, jobs // max(1, len(builds)))
    with cf.ThreadPoolExecutor(len(builds) or 1) as ex:
        def one(b):      # each build's lines together
            _tl.buf = []
            r = quick(b, refs, per)
            return b, r, "".join(l + "\n" for l in _tl.buf)
        results = list(ex.map(one, builds))
    skipped = 0
    for b, r, text in results:
        sys.stdout.write(text)
        if r is None:
            skipped += 1
        else:
            fails += r
    # the builds that must be identical, layout and all
    for a in builds:
        for b in builds:
            if a.variant > b.variant and twins(a, b):
                fails += twin_check(a, b)
    # the equivalences, as a table: every variant's hashes against the first's
    table(builds, refs)
    print(f"== variants: build {tb - t0:.0f}s, quick {time.time() - tb:.0f}s", flush=True)
    if args.tas:
        r = tas(builds, refs, args.polls, args.jobs or len(builds))
        fails += r or 0
    return None if skipped == len(builds) and not fails else fails


def twin_check(a, b):
    """two builds' last quick results, every hash the same; the failures"""
    try:
        ra, rb = (json.load(open(os.path.join(x.path, "test", "quick.json")))["results"] for x in (a, b))
    except OSError:
        say("FAIL", f"{a.variant} against {b.variant}", "no quick results")
        return 1
    fails = 0
    for n in sorted(set(ra) & set(rb)):
        d = compare(ra[n], rb[n], "all")
        label = f"{a.variant} {n} against {b.variant}"
        if d:
            say("FAIL", label, "; ".join(d))
            fails += 1
        else:
            say("PASS", label, "identical, the layout-dependent screenshots too")
    return fails


def table(builds, refs):
    """every build's scenarios against the references (or the first build's)"""
    got = []
    for b in builds:
        try:
            got.append((b, json.load(open(os.path.join(b.path, "test", "quick.json")))["results"]))
        except OSError:
            pass
    if not got:
        return
    version = got[0][0].version
    ref = refs.get("quick", {}).get(version)
    against = "the references" if ref else got[0][0].variant
    ref = ref or got[0][1]
    names = [n for n in scenarios_for(version) if SCENARIOS[n][4] is None]
    lay = refs.get("layout", {}).get(version, {})
    print(f"== against {against}: = identical, ~ but for layout-dependent screenshots, "
          f"k but for known failures, x differs")
    w = [max(8, len(n)) for n in names]
    print("   " + "".ljust(8) + " ".join(n.ljust(x) for n, x in zip(names, w)))
    for b, r in got:
        cells = []
        for n in names:
            if r.get(n) is None or ref.get(n) is None:     # (not run, or failed)
                cells.append("-")
                continue
            keys = diff_keys(r[n], ref[n])
            rest = [k for k in keys if k not in lay.get(n, [])]
            kn, _ = known_for(refs, b, n)
            cells.append("=" if not keys else "~" if not rest else
                         "k" if all(val(kn, k) == val(r[n], k) for k in rest) else "x")
        print("   " + b.variant.ljust(8) + " ".join(c.ljust(x) for c, x in zip(cells, w)))


def detail(builds):
    """every hash of the builds' last quick runs, against the first build's"""
    got = []
    for b in builds:
        try:
            got.append((b.variant, json.load(open(os.path.join(b.path, "test", "quick.json")))["results"]))
        except OSError:
            say("SKIP", b.name, "no quick results")
    if not got:
        return
    base = got[0][1]
    for n in base:
        print(f"== {n} (= as {got[0][0]}'s)")
        print("   " + "".ljust(7) + " ".join(v.ljust(6) for v, _ in got))
        keys = [("save", None), ("wav", None)] + [("shots", f) for f in sorted(base[n]["shots"], key=int)]
        for k, f in keys:
            row = []
            for v, r in got:
                a = base[n].get(k) if f is None else base[n][k].get(f)
                rn = r.get(n) or {}     # (None: the run failed)
                h = rn.get(k) if f is None else rn.get(k, {}).get(f)
                row.append("=" if h == a else (h or "none")[:6])
            print("   " + (f or k).ljust(7) + " ".join(c.ljust(6) for c in row))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter, epilog=__doc__)
    sub = ap.add_subparsers(dest="cmd", required=True)
    q = sub.add_parser("quick")
    q.add_argument("builds", nargs="+")
    q.add_argument("--against", help="another build dir whose quick results must be identical")
    q.add_argument("--update", action="store_true", help="write the results as the references")
    q.add_argument("--known", metavar="NOTE",
                   help="record the build's differences from the references as a known failure")
    q.add_argument("-j", "--jobs", type=int, default=max(1, (os.cpu_count() or 2) // 2))
    t = sub.add_parser("tas")
    t.add_argument("builds", nargs="+")
    t.add_argument("--polls", default=os.path.join(ROOT, "build", "tas", "run", "polls.csv"))
    t.add_argument("-j", "--jobs", type=int, default=0, help="replays at a time (default: all)")
    t.add_argument("--again", action="store_true", help="check the last replays' results again, without running")
    r = sub.add_parser("recomp")
    r.add_argument("--trials", type=int, default=48)
    d = sub.add_parser("table", help="every hash of the builds' last quick runs, against the first's")
    d.add_argument("builds", nargs="+")
    v = sub.add_parser("variants")
    v.add_argument("--version", default="us.v10")
    v.add_argument("--only", help="comma-separated variant names (" + ", ".join(VARIANTS) + ")")
    v.add_argument("--no-build", action="store_true", help="use build/test-* as they are")
    v.add_argument("--tas", action="store_true", help="and the TAS on each")
    v.add_argument("--emsdk", help="emsdk's directory, for the wasm variant (default: $EMSDK, or emcmake)")
    v.add_argument("--polls", default=os.path.join(ROOT, "build", "tas", "run", "polls.csv"))
    v.add_argument("-j", "--jobs", type=int, default=0, help="TAS replays at a time (default: all)")
    args = ap.parse_args()
    refs = load_refs()
    t0 = time.time()
    if args.cmd == "quick":
        against = Build(args.against) if args.against else None
        rs = [quick(Build(b), refs, args.jobs, against, args.update, args.known) for b in args.builds]
        fails = None if all(x is None for x in rs) else sum(x or 0 for x in rs)
    elif args.cmd == "tas":
        builds = [Build(b) for b in args.builds]
        fails = tas(builds, refs, args.polls, args.jobs or len(builds), args.again)
    elif args.cmd == "table":
        detail([Build(b) for b in args.builds])
        return
    elif args.cmd == "recomp":
        fails = recomp(args.trials)
    else:
        fails = variants(args, refs)
    dt = time.time() - t0
    if fails is None:
        print(f"== skipped ({dt:.0f}s)")
        sys.exit(SKIP)
    print(f"== {'all passed' if not fails else f'{fails} FAILED'} ({dt / 60:.1f} min)")
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
