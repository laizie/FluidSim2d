// 10x10 display circular

#define dt 0.01

// ----------------------Arduino----------------------

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

// ----------------------Math----------------------

void update(float dt) {
  particlesToGrid();        // §8  P2G  (+ save uPrev/vPrev)
  markCells();
  addGravity(dt);           // §6.3
  project(MAX_ITERS);       // §7  the expensive one
  gridToParticles(ALPHA);   // §8  G2P  + FLIP/PIC blend
  advect(dt);               // §9  move + circle collision
}

void particlesToGrid() {

}

void markCells() {

}

void addGravity(float dt) {
    
}

void project(int maxIterators) {

}

void gridToParticles(int alpha) {

}

void advect(float dt) {

}

// ----------------------RENDER----------------------

void render() {

}
