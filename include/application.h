#ifndef __APPLICATION__
#define __APPLICATION__

#include "Shell.h"
#include "processusManager.h"
#include "memory.h"

/* ============= */
/*     GLOBAL    */
/* ============= */

MyShell* shell;
ProcessusManager* manager;
Memory* memory;
int shouldRespondeCtrlC;

int buildGlobalVariables();

void ctrlCHandler();
void alrmHandler();
void childEndHandler();
void sleepHandler();

#endif
