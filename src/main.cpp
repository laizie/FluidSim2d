#include <cmath>
#include <vector>

// 10x10 display circular

/** 60 frames updated per second */
#define dt 0.0166666667f
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

// Initialize Grid Size Variables
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
  /**  */
  float gridVx[GIRD_AMT][GRID_AMT];
  /**  */
  float gridVy[GRID_AMT][GRID_AMT];
  /**  */
  float weight[GRID_AMT][GRID_AMT];
  /**  */
  float gridVxOld[GRID_AMT][GRID_AMT];
  /**  */
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

/** Initialize global state variable */
SimState state;

// ----------------------HELPERS----------------------

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
  for (int i = 0; i < GRID_AMT; i++) {
    for (int j = 0; j < GRID_AMT; j++) {
      grid[i][j] = 0.0f;
    }
  }

  // Scatter each particle into 4 surrounding cells
  for (float p: state.particles) {
    let i = floor(p.col), j = floor(p.row); // Lower left 
    let fx = p.col - i, fy = p.row - j; // Leftover fraction

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
  for (float p: state.particles) {
    float vxNew = sample(state.gridVx, p.col, p.row);
    float vyNew = sample(state.gridVy, p.col, p.row);
    float vxOld = sample(state.gridVxOld, p.col, p.row);
    float vyOld = sample(state.gridVyOld, p.col, p.row);

    float flipVx = p.vx + (vxNew - vxOld);
    float flipVy = p.vy + (vyNew - vyOld);
    float picVx = vxNew;
    float picVy = vyNew;

    p.vx = ALPHA * flipVx + (1 - ALPHA) * picVx;
    p.vx = ALPHA * flipVy + (1 - ALPHA) * picVy;
  }
}

/**
 * Pushes particles apart based on distance with eachother
 */
void pushParticlesApart() {

}

/**
 * 
 */
void applyCohesion() {

}

// ----------------------UPDATE----------------------

/**
 * 
 */
void update(float dt) {
  
}

// ----------------------RENDER----------------------

/**
 * 
 */
void render() {

}

// ----------------------ARDUINO----------------------

/**
 * Allocates the grids and the sets the particle array, seed the particles
 * into a starting position, initializes the display driver.
 */
void setup() {
  // allocate grids + particle array
  // seed particles into a starting region (a disc of water)
  // init the display driver
}

/**
 * Advances the physics by one step, and draws the particles to the screen.
 */
void loop() {
  update(dt);   // advance physics one step
  render();    // draw particles to the round screen
}