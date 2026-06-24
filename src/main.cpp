#include cmath

// 10x10 display circular

/** 60 frames updated per second */
#define dt 0.0166666667f
/** Amount of particles from left to right */
#define GRID_AMT 16
/** Force of gravity in simulation */
#define GRAVITY 100
/** Higher -> mroe FLIP Sim | Lower -> more SPH Sim */
#define ALPHA 0.90f
/** Higher -> particles bouncier | Lower -> particles smoother */
#define RESTITUTION 0.6f
/** Particles want to be about 1 unit of cell apart */
#define MIN_DIST 0.9f
/** How far to calculate for cohesion */
#define COHESION_REACH 1.0f
/** Strength of cohesion */
#define COHESION_STRENGTH 1.0f

// Initialize Grid Size Variables
/** Position of the center of grid */
int CENTER;
/** Radius of the grid from center */
int RADIUS;
/** Number of particles and scales with grid amount */
int NUM_PARTICLES;

/** Initialize global state variable */
SimState state;

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

// ----------------------MATH----------------------

/**
 * 
 */
void update(float dt) {
  
}

// ----------------------HELPERS----------------------

/**
 * 
 */
void makeGrid() {

}

/**
 * 
 */
void get() {

}

/**
 * 
 */
void copyGrid() {

}

/**
 * 
 */
void particlesToGrid() {

}

/**
 * 
 */
void splat() {

}

/**
 * 
 */
void sample() {

}

/**
 * 
 */
void gridToParticles() {

}

/**
 * 
 */
void pushParticlesApart() {

}

/**
 * 
 */
void applyCohesion() {

}

// ----------------------RENDER----------------------

/**
 * 
 */
void render() {

}

// ----------------------STRUCTS----------------------

/**
 * Particle structures for row, col, vy, vx
 */
struct Particle {
  /** Row position of particle */
  int row;
  /** Column position of particle */
  int col;
  /** Velocity in y direction of particle */
  float vy;
  /** Velocity in x direction of particle */
  float vx;
}

/**
 * The state of the simulation that is being updated for the particles,
 * grid velocities, weight for the grid, and the old velocities.
 */
struct SimState {
  /** Growing array of particles in simulation */
  std::vector<Particle> particles;
  /**  */
  float gridVx;
  /**  */
  float gridVy;
  /**  */
  float weight;
  /**  */
  float gridVxOld;
  /**  */
  float gridVyOld;
}
