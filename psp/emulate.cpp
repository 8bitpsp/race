#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pspgu.h>
#include <psptypes.h>
#include <psprtc.h>

#include "emulate.h"

#include "StdAfx.h"
#include "neopopsound.h"
#include "input.h"
#include "flash.h"
#include "tlcs900h.h"

#include "pl_snd.h"
#include "image.h"
#include "video.h"
#include "pl_perf.h"
#include "ctrl.h"
#include "pl_util.h"

PspImage *Screen;

static pl_perf_counter FpsCounter;
static int ScreenX, ScreenY, ScreenW, ScreenH;
static int ClearScreen;

int handleInputFile(char *romName);
static void AudioCallback(pl_snd_sample* buf,
                          unsigned int samples,
                          void *userdata);

/* Initialize emulation */
int InitEmulation()
{
  if (!(Screen = pspImageCreateVram(256, 152, PSP_IMAGE_16BPP)))
    return 0;

  Screen->Viewport.Width = 160;
  Screen->Viewport.Height = 152;

  sound_system_init();
  system_sound_chipreset();
  InitInput(NULL);

  pl_snd_set_callback(0, AudioCallback, NULL);

  return 1;
}

void graphics_paint()
{
  pspVideoBegin();

  /* Clear the buffer first, if necessary */
  if (ClearScreen >= 0)
  {
    ClearScreen--;
    pspVideoClearScreen();
  }

  /* Blit screen */
  pspVideoPutImage(Screen, 
                   0, 0,
                   Screen->Viewport.Width, 
                   Screen->Viewport.Height);

  /* Show FPS counter */
  if (1) // Options.ShowFps)
  {
    static char fps_display[64];
    sprintf(fps_display, " %3.02f ", pl_perf_update_counter(&FpsCounter));

    int width = pspFontGetTextWidth(&PspStockFont, fps_display);
    int height = pspFontGetLineHeight(&PspStockFont);

    pspVideoFillRect(SCR_WIDTH - width, 0, SCR_WIDTH, height, PSP_COLOR_BLACK);
    pspVideoPrint(&PspStockFont, SCR_WIDTH - width, 0, fps_display, PSP_COLOR_WHITE);
  }

  pspVideoEnd();

  pspVideoSwapBuffers();
}

/* Run emulation */
void RunEmulation()
{
  float ratio;

  /* Recompute screen size/position */
  switch (DISPLAY_MODE_FIT_HEIGHT) //Options.DisplayMode)
  {
  default:
  case DISPLAY_MODE_UNSCALED:
    ScreenW = Screen->Viewport.Width;
    ScreenH = Screen->Viewport.Height;
    break;
  case DISPLAY_MODE_FIT_HEIGHT:
    ratio = (float)SCR_HEIGHT / (float)Screen->Viewport.Height;
    ScreenW = (int)((float)Screen->Viewport.Width * ratio);
    ScreenH = SCR_HEIGHT;
    break;
  case DISPLAY_MODE_FILL_SCREEN:
    ScreenW = SCR_WIDTH;
    ScreenH = SCR_HEIGHT;
    break;
  }

  ScreenX = (SCR_WIDTH / 2) - (ScreenW / 2);
  ScreenY = (SCR_HEIGHT / 2) - (ScreenH / 2);

  /* Initialize performance counter */
  pl_perf_init_counter(&FpsCounter);
  ClearScreen = 1;

  /* TODO */
  char *romName = "sonic.rom";
  handleInputFile(romName);

  /* Initiate sound */
  pl_snd_resume(0);

  /* TODO EMULATE HERE */
  ngpc_run();

  /* Stop sound */
  pl_snd_pause(0);
}

static void AudioCallback(pl_snd_sample* buf,
                          unsigned int samples,
                          void *userdata)
{
  int length_bytes = samples << 1; /* 2 bytes per sample */
  sound_update((_u16*)buf, length_bytes); //Get sound data
  dac_update((_u8*)buf, length_bytes);
}

/* Release emulation resources */
void TrashEmulation()
{
  flashShutdown();

  pspImageDestroy(Screen);
}
