// FluidSim2d — the fluid solver. This is the part that ports to src/main.cpp.
// Loaded by learn/led-matrix-10.html BEFORE engine.js (it defines setup/update/render).
// Globals provided by engine.js: Matrix, gravityDir, setLED, clearLEDs, isLED, pointer.

const GRAVITY = 100;       // Gravity
const ALPHA = 0.90;       // Higher -> more FLIP | Lower -> more SPH
const RESTITUTION = 0.6;  // Higher -> particles bouncier | Lower -> particles smoother
const minDist = 0.9;      // particles want to be ~1 cell apart
const h = 1.0;            // Cohesion reach > minDist
const strength = 1;       // Pull strength for cohesion

// these follow the grid-size slider (10–24) — set in setup() from Matrix.COLS
let GRID_AMT = 16;        // Size of grid
let CENTER;               // Position of center  = (GRID_AMT - 1) / 2
let RADIUS;               // bowl radius         ≈ half the grid
let NUM_PARTICLES;        // scales with grid area so density stays ~constant

// makes a fresh N×N 2D array full of zeros
function makeGrid() {
  return Array.from({ length: GRID_AMT }, () => new Array(GRID_AMT).fill(0));
}

// read a grid cell, returning 0 if it's off the edge (avoids crashes)
function get(grid, i, j) {
  if (i < 0 || i >= GRID_AMT || j < 0 || j >= GRID_AMT) return 0;
  return grid[i][j];
}

// copy one grid into another (for saving the "before" state)
function copyGrid(src, dst) {
  for (let i = 0; i < GRID_AMT; i++)
    for (let j = 0; j < GRID_AMT; j++)
      dst[i][j] = src[i][j];
}

// Does P2G, particles to grid
function particlesToGrid() {
  // Wipe grid clean first
  for (let i = 0; i < GRID_AMT; i++) {
    for (let j = 0; j < GRID_AMT; j++) {
      state.gridVx[i][j] = state.gridVy[i][j] = state.weight[i][j] = 0;
    }
  }

  // Scatter each particle into 4 surrounding cells
  for (let p of state.particles) {
    let i = Math.floor(p.col), j = Math.floor(p.row); // Lower left 
    let fx = p.col - i, fy = p.row - j; // Leftover fraction

    // 4 bilinear weights
    splat(i, j, (1 - fx) * (1 - fy), p.vx, p.vy);
    splat(i + 1, j, fx * (1 - fy), p.vx, p.vy);
    splat(i, j + 1, (1 - fx) * fy, p.vx, p.vy);
    splat(i + 1, j + 1, fx * fy, p.vx, p.vy);
  }

  // Turn the sums into weighted averages
  for (let i = 0; i < GRID_AMT; i++) {
    for (let j = 0; j < GRID_AMT; j++) {
      if (state.weight[i][j] > 0) {
        state.gridVx[i][j] /= state.weight[i][j];
        state.gridVy[i][j] /= state.weight[i][j];
      }
    }
  }
}

// adds one particle's weighted velocity into a single cell
function splat(i, j, w, vx, vy) {
  if (i < 0 || i >= GRID_AMT || j < 0 || j >= GRID_AMT) return;  // skip off-grid corners
  state.gridVx[i][j] += w * vx;
  state.gridVy[i][j] += w * vy;
  state.weight[i][j] += w;
}

// Bilinearly read a value out of a grid at a floating col and row
function sample(grid, col, row) {
  let i = Math.floor(col), j = Math.floor(row);
  let fx = col - i, fy = row - j;
  return get(grid, i, j) * (1 - fx) * (1 - fy)
       + get(grid, i + 1, j) * fx * (1 - fy)
       + get(grid, i, j + 1) * (1 - fx) * fy
       + get(grid, i + 1, j + 1) *  fx *  fy;
}

function gridToParticles() {
  for (let p of state.particles) {
    let vxNew = sample(state.gridVx,    p.col, p.row);
    let vyNew = sample(state.gridVy,    p.col, p.row);
    let vxOld = sample(state.gridVxOld, p.col, p.row);
    let vyOld = sample(state.gridVyOld, p.col, p.row);

    let flipVx = p.vx + (vxNew - vxOld);   // FLIP: add the grid's CHANGE
    let flipVy = p.vy + (vyNew - vyOld);
    let picVx = vxNew;                       // PIC: just take the grid value
    let picVy = vyNew;

    p.vx = ALPHA * flipVx + (1 - ALPHA) * picVx;
    p.vy = ALPHA * flipVy + (1 - ALPHA) * picVy;
  }
}

function pushParticlesApart() {
  const minDist2 = minDist * minDist;
  const iterations = 2;             // relax a couple of times for a nicer spread
  const ps = state.particles;

  for (let iter = 0; iter < iterations; iter++) {
    for (let a = 0; a < ps.length; a++) {
      for (let b = a + 1; b < ps.length; b++) {       // each pair once
        let dx = ps[b].col - ps[a].col;
        let dy = ps[b].row - ps[a].row;
        let d2 = dx*dx + dy*dy;
        if (d2 > minDist2 || d2 === 0) continue;      // far enough, or exactly stacked
        let d = Math.sqrt(d2);
        let push = (minDist - d) * 0.5;               // split the overlap between them
        let nx = dx / d, ny = dy / d;                 // direction from a → b
        ps[a].col -= nx * push;  ps[a].row -= ny * push;   // a backs off
        ps[b].col += nx * push;  ps[b].row += ny * push;   // b backs off the other way
      }
    }
  }
}

function applyCohesion(dt) {
  const ps = state.particles;

  for (let a = 0; a < ps.length; a++) {
    for (let b = a + 1; b < ps.length; b++) {
      let dx = ps[b].col - ps[a].col;
      let dy = ps[b].row - ps[a].row;
      let d = Math.hypot(dx, dy);
      if (d <= minDist || d >= h) continue;

      let t = (d - minDist) / (h - minDist);         // 0 at minDist → 1 at h
      let falloff = 1 - Math.abs(2 * t - 1);         // tent: 0 → 1 → 0 across the band
      let f = strength * falloff * dt;
      let nx = dx / d, ny = dy / d;                  // direction a → b

      ps[a].vx += nx * f;  ps[a].vy += ny * f;       // a pulled toward b
      ps[b].vx -= nx * f;  ps[b].vy -= ny * f;       // b pulled toward a
    }
  }
}

let state = {};

function setup() {
  // pick up the current grid size from the harness (the "grid" slider, 10–24)
  GRID_AMT = Matrix.COLS;
  CENTER = (GRID_AMT - 1) / 2;
  RADIUS = (GRID_AMT - 1) / 2 + 0.3;
  NUM_PARTICLES = Math.round(GRID_AMT * GRID_AMT * 0.4);   // keep density ~constant

  state.particles = [];
  for (let i = 0; i < NUM_PARTICLES; i++) {
    let col, row;
    // keep retrying until the spawn lands inside the bowl
    do {
      col = Math.random() * (GRID_AMT - 1);
      row = Math.random() * (GRID_AMT - 1);
    } while (Math.hypot(col - CENTER, row - CENTER) > RADIUS);

    // Add to list
    state.particles.push({ row: row, vy: 0, col: col, vx: 0});
  }

  state.gridVx = makeGrid();
  state.gridVy = makeGrid();
  state.weight = makeGrid();
  state.gridVxOld = makeGrid();
  state.gridVyOld = makeGrid();
}

function update(dt) {
  particlesToGrid();
  copyGrid(state.gridVx, state.gridVxOld);
  copyGrid(state.gridVy, state.gridVyOld);

  for (let i = 0; i < GRID_AMT; i++) {
    for (let j = 0; j < GRID_AMT; j++) {
      if (state.weight[i][j] > 0) {
        state.gridVx[i][j] += gravityDir.x * GRAVITY * dt;
        state.gridVy[i][j] += gravityDir.y * GRAVITY * dt;
      }
    }
  }

  gridToParticles();
  applyCohesion(dt);
  pushParticlesApart();

  for (let p of state.particles) {
    p.row += p.vy * dt;
    p.col += p.vx * dt;

    // Distance from center 
    let dx = p.col - CENTER, dy = p.row - CENTER;
    let dist = Math.hypot(dx, dy);

    if (dist > RADIUS) {
      // If out of bounds, brings back to edge
      let newRow = CENTER + (dy / dist) * RADIUS;
      let newCol = CENTER + (dx / dist) * RADIUS;
      p.col = newCol;
      p.row = newRow;

      let nx = dx / dist, ny = dy / dist; // Normalize to find the radial direction
      let vDotN = p.vx * nx + p.vy * ny;  // How much moving into the wall
      if (vDotN > 0) {
        // Reflects in outward direction
        p.vx -= (1 + RESTITUTION) * vDotN * nx;
        p.vy -= (1 + RESTITUTION) * vDotN * ny;
      }
    }
  }
}

function render() {
  clearLEDs();
  for (let p of state.particles) {
    setLED(Math.round(p.col), Math.round(p.row), 0, 150, 255);
  }
}
