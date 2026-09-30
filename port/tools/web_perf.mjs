#!/usr/bin/env node
// The page's timing in a real browser (docs/PORT.md, "Performance").
//
//   node port/tools/web_perf.mjs BUILD_DIR ROM [options]
//
// Serves BUILD_DIR (a -DPORT_WASM_TARGET=web build) and ROM on a free local
// port, opens blastcorps.html in headless Chromium or Firefox (Playwright),
// presses Play and lets PORT_AUTOSTART=3 drive into Simian Acres.  It prints
// the port's PORT_PERF lines (the work per retrace and its parts, see
// port/host/perf.c) and, every 5 s, what reached the display: the page's
// animation frames, how many showed a new picture, how many repeated the
// last one, and how many came with two or more (a picture drawn but never
// seen).  At the end, a summary of the windows from --from on.
//
//   --browser chromium|firefox   (default chromium)
//   --gpu                        Chromium on the GPU (ANGLE on Vulkan); without
//                                it SwiftShader, the CPU's GL (pessimistic)
//   --throttle N                 Chromium: CDP's CPU throttling, N times slower
//   --secs S                     how long to play (default 75)
//   --args "..."                 the port's options (default "--interpolate --widescreen")
//   --env "A=1,B=2"              its environment (default PORT_AUTOSTART=3,PORT_PERF=600)
//   --from R                     summarize the windows ending at retrace R and after (default 3000)
//   --out PREFIX                 PREFIX.log gets the page's whole console (default web_perf)
//   --profile-at S --profile-secs N   Chromium: a CPU profile (PREFIX.cpuprofile)
//
// Environment: PLAYWRIGHT_CORE (a node_modules directory holding
// playwright-core, if node can't find it), CHROMIUM (default
// /usr/bin/chromium), FIREFOX (a Playwright Firefox, default the newest
// under ~/.cache/ms-playwright).
import { createRequire } from 'module';
import http from 'http';
import fs from 'fs';
import os from 'os';
import path from 'path';

const argv = process.argv.slice(2);
const opt = { browser: 'chromium', gpu: false, throttle: 1, secs: 75, args: '--interpolate --widescreen',
              env: 'PORT_AUTOSTART=3,PORT_PERF=600', from: 3000, out: 'web_perf', 'profile-at': 0, 'profile-secs': 8 };
const pos = [];
for (let i = 0; i < argv.length; i++) {
  const a = argv[i];
  if (a === '--gpu') opt.gpu = true;
  else if (a.startsWith('--')) opt[a.slice(2)] = argv[++i];
  else pos.push(a);
}
if (pos.length < 2) {
  console.error('usage: web_perf.mjs BUILD_DIR ROM [options] (see the top of this file)');
  process.exit(2);
}
const [dir, rom] = pos;
const require = createRequire(process.env.PLAYWRIGHT_CORE ? path.resolve(process.env.PLAYWRIGHT_CORE) + '/' : import.meta.url);
const pw = require('playwright-core');

function firefoxPath() {
  if (process.env.FIREFOX) return process.env.FIREFOX;
  const base = path.join(os.homedir(), '.cache', 'ms-playwright');
  const d = fs.existsSync(base) ? fs.readdirSync(base).filter(n => /^firefox-\d+$/.test(n)).sort().pop() : null;
  return d ? path.join(base, d, 'firefox', 'firefox') : undefined;
}

// the files, and the ROM at /rom.z64
const types = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm' };
const srv = http.createServer((q, r) => {
  const u = new URL(q.url, 'http://x');
  const f = u.pathname === '/rom.z64' ? rom : path.join(dir, path.normalize(u.pathname));
  fs.readFile(f, (e, d) => {
    if (e) { r.writeHead(404); r.end(); return; }
    r.writeHead(200, { 'Content-Type': types[path.extname(f)] || 'application/octet-stream', 'Cache-Control': 'no-store' });
    r.end(d);
  });
});
await new Promise(ok => srv.listen(0, '127.0.0.1', ok));
const log = fs.createWriteStream(opt.out + '.log');
const perf = [], mon = [];
function say(s) {
  log.write(s + '\n');
  if (/^(perf:|\[|gl: |pacing:|fatal)/.test(s)) console.log(s);
  const m = /^perf: to retrace (\d+),.*median ([\d.]+) p95 ([\d.]+) p99 ([\d.]+).*new images ([\d.]+)\/s/.exec(s), n = /^perf: to retrace \d+, (\d+) in/.exec(s);
  if (m) perf.push(m.slice(1).map(Number).concat([+n[1]]));
}

let b;
if (opt.browser === 'firefox') {
  b = await pw.firefox.launch({ executablePath: firefoxPath(), headless: true,
                                firefoxUserPrefs: { 'media.autoplay.default': 0 } });
} else {
  b = await pw.chromium.launch({ executablePath: process.env.CHROMIUM || '/usr/bin/chromium', headless: true,
    args: (opt.gpu ? ['--use-angle=vulkan', '--enable-gpu', '--ignore-gpu-blocklist', '--enable-features=Vulkan']
                   : ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'])
          .concat(['--autoplay-policy=no-user-gesture-required']) });
}
const ctx = await b.newContext({ viewport: { width: 1280, height: 800 } });
const page = await ctx.newPage();
await page.addInitScript(() => {
  /* the display's frames (their animation-frame times), to place the pictures in */
  const m = window.__mon = { frames: [] };
  function tick(t) { m.frames.push(t); requestAnimationFrame(tick); }
  requestAnimationFrame(tick);
});
page.on('console', m => say(m.text()));
page.on('pageerror', e => say('[pageerror] ' + e.message));
const q = new URLSearchParams({ rom: '/rom.z64', env: opt.env, args: opt.args });
await page.goto(`http://127.0.0.1:${srv.address().port}/blastcorps.html?${q}`);
if (opt.browser === 'chromium' && +opt.throttle > 1) {
  const cdp = await ctx.newCDPSession(page);
  await cdp.send('Emulation.setCPUThrottlingRate', { rate: +opt.throttle });
}
await page.waitForFunction(() => !document.getElementById('play').hidden, null, { timeout: 120000 });
await page.click('#play');
const t0 = Date.now();
say(`[web_perf] ${opt.browser} ${b.version()}${opt.gpu ? ' on the GPU' : ''}, CPU throttled ${opt.throttle}x`);
for (let t = 0; t < +opt.secs; t += 5) {
  await page.waitForTimeout(5000);
  const s = await page.evaluate(() => {
    /* the port pushes each new picture's time to Module.shown (perf.c); a
       picture is on screen from the first frame that starts after it was
       drawn, or whose animation callbacks drew it (3 ms of slack) */
    const f = window.__mon.frames, shown = (window.Module && Module.shown) || [];
    if (f.length < 4) return null;
    const upto = f[f.length - 1] - 3, pics = shown.filter(p => p < upto), hist = [0, 0, 0];
    let k = 0;
    for (let i = 1; i < f.length; i++) {
      let n = 0;
      for (; k < pics.length && pics[k] < f[i] + 3; k++) n++;
      hist[Math.min(n, 2)]++;
    }
    if (window.Module) Module.shown = shown.filter(p => p >= upto);
    const gaps = f.slice(1).map((x, i) => x - f[i]).sort((a, b) => a - b);
    window.__mon.frames = f.slice(-1);
    const ac = window.Module && Module.SDL2 && Module.SDL2.audioContext;
    return { frames: f.length - 1, hist, gap50: gaps[gaps.length >> 1], gap99: gaps[Math.floor(gaps.length * 0.99)],
             audio: ac ? `${ac.state} at ${ac.currentTime.toFixed(1)} s` : 'none' };
  });
  if (s && t > 0) {
    const r = x => (x / 5).toFixed(1);
    mon.push({ t: (Date.now() - t0) / 1000, fresh: (s.hist[1] + s.hist[2]) / 5, repeat: s.hist[0] / 5, lost: s.hist[2] / 5 });
    say(`[display] ${((Date.now() - t0) / 1000).toFixed(0)} s: ${r(s.frames)} frames/s (apart p50 ${s.gap50.toFixed(1)} ms, p99 ${s.gap99.toFixed(1)});` +
        ` a new picture in ${r(s.hist[1] + s.hist[2])}/s, the last one again in ${r(s.hist[0])}/s, two or more in ${r(s.hist[2])}/s; sound ${s.audio}`);
  }
  if (+opt['profile-at'] && t + 5 === +opt['profile-at'] && opt.browser === 'chromium') {
    const cdp = await ctx.newCDPSession(page);
    await cdp.send('Profiler.enable');
    await cdp.send('Profiler.setSamplingInterval', { interval: 200 });
    await cdp.send('Profiler.start');
    await page.waitForTimeout(+opt['profile-secs'] * 1000);
    const { profile } = await cdp.send('Profiler.stop');
    fs.writeFileSync(opt.out + '.cpuprofile', JSON.stringify(profile));
    say(`[web_perf] ${opt.out}.cpuprofile`);
  }
}
await b.close();
srv.close();
log.end();

// the summary: the windows from --from on, and the display from the first of them
const w = perf.filter(p => p[0] >= +opt.from);
if (w.length) {
  const med = w.map(p => p[1]).sort((a, b) => a - b)[w.length >> 1];
  const first = w[0][0] - w[0][5], secsFrom = first / 60;     // (the retraces are real time: 60 a second)
  const d = mon.filter(m => m.t >= secsFrom);
  const avg = k => d.length ? (d.reduce((a, m) => a + m[k], 0) / d.length).toFixed(1) : '?';
  console.log(`[summary] retraces ${first}-${w[w.length - 1][0]}: work per retrace median ${med} ms, ` +
              `p95 up to ${Math.max(...w.map(p => p[2]))}, p99 up to ${Math.max(...w.map(p => p[3]))}; ` +
              `drawn ${(w.reduce((a, p) => a + p[4], 0) / w.length).toFixed(1)} new pictures/s; on the display ` +
              `${avg('fresh')}/s new, ${avg('repeat')}/s repeated, ${avg('lost')}/s with a picture never seen`);
}
