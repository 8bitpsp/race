
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pspgu.h>
#include <psptypes.h>
#include <psprtc.h>

#include "emulate.h"

#include "pl_psp.h"
#include "pl_snd.h"
#include "image.h"
#include "video.h"
#include "pl_perf.h"
#include "pl_file.h"
#include "ctrl.h"
#include "pl_util.h"

#include "StdAfx.h"
#include "neopopsound.h"
#include "input.h"
#include "flash.h"
#include "tlcs900h.h"
#include "memory.h"

psp_ctrl_mask_to_index_map_t physical_to_emulated_button_map[] =
{
  { PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER, 16 },
  { PSP_CTRL_START    | PSP_CTRL_SELECT,   17 },
  { PSP_CTRL_SELECT   | PSP_CTRL_LTRIGGER, 18 },
  { PSP_CTRL_SELECT   | PSP_CTRL_RTRIGGER, 19 },
  { PSP_CTRL_ANALUP,   0 }, { PSP_CTRL_ANALDOWN,  1 },
  { PSP_CTRL_ANALLEFT, 2 }, { PSP_CTRL_ANALRIGHT, 3 },
  { PSP_CTRL_UP,   4 }, { PSP_CTRL_DOWN,  5 },
  { PSP_CTRL_LEFT, 6 }, { PSP_CTRL_RIGHT, 7 },
  { PSP_CTRL_SQUARE, 8 },  { PSP_CTRL_CROSS,     9 },
  { PSP_CTRL_CIRCLE, 10 }, { PSP_CTRL_TRIANGLE, 11 },
  { PSP_CTRL_LTRIGGER, 12 }, { PSP_CTRL_RTRIGGER, 13 },
  { PSP_CTRL_SELECT, 14 }, { PSP_CTRL_START, 15 },
  { 0, -1 }
};

PspImage *Screen;
extern int m_bIsActive;
extern psp_ctrl_map_t current_map;
extern pl_file_path ScreenshotPath;

static pl_perf_counter FpsCounter;
static int ScreenX, ScreenY, ScreenW, ScreenH;
static int ClearScreen;

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
  sceGuDisable(GU_BLEND);
  pspVideoPutImage(Screen,
                   ScreenX, ScreenY,
                   ScreenW, ScreenH);
  sceGuEnable(GU_BLEND);

  /* Show FPS counter */
  if (psp_options.show_fps)
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
  switch (psp_options.display_mode)
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

  /* Resume sound */
  pl_snd_resume(0);

  /* Resume emulation */
  m_bIsActive = 1;
  ngpc_run();

  /* Pause sound */
  pl_snd_pause(0);
}

void UpdateInputState()
{
  ngpInputState = 0;

  /* Parse input */
  static int autofire_status = 0;
  static SceCtrlData pad;

  if (pspCtrlPollControls(&pad))
  {
#ifdef PSP_DEBUG
    if ((pad.Buttons & (PSP_CTRL_SELECT | PSP_CTRL_START))
      == (PSP_CTRL_SELECT | PSP_CTRL_START))
        pl_util_save_vram_seq(ScreenshotPath, "game");
#endif

    if (--autofire_status < 0)
      autofire_status = psp_options.autofire;
    psp_ctrl_mask_to_index_map_t *current_mapping = physical_to_emulated_button_map;

    for (; current_mapping->mask; current_mapping++)
    {
      u32 code = current_map.button_map[current_mapping->index];
      u8  on = (pad.Buttons & current_mapping->mask) == current_mapping->mask;

      /* Check to see if a button set is pressed. If so, unset it, so it */
      /* doesn't trigger any other combination presses. */
      if (on) pad.Buttons &= ~current_mapping->mask;

      if (code & AFI)
      {
        if (on && (autofire_status == 0)) 
          ngpInputState |= CODE_MASK(code);
        continue;
      }
      else if (code & JST)
      {
        if (on) ngpInputState |= CODE_MASK(code);
        continue;
      }
      else if (code & SPC)
      {
        switch (CODE_MASK(code))
        {
        case SPC_MENU:
          if (on) m_bIsActive = 0;
          break;
        }
      }
    }
  }
}

static void AudioCallback(pl_snd_sample* buf,
                          unsigned int samples,
                          void *userdata)
{
  int length_bytes = samples << 1; /* 2 bytes per sample */
  sound_update((_u16*)buf, length_bytes); //Get sound data
  dac_update((_u16*)buf, length_bytes);
}

/* Release emulation resources */
void TrashEmulation()
{
  flashShutdown();

  pspImageDestroy(Screen);
}
