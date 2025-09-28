#include "collision.h"
#include "level.h"
#include "player.h"

bool isSolidTile(Level *lvl, int tileIndex) {
    // Loop through all layers and see if any are collidable
    for (int l = 0; l < lvl->layerCount; l++) {
        Layer *layer = &lvl->layers[l];
        if (!layer->collidable) continue;

        // If no solid list is provided, assume any non negative tile is solid
        if (layer->solidCount == 0) {
            if (tileIndex >= 0) return true;
        } else {
            // Check if tileIndex is in solidTiles
            for (int i = 0; i < layer->solidCount; i++) {
                if (layer->solidTiles[i] == tileIndex) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool isSolidTileInLayer(Layer *layer, int tileIndex) {
    if (!layer->collidable) return false;

    if (layer->solidCount == 0) {
        return (tileIndex >= 0);
    }

    // If solid list exists, check if tileIndex matches
    for (int i = 0; i < layer->solidCount; i++) {
        if (layer->solidTiles[i] == tileIndex) return true;
    }
    return false;
}

bool collisionCheck(SDL_Rect A, SDL_Rect B) {
    return (A.x < B.x + B.w &&
            A.x + A.w > B.x &&
            A.y < B.y + B.h &&
            A.y + A.h > B.y);
}


// Horizontal collisions
void checkEntityTileCollisionsX(PhysicsBody *body, Level *lvl, float deltaTime) {
    if (!body || !lvl) return;

    int left   = body->x;
    int right  = body->x + body->collisionRect.w - 1;
    int top    = body->y;
    int bottom = body->y + body->collisionRect.h - 1;

    for (int layerIndex = 0; layerIndex < lvl->layerCount; layerIndex++) {
        Layer *layer = &lvl->layers[layerIndex];
        if (!layer->collidable) continue;

        Tileset *ts = findTileset(lvl, layer->tileset_id);
        if (!ts) continue;

        int tileW = (int)(ts->tileSize * ts->scale);
        int tileH = tileW; // Assuming square

        int leftTile   = left   / tileW;
        int rightTile  = right  / tileW;
        int topTile    = top    / tileH;
        int bottomTile = bottom / tileH;

        // Clamp
        if (leftTile < 0) leftTile = 0;
        if (rightTile >= lvl->levelColumns) rightTile = lvl->levelColumns - 1;
        if (topTile < 0) topTile = 0;
        if (bottomTile >= lvl->levelRows) bottomTile = lvl->levelRows - 1;

        for (int row = topTile; row <= bottomTile; row++) {
            for (int col = leftTile; col <= rightTile; col++) {
                int tileIndex = layer->tiles[row * lvl->levelColumns + col];
                if (!isSolidTileInLayer(layer, tileIndex)) continue;

                SDL_Rect tileRect = { col * tileW, row * tileH, tileW, tileH };

                // Current/future rect using just updated body position
                // Yes I know I dont need this, I ran into errors when using it as its supposed to
                SDL_Rect futureRect = {
                    (int)body->x ,
                    (int)body->y,
                    body->collisionRect.w,
                    body->collisionRect.h
                };

                if (collisionCheck(futureRect, tileRect)) {
                    if (body->velocity_x > 0) {
                        // Moving right: stick to left edge of tile
                        body->x = tileRect.x - body->collisionRect.w;
                    } else if (body->velocity_x < 0) {
                        // Moving left: stick to right edge of tile
                        body->x = tileRect.x + tileRect.w;
                    }
                    body->velocity_x = 0;
                }
            }
        }
    }
}

// Vertical collisions
void checkEntityTileCollisionsY(PhysicsBody *body, Level *lvl, float deltaTime)
{
    if (!body || !lvl) return;

    int left   = (int)body->x;
    int right  = (int)(body->x + body->collisionRect.w);
    int top    = (int)body->y;
    int bottom = (int)(body->y + body->collisionRect.h);

    for (int layerIndex = 0; layerIndex < lvl->layerCount; layerIndex++) {
        Layer *layer = &lvl->layers[layerIndex];
        if (!layer->collidable) continue;

        Tileset *ts = findTileset(lvl, layer->tileset_id);
        if (!ts) continue;

        int tileW = (int)(ts->tileSize * ts->scale);
        int tileH = tileW;

        int leftTile   = left / tileW;
        int rightTile  = right / tileW;
        int topTile    = top / tileH;
        int bottomTile = bottom / tileH;

        if (leftTile < 0) leftTile = 0;
        if (rightTile >= lvl->levelColumns) rightTile = lvl->levelColumns - 1;
        if (topTile < 0) topTile = 0;
        if (bottomTile >= lvl->levelRows) bottomTile = lvl->levelRows - 1;

        for (int row = topTile; row <= bottomTile; row++) {
            for (int col = leftTile; col <= rightTile; col++) {
                int tileIndex = layer->tiles[row * lvl->levelColumns + col];
                if (!isSolidTileInLayer(layer, tileIndex)) continue;

                SDL_Rect tileRect = { col * tileW, row * tileH, tileW, tileH };

                SDL_Rect futureRect = { (int)body->x, (int)body->y, body->collisionRect.w, body->collisionRect.h };
                if (collisionCheck(futureRect, tileRect)) {

                    if (body->velocity_y > 0) { // Falling
                        body->y = tileRect.y - body->collisionRect.h;
                        body->velocity_y = 0;
                        body->isOnGround = true;
                    } else if (body->velocity_y < 0) { // Rising / hitting ceiling
                        body->y = tileRect.y + tileRect.h;
                        body->velocity_y = 0;
                    }

                }
            }
        }
    }
}

bool hasCeilingAbove(const PhysicsBody *body, Level *lvl, int extraHeight) {
    if (!body || !lvl) return false;

    int left   = (int)body->x;
    int right  = (int)(body->x + body->collisionRect.w - 1);
    int top    = (int)(body->y - extraHeight); // extend upward
    int bottom = (int)body->y; // current top of body

    for (int layerIndex = 0; layerIndex < lvl->layerCount; layerIndex++) {
        Layer *layer = &lvl->layers[layerIndex];
        if (!layer->collidable) continue;

        Tileset *ts = findTileset(lvl, layer->tileset_id);
        if (!ts) continue;

        int tileW = (int)(ts->tileSize * ts->scale);
        int tileH = tileW;

        int leftTile   = left / tileW;
        int rightTile  = right / tileW;
        int topTile    = top / tileH;
        int bottomTile = bottom / tileH;

        if (leftTile < 0) leftTile = 0;
        if (rightTile >= lvl->levelColumns) rightTile = lvl->levelColumns - 1;
        if (topTile < 0) topTile = 0;
        if (bottomTile >= lvl->levelRows) bottomTile = lvl->levelRows - 1;

        for (int row = topTile; row <= bottomTile; row++) {
            for (int col = leftTile; col <= rightTile; col++) {
                int tileIndex = layer->tiles[row * lvl->levelColumns + col];
                if (!isSolidTileInLayer(layer, tileIndex)) continue;

                SDL_Rect tileRect = { col * tileW, row * tileH, tileW, tileH };

                SDL_Rect futureRect = {
                    (int)body->x,
                    (int)(body->y - extraHeight), // simulate taller body
                    body->collisionRect.w,
                    body->collisionRect.h + extraHeight
                };

                if (collisionCheck(futureRect, tileRect)) {
                    return true; // there is a ceiling in the way
                }
            }
        }
    }

    return false;
}
