#include "RetroEngine.hpp"
#include <cmath>

int globalSFXCount = 0;
int stageSFXCount  = 0;

int masterVolume  = MAX_VOLUME;
int trackID       = -1;
int sfxVolume     = MAX_VOLUME;
int bgmVolume     = MAX_VOLUME;
bool audioEnabled = false;

bool musicEnabled = 0;
int musicStatus   = MUSIC_STOPPED;
int musicStartPos = 0;
int musicPosition = 0;
int musicRatio    = 0;
TrackInfo musicTracks[TRACK_COUNT];
SFXInfo sfxList[SFX_COUNT];
char sfxNames[SFX_COUNT][0x40];

ChannelInfo sfxChannels[CHANNEL_COUNT];

#if !RETRO_USE_ORIGINAL_CODE
MusicPlaybackInfo musInfo;
#endif

int trackBuffer = -1;


#define ADJUST_VOLUME(s, v) (s = (s * v) / MAX_VOLUME)
// Select first driver
static const audio_driver_t *audio_drv = audio_drivers[0];

#define MIX_BUFFER_SAMPLES (256)

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

static void ProcessMusicStream(Sint32 *stream, size_t bytes_wanted)
{
    if (!musInfo.loaded)
        return;
    switch (musicStatus) {
        case MUSIC_READY:
        case MUSIC_PLAYING: {
            if (audio_drv->availableStream)
            while (audio_drv->availableStream() < bytes_wanted) {
                // We need more samples: get some
                long bytes_read = ov_read(&musInfo.vorbisFile, (char *)musInfo.buffer, sizeof(musInfo.buffer), 0, 2, 1, &musInfo.vorbBitstream);

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
                if (audio_drv->streamPut)
                    if (audio_drv->streamPut(musInfo.buffer, bytes_read)== -1)
                        return;
            }

            // Now that we know there are enough samples, read them and mix them
            int bytes_done = -1;
            if (audio_drv->streamGet)
                bytes_done = audio_drv->streamGet(musInfo.buffer, bytes_wanted);
            
            if (bytes_done == -1) {
                return;
            }
            if (bytes_done != 0)
                ProcessAudioMixing(stream, musInfo.buffer, bytes_done / sizeof(Sint16), (bgmVolume * masterVolume) / MAX_VOLUME, 0);
            
            break;
        }
        case MUSIC_STOPPED:
        case MUSIC_PAUSED:
        case MUSIC_LOADING:
            // dont play
            break;
    }
}

static void ProcessAudioPlayback(void *userdata, Uint8 *stream, int len)
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

int InitAudioPlayback()
{
    StopAllSfx(); //"init"
#if !RETRO_USE_ORIGINAL_CODE
    if (audio_drv->init)
        audioEnabled = audio_drv->init(ProcessAudioPlayback);
#endif
    LoadGlobalSfx();

    return true;
}

void LoadGlobalSfx()
{
    FileInfo info;
    FileInfo infoStore;
    char strBuffer[0x100];
    byte fileBuffer = 0;
    int fileBuffer2 = 0;

    if (LoadFile("Data/Game/GameConfig.bin", &info)) {
        infoStore = info;

        FileRead(&fileBuffer, 1);
        FileRead(strBuffer, fileBuffer);

        FileRead(&fileBuffer, 1);
        FileRead(strBuffer, fileBuffer);

        byte buf[3];
        for (int c = 0; c < 0x60; ++c) FileRead(buf, 3);

        // Read Obect Names
        byte objectCount = 0;
        FileRead(&objectCount, 1);
        for (byte o = 0; o < objectCount; ++o) {
            FileRead(&fileBuffer, 1);
            FileRead(&strBuffer, fileBuffer);
        }

        // Read Script Paths
        for (byte s = 0; s < objectCount; ++s) {
            FileRead(&fileBuffer, 1);
            FileRead(&strBuffer, fileBuffer);
        }

        byte varCount = 0;
        FileRead(&varCount, 1);
        globalVariablesCount = varCount;
        for (byte v = 0; v < varCount; ++v) {
            // Read Variable Name
            FileRead(&fileBuffer, 1);
            FileRead(&globalVariableNames[v], fileBuffer);
            globalVariableNames[v][fileBuffer] = 0;

            // Read Variable Value
            FileRead(&fileBuffer2, 4);
        }

        // Read SFX
        globalSFXCount = 0;
        FileRead(&fileBuffer, 1);
        globalSFXCount = fileBuffer;
        for (byte s = 0; s < globalSFXCount; ++s) { // SFX Names
            FileRead(&fileBuffer, 1);
            FileRead(&strBuffer, fileBuffer);
            strBuffer[fileBuffer] = 0;

            SetSfxName(strBuffer, s);
        }
        for (byte s = 0; s < globalSFXCount; ++s) { // SFX Paths
            FileRead(&fileBuffer, 1);
            FileRead(&strBuffer, fileBuffer);
            strBuffer[fileBuffer] = 0;

            GetFileInfo(&infoStore);
            LoadSfx(strBuffer, s);
            SetFileInfo(&infoStore);
        }

        CloseFile();
    }

    for (int i = 0; i < CHANNEL_COUNT; ++i) sfxChannels[i].sfxID = -1;
}

size_t readVorbis(void *mem, size_t size, size_t nmemb, void *ptr)
{
#if !RETRO_USE_ORIGINAL_CODE
    MusicPlaybackInfo *info = (MusicPlaybackInfo *)ptr;
    return FileRead2(&info->fileInfo, mem, (int)(size * nmemb));
#endif
    return 0;
}
int seekVorbis(void *ptr, ogg_int64_t offset, int whence)
{
#if !RETRO_USE_ORIGINAL_CODE
    MusicPlaybackInfo *info = (MusicPlaybackInfo *)ptr;
    switch (whence) {
        case SEEK_SET: whence = 0; break;
        case SEEK_CUR: whence = (int)GetFilePosition2(&info->fileInfo); break;
        case SEEK_END: whence = info->fileInfo.vfileSize; break;
        default: break;
    }
    SetFilePosition2(&info->fileInfo, (int)(whence + offset));
    return (int)GetFilePosition2(&info->fileInfo) <= info->fileInfo.vfileSize;
#endif
    return 0;
}
long tellVorbis(void *ptr)
{
#if !RETRO_USE_ORIGINAL_CODE
    MusicPlaybackInfo *info = (MusicPlaybackInfo *)ptr;
    return GetFilePosition2(&info->fileInfo);
#endif
    return 0;
}
int closeVorbis(void *ptr)
{
#if !RETRO_USE_ORIGINAL_CODE
    return CloseFile2((FileInfo *)ptr);
#endif
    return 1;
}

size_t readVorbis_Sfx(void *mem, size_t size, size_t nmemb, void *ptr)
{
#if !RETRO_USE_ORIGINAL_CODE
    FileInfo *info = (FileInfo *)ptr;
    return FileRead2(info, mem, (int)(size * nmemb));
#endif
    return 0;
}
int seekVorbis_Sfx(void *ptr, ogg_int64_t offset, int whence)
{
#if !RETRO_USE_ORIGINAL_CODE
    FileInfo *info = (FileInfo *)ptr;
    switch (whence) {
        case SEEK_SET: whence = 0; break;
        case SEEK_CUR: whence = (int)GetFilePosition2(info); break;
        case SEEK_END: whence = info->vfileSize; break;
        default: break;
    }
    SetFilePosition2(info, (int)(whence + offset));
    return (int)GetFilePosition2(info) <= info->vfileSize;
#endif
    return 0;
}
long tellVorbis_Sfx(void *ptr)
{
#if !RETRO_USE_ORIGINAL_CODE
    FileInfo *info = (FileInfo *)ptr;
    return GetFilePosition2(info);
#endif
    return 0;
}
int closeVorbis_Sfx(void *ptr)
{
#if !RETRO_USE_ORIGINAL_CODE
    return CloseFile2((FileInfo *)ptr);
#endif
    return 0;
}

void LoadMusic(void *userdata)
{
    if (trackBuffer < 0 || trackBuffer >= TRACK_COUNT) {
        if (audio_drv->lock)
            audio_drv->lock();

        StopMusic(true);
        if (audio_drv->unlock)
            audio_drv->unlock();
        return;
    }

    TrackInfo *trackPtr = &musicTracks[trackBuffer];

    if (!trackPtr->fileName[0]) {
        if (audio_drv->lock)
            audio_drv->lock();
        StopMusic(true);
        if (audio_drv->unlock)
            audio_drv->unlock();
        return;
    }

    if (audio_drv->lock)
            audio_drv->lock();
    uint oldPos   = 0;
    uint oldTotal = 0;
    if (musInfo.loaded) {
        oldPos   = (uint)ov_pcm_tell(&musInfo.vorbisFile);
        oldTotal = (uint)ov_pcm_total(&musInfo.vorbisFile, -1);
        StopMusic(false);
    }

    if (LoadFile2(trackPtr->fileName, &musInfo.fileInfo)) {
        musInfo.trackLoop = trackPtr->trackLoop;
        musInfo.loopPoint = trackPtr->loopPoint;
        musInfo.loaded    = true;

        unsigned long long samples = 0;
        ov_callbacks callbacks;

        callbacks.read_func  = readVorbis;
        callbacks.seek_func  = seekVorbis;
        callbacks.tell_func  = tellVorbis;
        callbacks.close_func = closeVorbis;

        int error = ov_open_callbacks(&musInfo, &musInfo.vorbisFile, NULL, 0, callbacks);
        if (error == 0) {
            musInfo.vorbBitstream = -1;
            musInfo.vorbisFile.vi = ov_info(&musInfo.vorbisFile, -1);

            samples = (unsigned long long)ov_pcm_total(&musInfo.vorbisFile, -1);

            if (audio_drv->configure)
                audio_drv->configure(1, musInfo.vorbisFile.vi->channels, musInfo.vorbisFile.vi->rate, 0, 0);

            musInfo.buffer = new Sint16[MIX_BUFFER_SAMPLES];

            if (musicStartPos) {
                float newPos  = oldPos * ((float)musicRatio * 0.0001); // 8000 == 0.8 (ratio / 10,000)
                musicStartPos = fmod(newPos, samples);

                ov_pcm_seek(&musInfo.vorbisFile, musicStartPos);
            }
            musicStartPos = 0;

            musicStatus  = MUSIC_PLAYING;
            masterVolume = MAX_VOLUME;
            trackID      = trackBuffer;
            trackBuffer  = -1;
        }
        else {
            musicStatus = MUSIC_STOPPED;
            CloseFile2(&musInfo.fileInfo);
            printLog("Failed to load vorbis! error: %d", error);
            switch (error) {
                default: printLog("Vorbis open error: Unknown (%d)", error); break;
                case OV_EREAD: printLog("Vorbis open error: A read from media returned an error"); break;
                case OV_ENOTVORBIS: printLog("Vorbis open error: Bitstream does not contain any Vorbis data"); break;
                case OV_EVERSION: printLog("Vorbis open error: Vorbis version mismatch"); break;
                case OV_EBADHEADER: printLog("Vorbis open error: Invalid Vorbis bitstream header"); break;
                case OV_EFAULT: printLog("Vorbis open error: Internal logic fault; indicates a bug or heap / stack corruption"); break;
            }
        }
    }
    if (audio_drv->unlock)
            audio_drv->unlock();
}

void SetMusicTrack(const char *filePath, byte trackID, bool loop, uint loopPoint)
{
    if (audio_drv->lock)
            audio_drv->lock();
    TrackInfo *track = &musicTracks[trackID];
    StrCopy(track->fileName, "Data/Music/");
    StrAdd(track->fileName, filePath);
    track->trackLoop = loop;
    track->loopPoint = loopPoint;
    if (audio_drv->unlock)
            audio_drv->unlock();
}

void SwapMusicTrack(const char *filePath, byte trackID, uint loopPoint, uint ratio)
{
    if (StrLength(filePath) <= 0) {
        StopMusic(true);
    }
    else {
        if (audio_drv->lock)
            audio_drv->lock();
        TrackInfo *track = &musicTracks[trackID];
        StrCopy(track->fileName, "Data/Music/");
        StrAdd(track->fileName, filePath);
        track->trackLoop = true;
        track->loopPoint = loopPoint;
        musicRatio       = ratio;
        if (audio_drv->unlock)
            audio_drv->unlock();
        PlayMusic(trackID, 1);
    }
}

bool PlayMusic(int track, int musStartPos)
{
    if (!audioEnabled)
        return false;

    if (musicStatus != MUSIC_LOADING) {
        if (audio_drv->lock)
            audio_drv->lock();
        musicStartPos = musStartPos;
        if (track < 0 || track >= TRACK_COUNT) {
            StopMusic(true);
            trackBuffer = -1;
            return false;
        }
        trackBuffer = track;
        musicStatus = MUSIC_LOADING;
        if (audio_drv->createThread)
            audio_drv->createThread(LoadMusic, "LoadMusic");
        if (audio_drv->unlock)
            audio_drv->unlock();
        return true;
    }
    else {
        printLog("WARNING music tried to play while music was loading!");
    }
    return false;
}

void SetSfxName(const char *sfxName, int sfxID)
{
    int sfxNameID   = 0;
    int soundNameID = 0;
    while (sfxName[sfxNameID]) {
        if (sfxName[sfxNameID] != ' ')
            sfxNames[sfxID][soundNameID++] = sfxName[sfxNameID];
        ++sfxNameID;
    }
    sfxNames[sfxID][soundNameID] = 0;
    printLog("Set SFX (%d) name to: %s", sfxID, sfxName);
}

void LoadSfx(char *filePath, byte sfxID)
{
    if (!audioEnabled)
        return;

    FileInfo info;
    char fullPath[0x80];

    StrCopy(fullPath, "Data/SoundFX/");
    StrAdd(fullPath, filePath);

    if (LoadFile(fullPath, &info)) {
        byte type = fullPath[StrLength(fullPath) - 3];
        if (type == 'w') {
            byte *sfx = new byte[info.vfileSize];
            FileRead(sfx, info.vfileSize);
            CloseFile();

            // PLAY WAV
            if (audio_drv->loadWav)
                audio_drv->loadWav(filePath, &info, sfxID, sfx);
        }
        else if (type == 'o') {
            // ogg sfx :(
            OggVorbis_File vf;
            ov_callbacks callbacks = OV_CALLBACKS_NOCLOSE;
            vorbis_info *vinfo;
            byte *buf;
            int bitstream = -1;
            long samplesize;
            long samples;
            int read, toRead;

            callbacks.read_func  = readVorbis_Sfx;
            callbacks.seek_func  = seekVorbis_Sfx;
            callbacks.tell_func  = tellVorbis_Sfx;
            callbacks.close_func = closeVorbis_Sfx;

            info.cFileHandle = cFileHandle;
            cFileHandle      = nullptr;

            // GetFileInfo(&info);
            int error = ov_open_callbacks(&info, &vf, NULL, 0, callbacks);
            if (error != 0) {
                ov_clear(&vf);
                printLog("failed to load ogg sfx!");
                return;
            }

            vinfo = ov_info(&vf, -1);

            uint32_t audioLen  = 0;
            byte *audioBuf = NULL;
            samples = (long)ov_pcm_total(&vf, -1);

            audioLen             = (uint32_t)(samples * musInfo.vorbisFile.vi->channels * 2);
            audioBuf             = (byte *)malloc(audioLen);
            buf                  = audioBuf;
            toRead               = audioLen;

            for (read = (int)ov_read(&vf, (char *)buf, toRead, 0, 2, 1, &bitstream); read > 0;
                 read = (int)ov_read(&vf, (char *)buf, toRead, 0, 2, 1, &bitstream)) {
                if (read < 0) {
                    free(audioBuf);
                    ov_clear(&vf);
                    printLog("failed to read ogg sfx!");
                    return;
                }
                toRead -= read;
                buf += read;
            }

            ov_clear(&vf);

            /* Don't return a buffer that isn't a multiple of samplesize */
            // samplesize = ((AUDIO_S16 & 0xFF) / 8) * vinfo->channels;
            audioLen &= ~(samplesize - 1);
        }
        else {
            // wtf lol
            CloseFile();
            printLog("Sfx format not supported!");
        }
    }
}

void PlaySfx(int sfx, bool loop)
{
    if (audio_drv->lock)
            audio_drv->lock();
    int sfxChannelID = -1;
    for (int c = 0; c < CHANNEL_COUNT; ++c) {
        if (sfxChannels[c].sfxID == sfx || sfxChannels[c].sfxID == -1) {
            sfxChannelID = c;
            break;
        }
    }

    ChannelInfo *sfxInfo  = &sfxChannels[sfxChannelID];
    sfxInfo->sfxID        = sfx;
    sfxInfo->samplePtr    = sfxList[sfx].buffer;
    sfxInfo->sampleLength = sfxList[sfx].length;
    sfxInfo->loopSFX      = loop;
    sfxInfo->pan          = 0;
    if (audio_drv->unlock)
        audio_drv->unlock();
}
void SetSfxAttributes(int sfx, int loopCount, sbyte pan)
{
    if (audio_drv->lock)
            audio_drv->lock();
    int sfxChannel = -1;
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        if (sfxChannels[i].sfxID == sfx) {
            sfxChannel = i;
            break;
        }
    }
    if (sfxChannel == -1) {
        if (audio_drv->unlock)
            audio_drv->unlock();
        return; // wasn't found
    }

    ChannelInfo *sfxInfo = &sfxChannels[sfxChannel];
    sfxInfo->loopSFX     = loopCount == -1 ? sfxInfo->loopSFX : loopCount;
    sfxInfo->pan         = pan;
    sfxInfo->sfxID       = sfx;
    if (audio_drv->unlock)
        audio_drv->unlock();
}

void StopMusic(bool setStatus)
{
    if (setStatus)
        musicStatus = MUSIC_STOPPED;

    if (musInfo.loaded) {
        if (audio_drv->lock)
            audio_drv->lock();
        
        if (musInfo.buffer)
            delete[] musInfo.buffer;

        ov_clear(&musInfo.vorbisFile);
        musInfo.buffer = nullptr;
        musInfo.trackLoop = false;
        musInfo.loopPoint = 0;
        musInfo.loaded    = false;

        if (audio_drv->release)
            audio_drv->release();

        if (audio_drv->unlock)
            audio_drv->unlock();
    }
}

void StopAllSfx()
{
#if !RETRO_USE_ORIGINAL_CODE
    if (audio_drv->lock)
        audio_drv->lock();
#endif
    for (int i = 0; i < CHANNEL_COUNT; ++i) sfxChannels[i].sfxID = -1;
#if !RETRO_USE_ORIGINAL_CODE
    if (audio_drv->unlock)
        audio_drv->unlock();
#endif
}

void StopSfx(int sfx)
{
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        if (sfxChannels[i].sfxID == sfx) {
            MEM_ZERO(sfxChannels[i]);
            sfxChannels[i].sfxID = -1;
        }
    }
}

#if !RETRO_USE_ORIGINAL_CODE
// Helper Funcs
bool PlaySFXByName(const char *sfx, sbyte loopCnt)
{
    for (int s = 0; s < globalSFXCount + stageSFXCount; ++s) {
        if (StrComp(sfxNames[s], sfx)) {
            PlaySfx(s, loopCnt);
            return true;
        }
    }
    return false;
}

bool StopSFXByName(const char *sfx)
{
    for (int s = 0; s < globalSFXCount + stageSFXCount; ++s) {
        if (StrComp(sfxNames[s], sfx)) {
            StopSfx(s);
            return true;
        }
    }
    return false;
}
#endif

void SetMusicVolume(int volume)
{
    if (volume < 0)
        volume = 0;
    if (volume > MAX_VOLUME)
        volume = MAX_VOLUME;
    masterVolume = volume;
}

void SetGameVolumes(int bgmVolume, int sfxVolume)
{
    // musicVolumeSetting = bgmVolume;
    SetMusicVolume(masterVolume);
    // sfxVolumeSetting = ((sfxVolume << 7) / 100);
}

void PauseSound()
{
    if (musicStatus == MUSIC_PLAYING)
        musicStatus = MUSIC_PAUSED;
}

void ResumeSound()
{
    if (musicStatus == MUSIC_PAUSED)
        musicStatus = MUSIC_PLAYING;
}

void ReleaseGlobalSfx()
{
    for (int i = globalSFXCount - 1; i >= 0; --i) {
        if (sfxList[i].loaded) {
            StrCopy(sfxList[i].name, "");
            StrCopy(sfxNames[i], "");
            if (sfxList[i].buffer)
                free(sfxList[i].buffer);
            sfxList[i].buffer = NULL;
            sfxList[i].length = 0;
            sfxList[i].loaded = false;
        }
    }
    globalSFXCount = 0;
}

void ReleaseStageSfx()
{
    for (int i = (stageSFXCount + globalSFXCount) - 1; i >= globalSFXCount; --i) {
        if (sfxList[i].loaded) {
            StrCopy(sfxList[i].name, "");
            StrCopy(sfxNames[i], "");
            if (sfxList[i].buffer)
                free(sfxList[i].buffer);
            sfxList[i].buffer = NULL;
            sfxList[i].length = 0;
            sfxList[i].loaded = false;
        }
    }
    stageSFXCount = 0;
}

void ReleaseAudioDevice()
{
    StopMusic(true);
    StopAllSfx();
    ReleaseStageSfx();
    ReleaseGlobalSfx();
}