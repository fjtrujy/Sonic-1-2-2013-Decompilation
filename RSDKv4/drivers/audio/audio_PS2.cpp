#include "../../RetroEngine.hpp"

#if RETRO_PLATFORM == RETRO_PS2

#include <kernel.h>
#include <audsrv.h>

#define AUDIO_BUFFER 128 * 1024
#define AUDIO_CHANNELS 2
#define AUDIO_BITS 16

#define STACK_SIZE              16384

static int ee_threadID = 0;
static int ee_threadID2 = 0;
static int audio_running = 0;

static PlaybackFunction playbackFunc_reference;

static uint8_t *buff;
static int len;

void SDL_RunThread(void *data) {
    while(audio_running) {
        ProcessAudioPlayback(data, buff, len);
        audsrv_play_audio((const char*)buff, len);
    }
}

#endif

static int ps2_init(PlaybackFunction playbackFunc) { 
        // setup thread

    ee_thread_t th_attr;

    th_attr.stack_size 	 = STACK_SIZE;
	th_attr.gp_reg 		 = (void *)&_gp;
	th_attr.initial_priority = 64;
	th_attr.option 		 = 0;

    th_attr.func = (void *)SDL_RunThread;
	th_attr.stack = (void *)malloc(STACK_SIZE);
	if (th_attr.stack == NULL)
	{
		printf("libmikmod: error creating update thread\n");
        return 0;
	}

	ee_threadID = CreateThread(&th_attr);
	if (ee_threadID2 < 0) 
	{
		printf("libmikmod: Not enough resources to create update thread");
		return 0;
	}
    
    buff = (unsigned char*)malloc(1024*16);
    len = 1000;
    audio_running = 1;
	StartThread(ee_threadID, 0);
    printf("\n\n\n STARTING AUDIO THREAD %i!!!!\n\n\n", ee_threadID2);

    return 1; 
}

static void ps2_configure(int format, int channels, int rate, int freq, int samples) {
#if RETRO_PLATFORM == RETRO_PS2
    struct audsrv_fmt_t ps2_format;

   ps2_format.bits     = AUDIO_BITS;
   ps2_format.freq     = rate;
   ps2_format.channels = channels;

   audsrv_set_format(&ps2_format);
   audsrv_set_volume(MAX_VOLUME);
    printf("\n\n\n CONFIGURING AUDIO!!!!\n\n\n");
#endif
}
static void ps2_release() {}
static void ps2_createThread(ThreadFunction threadFunc, const char *name) {
    ee_thread_t th_attr;

    th_attr.stack_size 	 = STACK_SIZE;
	th_attr.gp_reg 		 = (void *)&_gp;
	th_attr.initial_priority = 64;
	th_attr.option 		 = 0;

    th_attr.func = (void *)threadFunc;
	th_attr.stack = (void *)malloc(STACK_SIZE);
	if (th_attr.stack == NULL)
	{
		printf("libmikmod: error creating update thread\n");
	}

	ee_threadID2 = CreateThread(&th_attr);
	if (ee_threadID2 < 0) 
	{
		printf("libmikmod: Not enough resources to create update thread");
	}

	StartThread(ee_threadID2, 0);
    printf("\n\n\n STARTING AUDIO THREAD %i!!!!\n\n\n", ee_threadID2);
}
static void ps2_lock() {}

static void ps2_unlock() {}
static void ps2_loadWav(const char *filePath, FileInfo *info, byte sfxID, byte *sfx) {
#if RETRO_PLATFORM == RETRO_PS2
    // printf("FJTRUJY: %s, %s:%i, filePath %s, sfxID %i, fileSize, %i, vfileSize %i\n", __FUNCTION__, __FILE__, __LINE__, filePath, sfxID, info->fileSize, info->vfileSize);
    // uint8_t buffer[info->fileSize];
    // memcpy(&buffer, sfx, info->fileSize); 
    // StrCopy(sfxList[sfxID].name, filePath);
    // sfxList[sfxID].buffer = (Sint16 *)convert.buf;
    // sfxList[sfxID].length = convert.len_cvt / sizeof(Sint16);
    // sfxList[sfxID].loaded = true;
#endif
}
static int ps2_availableStream() { return 0; }
static int ps2_streamPut(Sint16 *buffer, size_t size) { return size; }
static int ps2_streamGet(Sint16 *buffer, size_t size) { return size; }

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