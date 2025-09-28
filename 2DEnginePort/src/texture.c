#include "texture.h"
#include <stdlib.h>
#include <string.h>

static TextureCacheItem *textureCache = NULL;
static int textureCacheCount = 0;

// Loads a texture from file (no caching)
SDL_Texture* loadTexture(const char* filepath, SDL_Renderer* renderer) {
    SDL_Surface *surface = IMG_Load(filepath);
    if (!surface) return NULL;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

// Loads a texture from file or returns cached version if already loaded
SDL_Texture *getTextureCached(SDL_Renderer *renderer, const char *path) {
    for (int i = 0; i < textureCacheCount; i++) {
        if (strcmp(textureCache[i].path, path) == 0) {
            return textureCache[i].tex;
        }
    }
    SDL_Texture *tex = loadTexture(path, renderer);
    if (!tex) return NULL;
    textureCache = realloc(textureCache, sizeof(TextureCacheItem) * (textureCacheCount + 1));
    textureCache[textureCacheCount].path = _strdup(path);
    textureCache[textureCacheCount].tex = tex;
    textureCacheCount++;
    return tex;
}

// Frees a texture and removes it from the cache if present
void freeTexture(SDL_Texture *tex, SDL_Renderer *renderer) {
    if (!tex) return;
    // Remove from cache if present
    for (int i = 0; i < textureCacheCount; i++) {
        if (textureCache[i].tex == tex) {
            free(textureCache[i].path);
            // Shift remaining items
            for (int j = i; j < textureCacheCount - 1; j++) {
                textureCache[j] = textureCache[j + 1];
            }
            textureCacheCount--;
            if (textureCacheCount == 0) {
                free(textureCache);
                textureCache = NULL;
            } else {
                textureCache = realloc(textureCache, sizeof(TextureCacheItem) * textureCacheCount);
            }
            break;
        }
    }
    SDL_DestroyTexture(tex);
}

void freeAllTextures() {
    for (int i = 0; i < textureCacheCount; i++) {
        free(textureCache[i].path);
        SDL_DestroyTexture(textureCache[i].tex);
    }
    free(textureCache);
    textureCache = NULL;
    textureCacheCount = 0;
}
