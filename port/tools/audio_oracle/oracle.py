#!/usr/bin/env python3
"""The libaudio oracle's runner (docs/PORT.md, "The libaudio oracle").

  oracle.py record --version V --refs DIR [--rom ROM] [--build DIR] [--no-build]
  oracle.py replay --version V --refs DIR [--lib port|orig|both] [--out DIR] [--no-build]

record: configures and builds the recorder (a native-endian ILP32 port
with the original libaudio and PORT_AUDIO_RECORD, in build/oracle-rec-V
unless --build says otherwise) and plays test.py's reference scenarios
with it (quick's attract, auto1-3 and attract.long, the same runs as
test.py quick: headless, deterministic, the software renderer), writing
DIR/V-<scenario>.alog.  Each run's --wav is checked against the
scenario's reference hash (port/tools/test_refs.json) where there is one:
the original libaudio has to give the reference's sound, or the log isn't
one of the reference tree.

replay: builds the replay and the two libraries for i386 (make -C
port/tools/audio_oracle, into build/audio_oracle/V unless --out says
otherwise) and replays every DIR/V-*.alog into port.so (and orig.so with
--lib both or orig).  Passes with no mismatch in any.

--lock FILE holds an flock on FILE for the runs and the replays (the
builds run outside it).  --scenarios picks some of them.

Exit status: 0 when everything passed, 1 on a failure or a mismatch, 2 on
a usage error.
"""
import argparse
import concurrent.futures as cf
import contextlib
import fcntl
import glob
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
import test  # noqa: E402  (port/tools/test.py: the scenarios and their references)

ROOT = test.ROOT
SCENARIOS = ["attract", "auto1", "auto2", "auto3", "attract.long"]
RECORDER = ["-DPORT_64BIT=ON", "-DPORT_NATIVE_ENDIAN=ON", "-DPORT_AUDIO_RECORD=ON",
            "-DPORT_LIBAUDIO_ORIGINAL=ON"]
SUMMARY = re.compile(r"replay: (\d+) calls, (\d+) audio frames, (\d+) callbacks, (\d+) mismatches")


@contextlib.contextmanager
def locked(path):
    if not path:
        yield
        return
    with open(path, "a") as f:
        fcntl.flock(f, fcntl.LOCK_EX)
        try:
            yield
        finally:
            fcntl.flock(f, fcntl.LOCK_UN)


def run(cmd, **kw):
    print("+", " ".join(cmd), flush=True)
    return subprocess.call(cmd, **kw)


def sha(path):
    h = hashlib.sha1()
    with open(path, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()[:16]


def record(a):
    build = os.path.abspath(a.build or os.path.join(ROOT, "build", f"oracle-rec-{a.version}"))
    rom = os.path.abspath(a.rom or os.path.join(ROOT, f"baserom.{a.version}.z64"))
    if not os.path.exists(rom):
        print(f"oracle: no ROM at {rom}")
        return 2
    if not a.no_build:
        if run(["cmake", "-S", os.path.join(ROOT, "port"), "-B", build] + test.CMAKE
               + [f"-DPORT_VERSION={a.version}"] + RECORDER) != 0:
            return 1
        if run(["cmake", "--build", build]) != 0:
            return 1
    exe = os.path.join(build, "blastcorps")
    os.makedirs(a.refs, exist_ok=True)
    with open(test.REFS) as f:
        refs = json.load(f).get("quick", {}).get(a.version, {})
    rc = 0
    with locked(a.lock):
        for name in a.scenarios:
            autostart, frames = test.SCENARIOS[name][:2]
            log = os.path.join(a.refs, f"{a.version}-{name}.alog")
            work = tempfile.mkdtemp(prefix=f"oracle-{name}-", dir=os.path.dirname(build))
            env = {k: v for k, v in os.environ.items() if not k.startswith("PORT_")}
            env.update(PORT_AUTOSTART=autostart, SDL_VIDEODRIVER="offscreen", SDL_AUDIODRIVER="dummy",
                       PORT_AUDIO_LOG=log + ".part")
            cmd = [exe, "--headless", "--deterministic", "--frames", str(frames), "--renderer", "sw",
                   "--save", "save.eep", "--wav", "audio.wav", rom]
            with open(os.path.join(work, "log.txt"), "w") as out:
                r = subprocess.call(cmd, cwd=work, env=env, stdout=out, stderr=subprocess.STDOUT)
            if r != 0:
                print(f"FAIL  {name}: the recorder exits with {r} (in {work})")
                rc = 1
                continue
            os.replace(log + ".part", log)
            wav, want = sha(os.path.join(work, "audio.wav")), refs.get(name, {}).get("wav")
            size = os.path.getsize(log)
            if want is None:
                print(f"OK    {name}: {log} ({size} bytes; wav {wav}, no reference for {a.version})")
            elif wav == want:
                print(f"OK    {name}: {log} ({size} bytes; wav = the reference's)")
            else:
                print(f"FAIL  {name}: {log} ({size} bytes) but the wav is {wav}, the reference's {want}")
                rc = 1
            shutil.rmtree(work)
    return rc


def replay(a):
    out = os.path.abspath(a.out or os.path.join(ROOT, "build", "audio_oracle", a.version))
    if not a.no_build:
        if run(["make", "-C", HERE, f"VERSION={a.version}", f"OUT={out}", f"-j{os.cpu_count()}"]) != 0:
            return 1
    logs = sorted(glob.glob(os.path.join(a.refs, f"{a.version}-*.alog")))
    if a.scenarios != SCENARIOS:
        logs = [p for p in logs if os.path.basename(p)[len(a.version) + 1:-5] in a.scenarios]
    if not logs:
        print(f"oracle: no {a.version}-*.alog in {a.refs}")
        return 1
    libs = ["port", "orig"] if a.lib == "both" else [a.lib]
    jobs = [(lib, p) for p in logs for lib in libs]

    def one(job):
        lib, p = job
        r = subprocess.run([os.path.join(out, "replay"), os.path.join(out, f"{lib}.so"), p, "-q"],
                           capture_output=True, text=True)
        return job, r.returncode, r.stdout + r.stderr

    rc = 0
    with locked(a.lock), cf.ThreadPoolExecutor(a.jobs) as ex:
        for (lib, p), r, text in ex.map(one, jobs):
            m = SUMMARY.search(text)
            what = f"{lib}.so {os.path.basename(p)}"
            if r == 0 and m and m.group(4) == "0":
                print(f"PASS  {what}: {m.group(1)} calls, {m.group(2)} audio frames, no mismatch")
            else:
                rc = 1
                lines = [x for x in text.splitlines() if x.startswith(("MISMATCH", "DIVERGED", "replay:"))]
                print(f"FAIL  {what}:\n      " + "\n      ".join(lines[:5] or text.splitlines()[-5:]))
    return rc


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name in ("record", "replay"):
        p = sub.add_parser(name)
        p.add_argument("--version", default="us.v10", choices=["us.v10", "us.v11", "jp", "eu"])
        p.add_argument("--refs", required=True, help="the reference logs' directory")
        p.add_argument("--scenarios", nargs="+", default=SCENARIOS, choices=SCENARIOS)
        p.add_argument("--no-build", action="store_true", help="use what is built")
        p.add_argument("--lock", default=os.environ.get("ORACLE_LOCK"),
                       help="an flock held for the runs/replays ($ORACLE_LOCK)")
    r = sub.choices["record"]
    r.add_argument("--rom", help="the ROM (baserom.V.z64)")
    r.add_argument("--build", help="the recorder's build directory (build/oracle-rec-V)")
    p = sub.choices["replay"]
    p.add_argument("--lib", default="port", choices=["port", "orig", "both"])
    p.add_argument("--out", help="the replay's build directory (build/audio_oracle/V)")
    p.add_argument("-j", "--jobs", type=int, default=4)
    a = ap.parse_args()
    a.refs = os.path.abspath(a.refs)
    return record(a) if a.cmd == "record" else replay(a)


if __name__ == "__main__":
    sys.exit(main())
