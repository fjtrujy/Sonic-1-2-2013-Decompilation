#include "../../RetroEngine.hpp"

#if RETRO_USING_SDL1

#define AUDIO_FREQUENCY (44100)
#define AUDIO_FORMAT    (AUDIO_S16SYS) /**< Signed 16-bit samples */
#define AUDIO_SAMPLES   (0x800)
#define AUDIO_CHANNELS  (2)

#define ADJUST_VOLUME(s, v) (s = (s * v) / MAX_VOLUME)

static SDL_AudioSpec spec;
static SDL_AudioSpec audioDeviceFormat;

static void ProcessAudioMixing(Sint32 *dst, const Sint16 *src, int len, int volume, sbyte pan)
{
    if (volume == 0)
        return;

    if (volume > MAX_VOLUME)
        volume = MAX_VOLUME;

    float panL = 0.0;
    float panR = 0.0;
    int i      = 0;

    if (pan < 0) {
        panR = 1.0f - abs(pan / 100.0f);
        panL = 1.0f;
    }
    else if (pan > 0) {
        panL = 1.0f - abs(pan / 100.0f);
        panR = 1.0f;
    }

    while (len--) {
        Sint32 sample = *src++;
        ADJUST_VOLUME(sample, volume);

        if (pan != 0) {
            if ((i % 2) != 0) {
                sample *= panR;
            }
            else {
                sample *= panL;
            }
        }

        *dst++ += sample;

        i++;
    }
}

void ProcessMusicStream(Sint32 *stream, size_t bytes_wanted)
{
    if (!musInfo.loaded)
        return;
    switch (musicStatus) {
        case MUSIC_READY:
        case MUSIC_PLAYING: {
            size_t bytes_gotten = 0;
            byte *buffer        = (byte *)malloc(bytes_wanted);
            memset(buffer, 0, bytes_wanted);
            while (bytes_gotten < bytes_wanted) {
                // We need more samples: get some
                long bytes_read =
                    ov_read(&musInfo.vorbisFile, (char *)musInfo.buffer,
                            sizeof(musInfo.buffer) > (bytes_wanted - bytes_gotten) ? (bytes_wanted - bytes_gotten) : sizeof(musInfo.buffer), 0, 2, 1,
                            &musInfo.vorbBitstream);

                if (bytes_read == 0) {
                    // We've reached the end of the file
                    if (musInfo.trackLoop) {
                        ov_pcm_seek(&musInfo.vorbisFile, musInfo.loopPoint);
                        continue;
                    }
                    else {
                        musicStatus = MUSIC_STOPPED;
                        break;
                    }
                }

                if (bytes_read > 0) {
                    memcpy(buffer + bytes_gotten, musInfo.buffer, bytes_read);
                    bytes_gotten += bytes_read;
                }
                else {
                    printLog("Music read error: vorbis error: %d", bytes_read);
                }
            }

            if (bytes_gotten > 0) {
                SDL_AudioCVT convert;
                MEM_ZERO(convert);
                int cvtResult = SDL_BuildAudioCVT(&convert, musInfo.spec.format, musInfo.spec.channels, musInfo.spec.freq, audioDeviceFormat.format,
                                                  audioDeviceFormat.channels, audioDeviceFormat.freq);
                if (cvtResult == 0) {
                    if (convert.len_mult > 0) {
                        convert.buf = (byte *)malloc(bytes_gotten * convert.len_mult);
                        convert.len = bytes_gotten;
                        memcpy(convert.buf, buffer, bytes_gotten);
                        SDL_ConvertAudio(&convert);
                    }
                }

                // Now that we know there are enough samples, read them and mix them
                // int bytes_done = SDL_AudioStreamGet(musInfo.stream, musInfo.buffer, bytes_wanted);
                // if (bytes_done == -1) {
                //    return;
                //}

                if (cvtResult == 0)
                    ProcessAudioMixing(stream, (const Sint16 *)convert.buf, bytes_gotten / sizeof(Sint16), (bgmVolume * masterVolume) / MAX_VOLUME,
                                       0);

                if (convert.len > 0 && convert.buf)
                    free(convert.buf);
            }
            if (bytes_wanted > 0)
                free(buffer);

            break;
        }
        case MUSIC_STOPPED:
        case MUSIC_PAUSED:
        case MUSIC_LOADING:
            // dont play
            break;
    }
}

static static void ProcessAudioPlayback(void *userdata, Uint8 *stream, int len)
{
    (void)userdata; // Unused

    if (!audioEnabled)
        return;

    Sint16 *output_buffer = (Sint16 *)stream;

    size_t samples_remaining = (size_t)len / sizeof(Sint16);
    while (samples_remaining != 0) {
        Sint32 mix_buffer[MIX_BUFFER_SAMPLES];
        memset(mix_buffer, 0, sizeof(mix_buffer));

        const size_t samples_to_do = (samples_remaining < MIX_BUFFER_SAMPLES) ? samples_remaining : MIX_BUFFER_SAMPLES;

        // Mix music
        ProcessMusicStream(mix_buffer, samples_to_do * sizeof(Sint16));

        // Mix SFX
        for (byte i = 0; i < CHANNEL_COUNT; ++i) {
            ChannelInfo *sfx = &sfxChannels[i];
            if (sfx == NULL)
                continue;

            if (sfx->sfxID < 0)
                continue;

            if (sfx->samplePtr) {
                Sint16 buffer[MIX_BUFFER_SAMPLES];

                size_t samples_done = 0;
                while (samples_done != samples_to_do) {
                    size_t sampleLen = (sfx->sampleLength < samples_to_do - samples_done) ? sfx->sampleLength : samples_to_do - samples_done;
                    memcpy(&buffer[samples_done], sfx->samplePtr, sampleLen * sizeof(Sint16));

                    samples_done += sampleLen;
                    sfx->samplePtr += sampleLen;
                    sfx->sampleLength -= sampleLen;

                    if (sfx->sampleLength == 0) {
                        if (sfx->loopSFX) {
                            sfx->samplePtr    = sfxList[sfx->sfxID].buffer;
                            sfx->sampleLength = sfxList[sfx->sfxID].length;
                        }
                        else {
                            StopSfx(sfx->sfxID);
                            break;
                        }
                    }
                }

                ProcessAudioMixing(mix_buffer, buffer, (int)samples_done, sfxVolume, sfx->pan);
            }
        }

        // Clamp mixed samples back to 16-bit and write them to the output buffer
        for (size_t i = 0; i < sizeof(mix_buffer) / sizeof(*mix_buffer); ++i) {
            const Sint16 max_audioval = ((1 << (16 - 1)) - 1);
            const Sint16 min_audioval = -(1 << (16 - 1));

            const Sint32 sample = mix_buffer[i];

            if (sample > max_audioval)
                *output_buffer++ = max_audioval;
            else if (sample < min_audioval)
                *output_buffer++ = min_audioval;
            else
                *output_buffer++ = sample;
        }

        samples_remaining -= samples_to_do;
    }
}
#endif

static int sdl1_init() {
    bool audioEnabled = false;
#if RETRO_USING_SDL1
    SDL_AudioSpec want;
    want.freq     = AUDIO_FREQUENCY;
    want.format   = AUDIO_FORMAT;
    want.samples  = AUDIO_SAMPLES;
    want.channels = AUDIO_CHANNELS;
    want.callback = ProcessAudioPlayback;

    if (SDL_OpenAudio(&want, &audioDeviceFormat) == 0) {
        SDL_PauseAudio(0);
        audioEnabled = true;
    }
    else {
        printLog("Unable to open audio device: %s", SDL_GetError());
    }
#endif

    return audioEnabled;
}

static void sdl2_release() {
#if RETRO_USING_SDL1
    if (musInfo.buffer)
            delete[] musInfo.buffer;

    ov_clear(&musInfo.vorbisFile);
    musInfo.buffer = nullptr;
    musInfo.trackLoop = false;
    musInfo.loopPoint = 0;
    musInfo.loaded    = false;
#endif
}

static void sdl1_configure(int format, int channels, int rate, int freq, int samples) {
#if RETRO_USING_SDL1
            musInfo.spec.format   = AUDIO_S16;
            musInfo.spec.channels = musInfo.vorbisFile.vi->channels;
            musInfo.spec.freq     = (int)musInfo.vorbisFile.vi->rate;
#endif
}

static void sdl1_createThread(void func, const char *name) {
#if RETRO_USING_SDL1
        // SDL_CreateThread((SDL_ThreadFunction)func, name, NULL);
#endif
}

static void sdl2_lock() {
#if RETRO_USING_SDL1
    SDL_LockAudio();
#endif
}

static void sdl2_unlock() {
#if RETRO_USING_SDL1
    SDL_UnlockAudio();
#endif
}

input_driver_t input_SDL1 = {
    sdl1_init,
    sdl1_close,
    sdl1_initDevices,
    sdl1_releaseDevices,
    sdl1_processInput,
};