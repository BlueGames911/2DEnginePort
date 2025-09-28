#pragma once

#include "collision.h"
#include "animation.h"

#include <SDL.h>
#include <stdbool.h>

// Forward declarations
typedef struct Level Level;
struct mainSystems;
typedef struct GameSettings GameSettings;
typedef struct Camera Camera;

// Player States
typedef enum {
    PLAYER_IDLE,
    PLAYER_RUNNING,
    PLAYER_JUMPING,
    PLAYER_FALLING,
    PLAYER_CROUCH,
    PLAYER_CROUCH_WALKING,
    PLAYER_GROUND_ATTACKING,
    PLAYER_AIR_ATTACKING,
    PLAYER_ROLLING,
    PLAYER_STATE_COUNT
} PlayerState;

typedef struct {
    SDL_Scancode moveLeft;
    SDL_Scancode moveRight;
    SDL_Scancode jump;
    SDL_Scancode attack;
    SDL_Scancode crouch;
    SDL_Scancode dash;
} Controls;


typedef struct {
    Animation *movements;
    int movementCount;

    AttackDef *attacks;
    int attackCount;
} PlayerAnimations;


// Player Entity
typedef struct Player{
    // Position & motion
    float acceleration;
    float crouchLag;
    float crouchSpeed;
    float deceleration;

    // State
    PlayerState state;
    bool isCrouching;
    bool jumpRequested;
    bool isAttacking;
    bool isDashing;
    int attackStage;
    float attackTimer;
    bool comboExtendRequested;
    bool hitboxActive;
    bool canBeInterrupted;

    // Collision
    CollisionProfile *collisionProfiles;
    int profileCount;
    PhysicsBody body;

    // Attacks
    SDL_Rect *attackRects;     // runtime positioned hitboxes
    int currentAttackIndex;    // index of current attack 
    int currentAttackStage;    // index of current stage in the attack
    AttackHitbox activeHitbox;

    // Animation
    int currentAnimIndex;
    int currentFrame;
    float frameTimer;

    SpriteSheet *sheets;
    int sheetCount;
    float spriteScale;        // visual scaling factor

    PlayerAnimations anims;

    // Later
    //int stateToAnim[PLAYER_STATE_COUNT];
    Controls controls;

    // Physics config
    float runSpeed;
    float walkSpeed;
    float jumpForce;
    float gravity;
    float maxFallSpeed;
    float crouchHeightOffset;

    // Audio (future use)
    //char **soundMap;       // array of sound file paths per animation
    //int soundCount;
} Player;


// Lifecycle

bool Player_LoadConfig(Player *player, struct mainSystems *systems, const char *filePath);
void Player_Destroy(Player *player);

// Main loop

void Player_Update(Player *player, float deltatime, Level *lvl);
void Player_Render(const Player *player, const struct mainSystems *systems, const Camera *camera, bool debug);


// updates

void Player_HandleInput(Player *player);
void Player_UpdatePhysics(Player *player, float deltaTime, Level *lvl);
void Player_UpdateAnimation(Player *player, float deltaTime);
void Player_UpdateAttackHitbox(Player *player);
void Player_UpdateCollisionBox(Player *player, Level *lvl);


// State control

void Player_SetState(Player *player, PlayerState state);
int Player_FindAnimation(const Player *player, const char *name);
int Player_FindAttack(const Player *player, const char *name);
bool Player_PlayAnimation(Player *player, const char *name);
