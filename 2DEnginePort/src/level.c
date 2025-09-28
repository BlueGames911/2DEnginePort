#include "level.h"
#include <player.h>

#include "gameManager.h"
#include "settings.h"
#include "utils.h"

#include <string.h>



/*SDL_Texture *getTextureCached(SDL_Renderer *renderer, const char *path) {
}*/


LevelPaths *levelPaths(char *jSONPath)
{
    // Read the file into a string
    char *json = read_whole_file(jSONPath);
    if (!json) return NULL;

    // Parse JSON with cJSON_Parse 
    cJSON *levelPathsFile = cJSON_Parse(json);
    free(json);
    if (!levelPathsFile) return NULL;
    
    // Extract the levels array
    cJSON *levelArray = cJSON_GetObjectItemCaseSensitive(levelPathsFile, "levels");

    if (!levelArray || !cJSON_IsArray(levelArray)) {
        printf("LevelPaths: 'levels' is missing or not an array\n");
        cJSON_Delete(levelPathsFile);
        return NULL;
    }

    // Count how many levels
    int numOfLevels = cJSON_GetArraySize(levelArray);

    // Allocate an array of Level structs to hold name+path
    LevelPaths *lp = calloc(1, sizeof(LevelPaths));
    lp->count = numOfLevels;

    LevelEntry* levels = NULL;
    levels = calloc(numOfLevels, sizeof(LevelEntry));

    int idx = 0;
    cJSON *pths = NULL;
    cJSON_ArrayForEach(pths, levelArray)
    {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(pths, "name");
        cJSON *path = cJSON_GetObjectItemCaseSensitive(pths, "path");

        if (cJSON_IsString(name) && name->valuestring) {
            levels->name = _strdup(name->valuestring);
        }
        else
        {
            printf("Settings: Could not load level name\n");
        }

        if (cJSON_IsString(path) && path->valuestring) {
            levels->path = _strdup(path->valuestring);
        }
        else
        {
            printf("Settings: Could not load level name\n");
        }
        idx++;
    }
    printf("Loaded Level Paths and Name\n");

    cJSON *defaultLevel = cJSON_GetObjectItemCaseSensitive(levelPathsFile, "defaultLevel");
    if (cJSON_IsString(defaultLevel)) {
        size_t len = strlen(defaultLevel->valuestring);
        lp->defaultLevel = malloc(len + 1);
        if (lp->defaultLevel) {
            memcpy(lp->defaultLevel, defaultLevel->valuestring, len + 1);
        }
    }

    cJSON *lastLoadedLevel = cJSON_GetObjectItemCaseSensitive(levelPathsFile, "lastLoadedLevel");
    if (cJSON_IsString(lastLoadedLevel)) {
        size_t len = strlen(lastLoadedLevel->valuestring);
        lp->lastLoadedLevel = malloc(len + 1);
        if (lp->lastLoadedLevel) {
            memcpy(lp->lastLoadedLevel, lastLoadedLevel->valuestring, len + 1);
        }
    }

   cJSON_Delete(levelPathsFile);
   
   lp->levels = levels;
   return lp;

}

void freeLevelPaths(LevelPaths *lp)
{
    if (!lp) return;
    for (int i = 0; i < lp->count; i++) {
        free(lp->levels[i].name);
        free(lp->levels[i].path);
    }
    free(lp->levels);
    free(lp->defaultLevel);
    free(lp->lastLoadedLevel);
    free(lp);
}

int *loadLayerCSV(const char *csvPath, int levelRows, int levelCols)
{
    FILE *f = fopen(csvPath, "r");
    if (!f) return NULL;

    int *tiles = calloc(levelRows * levelCols, sizeof(int));
    if (!tiles) { fclose(f); return NULL; }

    char line[1024];
    int row = 0;
    while (fgets(line, sizeof(line), f) && row < levelRows) {
        char *token = strtok(line, ",");
        int col = 0;
        while (token && col < levelCols) {
            tiles[row * levelCols + col] = atoi(token);
            token = strtok(NULL, ",");
            col++;
        }
        row++;
    }

    fclose(f);
    return tiles;
}

Level *loadLevelFromJSON(const char *jsonPath, GameManager *gm)
{
    fprintf(stderr, "[Level] Loading level from JSON: %s\n", jsonPath);

    char *json = read_whole_file(jsonPath);
    if (!json) {
        fprintf(stderr, "[Level] ERROR: Failed to read file: %s\n", jsonPath);
        return NULL;
    }
    cJSON *jsonFile = cJSON_Parse(json);
    free(json);
    if (!jsonFile) {
        fprintf(stderr, "[Level] ERROR: Failed to parse JSON: %s\n", jsonPath);
        return NULL;
    }

    Level *lvl = calloc(1, sizeof(Level));
    // name
    cJSON *cname = cJSON_GetObjectItemCaseSensitive(jsonFile, "levelName");
    if (cJSON_IsString(cname)) {
        lvl->name = _strdup(cname->valuestring);
        fprintf(stderr, "[Level] Level name: %s\n", cname->valuestring);
    } else {
        fprintf(stderr, "[Level] WARNING: Level has no name in %s\n", jsonPath);
        lvl->name = _strdup("UNKNOWN");
    }

    // spawnColumn
    cJSON *spawn = cJSON_GetObjectItemCaseSensitive(jsonFile, "spawnColumn");
    lvl->spawnColumn = cJSON_IsNumber(spawn) ? spawn->valueint : 1;

    // level rows and columns
    cJSON *cols = cJSON_GetObjectItemCaseSensitive(jsonFile, "levelColumns");
    if (!cJSON_IsNumber(cols)) {
        fprintf(stderr, "[Level] ERROR: %s does not have a valid levelColumns value\n", lvl->name);
        cJSON_Delete(jsonFile);
        return NULL;
    } else {
        lvl->levelColumns = cols->valueint;
    }

    cJSON *rows = cJSON_GetObjectItemCaseSensitive(jsonFile, "levelRows");
    if (!cJSON_IsNumber(rows)) {
        fprintf(stderr, "[Level] ERROR: %s does not have a valid levelRows value\n", lvl->name);
        cJSON_Delete(jsonFile);
        return NULL;
    } else {
        lvl->levelRows = rows->valueint;
    }

    // tilesets array
    cJSON *tilesets = cJSON_GetObjectItemCaseSensitive(jsonFile, "tilesets");
    int tsCount = cJSON_GetArraySize(tilesets);
    lvl->tilesets = calloc(tsCount, sizeof(Tileset));
    lvl->tilesetCount = tsCount;
    int idx = 0;
    cJSON *ts = NULL;
    cJSON_ArrayForEach(ts, tilesets) {
        cJSON *id = cJSON_GetObjectItemCaseSensitive(ts, "id");
        cJSON *image = cJSON_GetObjectItemCaseSensitive(ts, "image");
        cJSON *tileSize = cJSON_GetObjectItemCaseSensitive(ts, "tileSize");
        cJSON *tilesPerRow = cJSON_GetObjectItemCaseSensitive(ts, "tilesPerRow");
        cJSON *scale = cJSON_GetObjectItemCaseSensitive(ts, "scale");

        if (!cJSON_IsString(id) || !cJSON_IsString(image)) {
            fprintf(stderr, "[Level] WARNING: Tileset entry missing id or image in %s\n", lvl->name);
            continue;
        }

        lvl->tilesets[idx].id = _strdup(id->valuestring);
        lvl->tilesets[idx].tileSize = cJSON_IsNumber(tileSize) ? tileSize->valueint : 16;
        lvl->tilesets[idx].tilesPerRow = cJSON_IsNumber(tilesPerRow) ? tilesPerRow->valueint : 8;
        lvl->tilesets[idx].scale = cJSON_IsNumber(scale) ? (float)scale->valuedouble : 2.0f;

        fprintf(stderr, "[Level] Loading tileset image: %s (id: %s)\n", image->valuestring, id->valuestring);
        SDL_Surface *surf = IMG_Load(image->valuestring);
        if (!surf) {
            fprintf(stderr, "[Level] ERROR: Failed to load tileset image '%s' for level '%s'\n", image->valuestring, lvl->name);
            cJSON_Delete(jsonFile);
            return NULL;
        }
        lvl->tilesets[idx].tex = SDL_CreateTextureFromSurface(gm->mainSystems.renderer, surf);
        SDL_FreeSurface(surf);
        idx++;
    }

    // loading CSV 
    cJSON *layers = cJSON_GetObjectItemCaseSensitive(jsonFile, "layers");
    int layerCount = cJSON_GetArraySize(layers);
    lvl->layers = calloc(layerCount, sizeof(Layer));
    lvl->layerCount = layerCount;
    idx = 0;
    cJSON *ln = NULL;
    cJSON_ArrayForEach(ln, layers) {
        cJSON *csv = cJSON_GetObjectItemCaseSensitive(ln, "csv");
        cJSON *tilesetId = cJSON_GetObjectItemCaseSensitive(ln, "tileset");
        cJSON *isCollidable = cJSON_GetObjectItemCaseSensitive(ln, "collidable");

        if (!cJSON_IsString(csv) || !cJSON_IsString(tilesetId)) {
            fprintf(stderr, "[Level] WARNING: Layer entry missing csv or tileset id in level '%s'\n", lvl->name);
            continue;
        }

        lvl->layers[idx].csvPath = _strdup(csv->valuestring);
        lvl->layers[idx].tileset_id = _strdup(tilesetId->valuestring);

        fprintf(stderr, "[Level] Loading layer CSV: %s (layer %d, level '%s')\n", csv->valuestring, idx, lvl->name);
        lvl->layers[idx].tiles = loadLayerCSV(csv->valuestring, lvl->levelRows, lvl->levelColumns);
        if (!lvl->layers[idx].tiles) {
            fprintf(stderr, "[Level] ERROR: Failed to load CSV '%s' for layer %d in level '%s'\n", csv->valuestring, idx, lvl->name);
            cJSON_Delete(jsonFile);
            return NULL;
        }

        lvl->layers[idx].collidable = cJSON_IsTrue(isCollidable);

        if (lvl->layers[idx].collidable) {
            cJSON *solidArray = cJSON_GetObjectItemCaseSensitive(ln, "solidTiles");
            if (cJSON_IsArray(solidArray)) {
                lvl->layers[idx].solidCount = cJSON_GetArraySize(solidArray);
                lvl->layers[idx].solidTiles = calloc(lvl->layers[idx].solidCount, sizeof(int));

                int solidIdx = 0;
                cJSON *solidTile = NULL;
                cJSON_ArrayForEach(solidTile, solidArray) {
                    if (cJSON_IsNumber(solidTile)) {
                        lvl->layers[idx].solidTiles[solidIdx++] = solidTile->valueint;
                    }
                }
            }
        }

        idx++;
    }

    // backgrounds
    cJSON *bgs = cJSON_GetObjectItemCaseSensitive(jsonFile, "backgrounds");
    lvl->bgCount = cJSON_GetArraySize(bgs);
    lvl->bgs = calloc(lvl->bgCount, sizeof(BackgroundLayer));
    idx = 0;
    cJSON* bg = NULL;
    cJSON_ArrayForEach(bg, bgs) {
        cJSON *img = cJSON_GetObjectItemCaseSensitive(bg, "image");
        cJSON *speed = cJSON_GetObjectItemCaseSensitive(bg, "scrollSpeed");
        cJSON *scale = cJSON_GetObjectItemCaseSensitive(bg, "scale");
        cJSON *offsetY = cJSON_GetObjectItemCaseSensitive(bg, "offsetY");
        if (!cJSON_IsString(img)) {
            fprintf(stderr, "[Level] WARNING: Background entry missing image in level '%s'\n", lvl->name);
            continue;
        }
        lvl->bgs[idx].imagePath = _strdup(img->valuestring);
        lvl->bgs[idx].tex = NULL;
        fprintf(stderr, "[Level] Loading background image: %s (bg %d, level '%s')\n", img->valuestring, idx, lvl->name);
        SDL_Surface *surf = IMG_Load(img->valuestring);
        if (!surf) {
            fprintf(stderr, "[Level] ERROR: Failed to load background image '%s' for level '%s'\n", img->valuestring, lvl->name);
            cJSON_Delete(jsonFile);
            return NULL;
        }
        lvl->bgs[idx].tex = SDL_CreateTextureFromSurface(gm->mainSystems.renderer, surf);
        SDL_FreeSurface(surf);
        lvl->bgs[idx].scrollSpeed = cJSON_IsNumber(speed) ? (float)speed->valuedouble : 0.3f;
        lvl->bgs[idx].scale = cJSON_IsNumber(scale) ? (float)scale->valuedouble : 1.f;
        lvl->bgs[idx].offsetY = cJSON_IsNumber(offsetY) ? (float)offsetY->valuedouble : 0.f;
        idx++;
    }

    cJSON_Delete(jsonFile);
    fprintf(stderr, "[Level] Successfully loaded level: %s\n", lvl->name);
    return lvl;
}

Tileset *findTileset(Level *lvl, const char *id)
{
    if (!lvl || !id) return NULL;
    for (int i = 0; i < lvl->tilesetCount; i++) {
        if (strcmp(lvl->tilesets[i].id, id) == 0) {
            return &lvl->tilesets[i];
        }
    }
    return NULL;
}

int getLevelTileWidth(Level *lvl) 
{
    if (!lvl || lvl->tilesetCount == 0) return 0;
    int firstWidth = (int)(lvl->tilesets[0].tileSize * lvl->tilesets[0].scale);
    for (int i = 1; i < lvl->tilesetCount; i++) {
        int w = (int)(lvl->tilesets[i].tileSize * lvl->tilesets[i].scale);
        if (w != firstWidth) {
            SDL_Log("Warning: Tileset %s has a different scaled width (%d vs %d)",
                    lvl->tilesets[i].id, w, firstWidth);
        }
    }
    return firstWidth;
}

bool level_isTileSolid(Level *lvl, int worldX, int worldY) 
{
    if (!lvl) return false;

    int tileW = getLevelTileWidth(lvl);
    int tileH = tileW; // square tiles
    int col = worldX / tileW;
    int row = worldY / tileH;

    // Out of bounds check
    if (col < 0 || col >= lvl->levelColumns ||
        row < 0 || row >= lvl->levelRows) {
        return false;
    }

    // Loop through all layers
    for (int i = 0; i < lvl->layerCount; i++) {
        Layer *layer = &lvl->layers[i];
        if (!layer->collidable) continue;

        int tileIndex = layer->tiles[row * lvl->levelColumns + col];
        // Check against solidTiles list
        for (int s = 0; s < layer->solidCount; s++) {
            if (tileIndex == layer->solidTiles[s]) {
                return true; // Found a solid tile
            }
        }
    }

    return false;
}


// Finds a spawn position based on any collidable layer.
// spawnCol: tile column in the level grid
// searchFromTop: if true, searches from top of map downward;
//                if false, searches from bottom up (like for platformers)
void spawnPlayerOnAnyCollidable(Player *p, int spawnCol, Level *lvl, bool searchFromTop)
{
    if (!p || !lvl) return;

    int tileW = getLevelTileWidth(lvl);
    int tileH = tileW;

    // Clamp spawnCol
    if (spawnCol < 0) spawnCol = 0;
    if (lvl->levelColumns > 0 && spawnCol >= lvl->levelColumns) spawnCol = lvl->levelColumns - 1;

    int ai = p->currentAnimIndex;
    if (ai < 0 || ai >= p->anims.movementCount) ai = 0;
    const Animation *anim = &p->anims.movements[ai];
    if (anim->sheetIndex < 0 || anim->sheetIndex >= p->sheetCount) return;
    const SpriteSheet *sheet = &p->sheets[anim->sheetIndex];

    int collW, collH;
    if (anim->collisionProfile) {
        collW = (int)(anim->collisionProfile->widthScale  * sheet->frameWidth  * p->spriteScale + 0.5f);
        collH = (int)(anim->collisionProfile->heightScale * sheet->frameHeight * p->spriteScale + 0.5f);
    } else {
        collW = (int)(sheet->frameWidth  * p->spriteScale);
        collH = (int)(sheet->frameHeight * p->spriteScale);
    }

    int xWorld = spawnCol * tileW;

    if (searchFromTop) {
        for (int row = 0; row < lvl->levelRows - 1; ++row) {
            int yWorld = row * tileH;
            bool currentSolid = level_isTileSolid(lvl, xWorld, yWorld);
            bool belowSolid   = level_isTileSolid(lvl, xWorld, (row + 1) * tileH);
            if (!currentSolid && belowSolid) {
                p->body.x = xWorld + (tileW - collW) / 2;
                p->body.y = ((row + 1) * tileH - collH); // feet on top of the solid tile
                Player_UpdateCollisionBox(p, lvl);
                return;
            }
        }
    } else {
        for (int row = lvl->levelRows - 2; row >= 0; --row) {
            int yWorld = row * tileH;
            bool currentSolid = level_isTileSolid(lvl, xWorld, yWorld);
            bool belowSolid   = level_isTileSolid(lvl, xWorld, (row + 1) * tileH);
            if (!currentSolid && belowSolid) {
                p->body.x = xWorld + (tileW - collW) / 2;
                p->body.y = ((row + 1) * tileH - collH) ;
                Player_UpdateCollisionBox(p, lvl);
                return;
            }
        }
    }

    p->body.x = (tileW - collW) / 2;
    p->body.y = 0;
    Player_UpdateCollisionBox(p, lvl);
}


void renderLayer(Layer *layer, Level *lvl, SDL_Renderer *renderer, int cameraX, int cameraY)
{
    Tileset *ts = findTileset(lvl, layer->tileset_id);
    if (!ts || !ts->tex) return;

    // consistent scaled tile size used for pos & size
    int scaledTile = (int)(ts->tileSize * ts->scale);

    for (int r = 0; r < lvl->levelRows; ++r) {
        for (int c = 0; c < lvl->levelColumns; ++c) {
            int rawIdx = layer->tiles[r * lvl->levelColumns + c];

            // CSV uses -1 for empty -> skip negatives only
            if (rawIdx < 0) continue;

            int localIdx = rawIdx;

            SDL_Rect src = {
                (localIdx % ts->tilesPerRow) * ts->tileSize,
                (localIdx / ts->tilesPerRow) * ts->tileSize,
                ts->tileSize,
                ts->tileSize
            };

            SDL_Rect dst = {
                c * scaledTile - cameraX,
                r * scaledTile - cameraY,
                scaledTile,
                scaledTile
            };

            SDL_RenderCopy(renderer, ts->tex, &src, &dst);
        }
    }
}


// Renders all background layers with parallax and looping
void renderBackgrounds(Level *lvl, SDL_Renderer *renderer, int cameraX, int cameraY, GameSettings *settings) 
{
    // Loop through all background layers
    for (int i = 0; i < lvl->bgCount; i++) 
    {
        BackgroundLayer *bg = &lvl->bgs[i];

        // Skip if no texture
        if (!bg->tex) continue;

        // Calculate horizontal offset based on cameraX and scrollSpeed
        float offsetX = fmod(cameraX * bg->scrollSpeed, settings->video.width); // bgWidth = bg width in pixels * scale
        if (offsetX < 0) offsetX += settings->video.width; // wrap around negative offsets

        // 4Calculate vertical offset based on cameraY and vertical scroll (if any)
        float offsetY = fmod(cameraY * bg->scrollSpeed, settings->video.height); // optional vertical scroll
        if (offsetY < 0) offsetY += settings->video.height;

        int tilesX = 2;
        int tilesY = 2;


        // Loop to tile horizontally and vertically to fill the screen
        for (int tx = 0; tx <= tilesX; tx++)   
        {
            for (int ty = 0; ty <= tilesY; ty++) 
            {
                SDL_Rect destRect;
                destRect.x = tx * settings->video.width - offsetX;
                destRect.y = ty * settings->video.height - offsetY;
                destRect.w = settings->video.width;
                destRect.h = settings->video.height;

                SDL_RenderCopy(renderer, bg->tex, NULL, &destRect);
            }
        }
    }
}

// Render a level: backgrounds first, then tile layers
void renderLevel(GameManager *gm)
{
    //Render all background layers
    renderBackgrounds(gm->level, gm->mainSystems.renderer, gm->camera.x, 
        gm->camera.y, &gm->settings);

    //Loop through all tile layers
    for (int i = 0; i < gm->level->layerCount; i++) 
    {
        Layer *layer = &gm->level->layers[i];

        //Skip if layer has no tiles
        if (!layer->tiles) continue;

        renderLayer(layer, gm->level, gm->mainSystems.renderer, gm->camera.x, gm->camera.y);
        if (gm->settings.gameplay.debugMode && layer->collidable) 
        {
            debug_draw_collidable_tiles(gm->level, gm->mainSystems.renderer, gm->camera.x, gm->camera.y);
        }
    }

}

//Generated by copilot
void debug_draw_collidable_tiles(Level *lvl, SDL_Renderer *renderer, int cameraX, int cameraY)
{
    if (!lvl) return;

    // Use *level* tile width for collision grid visualisation.
    // NOTE: If different layers use different tile sizes, consider per-layer scaledTile.
    int commonTileW = getLevelTileWidth(lvl);
    if (commonTileW == 0) return;

    // Draw grid lines (optional)
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); // dark lines
    for (int r = 0; r <= lvl->levelRows; ++r) {
        int y = r * commonTileW - cameraY;
        SDL_RenderDrawLine(renderer, -cameraX, y, lvl->levelColumns * commonTileW - cameraX, y);
    }
    for (int c = 0; c <= lvl->levelColumns; ++c) {
        int x = c * commonTileW - cameraX;
        SDL_RenderDrawLine(renderer, x, -cameraY, x, lvl->levelRows * commonTileW - cameraY);
    }

    // Draw a red rectangle for every collidable tile in any collidable layer
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 180);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int li = 0; li < lvl->layerCount; ++li) 
    {
        Layer *layer = &lvl->layers[li];
        if (!layer->collidable) continue;

        // get the tileset for this layer, to compute actual render positions
        Tileset *ts = findTileset(lvl, layer->tileset_id);
        int tileW = commonTileW;
        if (ts) tileW = (int)(ts->tileSize * ts->scale);

        for (int r = 0; r < lvl->levelRows; ++r) 
        {
            for (int c = 0; c < lvl->levelColumns; ++c) 
            {
                int idx = layer->tiles[r * lvl->levelColumns + c];
                if (idx < 0) continue;

                SDL_Rect rect = 
                {
                    c * tileW - cameraX,
                    r * tileW - cameraY,
                    tileW,
                    tileW
                };
                SDL_RenderDrawRect(renderer, &rect);
                // optionally fill a translucent rect:
                // SDL_RenderFillRect
            }
        }
    }
}

void unloadLevel(Level *level)
{
    if (!level) return;

    // Free tilesets
    for (int i = 0; i < level->tilesetCount; i++) {
        free(level->tilesets[i].id);
        level->tilesets[i].id = NULL;

        if (level->tilesets[i].tex) {
            SDL_DestroyTexture(level->tilesets[i].tex);
            level->tilesets[i].tex = NULL;
        }
    }
    free(level->tilesets);
    level->tilesets = NULL;
    level->tilesetCount = 0;

    // Free layers
    for (int i = 0; i < level->layerCount; i++) {
        free(level->layers[i].csvPath);
        free(level->layers[i].tileset_id);
        free(level->layers[i].tiles);
        free(level->layers[i].solidTiles);
    }
    free(level->layers);
    level->layers = NULL;
    level->layerCount = 0;

    // Free background layers
    for (int i = 0; i < level->bgCount; i++) {
        free(level->bgs[i].imagePath);
        if (level->bgs[i].tex) {
            SDL_DestroyTexture(level->bgs[i].tex);
            level->bgs[i].tex = NULL;
        }
    }
    free(level->bgs);
    level->bgs = NULL;
    level->bgCount = 0;

    // Free level name
    free(level->name);
    level->name = NULL;
}

        