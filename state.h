#ifndef _STATE_H
#define _STATE_H

#include "cz80.h"
#include "neopopsound.h"

//-----------------------------------------------------------------------------
// State Definitions:
//-----------------------------------------------------------------------------

#define INT_QUEUE_MAX 4

typedef struct
{
	//Save State Id
	u16 valid_state_id; // = 0x0050

	//Memory
	u8 mainram[0x38000];// 0xC000];

	//TLCS-900h Registers
	u32 pc, sr;
	u8 f_dash;
	u32 gpr[23];

  // these 5 may not be necessary
#if 0
  u8 interruptPendingLevel;
  u8 pendingInterrupts[7][INT_QUEUE_MAX];
  int state;
  int checkstate;
  int DMAstate;
#endif

#if 0
	BOOL eepromStatusEnable;
#endif

  //Z80 Registers
  cz80_struc RACE_cz80_struc;
  s32 Z80_ICount;

  //Sound Chips
  int sndCycles;
  SoundChip toneChip;
  SoundChip noiseChip;

	//Timers
  int timer0, timer1, timer2, timer3;

	//DMA
  u8 ldcRegs[64];

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

