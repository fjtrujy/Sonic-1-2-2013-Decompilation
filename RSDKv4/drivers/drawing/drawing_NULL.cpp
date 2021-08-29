#include "../../RetroEngine.hpp"

static int null_init(const char *gameTitle) { return 1; }
static void null_flipScreen() {}
static void null_release() {}

drawing_driver_t drawing_NULL = {
    null_init,
    null_flipScreen,
    null_release,
};