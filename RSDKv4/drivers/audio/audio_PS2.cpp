#include "../../RetroEngine.hpp"

static int ps2_init(PlaybackFunction playbackFunc) { return 0; }
static void ps2_configure(int format, int channels, int rate, int freq, int samples) {}
static void ps2_release() {}
static void ps2_createThread(ThreadFunction threadFunc, const char *name) {}
static void ps2_lock() {}
static void ps2_unlock() {}
static void ps2_loadWav(const char *filePath, FileInfo *info, byte sfxID, byte *sfx) {}
static int ps2_availableStream() { return 0; }
static int ps2_streamPut(Sint16 *buffer, size_t size) { return -1; }
static int ps2_streamGet(Sint16 *buffer, size_t size) { return -1; }

audio_driver_t audio_PS2 = {
   ps2_init,
   ps2_configure,
   ps2_createThread,
   ps2_lock,
   ps2_unlock,
   ps2_loadWav,
   ps2_availableStream,
   ps2_streamPut,
   ps2_streamGet,
   ps2_release,
};