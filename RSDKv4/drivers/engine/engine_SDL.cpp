#include "../../RetroEngine.hpp"

#if RETRO_USING_SDL1 || RETRO_USING_SDL2
static Uint64 frequency, frameStart, frameEnd;
static float frameDelta = 0.0f;
#endif


static void sdl_init() {
#if RETRO_USING_SDL1 || RETRO_USING_SDL2
    frequency = SDL_GetPerformanceFrequency();
    frameStart = SDL_GetPerformanceCounter(); 
    frameEnd = SDL_GetPerformanceCounter();
#endif
}

static bool sdl_run() {
#if RETRO_USING_SDL1 || RETRO_USING_SDL2
    frameStart = SDL_GetPerformanceCounter();
    frameDelta = frameStart - frameEnd;
    if (frameDelta < frequency / (float)Engine.refreshRate) {
        return false;
    }
    frameEnd = SDL_GetPerformanceCounter();
#endif
    return true;
}

static bool sdl_processEvent() {
#if RETRO_USING_SDL1 || RETRO_USING_SDL2
    while (SDL_PollEvent(&Engine.sdlEvents)) {
        // Main Events
        switch (Engine.sdlEvents.type) {
#if RETRO_USING_SDL2
            case SDL_WINDOWEVENT:
                switch (Engine.sdlEvents.window.event) {
                    case SDL_WINDOWEVENT_MAXIMIZED: {
                        SDL_RestoreWindow(Engine.window);
                        SDL_SetWindowFullscreen(Engine.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                        SDL_ShowCursor(SDL_FALSE);
                        Engine.isFullScreen = true;
                        break;
                    }
                    case SDL_WINDOWEVENT_CLOSE: return false;
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        if (Engine.gameMode == ENGINE_MAINGAME && !disableFocusPause)
                            Engine.gameMode = ENGINE_INITPAUSE;
                        break;
                }
                break;
            case SDL_CONTROLLERDEVICEADDED: controllerInit(Engine.sdlEvents.cdevice.which); break;
            case SDL_CONTROLLERDEVICEREMOVED: controllerClose(Engine.sdlEvents.cdevice.which); break;
            case SDL_APP_WILLENTERBACKGROUND:
                if (Engine.gameMode == ENGINE_MAINGAME && !disableFocusPause)
                    Engine.gameMode = ENGINE_INITPAUSE;
                break;
            case SDL_APP_TERMINATING: return false;
#endif

#ifdef RETRO_USING_MOUSE
            case SDL_MOUSEMOTION:
#if RETRO_USING_SDL2
                if (touches <= 1) { // Touch always takes priority over mouse
                    SDL_GetMouseState(&touchX[0], &touchY[0]);

                    int width = 0, height = 0;
                    SDL_GetWindowSize(Engine.window, &width, &height);
                    touchX[0] = (touchX[0] / (float)width) * SCREEN_XSIZE;
                    touchY[0] = (touchY[0] / (float)height) * SCREEN_YSIZE;
                }
#endif
                break;
            case SDL_MOUSEBUTTONDOWN:
#if RETRO_USING_SDL2
                if (touches <= 0) { // Touch always takes priority over mouse
#endif
                    switch (Engine.sdlEvents.button.button) {
                        case SDL_BUTTON_LEFT: touchDown[0] = 1; break;
                    }
                    touches = 1;
#if RETRO_USING_SDL2
                }
#endif
                break;
            case SDL_MOUSEBUTTONUP:
#if RETRO_USING_SDL2
                if (touches <= 1) { // Touch always takes priority over mouse
#endif
                    switch (Engine.sdlEvents.button.button) {
                        case SDL_BUTTON_LEFT: touchDown[0] = 0; break;
                    }
                    touches = 0;
#if RETRO_USING_SDL2
                }
#endif
                break;
#endif

#if defined(RETRO_USING_TOUCH) && RETRO_USING_SDL2
            case SDL_FINGERMOTION:
                touches = SDL_GetNumTouchFingers(Engine.sdlEvents.tfinger.touchId);
                for (int i = 0; i < touches; i++) {
                    SDL_Finger *finger = SDL_GetTouchFinger(Engine.sdlEvents.tfinger.touchId, i);
                    if (finger) {
                        touchDown[i] = true;
                        touchX[i]    = finger->x * SCREEN_XSIZE;
                        touchY[i]    = finger->y * SCREEN_YSIZE;
                    }
                }
                break;
            case SDL_FINGERDOWN:
                touches = SDL_GetNumTouchFingers(Engine.sdlEvents.tfinger.touchId);
                for (int i = 0; i < touches; i++) {
                    SDL_Finger *finger = SDL_GetTouchFinger(Engine.sdlEvents.tfinger.touchId, i);
                    if (finger) {
                        touchDown[i] = true;
                        touchX[i]    = finger->x * SCREEN_XSIZE;
                        touchY[i]    = finger->y * SCREEN_YSIZE;
                    }
                }
                break;
            case SDL_FINGERUP: touches = SDL_GetNumTouchFingers(Engine.sdlEvents.tfinger.touchId); break;
#endif //! RETRO_USING_SDL2

            case SDL_KEYDOWN:
                switch (Engine.sdlEvents.key.keysym.sym) {
                    default: break;
                    case SDLK_ESCAPE:
                        if (Engine.devMenu) {
#if RETRO_USE_MOD_LOADER
                            // hacky patch because people can escape
                            if ((Engine.gameMode == ENGINE_STARTMENU && stageMode == STARTMENU_MODMENU)
                                || (Engine.gameMode == ENGINE_DEVMENU && stageMode == DEVMENU_MODMENU)) {
                                // Reload entire engine
                                Engine.LoadGameConfig("Data/Game/GameConfig.bin");
#if RETRO_USING_SDL1 || RETRO_USING_SDL2
                                if (Engine.window) {
                                    char gameTitle[0x40];
                                    sprintf(gameTitle, "%s%s", Engine.gameWindowText, Engine.usingDataFile ? "" : " (Using Data Folder)");
                                    SDL_SetWindowTitle(Engine.window, gameTitle);
                                }
#endif

                                ReleaseStageSfx();
                                ReleaseGlobalSfx();
                                LoadGlobalSfx();

                                forceUseScripts = false;
                                skipStartMenu   = skipStartMenu_Config;
                                for (int m = 0; m < modList.size(); ++m) {
                                    if (modList[m].useScripts && modList[m].active)
                                        forceUseScripts = true;
                                    if (modList[m].skipStartMenu && modList[m].active)
                                        skipStartMenu = true;
                                }
                                saveMods();
                            }
#endif

                            Engine.gameMode = ENGINE_INITDEVMENU;
                        }
                        break;
                    case SDLK_F4:
                        Engine.isFullScreen ^= 1;
                        if (Engine.isFullScreen) {
#if RETRO_USING_SDL1
                            Engine.windowSurface = SDL_SetVideoMode(SCREEN_XSIZE * Engine.windowScale, SCREEN_YSIZE * Engine.windowScale, 16,
                                                                    SDL_SWSURFACE | SDL_FULLSCREEN);
                            SDL_ShowCursor(SDL_FALSE);
#elif RETRO_USING_SDL2
                            SDL_RestoreWindow(Engine.window);
                            SDL_SetWindowFullscreen(Engine.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                            SDL_ShowCursor(SDL_FALSE);
#endif
                        }
                        else {
#if RETRO_USING_SDL1
                            Engine.windowSurface =
                                SDL_SetVideoMode(SCREEN_XSIZE * Engine.windowScale, SCREEN_YSIZE * Engine.windowScale, 16, SDL_SWSURFACE);
                            SDL_ShowCursor(SDL_TRUE);
#elif RETRO_USING_SDL2
                            SDL_SetWindowFullscreen(Engine.window, false);
                            SDL_ShowCursor(SDL_TRUE);
                            SDL_SetWindowSize(Engine.window, SCREEN_XSIZE * Engine.windowScale, SCREEN_YSIZE * Engine.windowScale);
                            SDL_SetWindowPosition(Engine.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
                            SDL_RestoreWindow(Engine.window);
#endif
                        }
                        break;
                    case SDLK_F1:
                        if (Engine.devMenu) {
                            activeStageList   = 0;
                            stageListPosition = 0;
                            stageMode         = STAGEMODE_LOAD;
                            Engine.gameMode   = ENGINE_MAINGAME;
                        }
                        break;
                    case SDLK_F2:
                        if (Engine.devMenu) {
                            stageListPosition--;
                            if (stageListPosition < 0) {
                                activeStageList--;

                                if (activeStageList < 0) {
                                    activeStageList = 3;
                                }
                                stageListPosition = stageListCount[activeStageList] - 1;
                            }
                            stageMode       = STAGEMODE_LOAD;
                            Engine.gameMode = ENGINE_MAINGAME;
                            SetGlobalVariableByName("lampPostID", 0); // For S1
                            SetGlobalVariableByName("starPostID", 0); // For S2
                        }
                        break;
                    case SDLK_F3:
                        if (Engine.devMenu) {
                            stageListPosition++;
                            if (stageListPosition >= stageListCount[activeStageList]) {
                                activeStageList++;

                                stageListPosition = 0;

                                if (activeStageList >= 4) {
                                    activeStageList = 0;
                                }
                            }
                            stageMode       = STAGEMODE_LOAD;
                            Engine.gameMode = ENGINE_MAINGAME;
                            SetGlobalVariableByName("lampPostID", 0); // For S1
                            SetGlobalVariableByName("starPostID", 0); // For S2
                        }
                        break;
                    case SDLK_F10:
                        if (Engine.devMenu)
                            Engine.showPaletteOverlay ^= 1;
                        break;
                    case SDLK_BACKSPACE:
                        if (Engine.devMenu)
                            Engine.gameSpeed = Engine.fastForwardSpeed;
                        break;
#if RETRO_PLATFORM == RETRO_OSX
                    case SDLK_F6:
                        if (Engine.masterPaused)
                            Engine.frameStep = true;
                        break;
                    case SDLK_F7:
                        if (Engine.devMenu)
                            Engine.masterPaused ^= 1;
                        break;
#else
                    case SDLK_F11:
                    case SDLK_INSERT:
                        if (Engine.masterPaused)
                            Engine.frameStep = true;
                        break;
                    case SDLK_F12:
                    case SDLK_PAUSE:
                        if (Engine.devMenu)
                            Engine.masterPaused ^= 1;
                        break;
#endif
                }

#if RETRO_USING_SDL1
                keyState[Engine.sdlEvents.key.keysym.sym] = 1;
#endif
                break;
            case SDL_KEYUP:
                switch (Engine.sdlEvents.key.keysym.sym) {
                    default: break;
                    case SDLK_BACKSPACE: Engine.gameSpeed = 1; break;
                }
#if RETRO_USING_SDL1
                keyState[Engine.sdlEvents.key.keysym.sym] = 0;
#endif
                break;
            case SDL_QUIT: return false;
        }
    }
#endif
    return true;
}

static void sdl_quit() {
#if RETRO_USING_SDL1 || RETRO_USING_SDL2
    SDL_Quit();
#endif
}

engine_driver_t engine_SDL = {
    sdl_init,
    sdl_run,
    sdl_processEvent,
    sdl_quit,
};