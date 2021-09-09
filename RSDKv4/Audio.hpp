#ifndef AUDIO_H
#define AUDIO_H

#include "cmixer.hpp"

#if !RETRO_USING_SDL1 && !RETRO_USING_SDL2
typedef signed short	Sint16;
typedef signed int	Sint32;

typedef unsigned char	Uint8;
#endif

#define TRACK_COUNT (0x10)
#define SFX_COUNT   (0x100)

#define MAX_VOLUME (100)

struct TrackInfo {
    char fileName[0x40];
    bool trackLoop;
    uint loopPoint;
};

#if !RETRO_USE_ORIGINAL_CODE
struct MusicPlaybackInfo {
    OggVorbis_File vorbisFile;
    int vorbBitstream;
    Sint16 *buffer;
    FileInfo fileInfo;
    bool trackLoop;
    uint loopPoint;
    bool loaded;
};
#endif

typedef  void (*ThreadFunction) (void *data);
typedef  void (*PlaybackFunction) (void *userdata, Uint8 *stream, int len);

typedef struct audio_driver
{
   int (*init)(PlaybackFunction playbackFunc);
   void (*configure)(int format, int channels, int rate, int freq, int samples);
   void (*createThread)(ThreadFunction threadFunc, const char *name);
   void (*lock)();
   void (*unlock)();
   void (*loadWav)(const char *filePath, FileInfo *info, byte sfxID, byte *sfx);
   int (*availableStream)();
   int (*streamPut)(Sint16 *buffer, size_t size);
   int (*streamGet)(Sint16 *buffer, size_t size);
   void (*release)();
} audio_driver_t;

// DRIVERS
extern audio_driver_t audio_SDL2;
extern audio_driver_t audio_SDL1;
extern audio_driver_t audio_PS2;
extern audio_driver_t audio_NULL;


static const audio_driver_t *audio_drivers[] = {
#if RETRO_USING_SDL2
    &audio_SDL2,
#elif RETRO_USING_SDL1
    &audio_SDL1,
#elif RETRO_PLATFORM == RETRO_PS2
    &audio_PS2,
#else
    &audio_NULL,
#endif
};

struct ChannelInfo {
    size_t sampleLength;
    Sint16 *samplePtr;
    int sfxID;
    byte loopSFX;
    sbyte pan;
};

struct SFXInfo {
    struct cm_Source *source;
    void *buffer;
    char name[0x40];
};

enum MusicStatuses {
    MUSIC_STOPPED = 0,
    MUSIC_PLAYING = 1,
    MUSIC_PAUSED  = 2,
    MUSIC_LOADING = 3,
    MUSIC_READY   = 4,
};

extern int globalSFXCount;
extern int stageSFXCount;

extern int masterVolume;
extern int trackID;
extern int sfxVolume;
extern int bgmVolume;

extern bool musicEnabled;
extern int musicStatus;
extern int musicStartPos;
extern int musicPosition;
extern int musicRatio;
extern TrackInfo musicTracks[TRACK_COUNT];

extern struct SFXInfo *sfxList[SFX_COUNT];

#if !RETRO_USE_ORIGINAL_CODE
extern MusicPlaybackInfo musInfo;
#endif

int InitAudioPlayback();
void LoadGlobalSfx();
void SetMusicTrack(const char *filePath, byte trackID, bool loop, uint loopPoint);
void SwapMusicTrack(const char *filePath, byte trackID, uint loopPoint, uint ratio);
bool PlayMusic(int track, int musStartPos);
void StopMusic(bool setStatus);
void LoadSfx(char *filePath, byte sfxID);
void PlaySfx(int sfx, bool loop);
void StopSfx(int sfx);
void SetSfxAttributes(int sfx, int loopCount, sbyte pan);
void SetSfxName(const char *sfxName, int sfxID);

#if !RETRO_USE_ORIGINAL_CODE
// Helper Funcs
bool PlaySFXByName(const char *sfx, sbyte loopCnt);
bool StopSFXByName(const char *sfx);
#endif

void SetMusicVolume(int volume);
void SetGameVolumes(int bgmVolume, int sfxVolume);
void PauseSound();
void ResumeSound();
void StopAllSfx();
void ReleaseGlobalSfx();
void ReleaseStageSfx();
void ReleaseAudioDevice();

#endif // !AUDIO_H
