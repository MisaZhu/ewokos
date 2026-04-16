int listen_for_input(int game_over);

enum InputEvent {
  QUIT = -1,
  NO_INPUT = 0,
  ANY_INPUT = 99,
  LEFT = 2,
  RIGHT = 3,
  DOWN = 4,
  ROTATE = 5,
  SOFT_DROP = 6,
  HARD_DROP = 7,
};
