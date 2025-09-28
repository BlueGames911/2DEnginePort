#pragma once

// collision.h

#include <SDL.h>
#include <stdbool.h>
#include "constants.h"


typedef struct {
    float x, y;              // world position
    float velocity_x, velocity_y;
    SDL_Rect collisionRect;  // local collision box
    float colliderOffsetX, colliderOffsetY;
    bool isMovingLeft, isMovingRight, isOnGround;
    SDL_RendererFlip flip;
} PhysicsBody;


typedef struct Player Player;
typedef struct Level Level; 
typedef struct Layer Layer;

bool isSolidTile(Level *lvl, int tileIndex);
bool isSolidTileInLayer(Layer *layer, int tileIndex);
bool collisionCheck(SDL_Rect A, SDL_Rect B);
void checkEntityTileCollisionsX(PhysicsBody *body, Level *lvl, float deltaTime);
void checkEntityTileCollisionsY(PhysicsBody *body, Level *lvl, float deltaTime);
bool hasCeilingAbove(const PhysicsBody *body, Level *lvl, int extraHeight);