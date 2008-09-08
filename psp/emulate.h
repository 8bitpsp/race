#ifndef PSP_EMULATE_H
#define PSP_EMULATE_H

int  InitEmulation();
void RunEmulation();
void TrashEmulation();

#define DISPLAY_MODE_UNSCALED    0
#define DISPLAY_MODE_FIT_HEIGHT  1
#define DISPLAY_MODE_FILL_SCREEN 2

#endif
