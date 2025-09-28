#include "input.h"
#include "init.h"
#include "gameManager.h"
#include "player.h"

#include <SDL.h>
#include <stdbool.h>


void processInput(SDL_Event *event, GameManager *gm)
{
    if (!gm || !event) return;

    // Handle all SDL events
    while (SDL_PollEvent(event))
    {
        switch (event->type)
        {
            case SDL_QUIT:
                gm->running = false;
                break;

            case SDL_KEYDOWN:
                switch (event->key.keysym.scancode)
                {
                    case SDL_SCANCODE_ESCAPE:
                        /* Toggle pause state
                        if (gm->state == GAMESTATE_RUNNING)
                            gm->state = GAMESTATE_PAUSED;
                        else if (gm->state == GAMESTATE_PAUSED)
                            gm->state = GAMESTATE_RUNNING;
                        break;
                        */
                        gm->running = false;

                    case SDL_SCANCODE_F11:
                        {
                            Uint32 flags = SDL_GetWindowFlags(gm->mainSystems.window);
                            if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
                            {
                                SDL_SetWindowFullscreen(gm->mainSystems.window, 0);
                                gm->settings.video.fullscreen = false;
                            }
                            else
                            {
                                SDL_SetWindowFullscreen(gm->mainSystems.window, SDL_WINDOW_FULLSCREEN);
                                gm->settings.video.fullscreen = true;
                            }
                        }
                        break;

                    case SDL_SCANCODE_F1:
                        gm->settings.gameplay.debugMode = !gm->settings.gameplay.debugMode;
                        break;
                    default:
                        if (event->key.keysym.scancode == SDL_SCANCODE_F4 &&
                            (SDL_GetModState() & KMOD_ALT)) {
                            gm->running = false;
                        }
                        break;
                }
                break;

            default:
                break;
        }
    }

    if (gm->player != NULL) {
        Player_HandleInput(gm->player);
    } 
}
