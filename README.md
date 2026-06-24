# FluidSim2d

A real‑time **FLIP fluid simulation** for a **2‑inch round LED pocket watch**.

- Claude helped develop an html for being able to see the simulation and make live updates in a browser.
- Personally developed the javascript code to be ran in the html, next is to translate the javascript to c++ code to be ran on an ESP32.
- The plan is to have the physics designed to port cleanly to an ESP32 driving a circular addressable‑LED matrix.

## Run the simulator

No build step. `learn/led-matrix-10.html` loads the solver from `src/sim.js` and the harness from `src/engine.js`.

- **Quickest:** open `learn/led-matrix-10.html` in a browser.
- **With live‑reload while editing:** run `npx live-server` from the **repo root** (so the `src/` scripts resolve), then open `/learn/led-matrix-10.html`.

## How it works

It's a [FLIP](https://en.wikipedia.org/wiki/Fluid_implicit_particle) (Fluid‑Implicit‑Particle) solver:

1. **P2G** — splat particle velocities onto a grid.
2. **Forces** — add gravity (in the direction the watch is tilted).
3. **G2P** — read velocities back to the particles (FLIP/PIC blend).
4. **Advect** — move the particles; bounce them off the circular wall.
5. **Push‑apart + cohesion** — keep particles from overlapping, and let droplets
   ball up and break off (surface tension).

To learn, I had Claude help develop an explanation of flip fluids that are fit for my needs in [`learn/physics-and-code.html`](learn/physics-and-code.html).

## Hardware plan

- **MCU:** ESP32 (hardware FPU, dual‑core, plenty of RAM)
- **Display:** circular addressable‑LED matrix, ~10–24 across in a 2″ circle
- **Tilt:** an accelerometer/IMU (its gravity vector feeds the sim's `gravityDir` directly)
- **Board:** custom round PCB (KiCad project in [`pcb/`](pcb/))

## Status

See [ROADMAP.md](ROADMAP.md) for what's next.

## Credit
Idea credit fully given to [mitxela](https://mitxela.com/projects/fluid-pendante).
First given the idea from his video [here](https://www.youtube.com/watch?v=jis1MC5Tm8k).