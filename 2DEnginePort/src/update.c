#include "update.h"
#include "camera.h"
#include "player.h"
#include "gameManager.h"
#include "level.h"

void Update(GameManager *gm)
{
    Camera_Update(&gm->camera, gm->player, gm->level);
    Player_Update(gm->player, gm->deltaTime, gm->level);
}