#define SDL_MAIN_HANDLED
#include "gameManager.h"

#include <stdio.h>

int main(int argc, char* argv[])
{
    GameManager* gm = GameManager_Create("settings.json", "Game v0.3");
    if (!gm) {
        fprintf(stderr, "Failed to initialize GameManager\n");
        return EXIT_FAILURE;
    }

    float deltaTime;
    Uint32 last_ticks = SDL_GetTicks();
    Uint32 current_ticks;

    // Main game loop
    while (gm->running) {
        current_ticks = SDL_GetTicks();
        deltaTime = (current_ticks - last_ticks) / 1000.0f;
        last_ticks = current_ticks;

        GameManager_HandleInput(gm);
        GameManager_Update(gm, deltaTime);
        GameManager_Render(gm);

        SDL_Delay(15); // ~60 FPS cap
    }

    GameManager_Destroy(gm, EXIT_SUCCESS);
    printf("Successfully cleaned up and exiting.\n");
    return EXIT_SUCCESS;
}

