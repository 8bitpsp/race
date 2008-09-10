#ifndef PSP_EMULATE_H
#define PSP_EMULATE_H

int  InitEmulation();
void RunEmulation();
void TrashEmulation();

#define DISPLAY_MODE_UNSCALED    0
#define DISPLAY_MODE_FIT_HEIGHT  1
#define DISPLAY_MODE_FILL_SCREEN 2

#define MAP_BUTTONS 20

#define JST 0x100
#define SPC 0x400

#define SPC_MENU 1

#define CODE_MASK(x) (x & 0xff)

typedef struct psp_ctrl_map_t
{
  uint32_t button_map[MAP_BUTTONS];
} psp_ctrl_map_t;

typedef struct psp_ctrl_mask_to_index_map_t
{
  uint64_t mask;
  uint8_t  index;
} psp_ctrl_mask_to_index_map_t;

typedef struct psp_options_t
{
  uint8_t  display_mode;
  uint8_t  show_fps;
  uint8_t  frame_skip;
  uint16_t clock_freq;
} psp_options_t;

extern psp_options_t psp_options;

#endif
