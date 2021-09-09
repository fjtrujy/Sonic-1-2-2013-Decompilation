#ifndef RETROENGINE_H
#define RETROENGINE_H

#include "RetroConfig.hpp"

// ================
// STANDARD LIBS
// ================
#include <stdio.h>
#include <string.h>
#include <cmath>

// ================
// STANDARD TYPES
// ================
typedef unsigned char byte;
typedef signed char sbyte;
typedef unsigned short ushort;
typedef unsigned int uint;
// typedef unsigned long long ulong;

enum RetroLanguages {
    RETRO_EN = 0,
    RETRO_FR = 1,
    RETRO_IT = 2,
    RETRO_DE = 3,
    RETRO_ES = 4,
    RETRO_JP = 5,
    RETRO_PT = 6,
    RETRO_RU = 7,
    RETRO_KO = 8,
    RETRO_ZH = 9,
    RETRO_ZS = 10,
};

enum RetroStates {
    ENGINE_DEVMENU     = 0,
    ENGINE_MAINGAME    = 1,
    ENGINE_INITDEVMENU = 2,
    ENGINE_WAIT        = 3,
    ENGINE_SCRIPTERROR = 4,
    ENGINE_INITPAUSE   = 5,
    ENGINE_EXITPAUSE   = 6,
    ENGINE_ENDGAME     = 7,
    ENGINE_RESETGAME   = 8,

#if !RETRO_USE_ORIGINAL_CODE
    // Custom GameModes (required to make some features work)
    ENGINE_STARTMENU   = 0x80,
    ENGINE_CONNECT2PVS = 0x81,
    ENGINE_INITMODMENU = 0x82,
#endif
};

enum RetroGameType {
    GAME_UNKNOWN = 0,
    GAME_SONIC1  = 1,
    GAME_SONIC2  = 2,
};

// Engine Driver
typedef struct engine_driver
{
   void (*init)();
   bool (*run)();
   bool (*processEvent)();
   void (*quit)();
} engine_driver_t;

// DRIVERS
extern engine_driver_t engine_SDL;
extern engine_driver_t engine_PS2;
extern engine_driver_t engine_NULL;

static const engine_driver_t *engine_drivers[] = {
#if RETRO_USING_SDL1 || RETRO_USING_SDL2
    &engine_SDL,
#elif RETRO_PLATFORM == RETRO_PS2
    &engine_PS2,
#else
    &engine_NULL,
#endif
};

#if !RETRO_USE_ORIGINAL_CODE
extern bool usingCWD;
extern bool engineDebugMode;
#endif

// Utils
#if !RETRO_USE_ORIGINAL_CODE
#include "Ini.hpp"
#endif

#include "Math.hpp"
#include "Reader.hpp"
#include "String.hpp"
#include "Animation.hpp"
#include "Audio.hpp"
#include "Input.hpp"
#include "Object.hpp"
#include "Palette.hpp"
#include "Drawing.hpp"
#include "Scene3D.hpp"
#include "Collision.hpp"
#include "Scene.hpp"
#include "Script.hpp"
#include "Sprite.hpp"
#include "Text.hpp"
#include "Userdata.hpp"
#include "Debug.hpp"
#include "cmixer.hpp"

// Native Entities
#include "PauseMenu.hpp"
#include "RetroGameLoop.hpp"

class RetroEngine
{
public:
    RetroEngine()
    {
        if (RETRO_GAMEPLATFORM == RETRO_STANDARD)
            gamePlatform = "STANDARD";
        else
            gamePlatform = "MOBILE";
    }

    bool usingDataFile = false;
    bool usingBytecode = false;

    char dataFile[RETRO_PACKFILE_COUNT][0x80];

    bool initialised = false;
    bool running     = false;

    int gameMode = 1;
    int language = RETRO_EN;
    int message  = 0;

    bool trialMode      = false;
    bool onlineActive   = true;
    bool hapticsEnabled = true;

    int frameSkipSetting = 0;
    int frameSkipTimer   = 0;

#if !RETRO_USE_ORIGINAL_CODE
    // Ported from RSDKv5
    bool devMenu         = false;
    int startList        = -1;
    int startStage       = -1;
    int startPlayer      = -1;
    int startSave        = -1;
    int gameSpeed        = 1;
    int fastForwardSpeed = 8;
    bool masterPaused    = false;
    bool frameStep       = false;
    int dimTimer         = 0;
    int dimLimit         = 0;
    float dimPercent     = 1.0;
    float dimMax         = 1.0;

    bool showPaletteOverlay = false;
    bool useHQModes         = true;
#endif

    void Init();
    void Run();

    bool LoadGameConfig(const char *Filepath);

    int callbackMessage = 0;
    int prevMessage     = 0;
    int waitValue       = 0;

    char gameWindowText[0x40];
    char gameDescriptionText[0x100];
    const char *gameVersion  = "1.1.2";
    const char *gamePlatform = nullptr;

#if RETRO_RENDERTYPE == RETRO_SW_RENDER
    const char *gameRenderType = "SW_RENDERING";
#elif RETRO_RENDERTYPE == RETRO_HW_RENDER
    const char *gameRenderType = "HW_RENDERING";
#endif

#if RETRO_USE_HAPTICS
    const char *gameHapticSetting = "USE_F_FEEDBACK"; // None is default, but people with controllers exist
#else
    const char *gameHapticSetting = "NO_F_FEEDBACK";
#endif

#if !RETRO_USE_ORIGINAL_CODE
    byte gameType = GAME_UNKNOWN;
#if RETRO_USE_MOD_LOADER
    bool modMenuCalled = false;
#endif
#endif

#if RETRO_SOFTWARE_RENDER
    ushort *frameBuffer   = nullptr;
    ushort *frameBuffer2x = nullptr;
#endif

#if !RETRO_USE_ORIGINAL_CODE
    bool isFullScreen = false;

    bool startFullScreen  = false; // if should start as fullscreen
    bool borderless       = false;
    bool vsync            = false;
    int scalingMode       = RETRO_DEFAULTSCALINGMODE;
    int windowScale       = 2;
    int refreshRate       = 60; // user-picked screen update rate
    int screenRefreshRate = 60; // hardware screen update rate
    int targetRefreshRate = 60; // game logic update rate

    uint frameCount      = 0; // frames since scene load
    int renderFrameIndex = 0;
    int skipFrameIndex   = 0;

    int windowXSize; // width of window/screen in the previous frame
    int windowYSize; // height of window/screen in the previous frame
#endif

#if !RETRO_USE_ORIGINAL_CODE
#if RETRO_USING_SDL2
    SDL_Window *window     = nullptr;
    SDL_Renderer *renderer = nullptr;
#if RETRO_SOFTWARE_RENDER
    SDL_Texture *screenBuffer   = nullptr;
    SDL_Texture *screenBuffer2x = nullptr;
#endif // RETRO_SOFTWARE_RENDERER

    SDL_Event sdlEvents;

#if RETRO_USING_OPENGL
    SDL_GLContext m_glContext; // OpenGL context
#endif // RETRO_USING_OPENGL
#endif // RETRO_USING_SDL2

#if RETRO_USING_SDL1
    SDL_Surface *windowSurface = nullptr;

    SDL_Surface *screenBuffer   = nullptr;
    SDL_Surface *screenBuffer2x = nullptr;

    SDL_Event sdlEvents;
#endif // RETRO_USING_SDL1
#endif //! RETRO_USE_ORIGINAL_CODE
};

extern RetroEngine Engine;
#endif // !RETROENGINE_H
