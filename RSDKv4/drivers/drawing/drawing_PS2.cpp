#include "../../RetroEngine.hpp"

static int ps2_init(const char *gameTitle)
{
#if RETRO_PLATFORM == RETRO_PS2
#endif
    return 1;
}

static void ps2_flipScreen() {
#if RETRO_PLATFORM == RETRO_PS2
#endif
}

static void ps2_release() {
#if RETRO_PLATFORM == RETRO_PS2
#endif
}

drawing_driver_t drawing_PS2 = {
    ps2_init,
    ps2_flipScreen,
    ps2_release,
};