#include "settings.h"
#include "player.h"
#include "utils.h"
#include "gameManager.h"

#include <SDL.h>
#include <stdio.h>
#include <cJSON.h>


static float json_get_number(cJSON *obj, const char *key, float def) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    return cJSON_IsNumber(item) ? (float)item->valuedouble : def;
}

static int json_get_int(cJSON *obj, const char *key, int def) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    return cJSON_IsNumber(item) ? item->valueint : def;
}

static bool json_get_bool(cJSON *obj, const char *key, bool def) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    return cJSON_IsBool(item) ? cJSON_IsTrue(item) : def;
}

static SDL_Scancode json_get_scancode(cJSON *obj, const char *key, SDL_Scancode def) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    SDL_Scancode sc = def;
    if (cJSON_IsString(item)) {
        sc = SDL_GetScancodeFromName(item->valuestring);
        if (sc == SDL_SCANCODE_UNKNOWN) {
            fprintf(stderr, "[CONTROLS] Warning: '%s' is not a valid key for '%s'. Using default '%s'.\n",
                item->valuestring, key, SDL_GetScancodeName(def));
            sc = def;
        }
    } else if (!item) {
        fprintf(stderr, "[CONTROLS] Warning: Key '%s' missing in JSON. Using default '%s'.\n",
            key, SDL_GetScancodeName(def));
    }
    return sc;
}

const char *ScancodeToString(SDL_Scancode scancode) 
{
    return SDL_GetScancodeName(scancode);
}


bool Settings_Load(GameSettings *settings, const char *filePath) 
{
    char *text = read_whole_file(filePath);
    if (!text) {
        printf("Settings: failed to read %s\n", filePath);
        return false;
    }

    cJSON *root = cJSON_Parse(text);
    free(text);
    if (!root) {
        printf("Settings: invalid JSON in %s\n", filePath);
        return false;
    }

    // Controls
    cJSON *controls = cJSON_GetObjectItemCaseSensitive(root, "controls");
    if (!controls)
    {
        printf("Settings: 'controls' is missing or not an object\n");
        return false;
    }
    else{
        settings->controls.moveLeft  = json_get_scancode(controls, "moveLeft",  SDL_SCANCODE_LEFT);
        settings->controls.moveRight = json_get_scancode(controls, "moveRight", SDL_SCANCODE_RIGHT);
        settings->controls.jump      = json_get_scancode(controls, "jump",      SDL_SCANCODE_SPACE);
        settings->controls.crouch    = json_get_scancode(controls, "crouch",    SDL_SCANCODE_LSHIFT);
        settings->controls.attack    = json_get_scancode(controls, "attack",    SDL_SCANCODE_Z);
        settings->controls.dash      = json_get_scancode(controls, "dash",      SDL_SCANCODE_X);
    }

    // Audio
    cJSON *audio = cJSON_GetObjectItemCaseSensitive(root, "audio");
    if (audio) {
        settings->audio.masterVolume = json_get_number(audio, "masterVolume", settings->audio.masterVolume);
        settings->audio.musicVolume  = json_get_number(audio, "musicVolume",  settings->audio.musicVolume);
        settings->audio.sfxVolume    = json_get_number(audio, "sfxVolume",    settings->audio.sfxVolume);
    }

    // Video
    cJSON *video = cJSON_GetObjectItemCaseSensitive(root, "videoSettings");
    if (video) {
        settings->video.width      = json_get_int(video, "resolutionWidth",  settings->video.width);
        settings->video.height     = json_get_int(video, "resolutionHeight", settings->video.height);
        settings->video.fullscreen = json_get_bool(video, "fullscreen", settings->video.fullscreen);
        settings->video.vsync      = json_get_bool(video, "vSync",      settings->video.vsync);
        settings->video.scale      = json_get_number(video, "scale",    settings->video.scale);
    }

    // Gameplay
    cJSON *gameplay = cJSON_GetObjectItemCaseSensitive(root, "gameplaySettings");
    if (gameplay) {
        settings->gameplay.debugMode = json_get_bool(gameplay, "debugMode", settings->gameplay.debugMode);
    }

    cJSON_Delete(root);
    return true;
}


bool Settings_Save(const GameSettings *settings, const char *filePath) {
    cJSON *root = cJSON_CreateObject();

    // Controls
    cJSON *controls = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "controls", controls);

    cJSON_AddStringToObject(controls, "moveLeft",  SDL_GetScancodeName(settings->controls.moveLeft));
    cJSON_AddStringToObject(controls, "moveRight", SDL_GetScancodeName(settings->controls.moveRight));
    cJSON_AddStringToObject(controls, "jump",      SDL_GetScancodeName(settings->controls.jump));
    cJSON_AddStringToObject(controls, "crouch",    SDL_GetScancodeName(settings->controls.crouch));
    cJSON_AddStringToObject(controls, "attack",    SDL_GetScancodeName(settings->controls.attack));
    cJSON_AddStringToObject(controls, "dash",      SDL_GetScancodeName(settings->controls.dash));

    // Audio
    cJSON *audio = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "audio", audio);

    cJSON_AddNumberToObject(audio, "masterVolume", settings->audio.masterVolume);
    cJSON_AddNumberToObject(audio, "musicVolume",  settings->audio.musicVolume);
    cJSON_AddNumberToObject(audio, "sfxVolume",    settings->audio.sfxVolume);

    // Video
    cJSON *video = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "videoSettings", video);

    cJSON_AddNumberToObject(video, "resolutionWidth",  settings->video.width);
    cJSON_AddNumberToObject(video, "resolutionHeight", settings->video.height);
    cJSON_AddBoolToObject(video,   "fullscreen",       settings->video.fullscreen);
    cJSON_AddBoolToObject(video,   "vSync",            settings->video.vsync);
    cJSON_AddNumberToObject(video, "scale",            settings->video.scale);

    // Gameplay
    cJSON *gameplay = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "gameplaySettings", gameplay);
    cJSON_AddBoolToObject(gameplay, "debugMode", settings->gameplay.debugMode);

    char *jsonString = cJSON_Print(root); 

    FILE *fp = fopen(filePath, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open settings file for writing: %s\n", filePath);
        free(jsonString);
        cJSON_Delete(root);
        return false;
    }

    fputs(jsonString, fp);
    fclose(fp);

    // Cleanup
    free(jsonString);
    cJSON_Delete(root);
    return true;
}


void Settings_SetDefaults(GameSettings *settings) {

    settings->controls.moveLeft  = SDL_SCANCODE_LEFT;
    settings->controls.moveRight = SDL_SCANCODE_RIGHT;
    settings->controls.jump      = SDL_SCANCODE_SPACE;
    settings->controls.crouch    = SDL_SCANCODE_LSHIFT;
    settings->controls.attack    = SDL_SCANCODE_Z;
    settings->controls.dash      = SDL_SCANCODE_X;

    settings->video.width       = 1280;
    settings->video.height      = 720;
    settings->video.fullscreen  = false;
    settings->video.vsync       = true;
    settings->video.scale       = 1.0f;

    settings->audio.masterVolume = 0.8f;
    settings->audio.musicVolume  = 0.5f;
    settings->audio.sfxVolume    = 0.7f;

    settings->gameplay.debugMode = false;
}

bool Settings_Init(GameSettings *settings, const char *filePath) {
    Settings_SetDefaults(settings);
    if (!Settings_Load(settings, filePath)) {
        // Save defaults if file missing/corrupted
        Settings_Save(settings, filePath);
    }
    return true;
}

void Settings_Apply(const GameSettings *settings, mainSystems *systems, Player *player) {
    Settings_ApplyControls(player, &settings->controls);
    Settings_ApplyAudio(&settings->audio);
    Settings_ApplyVideo(systems, &settings->video); 
    Settings_ApplyGameplay(&settings->gameplay);
}

void Settings_ApplyControls(Player *player, const ControlsSettings *controls) 
{
    if (!player) return;
    player->controls.moveLeft  = controls->moveLeft;
    player->controls.moveRight = controls->moveRight;
    player->controls.jump      = controls->jump;
    player->controls.attack    = controls->attack;
    player->controls.crouch    = controls->crouch;
    player->controls.dash      = controls->dash;

    printf("Loaded controls: Left=%d Right=%d Jump=%d Crouch=%d Attack=%d Dash=%d\n",
       player->controls.moveLeft, player->controls.moveRight,
       player->controls.jump, player->controls.crouch,
       player->controls.attack, player->controls.dash);
}

void Settings_ApplyAudio(const AudioSettings *audio) {
    // Example: integrate with SDL_mixer
    // Mix_Volume(-1, (int)(audio->masterVolume * MIX_MAX_VOLUME));
    // Mix_VolumeMusic((int)(audio->musicVolume * MIX_MAX_VOLUME));
    // (might want a custom audio manager instead)
}

void Settings_ApplyVideo(mainSystems *systems, const VideoSettings *video) {
    Uint32 flags = video->fullscreen ? SDL_WINDOW_FULLSCREEN : 0;
    SDL_SetWindowFullscreen(systems->window, flags);

    SDL_SetWindowSize(systems->window, video->width, video->height);
    SDL_RenderSetScale(systems->renderer, video->scale, video->scale);
}

void Settings_ApplyGameplay(const GameplaySettings *gameplay) {
    // TODO
}
