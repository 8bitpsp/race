#include "state.h"
#include "tlcs900h.h"
#include "memory.h"
#include <string.h>

#ifdef PC
#undef PC
#endif

//-----------------------------------------------------------------------------
// state_restore()
//-----------------------------------------------------------------------------
BOOL state_restore(char* filename)
{
  RACE_STATE rs;

  // Load
  FILE *stream;
  if (!(stream = fopen(filename, "r")))
    return 0;

  if (fread(&rs, sizeof(rs), 1, stream) < 1)
  {
    fclose(stream);
    return 0;
  }

  fclose(stream);

  mem_init();
  tlcs_init();
  Z80_Init();
  Z80_Reset();

  // if neogeo pocket color rom, act if we are a neogeo pocket color
  tlcsMemWriteB(0x6F91,tlcsMemReadB(0x00200023));
  // pretend we're running in English mode
  tlcsMemWriteB(0x00006F87,0x01);
  // kludges & fixes
  switch (tlcsMemReadW(0x00200020))
  {
      case 0x0059:  // Sonic
      case 0x0061:  // Metal SLug 2nd
          *get_address(0x0020001F) = 0xFF;
          break;
  }

  ngpSoundOff();

// TODO 
  //Build a state description
//  state.valid_state_id = 0x0050;
// TODO  memcpy(&state.header, rom_header, sizeof(RomHeader));

//  state.eepromStatusEnable = eepromStatusEnable;

  //TLCS-900h Registers
  gen_regsPC = rs.pc;
  gen_regsSR = rs.sr;
  F2 = rs.f_dash;

  int i = 0, j;
  gen_regsXWA0 = rs.gpr[i++];
  gen_regsXBC0 = rs.gpr[i++];
  gen_regsXDE0 = rs.gpr[i++];
  gen_regsXHL0 = rs.gpr[i++];

  gen_regsXWA1 = rs.gpr[i++];
  gen_regsXBC1 = rs.gpr[i++];
  gen_regsXDE1 = rs.gpr[i++];
  gen_regsXHL1 = rs.gpr[i++];

  gen_regsXWA2 = rs.gpr[i++];
  gen_regsXBC2 = rs.gpr[i++];
  gen_regsXDE2 = rs.gpr[i++];
  gen_regsXHL2 = rs.gpr[i++];

  gen_regsXWA3 = rs.gpr[i++];
  gen_regsXBC3 = rs.gpr[i++];
  gen_regsXDE3 = rs.gpr[i++];
  gen_regsXHL3 = rs.gpr[i++];

  gen_regsXIX = rs.gpr[i++];
  gen_regsXIY = rs.gpr[i++];
  gen_regsXIZ = rs.gpr[i++];
  gen_regsXSP = rs.gpr[i++];

  gen_regsSP = rs.gpr[i++];
  gen_regsXSSP = rs.gpr[i++];
  gen_regsXNSP = rs.gpr[i++];
#if 1
  extern u8 interruptPendingLevel, pendingInterrupts[7][INT_QUEUE_MAX];
  interruptPendingLevel = rs.interruptPendingLevel;
  for (i = 0; i < 7; i++)
    for (j = 0; j < INT_QUEUE_MAX; j++)
      pendingInterrupts[i][j] = rs.pendingInterrupts[i][j];
  extern int state, checkstate, DMAstate;
  state = rs.state;
  checkstate = rs.checkstate;
  DMAstate = rs.DMAstate;
#endif
  //Z80 Registers
  extern cz80_struc RACE_cz80_struc;
  extern s32 Z80_ICount;
  unsigned char *mame4all_cz80_rom = &mainram[0x3000];

  memcpy(&RACE_cz80_struc, &rs.RACE_cz80_struc, sizeof(cz80_struc));

  Z80_ICount = rs.Z80_ICount;
  RACE_cz80_struc.PC = (rs.PC_is_null)
                       ? 0 : (u8*)((u32)mame4all_cz80_rom + rs.PC_offset);

  //Sound Chips
  extern int sndCycles;
  sndCycles = rs.sndCycles;
  memcpy(&toneChip, &rs.toneChip, sizeof(SoundChip));
  memcpy(&noiseChip, &rs.noiseChip, sizeof(SoundChip));

  //Timers
  timer0 = rs.timer0;
  timer1 = rs.timer1;
  timer2 = rs.timer2;
  timer3 = rs.timer3;

  //DMA
  memcpy(&ldcRegs, &rs.ldcRegs, sizeof(ldcRegs));

  //Memory
  memcpy(mainram, rs.mainram, sizeof(rs.mainram));

  tlcs_reinit();
  extern int ngpRunning;
  ngpRunning = 1;

  return 1;
}

int state_store(char* filename)
{
  RACE_STATE rs;

  //Build a state description
  rs.valid_state_id = 0x0050;
// TODO  memcpy(&state.header, rom_header, sizeof(RomHeader));

//  state.eepromStatusEnable = eepromStatusEnable;

  //TLCS-900h Registers
  rs.pc = gen_regsPC;
  rs.sr = gen_regsSR;
  rs.f_dash = F2;

  int i = 0, j;
  rs.gpr[i++] = gen_regsXWA0;
  rs.gpr[i++] = gen_regsXBC0;
  rs.gpr[i++] = gen_regsXDE0;
  rs.gpr[i++] = gen_regsXHL0;

  rs.gpr[i++] = gen_regsXWA1;
  rs.gpr[i++] = gen_regsXBC1;
  rs.gpr[i++] = gen_regsXDE1;
  rs.gpr[i++] = gen_regsXHL1;

  rs.gpr[i++] = gen_regsXWA2;
  rs.gpr[i++] = gen_regsXBC2;
  rs.gpr[i++] = gen_regsXDE2;
  rs.gpr[i++] = gen_regsXHL2;

  rs.gpr[i++] = gen_regsXWA3;
  rs.gpr[i++] = gen_regsXBC3;
  rs.gpr[i++] = gen_regsXDE3;
  rs.gpr[i++] = gen_regsXHL3;

  rs.gpr[i++] = gen_regsXIX;
  rs.gpr[i++] = gen_regsXIY;
  rs.gpr[i++] = gen_regsXIZ;
  rs.gpr[i++] = gen_regsXSP;

  rs.gpr[i++] = gen_regsSP;
  rs.gpr[i++] = gen_regsXSSP;
  rs.gpr[i++] = gen_regsXNSP;
#if 1
  extern u8 interruptPendingLevel, pendingInterrupts[7][INT_QUEUE_MAX];
  rs.interruptPendingLevel = interruptPendingLevel;
  for (i = 0; i < 7; i++)
    for (j = 0; j < INT_QUEUE_MAX; j++)
      rs.pendingInterrupts[i][j] = pendingInterrupts[i][j];

  extern int state, checkstate, DMAstate;
  rs.state = state;
  rs.checkstate = checkstate;
  rs.DMAstate = DMAstate;
#endif
  //Z80 Registers
  extern cz80_struc RACE_cz80_struc;
  extern s32 Z80_ICount;
  unsigned char *mame4all_cz80_rom = &mainram[0x3000];

  memcpy(&rs.RACE_cz80_struc, &RACE_cz80_struc, sizeof(cz80_struc));
  rs.Z80_ICount = Z80_ICount;

  rs.PC_is_null = !RACE_cz80_struc.PC;
  rs.PC_offset = (RACE_cz80_struc.PC) 
                 ? ((u32)RACE_cz80_struc.PC - (u32)mame4all_cz80_rom) : 0;

  //Sound Chips
  extern int sndCycles;
  rs.sndCycles = sndCycles;
  memcpy(&rs.toneChip, &toneChip, sizeof(SoundChip));
  memcpy(&rs.noiseChip, &noiseChip, sizeof(SoundChip));

  //Timers
  rs.timer0 = timer0;
  rs.timer1 = timer1;
  rs.timer2 = timer2;
  rs.timer3 = timer3;

  //DMA
  memcpy(&rs.ldcRegs, &ldcRegs, sizeof(ldcRegs));

  //Memory
  memcpy(rs.mainram, mainram, sizeof(rs.mainram));

  // Save
  FILE *stream;
  if (!(stream = fopen(filename, "w")))
    return 0;

  if (fwrite(&rs, sizeof(rs), 1, stream) < 1)
  {
    fclose(stream);
    return 0;
  }

  fclose(stream);

  return 1;
}
