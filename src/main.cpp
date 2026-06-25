#include <cmath>
#include <vector>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

// 10x10 display circular

// ----------------------DISPLAY----------------------



// ----------------------VARS----------------------

/** 60 frames updated per second */
#define DT 0.0166666667f

/** Amount of screen that will be water */
#define PERCENT_WATER 0.3f

/** Amount of coordinates from left to right */
#define GRID_AMT 16

/** Force of gravity in simulation */
#define GRAVITY 100

/** 
 * Lean towards which simulation scale
 * Higher -> more FLIP Sim (more alive) | Lower -> more PIC Sim (more smooth) 
 */
#define ALPHA 0.90f

/**
 * This is the amount of drop off from particles hitting a wall
 * Higher -> particles bouncier | Lower -> particles smoother 
 */
#define RESTITUTION 0.6f

/** Particles want to be about 1 unit of cell apart */
#define MIN_DIST 0.9f

/** How far to calculate for cohesion */
#define COHESION_REACH 1.0f

/** Strength of cohesion */
#define COHESION_STRENGTH 1.0f

// ----------------------GRID VARS----------------------
/** Position of the center of grid */
float CENTER;

/** Radius of the grid from center */
float RADIUS;

/** Number of particles and scales with grid amount */
int NUM_PARTICLES;

// ----------------------STRUCTS----------------------

/**
 * Particle structures for row, col, vy, vx
 */
struct Particle {
  /** Row position of particle */
  float row;

  /** Column position of particle */
  float col;

  /** Velocity in y direction of particle */
  float vy;

  /** Velocity in x direction of particle */
  float vx;
};

/**
 * The state of the simulation that is being updated for the particles,
 * grid velocities, weight for the grid, and the old velocities.
 */
struct SimState {
  /** Growing array of particles in simulation */
  std::vector<Particle> particles;

  /** Grid of velocity in x direction */
  float gridVx[GRID_AMT][GRID_AMT];

  /** Grid of velocity in y direction */
  float gridVy[GRID_AMT][GRID_AMT];

  /** Grid of weights of particles */
  float weight[GRID_AMT][GRID_AMT];

  /** Grid of old velocity in x direction */
  float gridVxOld[GRID_AMT][GRID_AMT];

  /** Grid of old velocity in y direction */
  float gridVyOld[GRID_AMT][GRID_AMT];
};

/**
 * Direction of gravity vectors
 */
struct GravityDir {
  /** Gravity vector in x direction */
  float gravX = 0.0f;

  /** Gravity vector in y direction */
  float gravY = 1.0f;
};

// ----------------------STRUCT INITS----------------------

/** Initialize global state struct */
SimState state;

/** Initialize global GravityDir struct */
GravityDir gd;


// ----------------------HELPERS----------------------

/**
 * Initialize the particles in a grid on the display
 */
void initializeParticles() {
  const int blockW = 9;   // Particles per row
  const int startCol = 3; // Left edge
  const int startRow = 3; // Top edge

  CENTER = (GRID_AMT - 1) / 2.0f;
  RADIUS = (GRID_AMT - 1) / 2.0f + 0.3;
  NUM_PARTICLES = round(GRID_AMT * GRID_AMT * PERCENT_WATER);

  for (int i = 0; i < NUM_PARTICLES; i++) {
    Particle p;
    p.col = startCol + (i % blockW);
    p.row = startRow + (i / blockW);
    p.vx = 0;
    p.vy = 0;
    state.particles.push_back(p);
  }
}

/**
 * Gets float of point on the grid at i, j.
 */
float get(float grid[][GRID_AMT], int i, int j) {
  if (i < 0 || i >= GRID_AMT || j < 0 || j >= GRID_AMT) return 0.0f;
  return grid[i][j];
}

/**
 * Copies a grid to another grid
 */
void copyGrid(float source[][GRID_AMT], float destination[][GRID_AMT]) {
  for (int i = 0; i < GRID_AMT; i++) {
    for (int j = 0; j < GRID_AMT; j++) {
      destination[i][j] = source[i][j];
    }
  }
}

/**
 * Makes a 2D array full of zeros
 */
void makeGrid(float grid[][GRID_AMT]) {
  for (int i = 0; i < GRID_AMT; i++) {
    for (int j = 0; j < GRID_AMT; j++) {
      grid[i][j] = 0.0f;
    }
  }
}

/**
 * Adds a particle's weighted velocity into a single cell
 */
void splat(int i, int j, float weight, float vx, float vy) {
  if (i < 0 || i >= GRID_AMT || j < 0 || j >= GRID_AMT) return; // Skips corners off grid
  state.gridVx[i][j] += weight * vx;
  state.gridVy[i][j] += weight * vy;
  state.weight[i][j] += weight;
}

/**
 * Read a value out of a grid at a floating column and row
 */
float sample(float grid[][GRID_AMT], float col, float row) {
  float i = floor(col), j = floor(row);
  float fx = col - i, fy = row - j;
  return get(grid, i, j) * (1 - fx) * (1 - fy)
       + get(grid, i + 1, j) * fx * (1 - fy)
       + get(grid, i, j + 1) * (1 - fx) * fy
       + get(grid, i + 1, j + 1) * fx * fy;
}

/**
 * Turns particles to grid
 */
void particlesToGrid() {
  // Clean grid first with 0s
  makeGrid(state.gridVx);
  makeGrid(state.gridVy);
  makeGrid(state.weight);

  // Scatter each particle into 4 surrounding cells
  for (Particle& p: state.particles) {
    float i = floor(p.col), j = floor(p.row); // Lower left 
    float fx = p.col - i, fy = p.row - j; // Leftover fraction

    // 4 bilinear weights
    splat(i, j, (1 - fx) * (1 - fy), p.vx, p.vy);
    splat(i + 1, j, fx * (1 - fy), p.vx, p.vy);
    splat(i, j + 1, (1 - fx) * fy, p.vx, p.vy);
    splat(i + 1, j + 1, fx * fy, p.vx, p.vy);
  }

  // Turn the sums into weighted averages
  for (int i = 0; i < GRID_AMT; i++) {
    for (int j = 0; j < GRID_AMT; j++) {
      if (state.weight[i][j] > 0) {
        state.gridVx[i][j] /= state.weight[i][j];
        state.gridVy[i][j] /= state.weight[i][j];
      }
    }
  }
}

/**
 * Turns to grid back to particles
 */
void gridToParticles() {
  for (Particle& p: state.particles) {
    float vxNew = sample(state.gridVx, p.col, p.row);
    float vyNew = sample(state.gridVy, p.col, p.row);
    float vxOld = sample(state.gridVxOld, p.col, p.row);
    float vyOld = sample(state.gridVyOld, p.col, p.row);

    float flipVx = p.vx + (vxNew - vxOld);
    float flipVy = p.vy + (vyNew - vyOld);
    float picVx = vxNew;
    float picVy = vyNew;

    p.vx = ALPHA * flipVx + (1 - ALPHA) * picVx;
    p.vy = ALPHA * flipVy + (1 - ALPHA) * picVy;
  }
}

/**
 * Pushes particles apart based on distance with eachother
 */
void pushParticlesApart() {
  const float minDist2 = MIN_DIST * MIN_DIST;
  const int iterations = 2;
  auto& ps = state.particles;

  for (int iterate = 0; iterate < iterations; iterate++) {
    for (int a = 0; a < ps.size(); a++) {
      for (int b = a + 1; b < ps.size(); b++) {             // each pair once
        float dx = ps[b].col - ps[a].col;
        float dy = ps[b].row - ps[a].row;
        float d2 = dx * dx + dy * dy;
        if (d2 > minDist2 || d2 == 0) continue;             // far enough, or exactly stacked
        float d = sqrt(d2);
        float push = (MIN_DIST - d) * 0.5;                   // split the overlap between them
        float nx = dx / d, ny = dy / d;                     // direction from a → b
        ps[a].col -= nx * push; ps[a].row -= ny * push;     // a backs off
        ps[b].col += nx * push; ps[b].row += ny * push;         // b backs off the other way
      }
    }
  }
}

/**
 * Applies cohesion to each particle
 */
void applyCohesion(float dt) {
  auto& ps = state.particles;

  for (int a = 0; a < ps.size(); a++) {
    for (int b = a + 1; b < ps.size(); b++) {
      float dx = ps[b].col - ps[a].col;
      float dy = ps[b].row - ps[a].row;
      float d = hypot(dx, dy);
      if (d <= MIN_DIST || d >= COHESION_REACH) continue;

      float t = (d - MIN_DIST) / (COHESION_REACH - MIN_DIST); // 0 at MIN_DIST → 1 at COHESION_REACH
      float falloff = 1 - fabs(2 * t - 1);
      float f = COHESION_STRENGTH * falloff * dt;
      float nx = dx / d, ny = dy / d;                         // direction a -> b

      ps[a].vx += nx * f; ps[a].vy += ny * f;                 // a pulled toward b
      ps[b].vx -= nx * f; ps[b].vy -= ny * f;                 // b pulled toward a
    }
  }
}

// ----------------------UPDATE----------------------

/**
 * Updates all particles on the grid
 */
void update(float dt) {
  particlesToGrid();
  copyGrid(state.gridVx, state.gridVxOld);
  copyGrid(state.gridVy, state.gridVyOld);

  // Start particles in the middle for however many there are
  for (int i = 0; i < GRID_AMT; i++) {
    for (int j = 0; j < GRID_AMT; j++) {
      if (state.weight[i][j] > 0) {
        state.gridVx[i][j] += gd.gravX * GRAVITY * dt;
        state.gridVy[i][j] += gd.gravY * GRAVITY * dt;
      }
    }
  }

  gridToParticles();
  applyCohesion(dt);
  pushParticlesApart();

  for (Particle& p: state.particles) {
    // Calculates for new velocity
    p.row += p.vy * dt;
    p.col += p.vx * dt;

    // Distance from center
    float dx = p.col - CENTER, dy = p.row - CENTER;
    float dist = hypot(dx, dy);

    if (dist > RADIUS) {
      // If out of bounds brings back to the edge
      float newRow = CENTER + (dy / dist) * RADIUS;
      float newCol = CENTER + (dx / dist) * RADIUS;
      p.col = newCol;
      p.row = newRow;

      float nx = dx / dist, ny = dy / dist; // Normalize to find the radial direction
      float vDotN = p.vx * nx + p.vy * ny;
      if(vDotN > 0) {
        // Reflects in opposite direction
        p.vx -= (1 + RESTITUTION) * vDotN * nx;
        p.vy -= (1 + RESTITUTION) * vDotN * ny;
      }
    }
  }
}

// ----------------------RENDER----------------------

/**
 * Renders the particles to the display
 */
void render() {

}

// ----------------------ARDUINO----------------------

/**
 * Allocates the grids and the sets the particle array, seed the particles
 * into a starting position, initializes the display driver.
 */
void setup() {
  // init the display driver
  // seed particles into a starting region (a disc of water)
  initializeParticles();

  // allocate grids + particle array
  makeGrid(state.gridVx);
  makeGrid(state.gridVy);
  makeGrid(state.gridVxOld);
  makeGrid(state.gridVyOld);
  makeGrid(state.weight);
}

/**
 * Advances the physics by one step, and draws the particles to the screen.
 */
void loop() {
  update(DT);   // advance physics one step
  render();     // draw particles to the round screen
}