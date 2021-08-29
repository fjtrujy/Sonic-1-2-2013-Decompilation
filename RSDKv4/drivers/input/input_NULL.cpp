#include "../../RetroEngine.hpp"

static void null_init(byte controllerID) {}
static void null_close(byte controllerID) {}
static void null_initDevices() {}
static void null_releaseDevices() {}
static void null_processInput() {}

input_driver_t input_NULL = {
    null_init,
    null_close,
    null_initDevices,
    null_releaseDevices,
    null_processInput,
};