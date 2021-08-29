#ifndef RETROCONFIG_H
#define RETROCONFIG_H

// Disables POSIX use c++ name blah blah stuff
#pragma warning(disable : 4996)

// Setting this to true removes (almost) ALL changes from the original code, the trade off is that a playable game cannot be built, it is advised to
// be set to true only for preservation purposes
#define RETRO_USE_ORIGINAL_CODE (0)
#define RETRO_USE_MOD_LOADER    (0)

#if !RETRO_USE_ORIGINAL_CODE
#undef RETRO_USE_MOD_LOADER
#define RETRO_USE_MOD_LOADER (1)
#endif //  !RETRO_USE_ORIGINAL_CODE

// Platforms (RSDKv4 only defines these 7 (I assume), but feel free to add your own custom platform define for easier platform code changes)
#define RETRO_WIN      (0)
#define RETRO_OSX      (1)
#define RETRO_XBOX_360 (2)
#define RETRO_PS3      (3)
#define RETRO_iOS      (4)
#define RETRO_ANDROID  (5)
#define RETRO_WP7      (6)
#define RETRO_PS2      (7)
// Custom Platforms start here
#define RETRO_UWP (8)

// Platform types (Game manages platform-specific code such as HUD position using this rather than the above)
#define RETRO_STANDARD (0)
#define RETRO_MOBILE   (1)

#if defined _WIN32

#if defined WINAPI_FAMILY
#if WINAPI_FAMILY != WINAPI_FAMILY_APP
#define RETRO_PLATFORM   (RETRO_WIN)
#define RETRO_DEVICETYPE (RETRO_STANDARD)
#else
#include <WInRTIncludes.hpp>

#define RETRO_PLATFORM   (RETRO_UWP)
#define RETRO_DEVICETYPE (UAP_GetRetroGamePlatform())
#endif
#else
#define RETRO_PLATFORM   (RETRO_WIN)
#define RETRO_DEVICETYPE (RETRO_STANDARD)
#endif

#elif defined PS2
#define RETRO_PLATFORM   (RETRO_PS2)
#define RETRO_DEVICETYPE (RETRO_STANDARD)
#elif defined __APPLE__
#if __IPHONEOS__
#define RETRO_PLATFORM   (RETRO_iOS)
#define RETRO_DEVICETYPE (RETRO_MOBILE)
#else
// #define RETRO_PLATFORM   (RETRO_OSX)
#define RETRO_PLATFORM   (RETRO_WIN)
#define RETRO_DEVICETYPE (RETRO_STANDARD)
#endif
#elif defined __ANDROID__
#define RETRO_PLATFORM   (RETRO_ANDROID)
#define RETRO_DEVICETYPE (RETRO_MOBILE)
#include <jni.h>
#else
#define RETRO_PLATFORM   (RETRO_WIN)
#define RETRO_DEVICETYPE (RETRO_STANDARD)
#endif

#define DEFAULT_SCREEN_XSIZE 424
#define DEFAULT_FULLSCREEN   false
#define RETRO_USING_MOUSE
#define RETRO_USING_TOUCH
// set this to 1 (integer scale) for other platforms that don't support bilinear and don't have an even screen size
#define RETRO_DEFAULTSCALINGMODE 2

#ifndef BASE_PATH
#define BASE_PATH ""
#endif

#if RETRO_PLATFORM == RETRO_WIN || RETRO_PLATFORM == RETRO_OSX || RETRO_PLATFORM == RETRO_UWP || RETRO_PLATFORM == RETRO_ANDROID
#define RETRO_USING_SDL1 (0)
#define RETRO_USING_SDL2 (1)
#else // Since its an else & not an elif these platforms probably aren't supported yet
#define RETRO_USING_SDL1 (0)
#define RETRO_USING_SDL2 (0)
#endif

#if RETRO_PLATFORM == RETRO_iOS || RETRO_PLATFORM == RETRO_ANDROID || RETRO_PLATFORM == RETRO_WP7
#define RETRO_GAMEPLATFORM (RETRO_MOBILE)
#elif RETRO_PLATFORM == RETRO_UWP
#define RETRO_GAMEPLATFORM (UAP_GetRetroGamePlatform())
#else
#define RETRO_GAMEPLATFORM (RETRO_STANDARD)
#endif

#define RETRO_SW_RENDER  (0)
#define RETRO_HW_RENDER  (1)
#define RETRO_RENDERTYPE (RETRO_SW_RENDER)

#ifdef USE_SW_REN
#undef RETRO_RENDERTYPE
#define RETRO_RENDERTYPE (RETRO_SW_RENDER)
#endif

#ifdef USE_HW_REN
#undef RETRO_RENDERTYPE
#define RETRO_RENDERTYPE (RETRO_HW_RENDER)
#endif

#if RETRO_RENDERTYPE == RETRO_SW_RENDER
#define RETRO_USING_OPENGL (0)
#elif RETRO_RENDERTYPE == RETRO_HW_RENDER
#define RETRO_USING_OPENGL (1)
#endif

#define RETRO_SOFTWARE_RENDER (RETRO_RENDERTYPE == RETRO_SW_RENDER)
#define RETRO_HARDWARE_RENDER (RETRO_RENDERTYPE == RETRO_HW_RENDER)

#if RETRO_USING_OPENGL
#include <GL/glew.h>
#include <GL/glu.h>

#if RETRO_USING_SDL2
#include <SDL_opengl.h>
#endif
#endif

#define RETRO_USE_HAPTICS (1)

// reverts opcode list back to how it was in earliest builds, fixes bugs on some datafiles
// generally advised to keep this set to 0
#define RETRO_REV01 (0)


// General Defines
#define SCREEN_YSIZE   (240)
#define SCREEN_CENTERY (SCREEN_YSIZE / 2)

#if RETRO_PLATFORM == RETRO_WIN || RETRO_PLATFORM == RETRO_UWP || RETRO_PLATFORM == RETRO_ANDROID || RETRO_PLATFORM == RETRO_PS2
#if RETRO_USING_SDL2
#include <SDL.h>
#elif RETRO_USING_SDL1
#include <SDL.h>
#endif
#include <vorbis/vorbisfile.h>
#elif RETRO_PLATFORM == RETRO_OSX
#include <SDL2/SDL.h>
#include <Vorbis/vorbisfile.h>

#include "cocoaHelpers.hpp"

#elif RETRO_USING_SDL2
#include <SDL2/SDL.h>
#include <vorbis/vorbisfile.h>
#else

#endif

#endif // !RETROCONFIG_H
