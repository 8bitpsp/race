#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <psptypes.h>
#include <pspkernel.h>
#include <pspgu.h>

#include "StdAfx.h"
#include "main.h"
#include "input.h"
#include "neopopsound.h"
#include "flash.h"

#include "menu.h"
#include "emulate.h"

#include "pl_psp.h"
#include "pl_snd.h"
#include "image.h"
#include "video.h"
#include "pl_perf.h"
#include "ctrl.h"
#include "pl_util.h"
#include "pl_file.h"
#include "pl_ini.h"
#include "ui.h"
#include "state.h"

#undef u32

extern PspImage *Screen;

pl_file_path CurrentGame = "",
             GamePath,
             SaveStatePath,
             ScreenshotPath;
static PspImage *Background;
static PspImage *NoSaveIcon;
static int TabIndex;
static int ResumeEmulation;

psp_ctrl_map_t current_map;
psp_options_t psp_options;

/* Default configuration */
static psp_ctrl_map_t default_map =
{
  {
    JST|0x01, /* Analog Up    */
    JST|0x02, /* Analog Down  */
    JST|0x04, /* Analog Left  */
    JST|0x08, /* Analog Right */
    JST|0x01, /* D-pad Up     */
    JST|0x02, /* D-pad Down   */
    JST|0x04, /* D-pad Left   */
    JST|0x08, /* D-pad Right  */
    0,        /* Square       */
    JST|0x10, /* Cross        */
    JST|0x20, /* Circle       */
    0,        /* Triangle     */
    0,        /* L Trigger    */
    0,        /* R Trigger    */
    JST|0x40, /* Select       */
    0,        /* Start        */
    SPC|SPC_MENU, /* L+R Triggers */
    0,            /* Start+Select */
    0,            /* Select + L   */
    0,            /* Select + R   */
  }
};

/* Tab labels */
static const char *TabLabel[] = 
{
  "Game",
  "Save/Load",
  "Controls",
  "Options",
  "System",
  "About"
};

static const char 
  PresentSlotText[] = "\026\244\020 Save\t\026\001\020 Load\t\026\243\020 Delete",
  EmptySlotText[]   = "\026\244\020 Save",
  ControlHelpText[] = "\026\250\020 Change mapping\t\026\001\020 Save to \271";

#define TAB_QUICKLOAD 0
#define TAB_STATE     1
#define TAB_CONTROLS  2
#define TAB_OPTIONS   3
#define TAB_SYSTEM    4
#define TAB_MAX       TAB_SYSTEM
#define TAB_ABOUT     (TAB_MAX+1)

#define OPTION_DISPLAY_MODE 0x01
#define OPTION_FRAME_SKIP   0x02
#define OPTION_CLOCK_FREQ   0x03
#define OPTION_SHOW_FPS     0x04
#define OPTION_CONTROL_MODE 0x06
#define OPTION_ANIMATE      0x07

#define SYSTEM_RESET        0x10
#define SYSTEM_SCRNSHOT     0x11

#define SET_AS_CURRENT_GAME(filename) \
  strncpy(CurrentGame, filename, sizeof(CurrentGame) - 1)
#define CURRENT_GAME (CurrentGame)
#define GAME_LOADED (CurrentGame[0] != '\0')

static void psp_init_controls();
static int  psp_load_controls();
static int  psp_save_controls();
static void psp_load_options();
static int  psp_save_options();

static void psp_discard_alpha(PspImage *image);

static void psp_display_control_tab();
static void psp_display_state_tab();

static PspImage* psp_load_state_icon(const char *path);
static int psp_load_state(const char *path);
static PspImage* psp_save_state(const char *path, PspImage *icon);

static const char *QuickloadFilter[] = { "ZIP", "NGP", '\0' };

static int OnGenericCancel(const void *uiobject, 
                           const void *param);
static void OnGenericRender(const void *uiobject,
                            const void *item_obj);
static int OnGenericButtonPress(const PspUiFileBrowser *browser,
                                const char *path,
                                u32 button_mask);

static int OnSplashButtonPress(const struct PspUiSplash *splash,
                               u32 button_mask);
static void OnSplashRender(const void *uiobject,
                           const void *null);
static const char* OnSplashGetStatusBarText(const struct PspUiSplash *splash);

static int OnMenuOk(const void *menu,
                    const void *item);
static int OnMenuButtonPress(const struct PspUiMenu *uimenu,
                             pl_menu_item* item,
                             u32 button_mask);
static int OnMenuItemChanged(const struct PspUiMenu *uimenu,
                             pl_menu_item* item,
                             const pl_menu_option* option);

static int OnSaveStateOk(const void *gallery, const void *item);
static int OnSaveStateButtonPress(const PspUiGallery *gallery,
                                  pl_menu_item *sel,
                                  u32 button_mask);

static int OnQuickloadOk(const void *browser, const void *path);

static void OnSystemRender(const void *uiobject,
                           const void *item_obj);

PspUiFileBrowser QuickloadBrowser = 
{
  OnGenericRender,
  OnQuickloadOk,
  OnGenericCancel,
  OnGenericButtonPress,
  QuickloadFilter,
  0
};

PspUiMenu
  OptionUiMenu =
  {
    OnGenericRender,       /* OnRender() */
    OnMenuOk,              /* OnOk() */
    OnGenericCancel,       /* OnCancel() */
    OnMenuButtonPress,     /* OnButtonPress() */
    OnMenuItemChanged,     /* OnItemChanged() */
  },
  SystemUiMenu =
  {
    OnSystemRender,        /* OnRender() */
    OnMenuOk,              /* OnOk() */
    OnGenericCancel,       /* OnCancel() */
    OnMenuButtonPress,     /* OnButtonPress() */
    OnMenuItemChanged,     /* OnItemChanged() */
  },
  ControlUiMenu =
  {
    OnGenericRender,       /* OnRender() */
    OnMenuOk,              /* OnOk() */
    OnGenericCancel,       /* OnCancel() */
    OnMenuButtonPress,     /* OnButtonPress() */
    OnMenuItemChanged,     /* OnItemChanged() */
  };

PspUiGallery SaveStateGallery = 
{
  OnGenericRender,             /* OnRender() */
  OnSaveStateOk,               /* OnOk() */
  OnGenericCancel,             /* OnCancel() */
  OnSaveStateButtonPress,      /* OnButtonPress() */
  NULL                         /* Userdata */
};

/* Menu options */
PL_MENU_OPTIONS_BEGIN(MappableButtons)
  /* Unmapped */
  PL_MENU_OPTION("None", 0)
  /* Special */
  PL_MENU_OPTION("Special: Open Menu", (SPC|SPC_MENU))
  /* Directions */
  PL_MENU_OPTION("Up",    (JST|0x01))
  PL_MENU_OPTION("Down",  (JST|0x02))
  PL_MENU_OPTION("Left",  (JST|0x04))
  PL_MENU_OPTION("Right", (JST|0x08))
  /* Buttons */
  PL_MENU_OPTION("A",      (JST|0x10))
  PL_MENU_OPTION("B",      (JST|0x20))
  PL_MENU_OPTION("Option", (JST|0x40))
  PL_MENU_OPTION("Test switch", (JST|0x80))
PL_MENU_OPTIONS_END
PL_MENU_OPTIONS_BEGIN(ToggleOptions)
  PL_MENU_OPTION("Disabled", 0)
  PL_MENU_OPTION("Enabled", 1)
PL_MENU_OPTIONS_END
PL_MENU_OPTIONS_BEGIN(ScreenSizeOptions)
  PL_MENU_OPTION("Actual size", DISPLAY_MODE_UNSCALED)
  PL_MENU_OPTION("4:3 scaled (fit height)", DISPLAY_MODE_FIT_HEIGHT)
  PL_MENU_OPTION("16:9 scaled (fit screen)", DISPLAY_MODE_FILL_SCREEN)
PL_MENU_OPTIONS_END
PL_MENU_OPTIONS_BEGIN(FrameskipOptions)
  PL_MENU_OPTION("No skipping", 0)
  PL_MENU_OPTION("1", 1)
  PL_MENU_OPTION("2", 2)
  PL_MENU_OPTION("3", 3)
PL_MENU_OPTIONS_END
PL_MENU_OPTIONS_BEGIN(PspClockFreqOptions)
  PL_MENU_OPTION("222 MHz", 222)
  PL_MENU_OPTION("266 MHz", 266)
  PL_MENU_OPTION("300 MHz", 300)
  PL_MENU_OPTION("333 MHz", 333)
PL_MENU_OPTIONS_END
PL_MENU_OPTIONS_BEGIN(ControlModeOptions)
  PL_MENU_OPTION("\026\242\020 cancels, \026\241\020 confirms (US)", 0)
  PL_MENU_OPTION("\026\241\020 cancels, \026\242\020 confirms (Japan)", 1)
PL_MENU_OPTIONS_END

/* Menu items */
PL_MENU_ITEMS_BEGIN(ControlMenuDef)
  PL_MENU_ITEM(PSP_CHAR_ANALUP,0,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_ANALDOWN,1,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_ANALLEFT,2,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_ANALRIGHT,3,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_UP,4,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_DOWN,5,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_LEFT,6,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_RIGHT,7,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_SQUARE,8,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_CROSS,9,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_CIRCLE,10,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_TRIANGLE,11,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_LTRIGGER,12,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_RTRIGGER,13,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_SELECT,14,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_START,15,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_LTRIGGER"+"PSP_CHAR_RTRIGGER,16,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_START"+"PSP_CHAR_SELECT,17,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_SELECT"+"PSP_CHAR_LTRIGGER,18,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_SELECT"+"PSP_CHAR_RTRIGGER,19,MappableButtons,ControlHelpText)
PL_MENU_ITEMS_END
PL_MENU_ITEMS_BEGIN(OptionMenuDef)
  PL_MENU_HEADER("Video")
  PL_MENU_ITEM("Screen size",OPTION_DISPLAY_MODE,ScreenSizeOptions,"\026\250\020 Change screen size")
  PL_MENU_HEADER("Performance")
  PL_MENU_ITEM("Frame skipping", OPTION_FRAME_SKIP,FrameskipOptions,"\026\250\020 Select number of frames to skip per update")
  PL_MENU_ITEM("PSP clock frequency",OPTION_CLOCK_FREQ,PspClockFreqOptions,"\026\250\020 Larger values: faster emulation, faster battery depletion (default: 222MHz)")
  PL_MENU_ITEM("Show FPS counter",OPTION_SHOW_FPS,ToggleOptions,"\026\250\020 Show/hide the frames-per-second counter")
  PL_MENU_HEADER("Menu")
  PL_MENU_ITEM("Button mode",OPTION_CONTROL_MODE,ControlModeOptions,"\026\250\020 Change OK and Cancel button mapping")
  PL_MENU_ITEM("Animations",OPTION_ANIMATE,ToggleOptions,"\026\250\020 Enable/disable menu animations")
PL_MENU_ITEMS_END
PL_MENU_ITEMS_BEGIN(SystemMenuDef)
  PL_MENU_HEADER("Options")
  PL_MENU_ITEM("Reset",SYSTEM_RESET,NULL,"\026\001\020 Reset system")
  PL_MENU_ITEM("Save screenshot",SYSTEM_SCRNSHOT,NULL,"\026\001\020 Save screenshot")
PL_MENU_ITEMS_END

PspUiSplash SplashScreen = 
{
  OnSplashRender,
  OnGenericCancel,
  OnSplashButtonPress,
  OnSplashGetStatusBarText
};

int InitMenu()
{
  if (!InitEmulation())
    return 0;

  /* Initialize paths */
  sprintf(SaveStatePath, "%sstates/", pl_psp_get_app_directory());
  sprintf(ScreenshotPath, "%sscreens/", pl_psp_get_app_directory());
  sprintf(GamePath, "%s", pl_psp_get_app_directory());

  /* Load the background image */
  Background = pspImageLoadPng("background.png");

  /* Initialize menus */
  pl_menu_create(&SystemUiMenu.Menu, SystemMenuDef);
  pl_menu_create(&OptionUiMenu.Menu, OptionMenuDef);
  pl_menu_create(&ControlUiMenu.Menu, ControlMenuDef);

  /* Init NoSaveState icon image */
  NoSaveIcon = pspImageCreate(160, 152, PSP_IMAGE_16BPP);
  pspImageClear(NoSaveIcon, RGB(0x44,0x00,0x00));

  /* Initialize state menu */
  int i;
  pl_menu_item *item;
  for (i = 0; i < 10; i++)
  {
    item = pl_menu_append_item(&SaveStateGallery.Menu, i, NULL);
    pl_menu_set_item_help_text(item, EmptySlotText);
  }

  /* Initialize options */
  psp_load_controls();
  psp_load_options();

  /* Initialize UI components */
  UiMetric.Background = Background;
  UiMetric.Font = &PspStockFont;
  UiMetric.Left = 8;
  UiMetric.Top = 24;
  UiMetric.Right = 472;
  UiMetric.Bottom = 250;
  UiMetric.ScrollbarColor = PSP_COLOR_GRAY;
  UiMetric.ScrollbarBgColor = 0x44ffffff;
  UiMetric.ScrollbarWidth = 10;
  UiMetric.TextColor = PSP_COLOR_GRAY;
  UiMetric.SelectedColor = COLOR(0xf7,0xc2,0x50,0xFF);
  UiMetric.SelectedBgColor = COLOR(0xd5,0xf1,0x17,0x99);
  UiMetric.StatusBarColor = PSP_COLOR_WHITE;
  UiMetric.BrowserFileColor = PSP_COLOR_GRAY;
  UiMetric.BrowserDirectoryColor = PSP_COLOR_YELLOW;
  UiMetric.GalleryIconsPerRow = 5;
  UiMetric.GalleryIconMarginWidth = 16;
  UiMetric.MenuItemMargin = 20;
  UiMetric.MenuSelOptionBg = PSP_COLOR_GRAY;
  UiMetric.MenuOptionBoxColor = PSP_COLOR_GRAY;
  UiMetric.MenuOptionBoxBg = COLOR(0x44,0x00,0x00,0xbb);
  UiMetric.MenuDecorColor = UiMetric.SelectedColor;
  UiMetric.DialogFogColor = COLOR(0xd5,0xf1,0x17,0xbb);
  UiMetric.TitlePadding = 4;
  UiMetric.TitleColor = PSP_COLOR_WHITE;
  UiMetric.MenuFps = 30;
  UiMetric.TabBgColor = COLOR(0xcc,0x73,0x73,0xff);

  TabIndex = TAB_ABOUT;

  return 1;
}

void DisplayMenu()
{
  pl_menu_item *item;

  /* Menu loop */
  do
  {
    ResumeEmulation = 0;

    /* Set normal clock frequency */
    pl_psp_set_clock_freq(222);
    /* Set buttons to autorepeat */
    pspCtrlSetPollingMode(PSP_CTRL_AUTOREPEAT);

    do
    {
      /* Display appropriate tab */
      switch (TabIndex)
      {
      case TAB_QUICKLOAD:
        pspUiOpenBrowser(&QuickloadBrowser,
                        (GAME_LOADED) ? CURRENT_GAME : GamePath);
        break;
      case TAB_CONTROLS:
        psp_display_control_tab();
        break;
      case TAB_OPTIONS:
        item = pl_menu_find_item_by_id(&OptionUiMenu.Menu, OPTION_DISPLAY_MODE);
        pl_menu_select_option_by_value(item, (void*)(int)psp_options.display_mode);
        item = pl_menu_find_item_by_id(&OptionUiMenu.Menu, OPTION_CLOCK_FREQ);
        pl_menu_select_option_by_value(item, (void*)(int)psp_options.clock_freq);
        item = pl_menu_find_item_by_id(&OptionUiMenu.Menu, OPTION_SHOW_FPS);
        pl_menu_select_option_by_value(item, (void*)(int)psp_options.show_fps);
        item = pl_menu_find_item_by_id(&OptionUiMenu.Menu, OPTION_CONTROL_MODE);
        pl_menu_select_option_by_value(item, (void*)(UiMetric.OkButton == PSP_CTRL_CIRCLE));
        item = pl_menu_find_item_by_id(&OptionUiMenu.Menu, OPTION_ANIMATE);
        pl_menu_select_option_by_value(item, (void*)(int)UiMetric.Animate);
        item = pl_menu_find_item_by_id(&OptionUiMenu.Menu, OPTION_FRAME_SKIP);
        pl_menu_select_option_by_value(item, (void*)(int)psp_options.frame_skip);

        pspUiOpenMenu(&OptionUiMenu, NULL);
        break;
      case TAB_STATE:
        psp_display_state_tab();
        break;
      case TAB_SYSTEM:
        pspUiOpenMenu(&SystemUiMenu, NULL);
        break;
      case TAB_ABOUT:
        pspUiSplashScreen(&SplashScreen);
        break;
      }
    } while (!ExitPSP && !ResumeEmulation);

    if (!ExitPSP)
    {
      /* Set clock frequency during emulation */
      pl_psp_set_clock_freq(psp_options.clock_freq);
      /* Set buttons to normal mode */
      pspCtrlSetPollingMode(PSP_CTRL_NORMAL);

      if (ResumeEmulation)
      {
        /* Resume emulation */
        if (UiMetric.Animate) pspUiFadeout();
        RunEmulation();
        if (UiMetric.Animate) pspUiFadeout();
      }
    }
  } while (!ExitPSP);
}

void TrashMenu()
{
  TrashEmulation();

  pl_menu_destroy(&SystemUiMenu.Menu);
  pl_menu_destroy(&OptionUiMenu.Menu);
  pl_menu_destroy(&ControlUiMenu.Menu);
  pl_menu_destroy(&SaveStateGallery.Menu);

  pspImageDestroy(NoSaveIcon);
  pspImageDestroy(Background);

  psp_save_options();
}

/* Handles drawing of generic items */
static void OnGenericRender(const void *uiobject,
                            const void *item_obj)
{
  /* Draw tabs */
  int i, x, width, height = pspFontGetLineHeight(UiMetric.Font);
  for (i = 0, x = 5; i <= TAB_MAX; i++, x += width + 10)
  {
    width = -10;

    if (!GAME_LOADED && (i == TAB_STATE || i == TAB_SYSTEM))
      continue;

    /* Determine width of text */
    width = pspFontGetTextWidth(UiMetric.Font, TabLabel[i]);

    /* Draw background of active tab */
    if (i == TabIndex)
      pspVideoFillRect(x - 5, 0, x + width + 5, height + 1, UiMetric.TabBgColor);

    /* Draw name of tab */
    pspVideoPrint(UiMetric.Font, x, 0, TabLabel[i], PSP_COLOR_WHITE);
  }
}

static int OnGenericButtonPress(const PspUiFileBrowser *browser, 
                                const char *path,
                                u32 button_mask)
{
  int tab_index;

  /* If L or R are pressed, switch tabs */
  if (button_mask & PSP_CTRL_LTRIGGER)
  {
    TabIndex--;
    do
    {
      tab_index = TabIndex;
      if (!GAME_LOADED && (TabIndex == TAB_STATE || TabIndex == TAB_SYSTEM)) TabIndex--;
      if (TabIndex < 0) TabIndex = TAB_MAX;
    } while (tab_index != TabIndex);
  }
  else if (button_mask & PSP_CTRL_RTRIGGER)
  {
    TabIndex++;
    do
    {
      tab_index = TabIndex;
      if (!GAME_LOADED && (TabIndex == TAB_STATE || TabIndex == TAB_SYSTEM)) TabIndex++;
      if (TabIndex > TAB_MAX) TabIndex = 0;
    } while (tab_index != TabIndex);
  }
  else if ((button_mask & (PSP_CTRL_START | PSP_CTRL_SELECT)) 
    == (PSP_CTRL_START | PSP_CTRL_SELECT))
  {
    if (pl_util_save_vram_seq(ScreenshotPath, "ui"))
      pspUiAlert("Saved successfully");
    else
      pspUiAlert("ERROR: Not saved");
    return 0;
  }
  else return 0;

  return 1;
}

static int OnGenericCancel(const void *uiobject,
                           const void* param)
{
  if (!GAME_LOADED)
    return 0;

  ResumeEmulation = 1;
  return 1;
}

static int OnQuickloadOk(const void *browser,
                         const void *path)
{
  pspUiFlashMessage("Loading, please wait...");

  if (!handleInputFile((char*)path))
  {
    pspUiAlert("Error loading cartridge");
    return 0;
  }

  SET_AS_CURRENT_GAME((char*)path);
  pl_file_get_parent_directory((const char*)path,
                               GamePath,
                               sizeof(GamePath));
  ResumeEmulation = 1;

  system_sound_chipreset(); /* Reset sound */

  return 1;
}

static int OnSaveStateOk(const void *gallery, const void *item)
{
  char *path;
  const char *config_name = pl_file_get_filename(CURRENT_GAME);

  path = (char*)malloc(strlen(SaveStatePath) + strlen(config_name) + 8);
  sprintf(path, "%s%s_%02i.rcs", SaveStatePath, config_name,
    ((const pl_menu_item*)item)->id);

  if (pl_file_exists(path) && pspUiConfirm("Load state?"))
  {
    if (psp_load_state(path))
    {
      ResumeEmulation = 1;
      pl_menu_find_item_by_id(&((PspUiGallery*)gallery)->Menu,
        ((pl_menu_item*)item)->id);
      free(path);

      return 1;
    }

    pspUiAlert("ERROR: State not loaded");
  }

  free(path);
  return 0;
}

static int OnSaveStateButtonPress(const PspUiGallery *gallery, 
                                  pl_menu_item *sel,
                                  u32 button_mask)
{
  if (button_mask & PSP_CTRL_SQUARE 
    || button_mask & PSP_CTRL_TRIANGLE)
  {
    char *path;
    char caption[32];
    const char *config_name = pl_file_get_filename(CURRENT_GAME);
    pl_menu_item *item = pl_menu_find_item_by_id(&gallery->Menu, sel->id);

    path = (char*)malloc(strlen(SaveStatePath) + strlen(config_name) + 8);
    sprintf(path, "%s%s_%02i.rcs", SaveStatePath, config_name, item->id);

    do /* not a real loop; flow control construct */
    {
      if (button_mask & PSP_CTRL_SQUARE)
      {
        if (pl_file_exists(path) && !pspUiConfirm("Overwrite existing state?"))
          break;

        pspUiFlashMessage("Saving, please wait ...");

        PspImage *icon;
        if (!(icon = psp_save_state(path, Screen)))
        {
          pspUiAlert("ERROR: State not saved");
          break;
        }

        SceIoStat stat;

        /* Trash the old icon (if any) */
        if (item->param && item->param != NoSaveIcon)
          pspImageDestroy((PspImage*)item->param);

        /* Update icon, help text */
        item->param = icon;
        pl_menu_set_item_help_text(item, PresentSlotText);

        /* Get file modification time/date */
        if (sceIoGetstat(path, &stat) < 0)
          sprintf(caption, "ERROR");
        else
          sprintf(caption, "%02i/%02i/%02i %02i:%02i", 
            stat.st_mtime.month,
            stat.st_mtime.day,
            stat.st_mtime.year - (stat.st_mtime.year / 100) * 100,
            stat.st_mtime.hour,
            stat.st_mtime.minute);

        pl_menu_set_item_caption(item, caption);
      }
      else if (button_mask & PSP_CTRL_TRIANGLE)
      {
        if (!pl_file_exists(path) || !pspUiConfirm("Delete state?"))
          break;

        if (!pl_file_rm(path))
        {
          pspUiAlert("ERROR: State not deleted");
          break;
        }

        /* Trash the old icon (if any) */
        if (item->param && item->param != NoSaveIcon)
          pspImageDestroy((PspImage*)item->param);

        /* Update icon, caption */
        item->param = NoSaveIcon;
        pl_menu_set_item_help_text(item, EmptySlotText);
        pl_menu_set_item_caption(item, "Empty");
      }
    } while (0);

    if (path) free(path);
    return 0;
  }

  return OnGenericButtonPress(NULL, NULL, button_mask);
}

static void OnSplashRender(const void *splash,
                           const void *null)
{
  int fh, i, x, y, height;
  const char *lines[] = 
  { 
    PSP_APP_NAME" version "PSP_APP_VER" ("__DATE__")",
    "\026http://psp.akop.org/race",
    " ",
    "2008 Akop Karapetyan (port)",
    "2008 Flavor (original PSP port, optimization)",
    "2006 Judge_ (emulation)",
    NULL
  };

  fh = pspFontGetLineHeight(UiMetric.Font);

  for (i = 0; lines[i]; i++);
  height = fh * (i - 1);

  /* Render lines */
  for (i = 0, y = SCR_HEIGHT / 2 - height / 2; lines[i]; i++, y += fh)
  {
    x = SCR_WIDTH / 2 - pspFontGetTextWidth(UiMetric.Font, lines[i]) / 2;
    pspVideoPrint(UiMetric.Font, x, y, lines[i], PSP_COLOR_GRAY);
  }

  /* Render PSP status */
  OnGenericRender(splash, null);
}

static int OnSplashButtonPress(const struct PspUiSplash *splash, 
                               u32 button_mask)
{
  return OnGenericButtonPress(NULL, NULL, button_mask);
}

static const char* OnSplashGetStatusBarText(const struct PspUiSplash *splash)
{
  return "\026\255\020/\026\256\020 Switch tabs";
}

static int OnMenuOk(const void *uimenu, const void* sel_item)
{
  if (uimenu == &ControlUiMenu)
  {
    /* Save to MS */
    if (psp_save_controls())
      pspUiAlert("Changes saved");
    else
      pspUiAlert("ERROR: Changes not saved");
  }
  else
  {
    switch (((const pl_menu_item*)sel_item)->id)
    {
    case SYSTEM_RESET:
      if (pspUiConfirm("Reset the system?"))
      {
        mainemuinit();
        ResumeEmulation = 1;
        return 1;
      }
      break;
    case SYSTEM_SCRNSHOT:
      /* Save screenshot */
      if (!pl_util_save_image_seq(ScreenshotPath,
                                  pl_file_get_filename(CURRENT_GAME),
                                  Screen))
        pspUiAlert("ERROR: Screenshot not saved");
      else
        pspUiAlert("Screenshot saved successfully");
      break;
    }
  }
  return 0;
}

static int OnMenuButtonPress(const struct PspUiMenu *uimenu,
                             pl_menu_item* sel_item,
                             u32 button_mask)
{
  if (uimenu == &ControlUiMenu)
  {
    if (button_mask & PSP_CTRL_TRIANGLE)
    {
      pl_menu_item *item;
      int i;

      /* Load default mapping */
      memcpy(&current_map, &default_map, sizeof(psp_ctrl_map_t));

      /* Modify the menu */
      for (item = ControlUiMenu.Menu.items, i = 0; item; item = item->next, i++)
        pl_menu_select_option_by_value(item, (void*)default_map.button_map[i]);

      return 0;
    }
  }

  return OnGenericButtonPress(NULL, NULL, button_mask);
}

static int OnMenuItemChanged(const struct PspUiMenu *uimenu,
                             pl_menu_item* item,
                             const pl_menu_option* option)
{
  if (uimenu == &ControlUiMenu)
  {
    current_map.button_map[item->id] = (unsigned int)option->value;
  }
  else
  {
    switch((int)item->id)
    {
    case OPTION_DISPLAY_MODE:
      psp_options.display_mode = (int)option->value;
      break;
    case OPTION_FRAME_SKIP:
      psp_options.frame_skip = (int)option->value;
      break;
    case OPTION_CLOCK_FREQ:
      psp_options.clock_freq = (int)option->value;
      break;
    case OPTION_SHOW_FPS:
      psp_options.show_fps = (int)option->value;
      break;
    case OPTION_CONTROL_MODE:
      UiMetric.OkButton = (!(int)option->value) 
                          ? PSP_CTRL_CROSS : PSP_CTRL_CIRCLE;
      UiMetric.CancelButton = (!(int)option->value) 
                              ? PSP_CTRL_CIRCLE : PSP_CTRL_CROSS;
      break;
    case OPTION_ANIMATE:
      UiMetric.Animate = (int)option->value;
      break;
    }
  }
  return 1;
}

static void OnSystemRender(const void *uiobject,
                           const void *item_obj)
{
  int w, h, x, y;
  w = Screen->Viewport.Width;
  h = Screen->Viewport.Height;
  x = UiMetric.Right - w - UiMetric.ScrollbarWidth;
  y = SCR_HEIGHT - h - 56;

  /* Draw a small representation of the screen */
  pspVideoShadowRect(x, y, x + w - 1, y + h - 1, PSP_COLOR_BLACK, 3);
  sceGuDisable(GU_BLEND);
  pspVideoPutImage(Screen, x, y, w, h);
  sceGuEnable(GU_BLEND);
  pspVideoDrawRect(x, y, x + w - 1, y + h - 1, PSP_COLOR_GRAY);

  OnGenericRender(uiobject, item_obj);
}

static void psp_init_controls()
{
  /* Initialize to default configuration */
  memcpy(&current_map, &default_map, sizeof(psp_ctrl_map_t));
}

static int psp_load_controls()
{
  psp_ctrl_map_t *config = &current_map;
  pl_file_path path;
  snprintf(path, sizeof(path) - 1, "%sbuttons.cnf",
           pl_psp_get_app_directory());

  /* If no configuration, load defaults */
  if (!pl_file_exists(path))
  {
    psp_init_controls();
    return 1;
  }

  /* Open file for reading */
  FILE *file = fopen(path, "r");
  if (!file) return 0;

  /* Read contents of struct */
  int nread = fread(config, sizeof(psp_ctrl_map_t), 1, file);
  fclose(file);

  if (nread != 1)
  {
    psp_init_controls();
    return 0;
  }

  return 1;
}

static int psp_save_controls()
{
  psp_ctrl_map_t *config = &current_map;
  pl_file_path path;
  snprintf(path, sizeof(path) - 1, "%sbuttons.cnf",
           pl_psp_get_app_directory());

  /* Open file for writing */
  FILE *file = fopen(path, "w");
  if (!file) return 0;

  /* Write contents of struct */
  int nwritten = fwrite(config, sizeof(psp_ctrl_map_t), 1, file);
  fclose(file);

  return (nwritten == 1);
}

static void psp_display_control_tab()
{
  pl_menu_item *item;
  int i;

  /* Load current button mappings */
  for (item = ControlUiMenu.Menu.items, i = 0; item; item = item->next, i++)
    pl_menu_select_option_by_value(item, (void*)current_map.button_map[i]);

  pspUiOpenMenu(&ControlUiMenu, NULL);
}

static void psp_display_state_tab()
{
  pl_menu_item *item, *sel = NULL;
  SceIoStat stat;
  ScePspDateTime latest;
  char caption[32];
  const char *config_name = pl_file_get_filename(CURRENT_GAME);
  char *path = (char*)malloc(strlen(SaveStatePath) + strlen(config_name) + 8);
  char *game_name = strdup(config_name);
  char *dot = strrchr(game_name, '.');
  if (dot) *dot='\0';

  memset(&latest,0,sizeof(latest));

  /* Initialize icons */
  for (item = SaveStateGallery.Menu.items; item; item = item->next)
  {
    sprintf(path, "%s%s_%02i.rcs", SaveStatePath, config_name, item->id);

    if (pl_file_exists(path))
    {
      if (sceIoGetstat(path, &stat) < 0)
        sprintf(caption, "ERROR");
      else
      {
        /* Determine the latest save state */
        if (pl_util_date_compare(&latest, &stat.st_mtime) < 0)
        {
          sel = item;
          latest = stat.st_mtime;
        }

        sprintf(caption, "%02i/%02i/%02i %02i:%02i", 
          stat.st_mtime.month,
          stat.st_mtime.day,
          stat.st_mtime.year - (stat.st_mtime.year / 100) * 100,
          stat.st_mtime.hour,
          stat.st_mtime.minute);
      }

      pl_menu_set_item_caption(item, caption);
      item->param = psp_load_state_icon(path);
      pl_menu_set_item_help_text(item, PresentSlotText);
    }
    else
    {
      pl_menu_set_item_caption(item, "Empty");
      item->param = NoSaveIcon;
      pl_menu_set_item_help_text(item, EmptySlotText);
    }
  }

  free(path);

  /* Highlight the latest save state if none are selected */
  if (SaveStateGallery.Menu.selected == NULL)
    SaveStateGallery.Menu.selected = sel;

  pspUiOpenGallery(&SaveStateGallery, game_name);
  free(game_name);

  /* Destroy any icons */
  for (item = SaveStateGallery.Menu.items; item; item = item->next)
    if (item->param != NULL && item->param != NoSaveIcon)
      pspImageDestroy((PspImage*)item->param);
}

/* Load state icon */
static PspImage* psp_load_state_icon(const char *path)
{
  FILE *f = fopen(path, "r");
  if (!f) return NULL;

  /* Load image */
  PspImage *image = pspImageLoadPngFd(f);
  fclose(f);

  return image;
}

/* Load state */
static int psp_load_state(const char *path)
{
  /* Open file for reading */
  FILE *f = fopen(path, "r");
  if (!f) return 0;

  /* Load image into temporary object */
  PspImage *image = pspImageLoadPngFd(f);
  pspImageDestroy(image);

  /* Load the state data */
  int status = state_restore(f);
  fclose(f);

  return status;
}

static void psp_discard_alpha(PspImage *image)
{
  int i, j;
  for (i = image->Viewport.Y; i < image->Viewport.Height; i++)
    for (j = image->Viewport.X; j < image->Viewport.Width; j++)
      ((u16*)image->Pixels)[(i * image->Width) + j] |= 0x8000;
}

/* Save state */
static PspImage* psp_save_state(const char *path, PspImage *icon)
{
  /* Open file for writing */
  FILE *f;
  if (!(f = fopen(path, "w")))
    return NULL;

  /* Create thumbnail */
  PspImage *thumb;
  thumb = pspImageCreateCopy(icon);

  if (!thumb) { fclose(f); return NULL; }

  psp_discard_alpha(thumb);

  /* Write the thumbnail */
  if (!pspImageSavePngFd(f, thumb))
  {
    pspImageDestroy(thumb);
    fclose(f);
    pl_file_rm(path);
    return NULL;
  }

  /* Write the state */
  if (!state_store(f))
  {
    pspImageDestroy(thumb);
    thumb = NULL;
  }

  fclose(f);
  return thumb;
}

static void psp_load_options()
{
  pl_file_path path;
  snprintf(path, sizeof(path) - 1, "%s%s",
           pl_psp_get_app_directory(), "options.ini");

  /* Load INI */
  pl_ini_file file;
  pl_ini_load(&file, path);

  psp_options.display_mode = pl_ini_get_int(&file, "Video", "Display Mode", 
                                            DISPLAY_MODE_UNSCALED);
  psp_options.frame_skip = pl_ini_get_int(&file, "Video", "Frame Skipping", 0);
  psp_options.clock_freq = pl_ini_get_int(&file, "Video", "PSP Clock Frequency", 333);
  psp_options.show_fps = pl_ini_get_int(&file, "Video", "Show FPS", 0);
  pl_ini_get_string(&file, "File", "Game Path", NULL, 
                    GamePath, sizeof(GamePath));

  int control_mode = pl_ini_get_int(&file, "Menu", "Control Mode", 0);
  UiMetric.Animate = pl_ini_get_int(&file, "Menu", "Animate", 1);
  UiMetric.OkButton = (!control_mode)
                      ? PSP_CTRL_CROSS : PSP_CTRL_CIRCLE;
  UiMetric.CancelButton = (!control_mode)
                          ? PSP_CTRL_CIRCLE : PSP_CTRL_CROSS;

  /* Clean up */
  pl_ini_destroy(&file);
}

static int psp_save_options()
{
  pl_file_path path;
  snprintf(path, sizeof(path)-1, "%s%s",
           pl_psp_get_app_directory(), "options.ini");

  /* Initialize INI structure */
  pl_ini_file file;
  pl_ini_create(&file);
  pl_ini_set_int(&file, "Video", "Display Mode", 
                 psp_options.display_mode);
  pl_ini_set_int(&file, "Video", "Frame Skipping", 
                 psp_options.frame_skip);
  pl_ini_set_int(&file, "Video", "PSP Clock Frequency", 
                 psp_options.clock_freq);
  pl_ini_set_int(&file, "Video", "Show FPS", 
                 psp_options.show_fps);
  pl_ini_set_int(&file, "Menu", "Control Mode", 
                 (UiMetric.OkButton == PSP_CTRL_CIRCLE));
  pl_ini_set_int(&file, "Menu", "Animate",
                 UiMetric.Animate);
  pl_ini_set_string(&file, "File", "Game Path",
                    GamePath);

  int status = pl_ini_save(&file, path);
  pl_ini_destroy(&file);

  return status;
}
