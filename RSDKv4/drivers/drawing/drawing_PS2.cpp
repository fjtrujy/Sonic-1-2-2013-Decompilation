#include "../../RetroEngine.hpp"

#if RETRO_PLATFORM == RETRO_PS2

#include <Kernel.h>
#include <gsKit.h>
#include <dmaKit.h>

#include <sys/times.h>
#include <sys/time.h>

struct VidMode {
    const char *name;
    s16 mode;
    s16 interlace;
    s16 field;
    int max_width;
    int max_height;
    int width;
    int height;
    int vck;
    int x_off;
    int y_off;
};

static const struct VidMode vid_modes[] = {
    // NTSC
    { "480i", GS_MODE_NTSC,      GS_INTERLACED,    GS_FIELD,  704,  480,  704,  452, 4, 0, 0 },
    { "480p", GS_MODE_DTV_480P,  GS_NONINTERLACED, GS_FRAME,  704,  480,  704,  452, 2, 0, 0 },
    // PAL
    { "576i", GS_MODE_PAL,       GS_INTERLACED,    GS_FIELD,  704,  576,  704,  536, 4, 0, 0 },
    { "576p", GS_MODE_DTV_576P,  GS_NONINTERLACED, GS_FRAME,  704,  576,  704,  536, 2, 0, 0 },
    // HDTV
    { "720p", GS_MODE_DTV_720P,  GS_NONINTERLACED, GS_FRAME, 1280,  720, 1280,  698, 1, 0, 0 },
    {"1080i", GS_MODE_DTV_1080I, GS_INTERLACED,    GS_FIELD, 1920, 1080, 1920, 1080, 1, 0, 0 },
};

static GSGLOBAL *gsGlobal;
static int vsync_sema_id;
static GSTEXTURE *mainTexture;

/* Generic tint color */
#define GS_TEXT GS_SETREG_RGBA(0x80,0x80,0x80,0x80)
/* turn black GS Screen */
#define GS_BLACK GS_SETREG_RGBA(0x00,0x00,0x00,0x80)

static void deinit_GSGlobal(GSGLOBAL *gsGlobal)
{
   gsKit_clear(gsGlobal, GS_BLACK);
   gsKit_vram_clear(gsGlobal);
   gsKit_deinit_global(gsGlobal);
}

/* Copy of gsKit_sync_flip, but without the 'flip' */
static void gsKit_sync(GSGLOBAL *gsGlobal)
{
    if (!gsGlobal->FirstFrame)
      WaitSema(vsync_sema_id);

   while (PollSema(vsync_sema_id) >= 0);
}

/* Copy of gsKit_sync_flip, but without the 'sync' */
static void gsKit_flip(GSGLOBAL *gsGlobal)
{
   if (!gsGlobal->FirstFrame)
   {
      if (gsGlobal->DoubleBuffering == GS_SETTING_ON)
      {
         GS_SET_DISPFB2( gsGlobal->ScreenBuffer[
               gsGlobal->ActiveBuffer & 1] / 8192,
               gsGlobal->Width / 64, gsGlobal->PSM, 0, 0 );

         gsGlobal->ActiveBuffer ^= 1;
      }

   }

   gsKit_setactive(gsGlobal);
}

/* PRIVATE METHODS */
static int vsync_handler()
{
   iSignalSema(vsync_sema_id);

   ExitHandler();
   return 0;
}

static void refreshScreen(void) {
    gsKit_sync(gsGlobal);
    gsKit_flip(gsGlobal);
    gsKit_queue_exec(gsGlobal);
    gsKit_TexManager_nextFrame(gsGlobal);
}

static void prim_texture(GSGLOBAL *gsGlobal, GSTEXTURE *texture, int zPosition, float aspect_ratio, bool scale_integer)
{
   float x1, y1, x2, y2;
   float visible_width  =  texture->Width;
   float visible_height =  texture->Height;

   if (scale_integer)
   {
      float width_proportion  = (float)gsGlobal->Width / (float)visible_width;
      float height_proportion = (float)gsGlobal->Height / (float)visible_height;
      int delta               = width_proportion < height_proportion ? width_proportion : height_proportion;
      float newWidth          = visible_width * delta;
      float newHeight         = visible_height * delta;

      x1 = (gsGlobal->Width - newWidth) / 2.0f;
      y1 = (gsGlobal->Height - newHeight) / 2.0f;
      x2 = newWidth + x1;
      y2 = newHeight + y1;
   }
   else if (aspect_ratio > 0)
   {
      float gs_aspect_ratio = (float)gsGlobal->Width / (float)gsGlobal->Height;
      float newWidth = (gs_aspect_ratio > aspect_ratio) ? gsGlobal->Height * aspect_ratio : gsGlobal->Width;
      float newHeight = (gs_aspect_ratio > aspect_ratio) ? gsGlobal->Height : gsGlobal->Width / aspect_ratio;

      x1 = (gsGlobal->Width - newWidth) / 2.0f;
      y1 = (gsGlobal->Height - newHeight) / 2.0f;
      x2 = newWidth + x1;
      y2 = newHeight + y1;
   }
   else
   {
      x1 = 0.0f;
      y1 = 0.0f;
      x2 = gsGlobal->Width;
      y2 = gsGlobal->Height;
   }

   gsKit_prim_sprite_texture( gsGlobal, texture,
         x1,            /* X1 */
         y1,            /* Y1 */
         0,  /* U1 */
         0,   /* V1 */
         x2,            /* X2 */
         y2,            /* Y2 */
         texture->Width,   /* U2 */
         texture->Height, /* V2 */
         zPosition,
         GS_TEXT);
}

static uint32_t ps2_frameCounter = 0;
static uint64_t lastTime = 0;

#endif

static int ps2_init(const char *gameTitle)
{
#if RETRO_PLATFORM == RETRO_PS2
    ee_sema_t sema;
    sema.init_count = 0;
    sema.max_count = 1;
    sema.option = 0;
    vsync_sema_id = CreateSema(&sema);

    gsGlobal = gsKit_init_global();

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);

    dmaKit_chan_init(DMA_CHANNEL_GIF);

    const struct VidMode *vid_mode = &vid_modes[0]; // NTCS

    gsGlobal->Mode = vid_mode->mode;
    gsGlobal->Width = vid_mode->width;
    gsGlobal->Height = vid_mode->height;
    gsGlobal->Interlace = vid_mode->interlace;
    gsGlobal->Field = vid_mode->field;
    gsGlobal->ZBuffering = GS_SETTING_ON;
    gsGlobal->DoubleBuffering = GS_SETTING_ON;
    gsGlobal->PrimAAEnable = GS_SETTING_OFF;
    gsGlobal->PSM = GS_PSM_CT16; // RGB565 color buffer
    gsGlobal->PSMZ = GS_PSMZ_16; // 16-bit unsigned zbuffer

     // Center the buffer in the coordinate space
    gsGlobal->OffsetX = ((4096 - gsGlobal->Width) / 2) * 16;
    gsGlobal->OffsetY = ((4096 - gsGlobal->Height) / 2) * 16;

    gsKit_init_screen(gsGlobal);
    gsKit_TexManager_init(gsGlobal);
    gsKit_set_display_offset(gsGlobal, 10 * vid_mode->vck, 6);
    gsKit_add_vsync_handler(vsync_handler);


    mainTexture  = (GSTEXTURE*)calloc(1, sizeof(GSTEXTURE));
    mainTexture->Width  = SCREEN_XSIZE;
    mainTexture->Height = SCREEN_YSIZE;
    mainTexture->PSM    = GS_PSM_CT16;
    mainTexture->Filter = GS_FILTER_LINEAR;
#endif
    return 1;
}

static void ps2_flipScreen() {
#if RETRO_PLATFORM == RETRO_PS2
    int rate = 240;
    struct tms begintime;
    uint64_t now, diff;
    if ((ps2_frameCounter % rate) == 0) {
        now = times(&begintime);
        diff = now - lastTime;
        printf("FPS: %.2f\n", 1000.0f/((float)diff/(float)rate));
        lastTime = now;
    }
    // if ((ps2_frameCounter % rate) > (rate/2)) {
    //     gsKit_clear(gsGlobal, GS_BLACK);
    // } else {
    //     gsKit_clear(gsGlobal, GS_TEXT);
    // }
    // printf("Frame counter %i\n", ps2_frameCounter);
    ps2_frameCounter++;

    mainTexture->Mem = (u32 *)Engine.frameBuffer;
    gsKit_TexManager_invalidate(gsGlobal, mainTexture);
    gsKit_TexManager_bind(gsGlobal, mainTexture);
    prim_texture(gsGlobal, mainTexture, 1, 0, 0);

    refreshScreen();
#endif
}

static void ps2_release() {
#if RETRO_PLATFORM == RETRO_PS2
    deinit_GSGlobal(gsGlobal);
#endif
}

drawing_driver_t drawing_PS2 = {
    ps2_init,
    ps2_flipScreen,
    ps2_release,
};