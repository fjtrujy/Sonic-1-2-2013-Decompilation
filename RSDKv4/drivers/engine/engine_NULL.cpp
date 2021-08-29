#include "../../RetroEngine.hpp"

static void null_init() {}
static bool null_run() { return true; }
static bool null_processEvent() { return true; }
static void null_quit() {}

engine_driver_t engine_NULL = {
    null_init,
    null_run,
    null_processEvent,
    null_quit,
};