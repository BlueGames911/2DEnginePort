#include "player.h"
#include "utils.h"
#include "texture.h"
#include "settings.h"
#include "gameManager.h"
#include "collision.h"
#include "camera.h"

#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COMBO_WINDOW_FRAMES 2


int wait = 0;

bool Player_LoadConfig(Player *player, struct mainSystems *systems, const char *filePath)
{
    //Read the JSON file into a string
    char *file = read_whole_file(filePath);
    if (!file)
    {
        fprintf(stderr, "[PLAYER] Failed to load %s\n", filePath);
        return false;
    }
    printf("Loaded player.json into memory\n");
    
    // Parse the JSON string into a cJSON root object
    cJSON *configFile = cJSON_Parse(file);
    if (!configFile) 
    {
        free(file);
        fprintf(stderr, "[PLAYER] Failed to parse %s into json file\n", filePath);
        return false;
    }
    free(file);
    printf("Parsed player.json\n");

    //Read sprite scale
    cJSON *scale = cJSON_GetObjectItemCaseSensitive(configFile, "spriteScale");
    if (cJSON_IsNumber(scale)) {
        player->spriteScale = scale->valuedouble;
        printf("Successfully read spriteScale: %.2f\n", player->spriteScale);
    } else {
        fprintf(stderr, "[PLAYER] Warning: 'spriteScale' missing or invalid in %s, defaulting to 1.0\n", filePath);
        player->spriteScale = 1.0f; // default scale
    }
    printf("Loaded Sprite Scale\n");

    // Load sheets array
    cJSON *sheets = cJSON_GetObjectItemCaseSensitive(configFile, "sheets");
    int numOfSheets = cJSON_GetArraySize(sheets);
    player->sheetCount = numOfSheets;
    SpriteSheet *spriteSheets = calloc(numOfSheets, sizeof(SpriteSheet));

    int idx = 0;
    cJSON *sheets_idx = NULL;
    cJSON_ArrayForEach(sheets_idx, sheets)
    {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(sheets_idx, "name");
        cJSON *path = cJSON_GetObjectItemCaseSensitive(sheets_idx, "path");
        cJSON *frameHeight = cJSON_GetObjectItemCaseSensitive(sheets_idx, "frameHeight");
        cJSON *frameWidth = cJSON_GetObjectItemCaseSensitive(sheets_idx, "frameWidth");

        if (cJSON_IsString(name)) {
            spriteSheets[idx].name = _strdup(name->valuestring);
        }

        if (cJSON_IsString(path)) {
            spriteSheets[idx].texture = loadTexture(path->valuestring, systems->renderer);
            if (!spriteSheets[idx].texture) {
                fprintf(stderr, "[PLAYER] Failed to create texture from %s\n", path->valuestring);
                cJSON_Delete(configFile);
                return false;    
            }
        }

        if (!cJSON_IsString(name) || !cJSON_IsString(path)) {
            fprintf(stderr, "[PLAYER] Missing or invalid 'name' or 'path' in sheet entry\n");
            cJSON_Delete(configFile);
            return false;
        }



        if (cJSON_IsNumber(frameHeight) && cJSON_IsNumber(frameWidth)) {
            spriteSheets[idx].frameHeight = frameHeight->valueint;
            spriteSheets[idx].frameWidth = frameWidth->valueint;
        }
        else {
            fprintf(stderr, "[PLAYER] Invalid Value for frameWidth/frameHeight!\n");
            cJSON_Delete(configFile);
            return false;
        }

        idx++;
    }

    player->sheets = spriteSheets;
    printf("Loaded Sprite Sheets\n");

    // Load animations object
    // animationsObj is NOT an array anymore, it's an object with "movements" and "attacks", I probably could've done it better
    cJSON *animationsObj = cJSON_GetObjectItemCaseSensitive(configFile, "animations");
    if (!cJSON_IsObject(animationsObj)) {
        fprintf(stderr, "[PLAYER] 'animations' is not a JSON object!\n");
        cJSON_Delete(configFile);
        return false;
    }

    // Parse "movements" array
    cJSON *movementsArr = cJSON_GetObjectItemCaseSensitive(animationsObj, "movements");
    player->anims.movementCount = cJSON_GetArraySize(movementsArr);
    player->anims.movements = calloc(player->anims.movementCount, sizeof(Animation));

    int animIndex = 0;
    cJSON *movAnim = NULL;
    cJSON_ArrayForEach(movAnim, movementsArr) {
        Animation *anim = &player->anims.movements[animIndex];

        cJSON *name = cJSON_GetObjectItemCaseSensitive(movAnim, "name");
        cJSON *sheet = cJSON_GetObjectItemCaseSensitive(movAnim, "sheet");
        cJSON *frameDuration = cJSON_GetObjectItemCaseSensitive(movAnim, "frameDuration");
        cJSON *loop = cJSON_GetObjectItemCaseSensitive(movAnim, "loop");

        anim->name = cJSON_IsString(name) ? _strdup(name->valuestring) : NULL;

        // Find sheet index
        int sheetIndex = -1;
        if (cJSON_IsString(sheet)) {
            for (int i = 0; i < player->sheetCount; i++) {
                if (strcmp(player->sheets[i].name, sheet->valuestring) == 0) {
                    sheetIndex = i;
                    break;
                }
            }
        }
        if (sheetIndex == -1) {
            fprintf(stderr, "[PLAYER] Unknown sheet name '%s' in movement anim\n",
                    cJSON_IsString(sheet) ? sheet->valuestring : "NULL");
            cJSON_Delete(configFile);
            return false;
        }
        anim->sheetIndex = sheetIndex;

        anim->frameDuration = cJSON_IsNumber(frameDuration) ? (float)frameDuration->valuedouble : 0.1f;
        anim->loop = cJSON_IsBool(loop) ? cJSON_IsTrue(loop) : true;

        // Frames
        cJSON *framesArray = cJSON_GetObjectItemCaseSensitive(movAnim, "frames");
        int frameCount = cJSON_GetArraySize(framesArray);
        anim->frameCount = frameCount;
        anim->frames = calloc(frameCount, sizeof(SDL_Rect));

        int frameIdx = 0;
        cJSON *frameObj = NULL;
        cJSON_ArrayForEach(frameObj, framesArray) {
            cJSON *x = cJSON_GetObjectItemCaseSensitive(frameObj, "x");
            cJSON *y = cJSON_GetObjectItemCaseSensitive(frameObj, "y");

            if (cJSON_IsNumber(x) && cJSON_IsNumber(y)) {
                anim->frames[frameIdx].x = x->valueint * player->sheets[sheetIndex].frameWidth;
                anim->frames[frameIdx].y = y->valueint * player->sheets[sheetIndex].frameHeight;
                anim->frames[frameIdx].w = player->sheets[sheetIndex].frameWidth;
                anim->frames[frameIdx].h = player->sheets[sheetIndex].frameHeight;
            } else {
                fprintf(stderr, "[PLAYER] Invalid frame coordinates in movement anim\n");
                cJSON_Delete(configFile);
                return false;
            }
            frameIdx++;
        }

        // Collision Boxes
        cJSON *collisionBox = cJSON_GetObjectItemCaseSensitive(movAnim, "collisionBox");
        if (collisionBox) {
            cJSON *w = cJSON_GetObjectItemCaseSensitive(collisionBox, "w");
            cJSON *h = cJSON_GetObjectItemCaseSensitive(collisionBox, "h");
            cJSON *offsetX = cJSON_GetObjectItemCaseSensitive(collisionBox, "offsetX");
            cJSON *offsetY = cJSON_GetObjectItemCaseSensitive(collisionBox, "offsetY");

            if (cJSON_IsNumber(w) && cJSON_IsNumber(h) && cJSON_IsNumber(offsetX) && cJSON_IsNumber(offsetY)) {
                anim->collisionProfile = malloc(sizeof(CollisionProfile));
                anim->collisionProfile->widthScale  = w->valuedouble / player->sheets[sheetIndex].frameWidth;
                anim->collisionProfile->heightScale = h->valuedouble / player->sheets[sheetIndex].frameHeight;
                anim->collisionProfile->offsetX     = offsetX->valuedouble * player->spriteScale;
                anim->collisionProfile->offsetY     = offsetY->valuedouble * player->spriteScale;
            }
        }

        // Events, their implementaion is a TODO for now 
        cJSON *eventsArr = cJSON_GetObjectItemCaseSensitive(movAnim, "events");
        if (cJSON_IsArray(eventsArr)) {
            int eventCount = cJSON_GetArraySize(eventsArr);
            anim->eventCount = eventCount;
            anim->events = calloc(eventCount, sizeof(AnimationEvent));
            int eventIdx = 0;
            cJSON *eventObj = NULL;
            cJSON_ArrayForEach(eventObj, eventsArr) {
                cJSON *frameIndex = cJSON_GetObjectItemCaseSensitive(eventObj, "frameIndex");
                cJSON *type = cJSON_GetObjectItemCaseSensitive(eventObj, "type");
                if (cJSON_IsNumber(frameIndex) && cJSON_IsString(type)) {
                    anim->events[eventIdx].frameIndex = frameIndex->valueint;
                    anim->events[eventIdx].eventName  = _strdup(type->valuestring);
                }
                eventIdx++;
            }
        }

        // Flags
        cJSON *invulnerable = cJSON_GetObjectItemCaseSensitive(movAnim, "invulnerable");
        anim->invulnerable = cJSON_IsBool(invulnerable) ? cJSON_IsTrue(invulnerable) : false;

        cJSON *canBeInterrupted = cJSON_GetObjectItemCaseSensitive(movAnim, "canBeInterrupted");
        anim->canBeInterrupted = cJSON_IsBool(canBeInterrupted) ? cJSON_IsTrue(canBeInterrupted) : true;

        animIndex++;
    }

    // Parse "attacks" array (multi-stage attacks)
    cJSON *attacksArr = cJSON_GetObjectItemCaseSensitive(animationsObj, "attacks");
    player->anims.attackCount = cJSON_GetArraySize(attacksArr);
    player->anims.attacks = calloc(player->anims.attackCount, sizeof(AttackDef));

    int attackIndex = 0;
    cJSON *atkObj = NULL;
    cJSON_ArrayForEach(atkObj, attacksArr)
    {
        AttackDef *attack = &player->anims.attacks[attackIndex];

        cJSON *atkName = cJSON_GetObjectItemCaseSensitive(atkObj, "name");
        cJSON *baseDamage = cJSON_GetObjectItemCaseSensitive(atkObj, "baseDamage");

        attack->name = cJSON_IsString(atkName) ? _strdup(atkName->valuestring) : NULL;
        attack->baseDamage = cJSON_IsNumber(baseDamage) ? baseDamage->valueint : 0;

        // Stages
        cJSON *stagesArr = cJSON_GetObjectItemCaseSensitive(atkObj, "stages");
        attack->stageCount = cJSON_GetArraySize(stagesArr);
        attack->stages = calloc(attack->stageCount, sizeof(AttackStage));
        attack->currentStage = 0;

        int stageIndex = 0;
        cJSON *stageObj = NULL;
        cJSON_ArrayForEach(stageObj, stagesArr)
        {
            AttackStage *stage = &attack->stages[stageIndex];

            cJSON *stageName = cJSON_GetObjectItemCaseSensitive(stageObj, "name");
            cJSON *sheet = cJSON_GetObjectItemCaseSensitive(stageObj, "sheet");
            cJSON *frameDuration = cJSON_GetObjectItemCaseSensitive(stageObj, "frameDuration");
            cJSON *loop = cJSON_GetObjectItemCaseSensitive(stageObj, "loop");
            cJSON *damage = cJSON_GetObjectItemCaseSensitive(stageObj, "damage");
            cJSON *canBeComboed = cJSON_GetObjectItemCaseSensitive(stageObj, "can_be_comboed");
            cJSON *canBeInterupted = cJSON_GetObjectItemCaseSensitive(stageObj, "can_be_interupted");

            stage->name = cJSON_IsString(stageName) ? _strdup(stageName->valuestring) : NULL;
            stage->canBeComboed = cJSON_IsBool(canBeComboed) ? cJSON_IsTrue(canBeComboed) : false;
            stage->baseDamage = cJSON_IsNumber(damage) ? damage->valueint : attack->baseDamage;

            // Animation 
            stage->animation.name = stage->name;
            stage->animation.frameDuration = cJSON_IsNumber(frameDuration) ? (float)frameDuration->valuedouble : 0.1f;
            stage->animation.loop = cJSON_IsBool(loop) ? cJSON_IsTrue(loop) : false;
            stage->animation.canBeInterrupted = cJSON_IsBool(canBeInterupted) ? cJSON_IsTrue(canBeInterupted) : false;

            // Find sheet index
            int sheetIndex = -1;
            if (cJSON_IsString(sheet)) {
                for (int i = 0; i < player->sheetCount; i++) {
                    if (strcmp(player->sheets[i].name, sheet->valuestring) == 0) {
                        sheetIndex = i;
                        break;
                    }
                }
            }
            if (sheetIndex == -1) {
                fprintf(stderr, "[PLAYER] Unknown sheet '%s' in attack stage '%s'\n",
                        cJSON_IsString(sheet) ? sheet->valuestring : "NULL",
                        stage->name ? stage->name : "unnamed");
                cJSON_Delete(configFile);
                return false;
            }
            stage->animation.sheetIndex = sheetIndex;

            // Frames 
            cJSON *framesArray = cJSON_GetObjectItemCaseSensitive(stageObj, "frames");
            int frameCount = cJSON_GetArraySize(framesArray);
            stage->animation.frameCount = frameCount;
            stage->animation.frames = calloc(frameCount, sizeof(SDL_Rect));

            int frameIdx = 0;
            cJSON *frameObj = NULL;
            cJSON_ArrayForEach(frameObj, framesArray) {
                cJSON *x = cJSON_GetObjectItemCaseSensitive(frameObj, "x");
                cJSON *y = cJSON_GetObjectItemCaseSensitive(frameObj, "y");

                if (cJSON_IsNumber(x) && cJSON_IsNumber(y)) {
                    stage->animation.frames[frameIdx].x = x->valueint * player->sheets[sheetIndex].frameWidth;
                    stage->animation.frames[frameIdx].y = y->valueint * player->sheets[sheetIndex].frameHeight;
                    stage->animation.frames[frameIdx].w = player->sheets[sheetIndex].frameWidth;
                    stage->animation.frames[frameIdx].h = player->sheets[sheetIndex].frameHeight;
                } else {
                    fprintf(stderr, "[PLAYER] Invalid frame in attack '%s' stage '%s'\n",
                            attack->name, stage->name);
                    cJSON_Delete(configFile);
                    return false;
                }
                frameIdx++;
            }

            // Collision Boxes 
            cJSON *collisionBox = cJSON_GetObjectItemCaseSensitive(stageObj, "collisionBox");
            if (collisionBox) {
                cJSON *w = cJSON_GetObjectItemCaseSensitive(collisionBox, "w");
                cJSON *h = cJSON_GetObjectItemCaseSensitive(collisionBox, "h");
                cJSON *offsetX = cJSON_GetObjectItemCaseSensitive(collisionBox, "offsetX");
                cJSON *offsetY = cJSON_GetObjectItemCaseSensitive(collisionBox, "offsetY");

                if (cJSON_IsNumber(w) && cJSON_IsNumber(h) &&
                    cJSON_IsNumber(offsetX) && cJSON_IsNumber(offsetY)) {
                    stage->animation.collisionProfile = malloc(sizeof(CollisionProfile));
                    stage->animation.collisionProfile->widthScale  = w->valuedouble / player->sheets[sheetIndex].frameWidth;
                    stage->animation.collisionProfile->heightScale = h->valuedouble / player->sheets[sheetIndex].frameHeight;
                    stage->animation.collisionProfile->offsetX     = offsetX->valuedouble * player->spriteScale;
                    stage->animation.collisionProfile->offsetY     = offsetY->valuedouble * player->spriteScale;
                }
            }

            /// Events
            cJSON *eventsArr = cJSON_GetObjectItemCaseSensitive(stageObj, "events");
            if (cJSON_IsArray(eventsArr)) 
            {
                int eventCount = cJSON_GetArraySize(eventsArr);
                stage->animation.eventCount = eventCount;
                stage->animation.events = calloc(eventCount, sizeof(AnimationEvent));
                //stage->animation.events = stage->animation.events; // sync pointer

                int eventIdx = 0;
                cJSON *eventObj = NULL;
                cJSON_ArrayForEach(eventObj, eventsArr)
                {
                    cJSON *frameIndex = cJSON_GetObjectItemCaseSensitive(eventObj, "frameIndex");
                    cJSON *type = cJSON_GetObjectItemCaseSensitive(eventObj, "type");
                    cJSON *value = cJSON_GetObjectItemCaseSensitive(eventObj, "value");

                    if (cJSON_IsNumber(frameIndex) && cJSON_IsString(type)) {
                        stage->animation.events[eventIdx].frameIndex = frameIndex->valueint;
                        stage->animation.events[eventIdx].eventName  = _strdup(type->valuestring);
                        if (cJSON_IsString(value)) {
                            stage->animation.events[eventIdx].value = _strdup(value->valuestring);
                        }
                        if (strcmp(type->valuestring, "hitbox") == 0) {
                            cJSON *w = cJSON_GetObjectItemCaseSensitive(eventObj, "w");
                            cJSON *h = cJSON_GetObjectItemCaseSensitive(eventObj, "h");
                            cJSON *offsetX = cJSON_GetObjectItemCaseSensitive(eventObj, "offsetX");
                            cJSON *offsetY = cJSON_GetObjectItemCaseSensitive(eventObj, "offsetY");

                            if (cJSON_IsNumber(w) && cJSON_IsNumber(h) &&
                                cJSON_IsNumber(offsetX) && cJSON_IsNumber(offsetY)) {
                                stage->hitbox.w = (w->valueint * player->spriteScale);
                                stage->hitbox.h = (h->valueint * player->spriteScale);
                                stage->hitbox.offsetX = offsetX->valueint * player->spriteScale;
                                stage->hitbox.offsetY = offsetY->valueint * player->spriteScale;
                            }
                            else {
                                fprintf(stderr, "[PLAYER] Invalid Hitbox values for attack: %s", stageName->valuestring);
                                return false;
                            }
                        }
                    }
                    eventIdx++;
                }
            }
            stageIndex++;
        }
        attackIndex++;
    }
    printf("Loaded player animations and attacks\n");


    // Load physics object
    cJSON *physics = cJSON_GetObjectItemCaseSensitive(configFile, "physics");
    if (physics) {
        cJSON *runSpeed = cJSON_GetObjectItemCaseSensitive(physics, "runSpeed");
        cJSON *walkSpeed = cJSON_GetObjectItemCaseSensitive(physics, "walkSpeed");
        cJSON *jumpForce = cJSON_GetObjectItemCaseSensitive(physics, "jumpForce");
        cJSON *gravity = cJSON_GetObjectItemCaseSensitive(physics, "gravity");
        cJSON *maxFallSpeed = cJSON_GetObjectItemCaseSensitive(physics, "maxFallSpeed");
        cJSON *crouchHeightOffset = cJSON_GetObjectItemCaseSensitive(physics, "crouchHeightOffset");
        cJSON *spriteScale = cJSON_GetObjectItemCaseSensitive(physics, "spriteScale");
        cJSON *acceleration = cJSON_GetObjectItemCaseSensitive(physics, "acceleration");
        cJSON *decceleration = cJSON_GetObjectItemCaseSensitive(physics, "decceleration");
        cJSON *crouchLag = cJSON_GetObjectItemCaseSensitive(physics, "crouchLag");
        cJSON *crouchSpeed = cJSON_GetObjectItemCaseSensitive(physics, "crouchSpeed");

        if (cJSON_IsNumber(crouchLag)) {
            player->crouchLag = crouchLag->valuedouble;
        } else {
            fprintf(stderr, "[PLAYER] Invalid or missing crouchLag in physics\n");
            cJSON_Delete(configFile);
            return false;
        }

        if (cJSON_IsNumber(decceleration)) {
            player->deceleration = decceleration->valueint;
        } else {
            fprintf(stderr, "[PLAYER] Invalid or missing decceleration in physics\n");
            cJSON_Delete(configFile);
            return false;
        }

        if (cJSON_IsNumber(acceleration)) {
            player->acceleration = acceleration->valueint;
        } else {
            fprintf(stderr, "[PLAYER] Invalid or missing acceleration in physics\n");
            cJSON_Delete(configFile);
            return false;
        }

        if (cJSON_IsNumber(runSpeed)) {
            player->runSpeed = runSpeed->valuedouble;
        } else {
            fprintf(stderr, "[PLAYER] Invalid or missing runSpeed in physics\n");
            cJSON_Delete(configFile);
            return false;
        }

        if (cJSON_IsNumber(crouchSpeed)) {
            player->crouchSpeed = crouchSpeed->valuedouble;
        } else {
            fprintf(stderr, "[PLAYER] Invalid or missing crouchSpeed in physics\n");
            cJSON_Delete(configFile);
            return false;
        }

        if (cJSON_IsNumber(walkSpeed)) {
            player->walkSpeed = walkSpeed->valuedouble;
        } else {
            fprintf(stderr, "[PLAYER] Invalid or missing walkSpeed in physics\n");
            cJSON_Delete(configFile);
            return false;
        }

        if (cJSON_IsNumber(jumpForce)) {
            player->jumpForce = jumpForce->valuedouble;
        } else {
            fprintf(stderr, "[PLAYER] Invalid or missing jumpForce in physics\n");
            cJSON_Delete(configFile);
            return false;
        }

        if (cJSON_IsNumber(gravity)) {
            player->gravity = gravity->valuedouble;
        } else {
            fprintf(stderr, "[PLAYER] Invalid or missing gravity in physics\n");
            cJSON_Delete(configFile);
            return false;
        }

        if (cJSON_IsNumber(maxFallSpeed)) {
            player->maxFallSpeed = maxFallSpeed->valuedouble;
        } else {
            fprintf(stderr, "[PLAYER] Invalid or missing maxFallSpeed in physics\n");
            cJSON_Delete(configFile);
            return false;
        }

        if (cJSON_IsNumber(crouchHeightOffset)) {
            player->crouchHeightOffset = crouchHeightOffset->valuedouble;
        } else {
            fprintf(stderr, "[PLAYER] Invalid or missing crouchHeightOffset in physics\n");
            cJSON_Delete(configFile);
            return false;
        }


    } else {
        fprintf(stderr, "[PLAYER] Missing physics object in config\n");
        cJSON_Delete(configFile);
        return false;
    }
    printf("Loaded Player Physics\n");

    // Note from CoPilot: (Optional) Load "stateMap"
    //    - If you add this to JSON:
    //        * For each PlayerState enum name in JSON, find the animation index and store in player->stateToAnim[state]

    cJSON_Delete(configFile);

    //  Return true if everything loaded successfully
    printf("Player spriteScale: %.2f\n", player->spriteScale);
    player->state = PLAYER_IDLE;

    player->currentAttackIndex = -1;
    player->currentAttackStage = 0;
    player->currentAnimIndex = Player_FindAnimation(player, "idle");
    player->currentFrame = 0;
    player->frameTimer = 0.0f;
    player->hitboxActive = false;
    player->comboExtendRequested = false;
    player->canBeInterrupted = true;

    return true;
}

void Player_Destroy(Player *player)
{
    if (!player) return;

    // Free sprite sheets
    for (int i = 0; i < player->sheetCount; i++) {
        if (player->sheets[i].name) free(player->sheets[i].name);
        if (player->sheets[i].texture) SDL_DestroyTexture(player->sheets[i].texture);
    }
    free(player->sheets);
    printf("Freed sprite sheets\n");

    // Free animations
    for (int i = 0; i < player->anims.movementCount; i++) {
        if (player->anims.movements[i].name) free(player->anims.movements[i].name);
        if (player->anims.movements[i].frames) free(player->anims.movements[i].frames);
        if (player->anims.movements[i].events) {
            for (int j = 0; j < player->anims.movements[i].eventCount; j++) {
                if (player->anims.movements[i].events[j].eventName) free(player->anims.movements[i].events[j].eventName);
            }
            free(player->anims.movements[i].events);
        }
        if (player->anims.movements[i].collisionProfile) {
            free(player->anims.movements[i].collisionProfile);
        }
    }
    free(player->anims.movements);
    printf("Freed Animations\n");

    // Free attacks
    for (int i = 0; i < player->anims.attackCount; i++) {
        if (player->anims.attacks[i].name) free(player->anims.attacks[i].name);
        //if (player->anims.attacks[i].category) free(player->anims.attacks[i].category);
        //if (player->anims.attacks[i].hitboxes) free(player->anims.attacks[i].hitboxes);
    }
    free(player->anims.attacks);
    printf("Freed Attacks\n");

    //This was giving me a few misbehaviours, I'll need to fix it later
    //free(player);

}

static Animation* Player_GetCurrentAnimation(Player *player) {
    if (!player) return NULL;

    // Prefer attack animation if an attack index is valid.
    if ((player->state == PLAYER_GROUND_ATTACKING || player->state == PLAYER_AIR_ATTACKING) &&
        player->currentAttackIndex >= 0 &&
        player->currentAttackIndex < player->anims.attackCount)
    {
        AttackDef *attack = &player->anims.attacks[player->currentAttackIndex];
        int s = player->currentAttackStage;
        if (attack && s >= 0 && s < attack->stageCount) {
            return &attack->stages[s].animation;
        }
    }

    // Fallback to movement animation with bounds checks.
    int idx = player->currentAnimIndex;
    if (idx < 0 || idx >= player->anims.movementCount) return NULL;
    return &player->anims.movements[idx];
}



// Start a specific attack stage for a given attack definition.
// NOTE: this writes runtime state only into player (index & stage). It doesn't modify AttackDef.
static void Player_StartAttackStage(Player *player, int attackIndex, int stageIdx)
{
    if (!player) return;
    if (!player->anims.attacks) return;
    if (attackIndex < 0 || attackIndex >= player->anims.attackCount) return;

    AttackDef *attack = &player->anims.attacks[attackIndex];
    if (stageIdx < 0 || stageIdx >= attack->stageCount) return;

    // Set runtime indexes on player
    player->currentAttackIndex = attackIndex;
    player->currentAttackStage = stageIdx;

    AttackStage *stage = &attack->stages[stageIdx];
    Animation *anim = &stage->animation;

    // Reset frame/timers
    player->currentAnimIndex = -1; // invalidate movement index while attacking
    player->currentFrame = 0;
    player->frameTimer = 0.0f;

    // Stage timing
    player->attackTimer = anim->frameDuration * (anim->frameCount > 0 ? anim->frameCount : 1);

    // Hitbox 
    player->activeHitbox = stage->hitbox;
    player->hitboxActive = false;

    // Lock input if animation says so (false = not interruptible)
    player->canBeInterrupted = anim->canBeInterrupted;

    // Debug
    printf("StartAttackStage: attack %d stage %d animFrames=%d\n", attackIndex, stageIdx, anim->frameCount);
}


void Player_Update(Player *player, float deltaTime, Level *lvl)
{

    if (!player || !lvl) return;
    if (player->state == PLAYER_GROUND_ATTACKING)
    {
        player->body.velocity_x = 0;
    }
    Player_UpdatePhysics(player, deltaTime, lvl);

    // Update state machine
    PlayerState newState = PLAYER_IDLE;

    if (player->isAttacking && player->body.isOnGround) {
        newState = PLAYER_GROUND_ATTACKING;
        player->body.isMovingLeft = false;
        player->body.isMovingRight = false;
    }
    else if (player->isAttacking && !player->body.isOnGround) {
        newState = PLAYER_AIR_ATTACKING;
    }
    else if (!player->body.isOnGround && player->body.velocity_y < 0) {
        newState = PLAYER_JUMPING;
    }
    else if (!player->body.isOnGround && player->body.velocity_y > 0) {
        newState = PLAYER_FALLING;
    }
    else if (player->isCrouching && (player->body.isMovingLeft || player->body.isMovingRight)) {
        newState = PLAYER_CROUCH_WALKING;
    }
    else if (player->isCrouching) {
        newState = PLAYER_CROUCH;
    }
    else if (player->body.isMovingLeft || player->body.isMovingRight) {
        newState = PLAYER_RUNNING;
    }

    if (newState == PLAYER_GROUND_ATTACKING)
    {
        player->body.velocity_x = 0;
    }

    // Apply the new state
    Player_SetState(player, newState);
    
    Player_UpdateAnimation(player, deltaTime);
    Player_UpdateCollisionBox(player, lvl);

    //Update attack hitbox if attacking
    Player_UpdateAttackHitbox(player);

}

void Player_UpdatePhysics(Player *player, float deltaTime, Level *lvl)
{
    if (!player || !lvl) return;

    // movement flags
    if (player->body.isMovingLeft) player->body.flip = SDL_FLIP_HORIZONTAL;
    if (player->body.isMovingRight) player->body.flip = SDL_FLIP_NONE;

    // Horizontal movement
    float accel = player->acceleration;
    float decel = player->deceleration;

    if (player->isCrouching) accel /= player->crouchLag;

    if (player->body.isMovingRight) player->body.velocity_x += accel * deltaTime;
    else if (player->body.isMovingLeft) player->body.velocity_x -= accel * deltaTime;
    else {
        if (player->body.velocity_x > 0) { player->body.velocity_x -= decel * deltaTime; if (player->body.velocity_x < 0) player->body.velocity_x = 0; }
        else if (player->body.velocity_x < 0) { player->body.velocity_x += decel * deltaTime; if (player->body.velocity_x > 0) player->body.velocity_x = 0; }
    }

    float maxSpeed = player->isCrouching ? player->crouchSpeed : player->runSpeed;
    if (player->body.velocity_x > maxSpeed) player->body.velocity_x = maxSpeed;
    if (player->body.velocity_x < -maxSpeed) player->body.velocity_x = -maxSpeed;

    if ((!player->body.isOnGround) && player->state == PLAYER_AIR_ATTACKING)
        player->body.velocity_x /= 1.3;


    // Vertical movement
    if (player->jumpRequested && player->body.isOnGround) {
        player->body.velocity_y = player->jumpForce; // negative for up (Its negative in the json, hopefully...)
        player->body.isOnGround = false;
        player->jumpRequested = false;
    }

    //Cant seem to figure this simple thing out
    float fallMultiplier = 1.15;
    
    if (player->state == PLAYER_FALLING)
    {
        player->body.velocity_y += player->gravity * fallMultiplier * deltaTime;
    }
    else
    {
        player->body.velocity_y += player->gravity * deltaTime;
    }


    // Apply gravity
    if (player->body.velocity_y > player->maxFallSpeed) player->body.velocity_y = player->maxFallSpeed;

    // Horizontal collision
    player->body.x += player->body.velocity_x * deltaTime;
    checkEntityTileCollisionsX(&player->body, lvl, deltaTime);

    // Vertical collision
    player->body.y += player->body.velocity_y * deltaTime;
    checkEntityTileCollisionsY(&player->body, lvl, deltaTime);
}

void Player_UpdateCollisionBox(Player *player, Level *lvl) {
    Animation *anim = Player_GetCurrentAnimation(player);
    if (!anim) return;

    // Clamp frame index before indexing
    if (player->currentFrame < 0) player->currentFrame = 0;
    if (anim->frameCount <= 0) return;

    if (player->currentFrame >= anim->frameCount) {
        player->currentFrame = anim->frameCount - 1;
    }

    // Safe collision profile
    CollisionProfile defaultProf = { 1.0f, 1.0f, 0.0f, 0.0f };
    CollisionProfile *profile = anim->collisionProfile
        ? anim->collisionProfile
        : (player->collisionProfiles ? &player->collisionProfiles[0] : &defaultProf);

    SDL_Rect frame = anim->frames[player->currentFrame];

    //Crouching offset, yes I know, I could use a json variable, I'll do it later
    int oldHeight = player->body.collisionRect.h;

    player->body.collisionRect.w = (int)(frame.w * player->spriteScale * profile->widthScale);
    player->body.collisionRect.h = (int)(frame.h * player->spriteScale * profile->heightScale);

    int newHeight = player->body.collisionRect.h;

    player->body.collisionRect.x = (int)(player->body.x);
    player->body.collisionRect.y = (int)(player->body.y);

    if (player->isCrouching) {
        player->body.y += (oldHeight - newHeight);
    } else {
        int extraHeight = newHeight - oldHeight;
        if (extraHeight > 0) {
            if (!hasCeilingAbove(&player->body, lvl, extraHeight)) {
                player->body.y += (oldHeight - newHeight);
            } else {
                player->isCrouching = true;
                player->body.collisionRect.h = oldHeight;
            }
        }
    }
}

void Player_UpdateAttackHitbox(Player *player)
{
    // Only update hitbox if currently attacking
    if (!player) return;

    if (player->state != PLAYER_GROUND_ATTACKING && player->state != PLAYER_AIR_ATTACKING) {
        player->hitboxActive = false;
        return;
    }

    int attackIdx = player->currentAttackIndex;
    if (attackIdx < 0 || attackIdx >= player->anims.attackCount) return;

    AttackDef *attack = &player->anims.attacks[attackIdx];
    int stageIdx = player->currentAttackStage;

    if (stageIdx < 0 || stageIdx >= attack->stageCount) return;

    AttackStage *stage = &attack->stages[stageIdx];

    // Keep hitbox active until explicitly replaced or animation ends
    for (int i = 0; i < stage->animation.eventCount; i++) {
        AnimationEvent *evt = &stage->animation.events[i];
        if (evt->frameIndex == player->currentFrame && strcmp(evt->eventName, "hitbox") == 0) {
            int baseX = player->body.collisionRect.x;
            int baseY = player->body.collisionRect.y;

            int hitboxX = baseX + (player->body.flip == SDL_FLIP_NONE 
                                   ? stage->hitbox.offsetX 
                                   : -stage->hitbox.offsetX - stage->hitbox.w);
            int hitboxY = baseY + stage->hitbox.offsetY;

            player->activeHitbox.x = hitboxX;
            player->activeHitbox.y = hitboxY;
            player->activeHitbox.w = stage->hitbox.w;
            player->activeHitbox.h = stage->hitbox.h;
            player->hitboxActive = true;

            break;
        }
    }

    // If we've finished the animation, disable hitbox
    Animation *anim = Player_GetCurrentAnimation(player);
    if (anim && player->currentFrame >= anim->frameCount) {
        player->hitboxActive = false;
    }
}

// Helper to find attack index by name, could'nt use the regular one
//Should be a simple fix though, I'll update it later
int Player_FindAttack(const Player *player, const char *name) {
    if (!player || !name) return -1;
    for (int i = 0; i < player->anims.attackCount; i++) {
        if (player->anims.attacks[i].name && strcmp(player->anims.attacks[i].name, name) == 0)
            return i;
    }
    return -1;
}

void Player_SetState(Player *player, PlayerState newState)
{
    if (!player) return;
    if (player->state == newState) return;

    // If current animation is uninterruptible, block state changes.
    if (!player->canBeInterrupted) {
        return;
    }

    // When leaving attack states
    if (player->state == PLAYER_GROUND_ATTACKING || player->state == PLAYER_AIR_ATTACKING) {
        if (newState != PLAYER_GROUND_ATTACKING && newState != PLAYER_AIR_ATTACKING) {
            player->currentAttackIndex = -1;
            player->currentAttackStage = 0;
            player->attackTimer = 0.0f;
            player->comboExtendRequested = false;
            player->hitboxActive = false;
            player->canBeInterrupted = true;
            player->isAttacking = false;
        }
    }

    // Apply new state
    player->state = newState;

    switch (newState) {
        case PLAYER_JUMPING:
            Player_PlayAnimation(player, "jump");
            break;

        case PLAYER_FALLING:
            Player_PlayAnimation(player, "fall");
            break;

        case PLAYER_CROUCH:
            Player_PlayAnimation(player, "crouch");
            break;

        case PLAYER_CROUCH_WALKING:
            Player_PlayAnimation(player, "crouch-walk");
            break;

        case PLAYER_RUNNING:
            Player_PlayAnimation(player, "run");
            break;

        case PLAYER_GROUND_ATTACKING: {
            int attackIdx = Player_FindAttack(player, "ground_slash");
            if (attackIdx >= 0) {
                player->currentAttackIndex = attackIdx;
                player->currentAttackStage = 0;
                Player_StartAttackStage(player, attackIdx, 0);
            } else {
                // fallback: ensure we don't remain in an invalid attacking state
                player->state = PLAYER_IDLE;
                player->isAttacking = false;
            }
            break;
        }

        case PLAYER_AIR_ATTACKING: {
            int attackIdx = Player_FindAttack(player, "air_slash");
            if (attackIdx >= 0) {
                player->currentAttackIndex = attackIdx;
                player->currentAttackStage = 0;
                Player_StartAttackStage(player, attackIdx, 0);
            } else {
                player->state = PLAYER_FALLING;
                player->isAttacking = false;
            }
            break;
        }

        default:
            Player_PlayAnimation(player, "idle");
            break;
    }

    // debug
    //printf("Player_SetState -> %d (attackIndex=%d stage=%d)\n", player->state, player->currentAttackIndex, player->currentAttackStage);
}

// Helper to find animation by name, only movements for now
int Player_FindAnimation(const Player *player, const char *name)
{
    if (!player || !name) return -1;

    for (int i = 0; i < player->anims.movementCount; i++) 
    {
        if (player->anims.movements[i].name && strcmp(player->anims.movements[i].name, name) == 0) 
            return i;
    }

    return -1;
}

bool Player_PlayAnimation(Player *player, const char *name)
{
    int index = Player_FindAnimation(player, name);
    if (index == -1 || index == player->currentAnimIndex) return false;

    player->currentAnimIndex = index;
    player->currentFrame = 0;
    player->frameTimer = 0;

    Animation *a = &player->anims.movements[index];
    player->canBeInterrupted = a->canBeInterrupted;
    return true;

    return true;
}

void Player_UpdateAnimation(Player *player, float deltaTime) 
{
    if (!player) return;

    Animation *anim = Player_GetCurrentAnimation(player);
    if (!anim) return;

    player->frameTimer += deltaTime;

    if (player->frameTimer >= anim->frameDuration) {
        player->frameTimer -= anim->frameDuration;
        player->currentFrame++;

        // Attack stage handling
        if (player->state == PLAYER_GROUND_ATTACKING || player->state == PLAYER_AIR_ATTACKING) {
            AttackDef *attack = &player->anims.attacks[player->currentAttackIndex];
            AttackStage *stage = &attack->stages[player->currentAttackStage];

            if (player->currentFrame >= stage->animation.frameCount) {
                // Stage finished
                bool grounded = player->body.isOnGround;

                if (player->comboExtendRequested && stage->canBeComboed &&
                    player->currentAttackStage + 1 < attack->stageCount) {
                    // Chain to next stage
                    player->comboExtendRequested = false;
                    Player_StartAttackStage(player, player->currentAttackIndex, player->currentAttackStage + 1);
                } else {
                    // Reset to movement state
                    player->currentAttackIndex = -1;
                    player->currentAttackStage = 0;
                    player->comboExtendRequested = false;
                    player->isAttacking = false;
                    player->canBeInterrupted = true;

                    if (player->state == PLAYER_AIR_ATTACKING && !grounded) 
                    {
                        Player_SetState(player, PLAYER_FALLING);
                        printf("Attack finished state=%d\n", player->state);
                    } else {
                        Player_SetState(player, PLAYER_IDLE);
                        printf("Attack finished state=%d\n", player->state);
                    }
                }
            }
        }
        else {
            // Non-attack animations
            if (player->currentFrame >= anim->frameCount) {
                player->currentFrame = anim->loop ? 0 : anim->frameCount - 1;
            }
        }
    }
}

void Player_Render(const Player *player, const struct mainSystems *systems, const Camera *camera, bool debug)
{
    if (!player || !systems || !systems->renderer) return;

    // get current animation
    Animation *anim = NULL;
    Player *p = (Player *)player;
    anim = Player_GetCurrentAnimation(p);
    if (!anim) return;

    // safety for frame index
    if (player->currentFrame < 0 || player->currentFrame >= anim->frameCount) return;

    // sheet must be valid
    if (!player->sheets) return;
    if (anim->sheetIndex < 0 || anim->sheetIndex >= player->sheetCount) return;
    SpriteSheet *sheet = &player->sheets[anim->sheetIndex];
    SDL_Rect frame = anim->frames[player->currentFrame];

    SDL_Rect destRect;
    destRect.w = (int)(frame.w * player->spriteScale);
    destRect.h = (int)(frame.h * player->spriteScale);

    // Using collisionRect as anchor 
    destRect.x = (int)(player->body.collisionRect.x - camera->x + (player->body.collisionRect.w - destRect.w) / 2);
    destRect.y = (int)(player->body.collisionRect.y - camera->y + (player->body.collisionRect.h - destRect.h));

    // apply collision profile offsets if present
    CollisionProfile *profile = anim->collisionProfile ? anim->collisionProfile
                                                       : (player->collisionProfiles ? &player->collisionProfiles[0] : NULL);
    if (profile) {
        int offsetX = (int)(profile->offsetX * player->spriteScale);
        destRect.x += (player->body.flip == SDL_FLIP_NONE) ? -offsetX : offsetX;
        destRect.y += (int)(profile->offsetY * player->spriteScale);
    }

    SDL_RenderCopyEx(systems->renderer, sheet->texture, &frame, &destRect, 0, NULL, player->body.flip);

    // debug draw hitbox & collision
    if (true) {
        // collision rect
        SDL_SetRenderDrawColor(systems->renderer, 0, 255, 0, 100);
        SDL_Rect collRect = {
            player->body.collisionRect.x - camera->x,
            player->body.collisionRect.y - camera->y,
            player->body.collisionRect.w,
            player->body.collisionRect.h
        };
        SDL_RenderDrawRect(systems->renderer, &collRect);

        // active hitbox
        if (player->hitboxActive) {
            SDL_SetRenderDrawColor(systems->renderer, 255, 0, 0, 100);
            SDL_Rect hitRect = {
                player->activeHitbox.x - camera->x,
                player->activeHitbox.y - camera->y,
                player->activeHitbox.w,
                player->activeHitbox.h
            };
            SDL_RenderDrawRect(systems->renderer, &hitRect);
        }
    }
}

void Player_HandleInput(Player *player) 
{
    if (!player) return;

    const Uint8 *state = SDL_GetKeyboardState(NULL);
    #define SAFE_STATE(scancode) ((scancode >= 0 && scancode < SDL_NUM_SCANCODES) ? state[scancode] : 0)

   // If attack animation is active, only allow combo requests in the last X frames
    if ((player->state == PLAYER_GROUND_ATTACKING || player->state == PLAYER_AIR_ATTACKING) && !player->canBeInterrupted) {
        int attackIdx = player->currentAttackIndex;
        int stageIdx  = player->currentAttackStage;

        if (attackIdx >= 0 && attackIdx < player->anims.attackCount) {
            AttackDef *attack = &player->anims.attacks[attackIdx];
            if (stageIdx >= 0 && stageIdx < attack->stageCount) {
                AttackStage *stage = &attack->stages[stageIdx];
                int framesLeft = stage->animation.frameCount - player->currentFrame;

                if (framesLeft <= COMBO_WINDOW_FRAMES && stage->canBeComboed) {
                    if (SAFE_STATE(player->controls.attack)) {
                        player->comboExtendRequested = true;
                    }
                }
            }
        }
        return; // ignore all other inputs until attack ends, hopefully
    }


    // Movement inputs
    player->body.isMovingLeft  = SAFE_STATE(player->controls.moveLeft);
    player->body.isMovingRight = SAFE_STATE(player->controls.moveRight);
    player->isCrouching        = SAFE_STATE(player->controls.crouch);
    player->isDashing          = SAFE_STATE(player->controls.dash);

    // Jump input
    player->jumpRequested = SAFE_STATE(player->controls.jump);
    if (!player->body.isOnGround) {
        player->jumpRequested = false; // prevent double jump, for now, hehehe
    }

    // Attack input
    if (SAFE_STATE(player->controls.attack)) {
        if (player->body.isOnGround) {
            player->isAttacking = false;
            Player_SetState(player, PLAYER_GROUND_ATTACKING);
        } else {
            player->isAttacking = false;
            Player_SetState(player, PLAYER_AIR_ATTACKING);
        }
    }

}



