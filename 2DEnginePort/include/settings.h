#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <SDL.h>

typedef struct Player Player;
typedef struct mainSystems mainSystems;

typedef struct {
    SDL_Scancode moveLeft;
    SDL_Scancode moveRight;
    SDL_Scancode jump;
    SDL_Scancode attack;
    SDL_Scancode crouch;
    SDL_Scancode dash;
} ControlsSettings;

typedef struct VideoSettings{
    int width;
    int height;
    bool fullscreen;
    bool vsync;
    float scale;
} VideoSettings;

typedef struct {
    float masterVolume;
    float musicVolume;
    float sfxVolume;
} AudioSettings;

typedef struct {
    bool debugMode;
} GameplaySettings;

typedef struct GameSettings {
    ControlsSettings controls;
    VideoSettings video;
    AudioSettings audio;
    GameplaySettings gameplay;
} GameSettings;


bool Settings_Load(GameSettings *settings, const char *filePath);
bool Settings_Save(const GameSettings *settings, const char *filePath);

void Settings_Apply(const GameSettings *settings, mainSystems *systems, Player *player);
void Settings_ApplyControls(Player *player, const ControlsSettings *controls);
void Settings_ApplyAudio(const AudioSettings *audio);
void Settings_ApplyVideo(mainSystems *systems, const VideoSettings *video);
void Settings_ApplyGameplay(const GameplaySettings *gameplay);
bool Settings_Init(GameSettings *settings, const char *filePath);

void ControlsSettings_ApplyToPlayer(Player *player, const ControlsSettings *controls);
void Settings_SetDefaults(GameSettings *settings); 
