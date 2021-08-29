#include "../../RetroEngine.hpp"

static void ps2_init() {}
static bool ps2_run() { return true; }
static bool ps2_processEvent() { return true; }
static void ps2_quit() {}

engine_driver_t engine_PS2 = {
    ps2_init,
    ps2_run,
    ps2_processEvent,
    ps2_quit,
};