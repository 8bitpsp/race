//---------------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version. See also the license.txt file for
//  additional informations.
//---------------------------------------------------------------------------

// state.h: state saving
//
//  09/11/2008 Initial version (Akop Karapetyan)
//
//////////////////////////////////////////////////////////////////////

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
  //Save state version
  u8 state_version; // = 0x10

  //Rom signature
  u8 rom_signature[0x40];

	//Memory
	u8 ram[0xc000];
  u8 cpuram[0x08a0];// 0xC000]; 0x38000 

	//TLCS-900h Registers
	u32 pc, sr;
	u8 f_dash;
	u32 gpr[23];

  //Z80 Registers
  cz80_struc RACE_cz80_struc;
  u32 PC_offset;
  s32 Z80_ICount;

  //Sound Chips
  int sndCycles;
  SoundChip toneChip;
  SoundChip noiseChip;

	//Timers
  int timer0, timer1, timer2, timer3;

	//DMA
  u8 ldcRegs[64];
}
RACE_STATE;

int state_store(char* filename);
int state_restore(char* filename);
int state_store(FILE *stream);
int state_restore(FILE *stream);

//=============================================================================
#endif // _STATE_H

