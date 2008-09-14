#include "state.h"
#include "tlcs900h.h"
#include "memory.h"
#include <string.h>

//-----------------------------------------------------------------------------
// state_restore()
//-----------------------------------------------------------------------------
BOOL state_restore(char* filename)
{
  RACE_STATE state;

  /* Load */
  FILE *stream;
  if (!(stream = fopen(filename, "r")))
    return 0;

  if (fread(&state, sizeof(state), 1, stream) < 1)
  {
    fclose(stream);
    return 0;
  }

  fclose(stream);

// TODO 
  //Build a state description
//  state.valid_state_id = 0x0050;
// TODO  memcpy(&state.header, rom_header, sizeof(RomHeader));

//  state.eepromStatusEnable = eepromStatusEnable;

  //TLCS-900h Registers
  gen_regsPC = state.pc;
  gen_regsSR = state.sr;
  F2 = state.f_dash;

  int i = 0;
  gen_regsXWA0 = state.gpr[i++];
  gen_regsXBC0 = state.gpr[i++];
  gen_regsXDE0 = state.gpr[i++];
  gen_regsXHL0 = state.gpr[i++];

  gen_regsXWA1 = state.gpr[i++];
  gen_regsXBC1 = state.gpr[i++];
  gen_regsXDE1 = state.gpr[i++];
  gen_regsXHL1 = state.gpr[i++];

  gen_regsXWA2 = state.gpr[i++];
  gen_regsXBC2 = state.gpr[i++];
  gen_regsXDE2 = state.gpr[i++];
  gen_regsXHL2 = state.gpr[i++];

  gen_regsXWA3 = state.gpr[i++];
  gen_regsXBC3 = state.gpr[i++];
  gen_regsXDE3 = state.gpr[i++];
  gen_regsXHL3 = state.gpr[i++];

  gen_regsXIX = state.gpr[i++];
  gen_regsXIY = state.gpr[i++];
  gen_regsXIZ = state.gpr[i++];
  gen_regsXSP = state.gpr[i++];

  gen_regsSP = state.gpr[i++];
  gen_regsXSSP = state.gpr[i++];
  gen_regsXNSP = state.gpr[i++];

  //Z80 Registers
  extern cz80_struc RACE_cz80_struc;
  memcpy(&RACE_cz80_struc, &state.RACE_cz80_struc, sizeof(cz80_struc));

  //Sound Chips
  extern int sndCycles;
  sndCycles = state.sndCycles;
  memcpy(&toneChip, &state.toneChip, sizeof(SoundChip));
  memcpy(&noiseChip, &state.noiseChip, sizeof(SoundChip));

  //Timers
  timer0 = state.timer0;
  timer1 = state.timer1;
  timer2 = state.timer2;
  timer3 = state.timer3;

  //DMA
  memcpy(&ldcRegs, &state.ldcRegs, sizeof(ldcRegs));

  //Memory
  memcpy(&mainram, &state.mainram, 0xC000);

  return 1;
}

int state_store(char* filename)
{
  RACE_STATE state;

  //Build a state description
  state.valid_state_id = 0x0050;
// TODO  memcpy(&state.header, rom_header, sizeof(RomHeader));

//  state.eepromStatusEnable = eepromStatusEnable;

  //TLCS-900h Registers
  state.pc = gen_regsPC;
  state.sr = gen_regsSR;
  state.f_dash = F2;

  int i = 0;
  state.gpr[i++] = gen_regsXWA0;
  state.gpr[i++] = gen_regsXBC0;
  state.gpr[i++] = gen_regsXDE0;
  state.gpr[i++] = gen_regsXHL0;

  state.gpr[i++] = gen_regsXWA1;
  state.gpr[i++] = gen_regsXBC1;
  state.gpr[i++] = gen_regsXDE1;
  state.gpr[i++] = gen_regsXHL1;

  state.gpr[i++] = gen_regsXWA2;
  state.gpr[i++] = gen_regsXBC2;
  state.gpr[i++] = gen_regsXDE2;
  state.gpr[i++] = gen_regsXHL2;

  state.gpr[i++] = gen_regsXWA3;
  state.gpr[i++] = gen_regsXBC3;
  state.gpr[i++] = gen_regsXDE3;
  state.gpr[i++] = gen_regsXHL3;

  state.gpr[i++] = gen_regsXIX;
  state.gpr[i++] = gen_regsXIY;
  state.gpr[i++] = gen_regsXIZ;
  state.gpr[i++] = gen_regsXSP;

  state.gpr[i++] = gen_regsSP;
  state.gpr[i++] = gen_regsXSSP;
  state.gpr[i++] = gen_regsXNSP;

  //Z80 Registers
  extern cz80_struc RACE_cz80_struc;
  memcpy(&state.RACE_cz80_struc, &RACE_cz80_struc, sizeof(cz80_struc));

  //Sound Chips
  extern int sndCycles;
  state.sndCycles = sndCycles;
  memcpy(&state.toneChip, &toneChip, sizeof(SoundChip));
  memcpy(&state.noiseChip, &noiseChip, sizeof(SoundChip));

  //Timers
  state.timer0 = timer0;
  state.timer1 = timer1;
  state.timer2 = timer2;
  state.timer3 = timer3;

  //DMA
  memcpy(&state.ldcRegs, &ldcRegs, sizeof(ldcRegs));

  //Memory
  memcpy(&state.mainram, &mainram, 0xC000);

  // Save
  FILE *stream;
  if (!(stream = fopen(filename, "w")))
    return 0;

  if (fwrite(&state, sizeof(state), 1, stream) < 1)
  {
    fclose(stream);
    return 0;
  }

  fclose(stream);

  return 1;
}

#if 0
//=============================================================================

static void read_state_0050(char* filename)
{
	NEOPOPSTATE0050	state;
	int i,j;

	if (system_io_state_read(filename, (_u8*)&state, sizeof(NEOPOPSTATE0050)))
	{
		//Verify correct rom...
		if (memcmp(rom_header, &state.header, sizeof(RomHeader)) != 0)
		{
			system_message(system_get_string(IDS_WRONGROM));
			return;
		}

		//Apply state description
		reset();

		eepromStatusEnable = state.eepromStatusEnable;

		//TLCS-900h Registers
		pc = state.pc;
		sr = state.sr;				changedSP();
		f_dash = state.f_dash;

		eepromStatusEnable = state.eepromStatusEnable;

		for (i = 0; i < 4; i++)
		{
			gpr[i] = state.gpr[i];
			for (j = 0; j < 4; j++)
				gprBank[i][j] = state.gprBank[i][j];
		}

		//Timers
		timer_hint = state.timer_hint;

		for (i = 0; i < 4; i++)	//Up-counters
			timer[i] = state.timer[i];

		timer_clock0 = state.timer_clock0;
		timer_clock1 = state.timer_clock1;
		timer_clock2 = state.timer_clock2;
		timer_clock3 = state.timer_clock3;

		//Z80 Registers
		memcpy(&Z80_regs, &state.Z80_regs, sizeof(Z80));

		//Sound Chips
		memcpy(&toneChip, &state.toneChip, sizeof(SoundChip));
		memcpy(&noiseChip, &state.noiseChip, sizeof(SoundChip));

		//DMA
		for (i = 0; i < 4; i++)
		{
			dmaS[i] = state.dmaS[i];
			dmaD[i] = state.dmaD[i];
			dmaC[i] = state.dmaC[i];
			dmaM[i] = state.dmaM[i];
		}

		//Memory
		memcpy(ram, &state.ram, 0xC000);
	}
}

//=============================================================================
#endif
