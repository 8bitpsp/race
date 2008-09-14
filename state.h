#ifndef _STATE_H
#define _STATE_H

#include "cz80.h"
#include "neopopsound.h"

//-----------------------------------------------------------------------------
// State Definitions:
//-----------------------------------------------------------------------------

typedef struct
{
	//Save State Id
	unsigned short valid_state_id; // = 0x0050

	//Memory
	unsigned char mainram[0xC000];

	//TLCS-900h Registers
	unsigned int pc, sr;
	unsigned char f_dash;
	unsigned int gpr[23];

#if 0
	BOOL eepromStatusEnable;
#endif

  //Z80 Registers
  cz80_struc RACE_cz80_struc;

  //Sound Chips
  int sndCycles;
  SoundChip toneChip;
  SoundChip noiseChip;

	//Timers
  int timer0, timer1, timer2, timer3;

	//DMA
  unsigned char ldcRegs[64];

#if 0
	//Rom Description
	RomHeader header;
#endif
}
RACE_STATE;

int state_store(char* filename);
int state_restore(char* filename);

//=============================================================================
#endif // _STATE_H

