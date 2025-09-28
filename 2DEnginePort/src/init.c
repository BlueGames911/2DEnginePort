#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>

#include "init.h"
#include "gameManager.h"
#include "settings.h"



bool initalizeSystems(GameManager *gm) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        return false;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        fprintf(stderr, "Failed to initialize SDL_image: %s\n", IMG_GetError());
        return false;
    }

    if (!(Mix_Init(MIX_INIT_MP3) & MIX_INIT_MP3)) {
        fprintf(stderr, "Failed to initialize SDL_mixer: %s\n", Mix_GetError());
        return false;
    }

    Uint32 windowFlags = SDL_WINDOW_OPENGL;
    if (gm->settings.video.fullscreen)
        windowFlags |= SDL_WINDOW_FULLSCREEN;

    gm->mainSystems.window = SDL_CreateWindow(
        gm->windowName,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        gm->settings.video.width, gm->settings.video.height,
        windowFlags
    );
    if (!gm->mainSystems.window) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        return false;
    }

    Uint32 rendererFlags = SDL_RENDERER_ACCELERATED;
    if (gm->settings.video.vsync)
        rendererFlags |= SDL_RENDERER_PRESENTVSYNC;

    gm->mainSystems.renderer = SDL_CreateRenderer(gm->mainSystems.window, -1, rendererFlags);
    if (!gm->mainSystems.renderer) {
        fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_RenderSetScale(gm->mainSystems.renderer, gm->settings.video.scale, gm->settings.video.scale);

    return true;
}


void cleanUp(GameManager *gm)
{
    SDL_DestroyRenderer(gm->mainSystems.renderer);
    SDL_DestroyWindow(gm->mainSystems.window);

    IMG_Quit();
    SDL_Quit();

}