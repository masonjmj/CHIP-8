#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chip8.h"

struct config {
    int scale;
    int clockspeed;
    char* rom_path;
};

static void handleOptions(int const argc, char* const* argv, struct config* config)
{
    static struct option long_options[] = {
        { "scale", required_argument, NULL, 's' },
        { "clock", required_argument, NULL, 'c' },
        { NULL, 0, NULL, 0 }
    };

    config->clockspeed = 500;
    config->scale = 10;

    int c;
    while ((c = getopt_long(argc, argv, "s:c:", long_options, NULL)) != -1) {
        switch (c) {
        case 's':
            if (atoi(optarg) != 0) {
                config->scale = atoi(optarg);
            } else {
                fprintf(stderr, "Scale must be a non-zero integer\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'c':
            if (atoi(optarg) >= 1) {
                config->clockspeed = atoi(optarg);
            } else {
                fprintf(stderr, "Clock must be a non-zero integer\n");
                exit(EXIT_FAILURE);
            }
            break;
        case '?':
        default:
            exit(EXIT_FAILURE);
            break;
        }
    }

    // Get input file-path
    if (optind < argc) {
        config->rom_path = argv[optind];
    } else {
        fprintf(stderr, "You must specify the path to the ROM you wish to load\n");
        exit(EXIT_FAILURE);
    }
}

struct appstate {
    SDL_Window* main_window;
    SDL_Renderer* renderer;
    SDL_Surface* surface;
    uint64_t clock_interval_nsec;
    uint64_t last_time;
    struct chip8 chip8;
};

SDL_AppResult
SDL_AppInit(void** appstate, int argc, char** argv)
{
    struct appstate* state = malloc(sizeof(struct appstate));
    *appstate = state;

    struct config config;
    handleOptions(argc, argv, &config);
    state->clock_interval_nsec = ((1.0 / config.clockspeed) * 1000000000);
    state->last_time = SDL_GetTicksNS();

    chip8_initialize(&state->chip8);
    chip8_load_ROM(&state->chip8, config.rom_path);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Chip 8", 64 * config.scale, 32 * config.scale, 0, &state->main_window, &state->renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_SetRenderVSync(state->renderer, 1)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't activate Vsync: %s", SDL_GetError());
    }

    if (!SDL_SetRenderLogicalPresentation(state->renderer, 64, 32, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't set renderer logical presentation dimensions: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!(state->surface = SDL_CreateSurface(64, 32, SDL_PIXELFORMAT_INDEX8))) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create surface: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Palette* palette = SDL_CreateSurfacePalette(state->surface);
    if (!palette) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create palette: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Color background = { 0, 0, 0, 255 };
    SDL_Color foreground = { 255, 255, 255, 255 };
    SDL_Color const colors[2] = { background, foreground };
    if (!SDL_SetPaletteColors(palette, colors, 0, 2)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't set palette colors: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Log("SDL Init Succesful");

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    struct appstate* state = (struct appstate*)appstate;

    // Make sure clock speed of the emulator is independent of framerate
    uint64_t now = SDL_GetTicksNS();
    uint64_t elapsed = now - state->last_time;

    if (elapsed < state->clock_interval_nsec) {
        return SDL_APP_CONTINUE;
    }

    state->last_time = now;

    while (elapsed >= state->clock_interval_nsec) {
        elapsed -= state->clock_interval_nsec;
        chip8_clock(&state->chip8);
    }

    state->last_time -= elapsed;

    if (!SDL_LockSurface(state->surface)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't lock surface: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    memcpy(state->surface->pixels, state->chip8.display, state->surface->h * state->surface->pitch);

    SDL_UnlockSurface(state->surface);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(state->renderer, state->surface);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    if (!texture) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture from surface: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_RenderTexture(state->renderer, texture, NULL, NULL)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't render texture to target: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_RenderPresent(state->renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't present render contents: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_DestroyTexture(texture);
    return SDL_APP_CONTINUE;
}

int const keymap[16] = {
    SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
    SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A,
    SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C,
    SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V
};

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    struct appstate* state = (struct appstate*)appstate;

    if (event->type == SDL_EVENT_KEY_UP || event->type == SDL_EVENT_KEY_DOWN) {
        int scancode = event->key.scancode;

        for (int i = 0; i < 16; i++) {
            if (scancode == keymap[i]) {
                state->chip8.keypad[i] = event->type == SDL_EVENT_KEY_DOWN;
            }
        }
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    struct appstate* state = (struct appstate*)appstate;

    SDL_DestroySurface(state->surface);
    SDL_DestroyRenderer(state->renderer);
    SDL_DestroyWindow(state->main_window);
    free(state);
    SDL_Log("Quitting %d\n", result);
}
