#pragma once

#include "constants.h"
#include <SDL.h>
#include <SDL_image.h>
#include <stdbool.h>

#include <cJSON.h>

struct mainSystems;
typedef struct Player Player;
typedef struct GameManager GameManager;

typedef struct {
    char *path;
    SDL_Texture *tex;
} TextureCacheItem;


typedef struct {
    char *name;  // e.g., "level1"
    char *path;  // e.g., "maps/level1/level1.json"
} LevelEntry;

typedef struct LevelPaths{
    LevelEntry *levels;      // array of levels
    int count;               // number of levels
    char *defaultLevel;      
    char *lastLoadedLevel;   
} LevelPaths;


typedef struct {
    char *id;              
    SDL_Texture *tex;
    int tileSize;           // in pixels 
    int tilesPerRow;        // how many tiles in source image row
    float scale;            // how much to scale when rendering
} Tileset;

typedef struct Layer {
    char *csvPath;          // path to load tile indices from
    int *tiles;             // dynamic array LEVEL_ROWS * LEVEL_COLS
    char *tileset_id;       // which tileset to use for this layer
    bool collidable;         // Is the layer collidable
    int *solidTiles;        // list of tile indices that are considered solid for this layer
    int solidCount;         // number of items in solidTiles
} Layer;

typedef struct {
    char *imagePath;
    SDL_Texture *tex;
    float scrollSpeed;
    float scale;
    float offsetY;          // vertical placement
} BackgroundLayer;

 struct Level{
    char *name;             // level name
    Tileset *tilesets;     
    int tilesetCount;
    Layer *layers;         // dynamic array of layer descriptions
    int layerCount;
    BackgroundLayer *bgs;
    int bgCount;
    int spawnColumn;
    int levelRows;
    int levelColumns;
} typedef Level;

void debug_draw_collidable_tiles(Level *lvl, SDL_Renderer *renderer, int cameraX, int cameraY);
LevelPaths *levelPaths(char *path);
void freeLevelPaths(LevelPaths *lp);
SDL_Texture *getTextureCached(SDL_Renderer *renderer, const char *path);
char *read_whole_file(const char *path);
Level *loadLevelFromJSON(const char *jsonPath, GameManager *gm);
int getLevelTileWidth(Level *lvl);
bool level_isTileSolid(Level *lvl, int worldX, int worldY);
void unloadLevel(Level *level);
void renderLevel(GameManager *gm);
int *loadLayerCSV(const char *csvPath, int levelRows, int levelCols);
void spawnPlayerOnAnyCollidable(Player *p, int spawnCol, Level *lvl, bool searchFromTop);
Tileset *findTileset(Level *lvl, const char *id);