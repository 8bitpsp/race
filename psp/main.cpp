#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pspkernel.h>

#include "pl_snd.h"
#include "video.h"
#include "pl_psp.h"
#include "ctrl.h"

PSP_MODULE_INFO(PSP_APP_NAME, 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#include "emulate.h"

extern int m_bIsActive;

static void ExitCallback(void* arg)
{
  m_bIsActive = 0;
  ExitPSP = 1;
}

int main(int argc, char **argv)
{
  /* Initialize PSP */
  pl_psp_init(argv[0]);
  pl_snd_init(512, 0);
  pspCtrlInit();
  pspVideoInit();

  /* Initialize callbacks */
  pl_psp_register_callback(PSP_EXIT_CALLBACK,
                           ExitCallback,
                           NULL);
  pl_psp_start_callback_thread();

  if (InitEmulation())
  {
    RunEmulation();
    TrashEmulation();
  }

  /* Release PSP resources */
  pl_snd_shutdown();
  pspVideoShutdown();
  pl_psp_shutdown();

  return(0);
}
