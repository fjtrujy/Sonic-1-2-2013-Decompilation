#include "../../RetroEngine.hpp"

#if RETRO_PLATFORM == RETRO_PS2

#include <kernel.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sbv_patches.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <loadfile.h>
#include <libmtap.h>
#include <libpad.h>
#include <audsrv.h>

extern unsigned char sio2man_irx;
extern unsigned int size_sio2man_irx;

extern unsigned char iomanX_irx;
extern unsigned int size_iomanX_irx;

extern unsigned char fileXio_irx;
extern unsigned int size_fileXio_irx;

extern unsigned char usbd_irx;
extern unsigned int size_usbd_irx;

extern unsigned char bdm_irx;
extern unsigned int size_bdm_irx;

extern unsigned char bdmfs_vfat_irx;
extern unsigned int size_bdmfs_vfat_irx;

extern unsigned char usbmass_bd_irx;
extern unsigned int size_usbmass_bd_irx;

extern unsigned char mtapman_irx;
extern unsigned int size_mtapman_irx;

extern unsigned char padman_irx;
extern unsigned int size_padman_irx;

extern unsigned char libsd_irx;
extern unsigned int size_libsd_irx;

extern unsigned char audsrv_irx;
extern unsigned int size_audsrv_irx;

static void load_modules() 
{
   /* I/O Files */
   SifExecModuleBuffer(&iomanX_irx, size_iomanX_irx, 0, NULL, NULL);
   SifExecModuleBuffer(&fileXio_irx, size_fileXio_irx, 0, NULL, NULL);
   SifExecModuleBuffer(&sio2man_irx, size_sio2man_irx, 0, NULL, NULL);

   /* USB */
   SifExecModuleBuffer(&usbd_irx, size_usbd_irx, 0, NULL, NULL);
   SifExecModuleBuffer(&bdm_irx, size_bdm_irx, 0, NULL, NULL);
   SifExecModuleBuffer(&bdmfs_vfat_irx, size_bdmfs_vfat_irx, 0, NULL, NULL);
   SifExecModuleBuffer(&usbmass_bd_irx, size_usbmass_bd_irx, 0, NULL, NULL);

    /* Controllers */
   SifExecModuleBuffer(&mtapman_irx, size_mtapman_irx, 0, NULL, NULL);
   SifExecModuleBuffer(&padman_irx, size_padman_irx, 0, NULL, NULL);

   /* Audio */
   SifExecModuleBuffer(&libsd_irx, size_libsd_irx, 0, NULL, NULL);
   SifExecModuleBuffer(&audsrv_irx, size_audsrv_irx, 0, NULL, NULL);

   if (mtapInit() != 1)
   {
      printLog("mtapInit library not initalizated\n");
   }
   if (padInit(0) != 1)
   {
      printLog("padInit library not initalizated\n");
   }

   /* Initializes audsrv library */
   if (audsrv_init())
   {
      printLog("audsrv library not initalizated\n");
   }
}

static void reset_IOP()
{
   SifInitRpc(0);
#if !defined(DEBUG) || defined(BUILD_FOR_PCSX2)
   /* Comment this line if you don't wanna debug the output */
   while(!SifIopReset(NULL, 0)){};
#endif

   while(!SifIopSync()){};
   SifInitRpc(0);
   sbv_patch_enable_lmb();
   sbv_patch_disable_prefix_check();
}

bool waitUntilDeviceIsReady(char *path)
{
   struct stat buffer;
   int ret = -1;
   int retries = 50;

   while(ret != 0 && retries > 0)
   {
      ret = stat(path, &buffer);
      /* Wait untill the device is ready */
      nopdelay();

      retries--;
   }

   return ret == 0;
}


#endif

static void ps2_init() {
#if RETRO_PLATFORM == RETRO_PS2
    char cwd[FILENAME_MAX];
    reset_IOP();
    load_modules();

    getcwd(cwd, sizeof(cwd));
    waitUntilDeviceIsReady(cwd);
#endif
}
static bool ps2_run() { return true; }
static bool ps2_processEvent() { return true; }
static void ps2_quit() {}

engine_driver_t engine_PS2 = {
    ps2_init,
    ps2_run,
    ps2_processEvent,
    ps2_quit,
};