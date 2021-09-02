#include "../../RetroEngine.hpp"

#if RETRO_USING_SDL2

#define AUDIO_FREQUENCY (44100)
#define AUDIO_FORMAT    (AUDIO_S16SYS) /**< Signed 16-bit samples */
#define AUDIO_SAMPLES   (0x800)
#define AUDIO_CHANNELS  (2)

static SDL_AudioStream *stream;
static SDL_AudioDeviceID audioDevice;
static SDL_AudioSpec audioDeviceFormat;

#endif

static int sdl2_init(PlaybackFunction playbackFunc) {
    bool audioEnabled = false;
#if RETRO_USING_SDL2
    SDL_AudioSpec want;
    want.freq     = AUDIO_FREQUENCY;
    want.format   = AUDIO_FORMAT;
    want.samples  = AUDIO_SAMPLES;
    want.channels = AUDIO_CHANNELS;
    want.callback = playbackFunc;

    if ((audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &audioDeviceFormat, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE)) > 0) {
        SDL_PauseAudioDevice(audioDevice, 0);
        audioEnabled = true;
    }
    else {
        printLog("Unable to open audio device: %s", SDL_GetError());
    }

#endif
    return audioEnabled;
}

static void sdl2_configure(int format, int channels, int rate, int freq, int samples) {
#if RETRO_USING_SDL2
    stream = SDL_NewAudioStream(AUDIO_S16, musInfo.vorbisFile.vi->channels, (int)musInfo.vorbisFile.vi->rate,
                                                audioDeviceFormat.format, audioDeviceFormat.channels, audioDeviceFormat.freq);
    if (!stream)
        printLog("Failed to create stream: %s", SDL_GetError());
#endif
}

static void sdl2_release() {
#if RETRO_USING_SDL2
    if (stream)
        SDL_FreeAudioStream(stream);
    stream = nullptr;
#endif
}

static void sdl2_createThread(ThreadFunction threadFunc, const char *name) {
#if RETRO_USING_SDL2
        SDL_CreateThread((SDL_ThreadFunction)threadFunc, name, NULL);
#endif
}

static void sdl2_lock() {
#if RETRO_USING_SDL2
    SDL_LockAudio();
#endif
}

static void sdl2_unlock() {
#if RETRO_USING_SDL2
    SDL_UnlockAudio();
#endif
}

static void sdl2_loadWav(const char *filePath, FileInfo *info, byte sfxID, byte *sfx)
{
#if RETRO_USING_SDL2
    SDL_RWops *src = SDL_RWFromMem(sfx, info->vfileSize);
    if (src == NULL) {
        printLog("Unable to open sfx: %s", info->fileName);
    }
    else {
        SDL_AudioSpec wav_spec;
        uint wav_length;
        byte *wav_buffer;
        SDL_AudioSpec *wav = SDL_LoadWAV_RW(src, 0, &wav_spec, &wav_buffer, &wav_length);

        SDL_RWclose(src);
        delete[] sfx;
        if (wav == NULL) {
            printLog("Unable to read sfx: %s", info->fileName);
        }
        else {
            SDL_AudioCVT convert;
            if (SDL_BuildAudioCVT(&convert, wav->format, wav->channels, wav->freq, audioDeviceFormat.format, audioDeviceFormat.channels,
                                    audioDeviceFormat.freq)
                > 0) {
                convert.buf = (byte *)malloc(wav_length * convert.len_mult);
                convert.len = wav_length;
                memcpy(convert.buf, wav_buffer, wav_length);
                SDL_ConvertAudio(&convert);

                sdl2_lock();

                StrCopy(sfxList[sfxID].name, filePath);
                sfxList[sfxID].buffer = (Sint16 *)convert.buf;
                sfxList[sfxID].length = convert.len_cvt / sizeof(Sint16);
                sfxList[sfxID].loaded = true;
                
                sdl2_unlock();

                SDL_FreeWAV(wav_buffer);
            }
            else { //this causes errors, actually
                printLog("Unable to read sfx: %s (error: %s)", info->fileName, SDL_GetError());
                sfxList[sfxID].loaded = false;
                SDL_FreeWAV(wav_buffer);
                //if (audio_drv->lock)
                //  audio_drv->lock();
                //StrCopy(sfxList[sfxID].name, filePath);
                //sfxList[sfxID].buffer = (Sint16 *)wav_buffer;
                //sfxList[sfxID].length = wav_length / sizeof(Sint16);
                //sfxList[sfxID].loaded = false;
                //if (audio_drv->unlock)
                //  audio_drv->unlock();
            }
        }
    }
#endif
}

static int sdl2_availableStream() {
#if RETRO_USING_SDL2
    return SDL_AudioStreamAvailable(stream);
#endif
}

static int sdl2_streamPut(Sint16 *buffer, size_t size) {
#if RETRO_USING_SDL2
    return SDL_AudioStreamPut(stream, buffer, (int)size);
#endif
}

static int sdl2_streamGet(Sint16 *buffer, size_t size) {
#if RETRO_USING_SDL2
    return SDL_AudioStreamGet(stream, buffer, (int)size);
#endif
}

audio_driver_t audio_SDL2 = {
   sdl2_init,
   sdl2_configure,
   sdl2_createThread,
   sdl2_lock,
   sdl2_unlock,
   sdl2_loadWav,
   sdl2_availableStream,
   sdl2_streamPut,
   sdl2_streamGet,
   sdl2_release,
};