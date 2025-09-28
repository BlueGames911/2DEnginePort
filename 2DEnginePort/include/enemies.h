#pragma once

#include "gameManager.h"
#include "collision.h"
#include "animation.h"
#include "enemy_behaviour.h"

#include <SDL.h>

typedef enum state
{
	ENEMY_IDLE,
	ENEMY_FALLING,
	ENEMY_SCOUTING,
	ENEMY_WALKING,
	ENEMY_ATTACKING,
	ENEMY_HURT,
	ENEMY_DIE,
	ENEMY_STATE_COUNT
}state;

typedef struct {
	Animation* movements;
	int movementCount;

	AttackDef* attacks;
	int attackCount;
} EnemyAnimations;

typedef struct enemy
{
	bool isAttacking;
	bool isDead;
	int level;
	int id;
	char* enemyType;

	float acceleration;
	float decceleration;
	float runSpeed;
	float gravity;

	EnemyAnimations animations;

	const EnemyBehavior* behavior; // which behavior/vtable this enemy uses
	void* behaviorData; // per-enemy custom state

	SpriteSheet* sheets;
	bool isOnScreen;
	int sheetCount;
	float spriteScale;

	state EnemyState;
	PhysicsBody body;
}enemy;

// Will change these functions later, most are unnecesary Im sure
bool Enemy_LoadFromFile(enemy* e, const char* path);
bool Enemy_Load(enemy* enemy);
void Enemy_Spawn(enemy* enemy);

void Enemy_Update(enemy* enemy, float deltaTime, Player* player);
void Enemy_UpdatePhysics(enemy* enemy);
void Enemy_Attack(enemy* enemy, Player *player);

void Enemy_Render(enemy* enemy, mainSystems* system);
void Enemy_Die(enemy* enemy);
void Enemy_Unload(enemy* enemy);