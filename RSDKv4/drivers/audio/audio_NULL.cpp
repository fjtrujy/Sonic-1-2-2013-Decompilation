#include "../../RetroEngine.hpp"

static int null_init(PlaybackFunction playbackFunc) {}
static void null_configure(int format, int channels, int rate, int freq, int samples) {}
static void null_release() {}
static void null_createThread(ThreadFunction threadFunc, const char *name) {}
static void null_lock() {}
static void null_unlock() {}
static void null_loadWav(const char *filePath, FileInfo *info, byte sfxID, byte *sfx) {}
static int null_availableStream() {}
static int null_streamPut(Sint16 *buffer, size_t size) {}
static int null_streamGet(Sint16 *buffer, size_t size) {}

audio_driver_t audio_NULL = {
   null_init,
   null_configure,
   null_createThread,
   null_lock,
   null_unlock,
   null_loadWav,
   null_availableStream,
   null_streamPut,
   null_streamGet,
   null_release,
};