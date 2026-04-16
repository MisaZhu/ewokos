#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
#include <ewoksys/klog.h>
#include <ewoksys/keydef.h>
#include <x/x.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define TEST_WAV_FILE "/data/sounds/test.wav"
#define TEST_OGG_FILE "/data/sounds/test.ogg"
#define TEST_MP3_FILE "/data/sounds/test.mp3"

static Mix_Music *current_music = NULL;
static int current_music_type = 0;
static int music_paused = 0;

int play_music(const char *filename, int type) {
    if (current_music != NULL) {
        Mix_HaltMusic();
        Mix_FreeMusic(current_music);
        current_music = NULL;
    }

    printf("Loading: %s\n", filename);
    current_music = Mix_LoadMUS(filename);
    if (current_music == NULL) {
        printf("Failed to load music: %s - %s\n", filename, Mix_GetError());
        return -1;
    }

    current_music_type = type;
    music_paused = 0;

    printf("Starting playback...\n");
    if (Mix_PlayMusic(current_music, -1) == -1) {
        printf("Failed to play music: %s\n", Mix_GetError());
        Mix_FreeMusic(current_music);
        current_music = NULL;
        return -1;
    }

    printf("Playing: %s\n", filename);
    return 0;
}

void render_status(TTF_Font *font, SDL_Renderer *renderer, int y_offset) {
    SDL_Color textColor = {255, 255, 255, 255};
    char status[256];

    const char *type_str = "None";
    switch (current_music_type) {
        case 1: type_str = "WAV"; break;
        case 2: type_str = "OGG"; break;
        case 3: type_str = "MP3"; break;
    }

    const char *state_str = "Stopped";
    if (current_music != NULL) {
        if (music_paused) {
            state_str = "Paused";
        } else if (Mix_PlayingMusic()) {
            state_str = "Playing";
        }
    }

    snprintf(status, sizeof(status), "Format: %s | State: %s | Volume: %d%%",
             type_str, state_str, Mix_VolumeMusic(-1) * 100 / MIX_MAX_VOLUME);

    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, status, textColor);
    if (surface) {
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {10, y_offset, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
    }
}

void render_help(TTF_Font *font, SDL_Renderer *renderer, int y_offset) {
    SDL_Color textColor = {200, 200, 200, 255};
    const char *help_text =
        "Controls:\n"
        "  1 - Play WAV file\n"
        "  2 - Play OGG file\n"
        "  3 - Play MP3 file\n"
        "  SPACE - Pause/Resume\n"
        "  S - Stop\n"
        "  UP/DOWN - Volume +/-\n"
        "  ESC - Quit";

    char *text_copy = strdup(help_text);
    char *line = strtok(text_copy, "\n");
    int y = y_offset;

    while (line != NULL) {
        SDL_Surface *surface = TTF_RenderUTF8_Blended(font, line, textColor);
        if (surface) {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect rect = {10, y, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
            y += 25;
        }
        line = strtok(NULL, "\n");
    }

    free(text_copy);
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("Initializing SDL...\n");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    printf("Initializing SDL_mixer...\n");
    int flags = MIX_INIT_OGG | MIX_INIT_MP3;
    int initted = Mix_Init(flags);
    if ((initted & flags) != flags) {
        printf("Mix_Init failed: %s\n", Mix_GetError());
    }

    printf("Opening audio device...\n");
    if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 2048) < 0) {
        printf("Mix_OpenAudio failed: %s\n", Mix_GetError());
        Mix_Quit();
        SDL_Quit();
        return 1;
    }

    printf("Mix_OpenAudio success!\n");
    printf("Checking available decoders:\n");
    printf("  Number of music decoders: %d\n", Mix_GetNumMusicDecoders());
    for (int i = 0; i < Mix_GetNumMusicDecoders(); i++) {
        printf("  Decoder %d: %s\n", i, Mix_GetMusicDecoder(i));
    }

    printf("Initializing TTF...\n");
    if (TTF_Init() == -1) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
        Mix_CloseAudio();
        Mix_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("/usr/system/fonts/system-cn.ttf", 20);
    if (font == NULL) {
        printf("Failed to load font: %s\n", TTF_GetError());
        TTF_Quit();
        Mix_CloseAudio();
        Mix_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "SDL2 Mixer Test",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_SHOWN
    );

    if (window == NULL) {
        printf("Failed to create window: %s\n", SDL_GetError());
        TTF_CloseFont(font);
        TTF_Quit();
        Mix_CloseAudio();
        Mix_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_CloseFont(font);
        TTF_Quit();
        Mix_CloseAudio();
        Mix_Quit();
        SDL_Quit();
        return 1;
    }

    IMG_Init(IMG_INIT_PNG);

    int quit = 0;
    int volume = MIX_MAX_VOLUME / 2;
    Mix_VolumeMusic(volume);

    printf("SDL2 Mixer Test Started\n");
    printf("Supported music formats: WAV, OGG, MP3\n");
    printf("Press 1/2/3 to play WAV/OGG/MP3 files\n");

    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = 1;
                    break;

                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_1:
                            play_music(TEST_WAV_FILE, 1);
                            break;

                        case SDLK_2:
                            play_music(TEST_OGG_FILE, 2);
                            break;

                        case SDLK_3:
                            play_music(TEST_MP3_FILE, 3);
                            break;

                        case SDLK_SPACE:
                            if (current_music != NULL) {
                                if (music_paused) {
                                    Mix_ResumeMusic();
                                    music_paused = 0;
                                    printf("Resumed\n");
                                } else {
                                    Mix_PauseMusic();
                                    music_paused = 1;
                                    printf("Paused\n");
                                }
                            }
                            break;

                        case SDLK_s:
                            Mix_HaltMusic();
                            printf("Stopped\n");
                            break;

                        case SDLK_UP:
                            volume += 8;
                            if (volume > MIX_MAX_VOLUME) {
                                volume = MIX_MAX_VOLUME;
                            }
                            Mix_VolumeMusic(volume);
                            printf("Volume: %d%%\n", volume * 100 / MIX_MAX_VOLUME);
                            break;

                        case SDLK_DOWN:
                            volume -= 8;
                            if (volume < 0) {
                                volume = 0;
                            }
                            Mix_VolumeMusic(volume);
                            printf("Volume: %d%%\n", volume * 100 / MIX_MAX_VOLUME);
                            break;

                        case SDLK_ESCAPE:
                            quit = 1;
                            break;
                    }
                    break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 40, 40, 60, 255);
        SDL_RenderClear(renderer);

        SDL_Color titleColor = {0, 255, 128, 255};
        SDL_Surface *titleSurface = TTF_RenderUTF8_Blended(font, "SDL2 Mixer Audio Test", titleColor);
        if (titleSurface) {
            SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
            SDL_Rect titleRect = {10, 10, titleSurface->w, titleSurface->h};
            SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
            SDL_DestroyTexture(titleTexture);
            SDL_FreeSurface(titleSurface);
        }

        render_status(font, renderer, 50);
        render_help(font, renderer, 100);

        if (Mix_PlayingMusic() && !music_paused) {
            int bar_width = 20;
            int bar_gap = 5;
            int start_x = 10;
            int base_y = 350;
            int max_height = 100;

            for (int i = 0; i < 20; i++) {
                int height = (i * 7 + (int)(SDL_GetTicks() / 50 + i * 3)) % max_height;
                if (height < 10) height = 10;

                Uint8 r = 128 + height;
                Uint8 g = 255 - height;
                Uint8 b = 128;

                roundedBoxColor(renderer,
                    start_x + i * (bar_width + bar_gap),
                    base_y - height,
                    start_x + i * (bar_width + bar_gap) + bar_width,
                    base_y,
                    3,
                    (r << 24) | (g << 16) | (b << 8) | 0xFF);
            }
        }

        SDL_RenderPresent(renderer);
        usleep(30000);
    }

    if (current_music != NULL) {
        Mix_HaltMusic();
        Mix_FreeMusic(current_music);
    }

    IMG_Quit();
    TTF_CloseFont(font);
    TTF_Quit();
    Mix_CloseAudio();
    Mix_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
