#!/usr/bin/env python3
"""A node --cpu-prof profile (the WebAssembly build) as self time by function.

  node --cpu-prof --cpu-prof-interval 200 --cpu-prof-dir DIR build/wasm/blastcorps.js ROM --headless ...
  cpuprofile_report.py DIR/*.cpuprofile [--top N] [--area] [--only AREA]
  cpuprofile_report.py PAGE.cpuprofile --kinds [--list KIND]

(the build linked with --profiling-funcs, e.g. -DCMAKE_EXE_LINKER_FLAGS=--profiling-funcs,
so that the functions have their names)

Self time from the samples' leaf nodes, by function name (the module's
name section: the C functions' own names), with the idle and the garbage
collector as node reports them.

--kinds sorts a page's profile (web_perf.mjs --profile-at, Chromium) by
the kind of work, from each sample's stack: the renderer (anything under
gfx_*), the sound (the microcode, the AI, SDL's audio or the worklet's
glue), the input (SDL's joysticks and events), the engine, the game's C
and the host by where their functions are defined, other wasm (libc,
SDL, libaudio), and on the JavaScript side the loop's sleeps and yields
(Asyncify's handleSleep, the rewinds) and the fiber switches
(emscripten_fiber_swap's trampoline); "(program)" is the browser's own.
--list KIND shows a kind's stacks.
"""
import collections
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sprof_report import defined_names  # noqa: E402


AUDIO = {'host_audio_task', 'host_ai_submit', 'HandleAudioProcess', 'SDL_BufferQueueDrainCallback',
         'FeedAudioDevice', 'SDL_ResampleAudio', 'SDL_AudioStreamGet', 'SDL_AudioStreamPut', 'SDL_QueueAudio',
         'SDL_GetQueuedAudioSize', 'SDL_ClearQueuedAudio', 'web_audio_push'}
INPUT = {'SDL_JoystickUpdate', 'SDL_PumpEvents', 'SDL_PollEvent', 'SDL_GameControllerUpdate', 'Emscripten_PumpEvents'}
FIBER = {'emscripten_fiber_swap', '_emscripten_fiber_swap', 'finishContextSwitch', 'trampoline', 'f_run', 'f_yield'}
SLEEP = {'handleSleep', '__asyncjs__wait_display', '__asyncjs__yield_now', 'emscripten_sleep', 'paced_wait',
         'maybeStopUnwind', 'doRewind', 'callUserCallback'}


def kinds(path):
    names = defined_names()
    prof = json.load(open(path))
    nodes = {n["id"]: n for n in prof["nodes"]}
    parent = {c: n["id"] for n in prof["nodes"] for c in n.get("children", [])}
    deltas = prof.get("timeDeltas", [])

    def kind(nid):
        st = []
        p = nid
        while p is not None:
            st.append(nodes[p]["callFrame"]["functionName"] or "(anonymous)")
            p = parent.get(p)
        leaf = st[0]
        if leaf in ("(idle)", "(program)", "(garbage collector)", "(root)"):
            return leaf, st
        if any(f in AUDIO or f.startswith("SDL_") and "Audio" in f or "onaudioprocess" in f for f in st):
            return "sound", st
        if any(names.get(f) == "renderer" or f.startswith("gfx_") for f in st):
            return "renderer", st
        if any(f in INPUT or "Joystick" in f or "amepad" in f for f in st):
            return "input", st
        if names.get(leaf) in ("engine", "game C", "host"):
            return names[leaf], st
        if leaf.startswith(("func_", "os", "__os")):
            return "game C", st
        if "wasm" in nodes[nid]["callFrame"]["url"] and leaf not in FIBER and not leaf.startswith("emscripten_"):
            return "other wasm", st
        if any(f in FIBER for f in st[:12]):
            return "fiber switches", st
        if any(f in SLEEP for f in st[:12]):
            return "loop sleeps/yields", st
        return "other JS", st

    tot = collections.Counter()
    stacks = collections.defaultdict(collections.Counter)
    total = 0
    for k, nid in enumerate(prof["samples"]):
        dt = deltas[k + 1] if k + 1 < len(deltas) else 0
        c, st = kind(nid)
        tot[c] += dt
        stacks[c][" < ".join(st[:5])] += dt
        total += dt
    busy = total - tot["(idle)"]
    print("%.2f s sampled, %.2f s busy" % (total / 1e6, busy / 1e6))
    for c, us in tot.most_common():
        if c != "(idle)":
            print("  %-20s %6.2f%% of the busy time" % (c, 100.0 * us / busy))
    if "--list" in sys.argv:
        c = sys.argv[sys.argv.index("--list") + 1]
        for st, us in stacks[c].most_common(40):
            print("%6.2f%%  %s" % (100.0 * us / busy, st))


def main():
    if "--kinds" in sys.argv:
        kinds([a for a in sys.argv[1:] if a.endswith(".cpuprofile")][0])
        return
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    top = 40
    if "--top" in sys.argv:
        top = int(sys.argv[sys.argv.index("--top") + 1])
        args.remove(str(top))
    only = sys.argv[sys.argv.index("--only") + 1] if "--only" in sys.argv else None
    if only:
        args.remove(only)
    self_us = collections.Counter()
    total = 0
    for path in args:
        prof = json.load(open(path))
        nodes = {n["id"]: n for n in prof["nodes"]}
        deltas = prof.get("timeDeltas", [])
        samples = prof["samples"]
        for k, nid in enumerate(samples):
            dt = deltas[k + 1] if k + 1 < len(deltas) else 0
            name = nodes[nid]["callFrame"]["functionName"] or "(anonymous)"
            self_us[name] += dt
            total += dt
    print("%.2f s sampled" % (total / 1e6))
    names = defined_names()
    if "--area" in sys.argv:
        areas = collections.Counter()
        for name, us in self_us.items():
            areas[names.get(name, "other")] += us
        for a, us in areas.most_common():
            print("  %-10s %6.2f%%" % (a, 100.0 * us / total))
    for name, us in self_us.most_common():
        if only and names.get(name, "other") != only:
            continue
        print("%7.2f%%  %s" % (100.0 * us / total, name))
        top -= 1
        if top == 0:
            break


if __name__ == "__main__":
    main()
