#pragma once
#include <SDL.h>
#include "level.h"

typedef struct Player Player;

typedef struct Camera{
    int x, y;
    int w, h;
    int deadZoneW, deadZoneH;
    float followSpeed;
    int lookAheadX; // optional for future use
} Camera;


void Camera_Init(Camera *cam, int viewportW, int viewportH);
void Camera_Update(Camera *cam, const Player *player, Level *lvl);
SDL_Rect Camera_GetViewRect(const Camera *cam);
