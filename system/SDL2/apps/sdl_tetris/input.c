#include "input.h"
#include "definitions.h"

static int handle_key_down(SDL_Keycode key_code) {
  switch (key_code) {
  case SDLK_ESCAPE:
    return QUIT;
  case SDLK_LEFT:
    return LEFT;
  case SDLK_RIGHT:
    return RIGHT;
  case SDLK_UP:
  case SDLK_a:
  case SDLK_RETURN:
    return ROTATE;
  case SDLK_DOWN:
    return SOFT_DROP;
  case SDLK_SPACE:
    return HARD_DROP;
  }
  return ANY_INPUT;
}

int listen_for_input(int game_over) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      return QUIT;
    }
    if (event.type == SDL_KEYDOWN) {
      return handle_key_down(event.key.keysym.sym);
    }
  }

  return NO_INPUT;
}
