#include "camera.h"
#include "gameManager.h"
#include "level.h"
#include "player.h"
#include <SDL.h>

// Camera Initialization
void Camera_Init(Camera *cam, int viewportW, int viewportH)
{
    if (!cam) return;
    cam->x = 0;
    cam->y = 0;
    cam->w = viewportW;
    cam->h = viewportH;
    cam->lookAheadX = 0;
    cam->deadZoneW = viewportW / 12; 
    cam->deadZoneH = viewportH / 12; 
    cam->followSpeed = 0.15f;       // smoothness factor
}

// Used copilot to do this, though it needs updates, I'll take a look at it soon
void Camera_Update(Camera *cam, const Player *player, Level *lvl)
{
    if (!cam || !player || !lvl) return;

    int tileW = getLevelTileWidth(lvl);
    int mapW = lvl->levelColumns * tileW;
    int mapH = lvl->levelRows * tileW;


    int playerCenterX = player->body.x + player->body.collisionRect.w / 2;
    int playerCenterY = player->body.y + player->body.collisionRect.h / 2;

    // Look-ahead offset based on facing direction
    int offsetX = (player->body.flip == SDL_FLIP_NONE) ? cam->deadZoneW : -cam->deadZoneW;

    int targetX = playerCenterX + offsetX - cam->w / 2;
    int targetY = playerCenterY - cam->h / 2;


    // dead zone logic
    int camCenterX = cam->x + cam->w / 2;
    int camCenterY = cam->y + cam->h / 2;

    int dx = playerCenterX - camCenterX;
    int dy = playerCenterY - camCenterY;

    if (abs(dx) > cam->deadZoneW) {
        cam->x += (int)((dx - (dx > 0 ? cam->deadZoneW : -cam->deadZoneW)) * cam->followSpeed);
    }

    if (abs(dy) > cam->deadZoneH) {
        cam->y += (int)((dy - (dy > 0 ? cam->deadZoneH : -cam->deadZoneH)) * cam->followSpeed);
    }

    // Clamp to map bounds
    if (cam->x < 0) cam->x = 0;
    if (cam->y < 0) cam->y = 0;
    if (cam->x > mapW - cam->w) cam->x = mapW - cam->w;
    if (cam->y > mapH - cam->h) cam->y = mapH - cam->h;
}

// Get Camera View Rect
SDL_Rect Camera_GetViewRect(const Camera *cam)
{
    SDL_Rect rect = {0, 0, 0, 0};
    if (!cam) return rect;
    rect.x = cam->x;
    rect.y = cam->y;
    rect.w = cam->w;
    rect.h = cam->h;
    return rect;
}
