#pragma once

#include <SDL.h>
#include <stdbool.h>
#include "settings.h"
#include "camera.h"


// Forward declarations 
typedef struct Player Player;
typedef struct Level Level;
typedef struct TextureCache TextureCache;
typedef struct LevelPaths LevelPaths;


typedef struct mainSystems {
    SDL_Window *window;
    SDL_Renderer *renderer;
} mainSystems;

#define MAX_LEVELS 32
#define MAX_LEVEL_NAME_LEN 64

typedef struct {
    char levelNames[MAX_LEVELS][MAX_LEVEL_NAME_LEN];
    int levelCount;
    int currentLevelIndex;
} LevelList;


typedef enum {
    GAMESTATE_RUNNING,
    GAMESTATE_PAUSED,
    GAMESTATE_MENU,
    GAMESTATE_QUIT
} GameState;



typedef struct GameManager{
    mainSystems mainSystems;
    char* windowName;
    TextureCache *cache;   // TODO
    Level *level;          // Current level
    Player *player;        // Single player instance

    GameSettings settings;
    Camera camera;
    GameState state;       // TODO
    char *currentLevelName;
    float deltaTime;
    bool running;
    LevelPaths *levelPaths; 

} GameManager;

// --- Lifecycle ---
GameManager *GameManager_Create(const char *settingsPath, const char *windowName);
void GameManager_Destroy(GameManager *gm, int exitCode);

// --- State management ---
void GameManager_SetState(GameManager *gm, GameState state);
GameState GameManager_GetState(GameManager *gm);

// --- Level management ---
bool GameManager_LoadLevel(GameManager *gm, const char *levelName);
void GameManager_UnloadLevel(GameManager *gm);

// --- Main loop hooks ---
void GameManager_HandleInput(GameManager *gm);
void GameManager_Update(GameManager *gm, float deltaTime);
void GameManager_Render(GameManager *gm);

