#include "../../RetroEngine.hpp"

#if RETRO_PLATFORM == RETRO_PS2

#include <kernel.h>
#include <audsrv.h>

#define AUDIO_CHANNELS 2
#define AUDIO_BITS 16

#define STACK_SIZE 16384
#define BUFFER_SIZE 8192

int ee_threadID = 0;
char chunk[BUFFER_SIZE];
int fillbuffer_sema;

void lock_handler(cm_Event *e) {
  if (e->type == CM_EVENT_LOCK) {
    WaitSema(fillbuffer_sema);
  }
  if (e->type == CM_EVENT_UNLOCK) {
    iSignalSema(fillbuffer_sema);
  }
}

void audioThread(void *data) {
  while (1) {
      if (musicStatus = MUSIC_PLAYING) {
            cm_process((short int *)chunk, BUFFER_SIZE / 2);
            audsrv_wait_audio(BUFFER_SIZE);
            audsrv_play_audio(chunk, BUFFER_SIZE);
      }
  }
}

static void configureSemaphore() {
    /* Semaphore */
    ee_sema_t sema;
    sema.init_count = 1;
    sema.max_count = 1;
	sema.option = 0;
	fillbuffer_sema = CreateSema(&sema);
}

static int createAudioThread() {
  /* Create thread */
  ee_thread_t th_attr;
  th_attr.stack_size 	 = STACK_SIZE;
	th_attr.gp_reg 		 = (void *)&_gp;
	th_attr.initial_priority = 64;
	th_attr.option 		 = 0;

  th_attr.func = (void *)audioThread;
	th_attr.stack = (void *)malloc(STACK_SIZE);
	if (th_attr.stack == NULL) {
	  printLog("Error: Creating audio thread, No memory\n");
        return -1;
	}

	ee_threadID = CreateThread(&th_attr);
	if (ee_threadID < 0) {
		printLog("Error: Creating audio thread, No thread\n");
		return -1;
	}

	StartThread(ee_threadID, 0);
  return ee_threadID > 0 ? 0 : -1;
}

#endif

static int ps2_init(PlaybackFunction playbackFunc) { 
    configureSemaphore();

    printf("\n\n\n\n\n\nAUDIO INITTTT\n\n\n\n\n\n");

    struct audsrv_fmt_t ps2_format;
    ps2_format.bits     = AUDIO_BITS;
    ps2_format.freq     = 44100;
    ps2_format.channels = 2;

    audsrv_set_format(&ps2_format);
    audsrv_set_volume(MAX_VOLUME);

      // /* Init library */
    cm_init(ps2_format.freq);
    cm_set_lock(lock_handler);
    // cm_set_master_gain(0.5);

    if (createAudioThread()){
        printLog("Error: Creating audio thread\n");
        return 0;
  }

    return 1; 
}

static void ps2_configure(int format, int channels, int rate, int freq, int samples) {
#if RETRO_PLATFORM == RETRO_PS2
//     struct audsrv_fmt_t ps2_format;

//    ps2_format.bits     = AUDIO_BITS;
//    ps2_format.freq     = rate;
//    ps2_format.channels = channels;

//    audsrv_set_format(&ps2_format);
//    audsrv_set_volume(MAX_VOLUME);
//     printf("\n\n\n CONFIGURING AUDIO!!!!\n\n\n");
#endif
}
static void ps2_release() {}
static void ps2_createThread(ThreadFunction threadFunc, const char *name) {
    // ee_thread_t th_attr;

    // th_attr.stack_size 	 = STACK_SIZE;
	// th_attr.gp_reg 		 = (void *)&_gp;
	// th_attr.initial_priority = 64;
	// th_attr.option 		 = 0;

    // th_attr.func = (void *)threadFunc;
	// th_attr.stack = (void *)malloc(STACK_SIZE);
	// if (th_attr.stack == NULL)
	// {
	// 	printf("libmikmod: error creating update thread\n");
	// }

	// ee_threadID2 = CreateThread(&th_attr);
	// if (ee_threadID2 < 0) 
	// {
	// 	printf("libmikmod: Not enough resources to create update thread");
	// }

	// StartThread(ee_threadID2, 0);
    // printf("\n\n\n STARTING AUDIO THREAD %i!!!!\n\n\n", ee_threadID2);
}
static void ps2_lock() {}

static void ps2_unlock() {}
static void ps2_loadWav(const char *filePath, FileInfo *info, byte sfxID, byte *sfx) {
#if RETRO_PLATFORM == RETRO_PS2
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