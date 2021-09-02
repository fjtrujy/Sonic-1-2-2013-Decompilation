#include "../../RetroEngine.hpp"

#if RETRO_PLATFORM == RETRO_PS2

#include <libmtap.h>
#include <libpad.h>

#define DEADZONE    24
#define DEADZONE_SQ (DEADZONE * DEADZONE)

static uint8_t padbuf[256] __attribute__((aligned(64)));
static int init_done = 0;

static int joy_port = -1;
static int joy_slot = -1;
static int joy_id = -1;
static struct padButtonStatus joy_buttons __attribute__((aligned(64)));

static inline int wait_pad(int tries) {
    int state = padGetState(joy_port, joy_slot);
    if (state == PAD_STATE_DISCONN) {
        joy_id = -1;
        return -1;
    }

    while ((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1)) {
        state = padGetState(joy_port, joy_slot);
        if (--tries == 0) break;
    }

    return 0;
}

static int detect_pad(void) {
    int id = padInfoMode(joy_port, joy_slot, PAD_MODECURID, 0);
    if (id <= 0) return -1;

    const int ext = padInfoMode(joy_port, joy_slot, PAD_MODECUREXID, 0);
    if (ext) id = ext;

    printf("controller_ps2: detected pad type %d\n", id);

    if (id == PAD_TYPE_DIGITAL || id == PAD_TYPE_DUALSHOCK)
        padSetMainMode(joy_port, joy_slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);

    return id;
}

#endif

static void ps2_init(byte controllerID) {
#if RETRO_PLATFORM == RETRO_PS2
    padInit(0);
#endif
}
static void ps2_close(byte controllerID) {
#if RETRO_PLATFORM == RETRO_PS2

#endif
}
static void ps2_initDevices() {
#if RETRO_PLATFORM == RETRO_PS2
    const int numports = padGetPortMax();

    for (int port = 0; port < numports && joy_port < 0; ++port) {
        mtapPortOpen(port);
        const int maxslots = padGetSlotMax(port);
        for (int slot = 0; slot < maxslots; ++slot) {
            if (padPortOpen(port, slot, padbuf) >= 0) {
                joy_port = port;
                joy_slot = slot;
                printf("controller_ps2: using pad (%d, %d)\n", port, slot);
                break;
            }
        }
    }

    if (joy_slot < 0 || joy_port < 0) {
        printf("controller_ps2: could not open a single port\n");
        return;
    }
#endif
}
static void ps2_releaseDevices() {
#if RETRO_PLATFORM == RETRO_PS2

#endif
}
static void ps2_processInput() {
#if RETRO_PLATFORM == RETRO_PS2
    if (wait_pad(10) < 0)
        return; // nothing received

    if (joy_id < 0) {
        // pad not detected yet, do it
        joy_id = detect_pad();
        if (joy_id < 0) return; // still nothing
        if (wait_pad(10) < 0) return;
    }

    if (padRead(joy_port, joy_slot, &joy_buttons)) {
        const uint32_t btns = 0xffff ^ joy_buttons.btns;
        
        for (int i = 0; i < INPUT_ANY; i++) {
            if (btns & inputDevice[i].contMappings){
                inputDevice[i].setHeld();
                if (!inputDevice[INPUT_ANY].hold)
                    inputDevice[INPUT_ANY].setHeld();
            }
            else if (inputDevice[i].hold)
                inputDevice[i].setReleased();

        }
    }
#endif
}

input_driver_t input_PS2 = {
    ps2_init,
    ps2_close,
    ps2_initDevices,
    ps2_releaseDevices,
    ps2_processInput,
};