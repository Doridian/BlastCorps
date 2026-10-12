#!/usr/bin/env python3
"""The libaudio oracle's runner (docs/PORT.md, "The libaudio oracle").

  oracle.py replay --version V --refs DIR [--lib port orig] [--out DIR] [--no-build]

The logs, DIR/V-<scenario>.alog, are the recorder's: a native-endian ILP32
build of the port with the original libaudio, at the N64's addresses,
which played test.py's reference scenarios (quick's attract, auto1-3 and
attract.long) and gave the references' sound (gone with P10's variant
builds; the logs stay the references).

replay: builds the replay and the two libraries for i386 (make -C
port/tools/audio_oracle, into build/audio_oracle/V unless --out says
otherwise) and replays every DIR/V-*.alog into port.so, or the libraries
--lib names: orig.so too.
Passes with no mismatch in any.

--lock FILE holds an flock on FILE for the replays (the builds run
outside it).  --scenarios picks some of them.

Exit status: 0 when everything passed, 1 on a failure or a mismatch, 2 on
a usage error.
"""
import argparse
import concurrent.futures as cf
import contextlib
import fcntl
import glob
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
import test  # noqa: E402  (port/tools/test.py: the scenarios and their references)

ROOT = test.ROOT
SCENARIOS = ["attract", "auto1", "auto2", "auto3", "attract.long"]
SUMMARY = re.compile(r"replay: (\d+) calls, (\d+) audio frames, (\d+) callbacks, (\d+) mismatches")
BANKMAP = re.compile(r"replay: bank objects: .*")


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
    libs = a.lib
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
            x = BANKMAP.search(text)
            what = f"{lib}.so {os.path.basename(p)}"
            if r == 0 and m and m.group(4) == "0":
                print(f"PASS  {what}: {m.group(1)} calls, {m.group(2)} audio frames, no mismatch"
                      + (f" ({x.group(0)[len('replay: '):]})" if x else ""))
            else:
                rc = 1
                lines = [x for x in text.splitlines() if x.startswith(("MISMATCH", "DIVERGED", "replay:"))]
                print(f"FAIL  {what}:\n      " + "\n      ".join(lines[:5] or text.splitlines()[-5:]))
    return rc


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("replay")
    p.add_argument("--version", default="us.v10", choices=["us.v10", "us.v11", "jp", "eu"])
    p.add_argument("--refs", required=True, help="the reference logs' directory")
    p.add_argument("--scenarios", nargs="+", default=SCENARIOS, choices=SCENARIOS)
    p.add_argument("--no-build", action="store_true", help="use what is built")
    p.add_argument("--lock", default=os.environ.get("ORACLE_LOCK"),
                   help="an flock held for the replays ($ORACLE_LOCK)")
    p.add_argument("--lib", nargs="+", default=["port"], choices=["port", "orig"])
    p.add_argument("--out", help="the replay's build directory (build/audio_oracle/V)")
    p.add_argument("-j", "--jobs", type=int, default=4)
    a = ap.parse_args()
    a.refs = os.path.abspath(a.refs)
    return replay(a)


if __name__ == "__main__":
    sys.exit(main())
