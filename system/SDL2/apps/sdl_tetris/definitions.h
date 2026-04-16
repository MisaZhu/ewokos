#include "SDL2/SDL.h"
#include "SDL2/SDL_timer.h"
#include "SDL2/SDL_ttf.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#endif

#define BLOCK_SIZE 14
#define GRID_WIDTH 10
#define GRID_HEIGHT 20
