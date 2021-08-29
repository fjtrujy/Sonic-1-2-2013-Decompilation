#include "../../RetroEngine.hpp"

#if RETRO_USING_SDL1
struct InputDevice {
    int id;
};

byte keyState[SDLK_LAST];

SDL_Joystick *controller = nullptr;
#endif

static void sdl1_init(byte controllerID)
{
#if RETRO_USING_SDL1
    for (int i = 0; i < controllers.size(); ++i) {
        if (controllers[i].id == controllerID) {
            return; // we already opened this one!
        }
    }

    SDL_GameController *controller = SDL_GameControllerOpen(controllerID);
    if (controller) {
        InputDevice device;
        device.id        = 0;
        device.devicePtr = controller;

        controllers.push_back(device);
        inputType = 1;
    }
    else {
        printLog("Could not open controller...\nSDL_GetError() -> %s", SDL_GetError());
    }
#endif
}

static void sdl1_close(byte controllerID) {
#if RETRO_USING_SDL1
    SDL_GameController *controller = SDL_GameControllerFromInstanceID(controllerID);
    if (controller) {
        SDL_GameControllerClose(controller);
        for (int i = 0; i < controllers.size(); ++i) {
            if (controllers[i].id == controllerID) {
                controllers.erase(controllers.begin() + controllerID);
                break;
            }
        }
    }

    if (controllers.empty())
        inputType = 0;
#endif
}

static void sdl1_initDevices() {}
static void sdl1_releaseDevices() {}

static void sdl1_processInput() {
#if RETRO_USING_SDL1
    if (SDL_NumJoysticks() > 0) {
        controller = SDL_JoystickOpen(0);

        // There's a problem opening the joystick
        if (controller == NULL) {
            // Uh oh
        }
        else {
            inputType = 1;
        }
    }
    else {
        if (controller) {
            // Close the joystick
            SDL_JoystickClose(controller);
        }
        controller = nullptr;
        inputType  = 0;
    }

    if (inputType == 0) {
        for (int i = 0; i < INPUT_MAX; i++) {
            if (keyState[inputDevice[i].keyMappings]) {
                inputDevice[i].setHeld();
                inputDevice[INPUT_ANY].setHeld();
                continue;
            }
            else if (inputDevice[i].hold)
                inputDevice[i].setReleased();
        }
    }
    else if (inputType == 1 && controller) {
        for (int i = 0; i < INPUT_MAX; i++) {
            if (SDL_JoystickGetButton(controller, inputDevice[i].contMappings)) {
                inputDevice[i].setHeld();
                inputDevice[INPUT_ANY].setHeld();
                continue;
            }
            else if (inputDevice[i].hold)
                inputDevice[i].setReleased();
        }
    }

    bool isPressed = false;
    for (int i = 0; i < INPUT_MAX; i++) {
        if (keyState[inputDevice[i].keyMappings]) {
            isPressed = true;
            break;
        }
    }
    if (isPressed)
        inputType = 0;
    else if (inputType == 0)
        inputDevice[INPUT_ANY].setReleased();

    int buttonCnt = 0;
    if (controller)
        buttonCnt = SDL_JoystickNumButtons(controller);
    bool flag = false;
    for (int i = 0; i < buttonCnt; ++i) {
        flag      = true;
        inputType = 1;
    }
    if (!flag && inputType == 1) {
        inputDevice[INPUT_ANY].setReleased();
    }
#endif
}

input_driver_t input_SDL1 = {
    sdl1_init,
    sdl1_close,
    sdl1_initDevices,
    sdl1_releaseDevices,
    sdl1_processInput,
};