#include "game.h"
#include "graphics.h"
#include "input.h"

// Grid is represented as m x n int matrix. Values are color indices for
// occupied cells or 0 for empty cells
static uint8_t grid[GRID_WIDTH][GRID_HEIGHT] = {0};
// Array of rows that need to be destroyed
static uint8_t to_destroy[GRID_HEIGHT] = {0};

static uint8_t game_over = 0;
static uint32_t score = 0;

static uint8_t iteration = 0;
static uint8_t lines_cleared = 0;

static uint8_t current_level = 0;
static uint8_t fall_freq = 48;
static const uint8_t MAX_LEVEL_FREQ = 15;
static const uint8_t LEVEL_FREQS[15] = {48, 43, 38, 33, 28, 23, 18, 13,
                                        8,  6,  5,  4,  3,  2,  1};
static const uint8_t SOFT_FREQ = 3;
static const uint8_t HARD_FREQ = 1;

static const uint8_t N_COLORS = 14;
static const uint32_t COLORS[14] = {
    0x111111, 0xFFC82E, 0xFEFB34, 0x53DA3F, // first el is an empty cell
    0x01EDFA, 0xDD0AB2, 0xEA141C, 0xFE4819, 0xFF910C,
    0x39892F, 0x0077D3, 0x78256F, 0x2E2E84, 0x485DC5,
};

// Array of blocks in the current shape
// Each value pair corresponds to the shift from the shape
// position over x and y axis
static int8_t current_shape[8] = {0};
// Index of the color in the COLORS array
static uint8_t current_shape_color = 0;
// Index of the current shape in the SHAPES array
static uint8_t current_shape_type;

// Current shape coordinates
static int8_t current_x = 0, current_y = 0;

// Represent shapes as an array of 8 ints.
// Each int pair represents the shift from the shape position over x and y axis
static const uint8_t N_SHAPES = 7;
static int8_t SHAPES[7][8] = {
    {0, 0, 1, 0, 0, 1, 1, 1},   // O
    {0, 0, -1, 0, 1, 0, 0, 1},  // T
    {0, 0, 0, -1, 0, 1, 1, 1},  // L
    {0, 0, 0, -1, 0, 1, -1, 1}, // J
    {0, 0, 0, -1, 0, 1, 0, 2},  // I
    {0, 0, 1, 0, 0, 1, -1, 1},  // S
    {0, 0, -1, 0, 0, 1, 1, 1},  // Z
};

static const uint8_t FRAME_DELAY = 16; // 1000 / 16 ~= 60fps
static const uint16_t RESTART_DELAY = 300;

static const uint8_t SCORE_SINGLE = 1;
static const uint8_t SCORE_LINE = 100;

static uint8_t get_curr_fall_freq() {
  if (current_level >= MAX_LEVEL_FREQ) {
    return LEVEL_FREQS[MAX_LEVEL_FREQ - 1];
  }
  return LEVEL_FREQS[current_level];
};

static uint8_t state_changed = 0;

void reset_fall_freq() { fall_freq = get_curr_fall_freq(); }

void update_fall_freq(int new) {
  uint8_t calculated = get_curr_fall_freq();
  if (calculated < new) {
    fall_freq = calculated;
  } else {
    fall_freq = new;
  }
}

void end_game() {
  game_over = 1;
  clear_screen();
}

void spawn_shape() {
  state_changed = 1;

  current_shape_type = rand() % N_SHAPES;
  current_shape_color = rand() % (N_COLORS - 1) + 1;

  for (int8_t i = 0; i < 8; i++) {
    current_shape[i] = SHAPES[current_shape_type][i];
  }

  current_x = GRID_WIDTH / 2;

  // Check for top collisions with existing blocks in the grid
  // If we spot any collision, we'll start with negative current_y
  int8_t x, y;
  for (current_y = -2; current_y < 0; current_y++) {
    for (int8_t i = 0; i < 4; i++) {
      x = current_shape[i * 2] + current_x;
      y = current_shape[i * 2 + 1] + current_y + 1;

      if (y >= 0 && grid[x][y] != 0) {
        return;
      }
    }
  };
}

void restart_game() {
  for (int8_t i = 0; i < GRID_WIDTH; i++) {
    for (int8_t j = 0; j < GRID_HEIGHT; j++) {
      grid[i][j] = 0;
    }
  };

  game_over = 0;
  current_level = 0;
  lines_cleared = 0;
  score = 0;

  spawn_shape();
  SDL_Delay(RESTART_DELAY);
}

void destroy_row(int row) {
  for (int8_t j = row; j > 0; j--) {
    for (int8_t i = 0; i < GRID_WIDTH; i++) {
      grid[i][j] = grid[i][j - 1];
    }
  }
  lines_cleared++;
  if (lines_cleared % 10 == 0) {
    current_level++;
  }
}

void clean_destroyed_blocks() {
  int count = 0;

  for (int8_t j = 0; j < GRID_HEIGHT; j++) {
    if (to_destroy[j]) {
      count++;

      to_destroy[j] = 0;
      for (int8_t i = 0; i < GRID_WIDTH; i++) {
        grid[i][j] = 0;
      }
      destroy_row(j);
    }
  }

  if (count) {
    score += SCORE_LINE * (1 + 2 * (count - 1));
  }
}

int row_is_full(int8_t y) {
  if (y < 0 || to_destroy[y]) { // can be negative at the end of the game
    return 1;
  }

  for (int8_t i = 0; i < GRID_WIDTH; i++) {
    if (grid[i][y] == 0) {
      return 0;
    }
  }

  to_destroy[y] = 1;
  return 1;
}

void lock_shape() {
  int8_t x, y;

  uint8_t to_destroy = 0;

  for (int8_t i = 0; i < 4; i++) {
    x = current_shape[i * 2] + current_x;
    y = current_shape[i * 2 + 1] + current_y;

    if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
      grid[x][y] = current_shape_color;
    }

    if (row_is_full(y)) {
      to_destroy++;
    } else {
      if (y <= 0) {
        end_game();
      }
    }
  }

  if (to_destroy) {
    clean_destroyed_blocks();
  }

  iteration = 0;
  score += SCORE_SINGLE;
  reset_fall_freq();
  spawn_shape();
}

int8_t detect_collision(int8_t x, int8_t y) {
  if (x < 0 || x >= GRID_WIDTH) {
    return 1;
  }

  if (y >= GRID_HEIGHT) { // collisions at the top are OK
    return 1;
  }

  if (y >= 0 && grid[x][y]) {
    return 1;
  }

  return 0;
}

void rotate_shape() {
  reset_fall_freq();

  if (current_shape_type == 0) {
    return; // O-shape should not be rotated
  }

  state_changed = 1;

  int8_t temp[8] = {0};

  int8_t x, y;

  for (int8_t i = 0; i < 4; i++) {
    temp[i * 2] = current_shape[i * 2 + 1];
    temp[i * 2 + 1] = -current_shape[i * 2];

    x = temp[i * 2] + current_x;
    y = temp[i * 2 + 1] + current_y;

    if (detect_collision(x, y)) {
      return;
    }
  };

  for (int8_t i = 0; i < 8; i++) {
    current_shape[i] = temp[i];
  }
}

void move_side(int direction) {
  reset_fall_freq();

  int8_t x, y;

  for (int8_t i = 0; i < 4; i++) {
    x = current_shape[i * 2] + current_x + direction;
    y = current_shape[i * 2 + 1] + current_y;

    if (detect_collision(x, y)) {
      return;
    }
  }

  current_x += direction;
  state_changed = 1;
}

void fall() {
  iteration++;
  // Fall in `fall_freq` times
  if (iteration < fall_freq) {
    return;
  }

  iteration = 0;

  int8_t x, y;

  for (int8_t i = 0; i < 4; i++) {
    x = current_shape[i * 2] + current_x;
    y = current_shape[i * 2 + 1] + current_y + 1;

    if (detect_collision(x, y)) {
      return lock_shape();
    }
  }

  current_y += 1;
  state_changed = 1;
}

void handle_input_event(enum InputEvent event) {
  switch (event) {
  case LEFT:
    return move_side(-1);
  case RIGHT:
    return move_side(1);
  case ROTATE:
    return rotate_shape();
  case HARD_DROP:
    return update_fall_freq(HARD_FREQ);
  case SOFT_DROP:
    return update_fall_freq(SOFT_FREQ);
  default:
    return;
  }
}

void update_frame() {
  if (game_over) {
    return render_game_over_message(score);
  }

  if (!state_changed) {
    return; // no need to rerender if all blocks remain at the same positions
  }

  clear_screen();

  for (int8_t i = 0; i < GRID_WIDTH; ++i) {
    for (int8_t j = 0; j < GRID_HEIGHT; ++j) {
      draw_block(i, j, COLORS[grid[i][j]]);
    }
  }

  int8_t x, y;

  for (int8_t i = 0; i < 4; i++) {
    x = current_shape[i * 2] + current_x;
    y = current_shape[i * 2 + 1] + current_y;

    if (y >= 0) { // skip overflowed
      draw_block(x, y, COLORS[current_shape_color]);
    }
  }

  render_frame(score, current_level);
  state_changed = 0;
}

int8_t init_game() {
  spawn_shape();

  return init_graphics();
}

int8_t game_loop() {
  enum InputEvent event = listen_for_input(game_over);
  if (event == QUIT) {
    return 1;
  }

  if (game_over) {
    if (event > 0) {
      restart_game();
    }
  } else {
    handle_input_event(event);
    fall();
    update_frame();

    SDL_Delay(FRAME_DELAY);
  }

  return 0;
}

int8_t terminate_game() {
  release_resources();
  return 0;
}
