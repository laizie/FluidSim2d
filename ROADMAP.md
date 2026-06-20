# Making it feel more like water

---

## Already done 

- Particles falling under gravity, bouncing inside the round bowl
- FLIP transfers — particles → grid (P2G) and back (G2P) with the FLIP/PIC blend
- Incompressibility via **push‑apart** (particles refuse to overlap)
- **Cohesion** / surface tension (droplets ball up; the body holds together; flung
  bits break free)
- Tilt → gravity direction (the `gravityDir` vector — stands in for an accelerometer)
- Tuning knobs that actually mean something: gravity, FLIP `alpha`, restitution
- Resizable grid (10×10 up to 24×24) to feel out resolution
- A 2″ pocket‑watch case around the display, so it reads at real size

---

## 1. Render the body, not the dots

Right now each particle lights one LED, so it reads as scattered specks. Instead,
light each cell by **how much water is in it**, which turns specks into a connected,
glowing body with a surface.

The nice part: that field already exists. `state.weight[i][j]` from P2G *is* a
per‑cell density, and it survives until the next frame's wipe — so `render()` can
just read it.

- **Density → brightness:** brighter where `weight` is high, dark where it's empty.
- **Speed → foam:** take each cell's speed from `hypot(gridVx, gridVy)` - fast cells
  go pale/white (spray and foam), slow cells stay deep blue. Splashes flash white,
  which sells the whole thing.

Biggest visual payoff, moderate effort, mostly reuses data we already compute.

## 2. Viscosity — make it move as one body

Nudge each particle's velocity a little toward its neighbours' average. The mass
then flows **together** instead of as a bag of independent dots, and leftover jitter
calms down. It's a short extra pairwise pass (or just leaning on more PIC in the
blend). Cheap, and it's the difference between "granular pile" and "liquid."


## Smaller tweaks

- **More particles** — denser coverage reads more like a liquid than sparse dots.
- **Substeps** — run the physics a few times per frame with a smaller `dt` so we can
  crank gravity (snappier splashes) without the sim going unstable.

---

## The tradeoffs to remember

- **Lively ↔ calm** is one dial pulled by gravity / FLIP `alpha` / restitution. High
  = splashy and responsive; low = settled and still. Pick per the feel you want.
- **Resolution ↔ payoff:** finer grids show off the pressure solve and smoother flow,
  but cost more compute — which matters a lot once this runs on the ESP32, not just
  the browser.
- **Rendering beats physics at low res.** On a chunky LED panel, #1 above will do more
  for the look than #2 and #3 combined.
