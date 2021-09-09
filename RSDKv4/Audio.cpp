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
struct SFXInfo *sfxList[SFX_COUNT];

struct cm_Source *currentTrack;
void *currentTrackBuffer;

#if !RETRO_USE_ORIGINAL_CODE
MusicPlaybackInfo musInfo;
#endif

int trackBuffer = -1;


#define ADJUST_VOLUME(s, v) (s = (s * v) / MAX_VOLUME)
// Select first driver
static const audio_driver_t *audio_drv = audio_drivers[0];

#define MIX_BUFFER_SAMPLES (256)

void ProcessAudioPlayback(void *userdata, Uint8 *stream, int len) {}

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

    int i;
    for (i = 0; i <SFX_COUNT; i++) {
        if(!sfxList[i])
            sfxList[i] = (SFXInfo *)calloc(1, sizeof(SFXInfo));
    }

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
}

void SetMusicTrack(const char *filePath, byte trackID, bool loop, uint loopPoint)
{
    printf("%s\n", __FUNCTION__);
    // if (audio_drv->lock)
    //         audio_drv->lock();
    TrackInfo *track = &musicTracks[trackID];
    StrCopy(track->fileName, "Data/Music/");
    StrAdd(track->fileName, filePath);
    track->trackLoop = loop;
    track->loopPoint = loopPoint;
    // if (audio_drv->unlock)
    //         audio_drv->unlock();

    printf("%s, fileName %s, trackID %i\n", __FUNCTION__, track->fileName, trackID);
}

void SwapMusicTrack(const char *filePath, byte trackID, uint loopPoint, uint ratio)
{
    printf("%s\n", __FUNCTION__);
    // if (StrLength(filePath) <= 0) {
    //     StopMusic(true);
    // }
    // else {
    //     if (audio_drv->lock)
    //         audio_drv->lock();
    //     TrackInfo *track = &musicTracks[trackID];
    //     StrCopy(track->fileName, "Data/Music/");
    //     StrAdd(track->fileName, filePath);
    //     track->trackLoop = true;
    //     track->loopPoint = loopPoint;
    //     musicRatio       = ratio;
    //     if (audio_drv->unlock)
    //         audio_drv->unlock();
    //     PlayMusic(trackID, 1);
    // }
}

static void stopCurrentTrack() {

    if (currentTrack) {
        cm_stop(currentTrack);
        cm_destroy_source(currentTrack);
    }
    if (currentTrackBuffer)
        free(currentTrackBuffer);
    
    currentTrack = NULL;
    currentTrackBuffer = NULL;
}

bool PlayMusic(int track, int musStartPos)
{
    FileInfo info;
    TrackInfo *trackPtr = &musicTracks[track];
    
    stopCurrentTrack();

    if (!trackPtr->fileName[0]) {
        return false;
    }

    if (LoadFile(trackPtr->fileName, &info)) {
        printf("%s track %i loop %i, loopPoint %i\n", __FUNCTION__, track, trackPtr->trackLoop, trackPtr->loopPoint);
        printf("Track %s, size %i\n", trackPtr->fileName, info.vfileSize);
        void *sfx = malloc(info.vfileSize); 
        FileRead(sfx, info.vfileSize);
        CloseFile();

        currentTrackBuffer = sfx;
        currentTrack = cm_new_source_from_mem(sfx, info.vfileSize);
        if (!currentTrack) {
            printf("Error creating MUSIC item\n");
            return false;
        }
        // currentTrack->loop = trackPtr->loopPoint;
        // if (trackPtr->trackLoop)
            cm_set_loop(currentTrack, trackPtr->loopPoint);
        cm_play(currentTrack);

        // FILE *pFile;
        // /* Write your buffer to disk. */
        // pFile = fopen(trackPtr->fileName,"wb");

        // if (pFile){
        //     fwrite(sfx, info.vfileSize, 1, pFile);
        //     fclose(pFile);
        // }

        // free(sfx);
        return true;
    }

    // if (!audioEnabled)
    //     return false;

    // if (musicStatus != MUSIC_LOADING) {
    //     if (audio_drv->lock)
    //         audio_drv->lock();
    //     musicStartPos = musStartPos;
    //     if (track < 0 || track >= TRACK_COUNT) {
    //         StopMusic(true);
    //         trackBuffer = -1;
    //         return false;
    //     }
    //     trackBuffer = track;
    //     musicStatus = MUSIC_LOADING;
    //     if (audio_drv->createThread)
    //         audio_drv->createThread(LoadMusic, "LoadMusic");
    //     if (audio_drv->unlock)
    //         audio_drv->unlock();
    //     return true;
    // }
    // else {
    //     printLog("WARNING music tried to play while music was loading!");
    // }
    // return false;
}

void SetSfxName(const char *sfxName, int sfxID)
{
    if (sfxList[sfxID]) {
        struct SFXInfo *info = sfxList[sfxID];
        strcpy(info->name, sfxName);
        printLog("Set SFX (%d) name to: %s", sfxID, sfxName);
    }
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
        void *sfx = malloc(info.vfileSize); 
        FileRead(sfx, info.vfileSize);
        CloseFile();

        if (sfxList[sfxID]) {
            struct SFXInfo *sfx_info = sfxList[sfxID];
            sfx_info->buffer = sfx;
            sfx_info->source = cm_new_source_from_mem(sfx, info.vfileSize);
            if (!sfx_info->source)
                printf("Error creating SFX item");
        }
    }
}

void PlaySfx(int sfx, bool loop)
{
    if (sfxList[sfx]) {
        struct SFXInfo *info = sfxList[sfx];
        cm_set_loop(info->source, loop);
        cm_play(info->source);
    }

}
void SetSfxAttributes(int sfx, int loopCount, sbyte pan)
{
    // if (audio_drv->lock)
    //         audio_drv->lock();
    // int sfxChannel = -1;
    // if (sfxChannel == -1) {
    //     if (audio_drv->unlock)
    //         audio_drv->unlock();
    //     return; // wasn't found
    // }

    // ChannelInfo *sfxInfo = &sfxChannels[sfxChannel];
    // sfxInfo->loopSFX     = loopCount == -1 ? sfxInfo->loopSFX : loopCount;
    // sfxInfo->pan         = pan;
    // sfxInfo->sfxID       = sfx;
    // if (audio_drv->unlock)
    //     audio_drv->unlock();
}

void StopMusic(bool setStatus)
{
    stopCurrentTrack();
    // if (setStatus)
    //     musicStatus = MUSIC_STOPPED;

    // if (musInfo.loaded) {
    //     if (audio_drv->lock)
    //         audio_drv->lock();
        
    //     if (musInfo.buffer)
    //         delete[] musInfo.buffer;

    //     ov_clear(&musInfo.vorbisFile);
    //     musInfo.buffer = nullptr;
    //     musInfo.trackLoop = false;
    //     musInfo.loopPoint = 0;
    //     musInfo.loaded    = false;

    //     if (audio_drv->release)
    //         audio_drv->release();

    //     if (audio_drv->unlock)
    //         audio_drv->unlock();
    // }
}

void StopSfx(int sfx)
{
    if (sfxList[sfx]) {
        struct SFXInfo *info = sfxList[sfx];
        if (info->source)
            cm_stop(info->source);
    }
}

void StopAllSfx()
{
    int i;
    for (i = 0; i <SFX_COUNT; i++) {
        StopSfx(i);
    }
}

#if !RETRO_USE_ORIGINAL_CODE
// Helper Funcs
bool PlaySFXByName(const char *sfx, sbyte loopCnt)
{
    // for (int s = 0; s < globalSFXCount + stageSFXCount; ++s) {
    //     if (StrComp(sfxNames[s], sfx)) {
    //         PlaySfx(s, loopCnt);
    //         return true;
    //     }
    // }
    return false;
}

bool StopSFXByName(const char *sfx)
{
    // for (int s = 0; s < globalSFXCount + stageSFXCount; ++s) {
    //     if (StrComp(sfxNames[s], sfx)) {
    //         StopSfx(s);
    //         return true;
    //     }
    // }
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
        if (sfxList[i]) {
            struct SFXInfo *info = sfxList[i];
            cm_stop(info->source);
            // free(info->buffer);
            StrCopy(info->name, "");
            // free(info->source);
        }
    }
    globalSFXCount = 0;
}

void ReleaseStageSfx()
{
    for (int i = (stageSFXCount + globalSFXCount) - 1; i >= globalSFXCount; --i) {
        if (sfxList[i]) {
            struct SFXInfo *info = sfxList[i];
            cm_stop(info->source);
            // free(info->buffer);
            StrCopy(info->name, "");
            // free(info->source);
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

    int i;
    for (i = 0; i <SFX_COUNT; i++) {
        if(sfxList[i])
            free(sfxList[i]);
    }
}