// FluidSim2d — browser harness (the "hardware"). Browser-only; does NOT port to the ESP32.
// Loaded AFTER sim.js; builds the LED panel and calls setup()/update()/render().

/* =====================================================================
   ▼▼▼  ENGINE BELOW — the "hardware". You shouldn't need to edit it. ▼▼▼
   Builds the circular LED layout, the serpentine data chain, the
   setLED/isLED/ledIndex API, runs the loop, and estimates current.
   ===================================================================== */
(() => {
  const CV = 480, CTR = CV / 2, R = 236;          // panel radius (px) — thin bezel beyond
  const MA_PER_LED_WHITE = 60;                     // ~3×20mA, 5050-class
  const screen = document.getElementById('screen');
  const ctx = screen.getContext('2d');
  const el = id => document.getElementById(id);

  // grid size is adjustable at runtime (10–24); the layout rebuilds when it changes
  let GRID = 10, PITCH, KEEP, activeIdx, leds, colors;
  function buildLayout() {
    PITCH = (R - 8) / ((GRID - 1) / 2 + 0.34);     // pitch tightens as the grid grows
    KEEP  = PITCH * ((GRID - 1) / 2 + 0.35);        // keep LEDs whose centre lands in the disc
    activeIdx = Array.from({ length: GRID }, () => new Array(GRID).fill(-1));
    leds = [];                                      // index === position in the data chain
    for (let row = 0; row < GRID; row++) {
      const ltr = (row % 2 === 0);                  // serpentine: alternate direction per row
      for (let k = 0; k < GRID; k++) {
        const col = ltr ? k : (GRID - 1 - k);
        const cx = CTR + (col - (GRID - 1) / 2) * PITCH;
        const cy = CTR + (row - (GRID - 1) / 2) * PITCH;
        if (Math.hypot(cx - CTR, cy - CTR) <= KEEP) {
          activeIdx[col][row] = leds.length;
          leds.push({ col, row, cx, cy });
        }
      }
    }
    colors = new Uint8ClampedArray(leds.length * 3);
    window.Matrix = { COLS: GRID, ROWS: GRID, count: leds.length };
    el('sCount').textContent = leds.length;
  }

  // --- API exposed to the student code -------------------------------
  window.pointer = { col: -1, row: -1, down: false };
  window.isLED = (c, r) => c >= 0 && c < GRID && r >= 0 && r < GRID && activeIdx[c][r] >= 0;
  window.ledIndex = (c, r) => (window.isLED(c, r) ? activeIdx[c][r] : -1);
  window.clearLEDs = () => colors.fill(0);
  window.setLED = (c, r, red, g, b) => {
    if (!window.isLED(c, r)) return;
    const i = activeIdx[c][r] * 3;
    colors[i] = red; colors[i+1] = g; colors[i+2] = b;
  };

  // --- presentation --------------------------------------------------
  let useDiffuser = true, showChain = false, rotation = 0;   // rotation in radians
  // "down" expressed in the grid's own frame — this is what the sim should follow
  const gravityDir = window.gravityDir = { x: 0, y: 1 };
  function setRotation(rad) {
    rotation = rad;
    gravityDir.x = Math.sin(rad);
    gravityDir.y = Math.cos(rad);
  }
  function present() {
    ctx.clearRect(0, 0, CV, CV);
    // panel + bezel (a circle, so rotation leaves it unchanged)
    ctx.fillStyle = '#05070a'; ctx.beginPath(); ctx.arc(CTR, CTR, R, 0, 7); ctx.fill();

    ctx.save();   // spin the whole LED layout — like physically turning the watch
    ctx.translate(CTR, CTR); ctx.rotate(rotation); ctx.translate(-CTR, -CTR);

    // data chain path
    if (showChain) {
      ctx.strokeStyle = 'rgba(90,209,168,.35)'; ctx.lineWidth = 1.5;
      ctx.beginPath();
      leds.forEach((l, i) => i ? ctx.lineTo(l.cx, l.cy) : ctx.moveTo(l.cx, l.cy));
      ctx.stroke();
      ctx.fillStyle = '#5ad1a8';
      ctx.beginPath(); ctx.arc(leds[0].cx, leds[0].cy, 4, 0, 7); ctx.fill();  // chain start
    }

    let litMa = 0, litN = 0;
    ctx.globalCompositeOperation = useDiffuser ? 'lighter' : 'source-over';
    for (let n = 0; n < leds.length; n++) {
      const l = leds[n], i = n * 3, rr = colors[i], gg = colors[i+1], bb = colors[i+2];
      const on = (rr + gg + bb) > 0;
      if (on) { litN++; litMa += (rr + gg + bb) / 765 * MA_PER_LED_WHITE; }

      if (!on) {                                   // unlit LED: faint marker
        ctx.globalCompositeOperation = 'source-over';
        ctx.fillStyle = '#10161d';
        ctx.beginPath(); ctx.arc(l.cx, l.cy, PITCH * 0.16, 0, 7); ctx.fill();
        ctx.globalCompositeOperation = useDiffuser ? 'lighter' : 'source-over';
        continue;
      }
      if (useDiffuser) {                           // soft glow ≈ a diffuser layer
        const g = ctx.createRadialGradient(l.cx, l.cy, 0, l.cx, l.cy, PITCH * 0.85);
        g.addColorStop(0, `rgba(${rr},${gg},${bb},0.95)`);
        g.addColorStop(0.5, `rgba(${rr},${gg},${bb},0.35)`);
        g.addColorStop(1, 'rgba(0,0,0,0)');
        ctx.fillStyle = g;
        ctx.beginPath(); ctx.arc(l.cx, l.cy, PITCH * 0.85, 0, 7); ctx.fill();
      } else {                                     // raw emitter
        ctx.fillStyle = `rgb(${rr},${gg},${bb})`;
        ctx.beginPath(); ctx.arc(l.cx, l.cy, PITCH * 0.3, 0, 7); ctx.fill();
      }
    }
    ctx.globalCompositeOperation = 'source-over';
    ctx.restore();   // back to the un-rotated frame

    el('sLit').textContent = litN;
    const amps = litMa / 1000;
    const ampEl = el('sAmp');
    ampEl.textContent = amps < 1 ? `${litMa.toFixed(0)} mA` : `${amps.toFixed(2)} A`;
    ampEl.className = 'v ' + (amps < 1 ? 'good' : amps < 2.5 ? 'warn' : 'bad');
  }

  // --- loop / timing -------------------------------------------------
  let running = false, frame = 0, speed = 1, lastTs = 0, fpsAcc = 0, fpsN = 0;
  const BASE_DT = 1 / 60;

  function showError(e) {
    running = false; el('btnPlay').textContent = '▶ Play';
    el('sState').textContent = 'error'; el('sState').className = 'v bad';
    const box = el('err'); box.style.display = 'block';
    box.textContent = (e && e.stack) ? e.stack : String(e);
  }
  const clearError = () => el('err').style.display = 'none';

  function stepOnce(dt) {
    const t0 = performance.now();
    try { update(dt); render(); } catch (e) { showError(e); return; }
    const ms = performance.now() - t0;
    present();
    frame++; el('sFrame').textContent = frame;
    const msEl = el('sMs'); msEl.textContent = ms.toFixed(2) + ' ms';
    msEl.className = 'v ' + (ms < 8 ? 'good' : ms < 16 ? 'warn' : 'bad');
  }
  function tick(ts) {
    if (!running) return;
    if (lastTs) { fpsAcc += ts - lastTs; fpsN++;
      if (fpsAcc > 500) { el('sFps').textContent = (1000 * fpsN / fpsAcc).toFixed(0); fpsAcc = 0; fpsN = 0; } }
    lastTs = ts; stepOnce(BASE_DT * speed); requestAnimationFrame(tick);
  }
  function play(){ if (running) return; clearError(); running = true; lastTs = 0;
    el('btnPlay').textContent = '⏸ Pause'; el('sState').textContent = 'running';
    el('sState').className = 'v good'; requestAnimationFrame(tick); }
  function pause(){ running = false; el('btnPlay').textContent = '▶ Play';
    el('sState').textContent = 'paused'; el('sState').className = 'v'; }
  function reset(){ pause(); clearError(); frame = 0; state = {};
    el('sFrame').textContent = '0'; el('sFps').textContent = '—';
    try { setup(); render(); present(); } catch(e){ showError(e); } }

  el('btnPlay').onclick = () => running ? pause() : play();
  el('btnStep').onclick = () => { if (running) pause(); clearError(); stepOnce(BASE_DT * speed); };
  el('btnReset').onclick = reset;
  el('speed').oninput = e => { speed = e.target.value / 100; el('speedV').textContent = speed.toFixed(1) + '×'; };
  el('diff').onchange = e => { useDiffuser = e.target.checked; present(); };
  el('chain').onchange = e => { showChain = e.target.checked; present(); };
  el('rot').oninput = e => {
    setRotation(e.target.value * Math.PI / 180);
    el('rotV').textContent = e.target.value + '°';
    el('sGrav').textContent = `(${gravityDir.x.toFixed(2)}, ${gravityDir.y.toFixed(2)})`;
    present();
  };
  el('grid').oninput = e => {
    const wasRunning = running;
    GRID = parseInt(e.target.value, 10);
    el('gridV').textContent = GRID + '×' + GRID;
    buildLayout();      // rebuild the LED panel for the new size
    reset();            // re-run setup() so the sim reallocates + respawns
    if (wasRunning) play();
  };

  // pointer → nearest LED grid coords
  function setPtr(ev, down) {
    const r = screen.getBoundingClientRect();
    const x = (ev.clientX - r.left) / r.width * CV;
    const y = (ev.clientY - r.top) / r.height * CV;
    // un-rotate the click back into the grid frame before finding the cell
    const ox = x - CTR, oy = y - CTR, c = Math.cos(rotation), s = Math.sin(rotation);
    const ux = ox * c + oy * s, uy = -ox * s + oy * c;
    const col = Math.round(ux / PITCH + (GRID - 1) / 2);
    const row = Math.round(uy / PITCH + (GRID - 1) / 2);
    pointer.col = window.isLED(col, row) ? col : -1;
    pointer.row = pointer.col >= 0 ? row : -1;
    if (down !== undefined) pointer.down = down;
  }
  screen.addEventListener('mousemove', e => setPtr(e));
  screen.addEventListener('mousedown', e => setPtr(e, true));
  window.addEventListener('mouseup',   e => setPtr(e, false));

  buildLayout();   // build the initial LED panel (also sets the LED count)
  reset();
})();
