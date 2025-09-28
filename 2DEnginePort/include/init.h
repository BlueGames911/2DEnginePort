#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <stdbool.h>
#include <SDL_mixer.h>

typedef struct GameManager GameManager;


bool initalizeSystems(GameManager *gm);
void cleanUp(GameManager *gm);