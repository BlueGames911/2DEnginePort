#include "render.h"
#include "player.h"
#include "gameManager.h"

#include "SDL_render.h"
#include "camera.h"

void render(GameManager *gm)
{
    SDL_SetRenderDrawColor(gm->mainSystems.renderer, 0, 0, 0, 255);
    SDL_RenderClear(gm->mainSystems.renderer);
    renderLevel(gm);
    Player_Render(gm->player, &gm->mainSystems, &gm->camera, gm->settings.gameplay.debugMode);
    SDL_RenderPresent(gm->mainSystems.renderer);
}