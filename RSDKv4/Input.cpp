#include "RetroEngine.hpp"

InputData keyPress = InputData();
InputData keyDown  = InputData();

int touchDown[8];
int touchX[8];
int touchY[8];
int touchID[8];
int touches = 0;

#if !RETRO_USE_ORIGINAL_CODE
#include <algorithm>
#include <vector>

InputButton inputDevice[INPUT_MAX];
int inputType = 0;

//mania deadzone vals lol
float LSTICK_DEADZONE   = 0.3;
float RSTICK_DEADZONE   = 0.3;
float LTRIGGER_DEADZONE = 0.3;
float RTRIGGER_DEADZONE = 0.3;

int mouseHideTimer = 0;
int lastMouseX = 0;
int lastMouseY = 0;

// Select first driver
static const input_driver_t *input_drv = input_drivers[0];

void controllerInit(byte controllerID)
{
    if (input_drv->init)
        input_drv->init(controllerID);
}

void controllerClose(byte controllerID)
{
    if (input_drv->close)
        input_drv->close(controllerID);
}

void InitInputDevices()
{
    if (input_drv->initDevices)
        input_drv->initDevices();
}

void ReleaseInputDevices()
{
    if (input_drv->releaseDevices)
        input_drv->releaseDevices();
}

void ProcessInput()
{
    if (input_drv->processInput)
        input_drv->processInput();
}
#endif

// Pretty much is this code in the original, just formatted differently
void CheckKeyPress(InputData *input)
{
#if !RETRO_USE_ORIGINAL_CODE
    input->up     = inputDevice[INPUT_UP].press;
    input->down   = inputDevice[INPUT_DOWN].press;
    input->left   = inputDevice[INPUT_LEFT].press;
    input->right  = inputDevice[INPUT_RIGHT].press;
    input->A      = inputDevice[INPUT_BUTTONA].press;
    input->B      = inputDevice[INPUT_BUTTONB].press;
    input->C      = inputDevice[INPUT_BUTTONC].press;
    input->X      = inputDevice[INPUT_BUTTONX].press;
    input->Y      = inputDevice[INPUT_BUTTONY].press;
    input->Z      = inputDevice[INPUT_BUTTONZ].press;
    input->L      = inputDevice[INPUT_BUTTONL].press;
    input->R      = inputDevice[INPUT_BUTTONR].press;
    input->start  = inputDevice[INPUT_START].press;
    input->select = inputDevice[INPUT_SELECT].press;
#endif
}

void CheckKeyDown(InputData *input)
{
#if !RETRO_USE_ORIGINAL_CODE
    input->up     = inputDevice[INPUT_UP].hold;
    input->down   = inputDevice[INPUT_DOWN].hold;
    input->left   = inputDevice[INPUT_LEFT].hold;
    input->right  = inputDevice[INPUT_RIGHT].hold;
    input->A      = inputDevice[INPUT_BUTTONA].hold;
    input->B      = inputDevice[INPUT_BUTTONB].hold;
    input->C      = inputDevice[INPUT_BUTTONC].hold;
    input->X      = inputDevice[INPUT_BUTTONX].hold;
    input->Y      = inputDevice[INPUT_BUTTONY].hold;
    input->Z      = inputDevice[INPUT_BUTTONZ].hold;
    input->L      = inputDevice[INPUT_BUTTONL].hold;
    input->R      = inputDevice[INPUT_BUTTONR].hold;
    input->start  = inputDevice[INPUT_START].hold;
    input->select = inputDevice[INPUT_SELECT].hold;
#endif
}