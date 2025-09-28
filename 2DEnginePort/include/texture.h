#pragma once

#include <SDL.h>
#include <SDL_image.h>

// Simple texture cache structure
typedef struct {
    char* path;
    SDL_Texture* tex;
} TextureCacheItem;

SDL_Texture* loadTexture(const char* filepath, SDL_Renderer* renderer);
SDL_Texture *getTextureCached(SDL_Renderer *renderer, const char *path);
void freeTexture(SDL_Texture *tex, SDL_Renderer *renderer);
void freeAllTextures();
