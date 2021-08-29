#include "../../RetroEngine.hpp"

static void ps2_init(byte controllerID) {}
static void ps2_close(byte controllerID) {}
static void ps2_initDevices() {}
static void ps2_releaseDevices() {}
static void ps2_processInput() {}

input_driver_t input_PS2 = {
    ps2_init,
    ps2_close,
    ps2_initDevices,
    ps2_releaseDevices,
    ps2_processInput,
};