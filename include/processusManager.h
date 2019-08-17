#ifndef __PROCESSUS_MANAGER__
#define __PROCESSUS_MANAGER__

#include "processus.h"

struct ProcessusManager
{
  int maxProc;
  int currentProc;
  Processus** processus;

  int lastForegroundJobNumber;
  int lastForegroundJobStatus;
  int lastForegroundJobNormallyFinished;
  char* cmdLastForeground;
  
  int isExecutingForegroundTask;
  int pidForegroundTask;
  
  int nbJobs;
};

typedef struct ProcessusManager ProcessusManager;

void childEndHandler();

ProcessusManager* constructProcessusManager(int* status);
int initProcessusManager(ProcessusManager* pm);
void freeProcessusManager(ProcessusManager* pm);

int isStillProcessusInBackground(const ProcessusManager* pm);
Processus* getProcessusFromHigherJobBackground(const ProcessusManager* pm);
Processus* getProcessusFromHigherJobForeground(const ProcessusManager* pm);
Processus* getProcessusFromPid(ProcessusManager* pm, int pid);
Processus* getProcessusFromJob(ProcessusManager* pm, int job);
Processus* getProcessusFromJobAndBackground(ProcessusManager* pm, int job);
Processus* getProcessusFromJobAndForeground(ProcessusManager* pm, int job);

int getIndexProcessusFromPid(ProcessusManager* pm, int pid);

int removeProcessusFromManager(ProcessusManager* pm, int pid);
int addProcessusToManager(ProcessusManager* pm, Processus* proc);

int setProcessusOptions(ProcessusManager* pm);


int waitFromChild(ProcessusManager* pm, int pidToWait);
int prepareForRedirection(Processus* proc);

int executeMyFg(ProcessusManager* pm, Processus* proc);
int executeMyBg(ProcessusManager* pm, Processus* proc);
int executeMyJobs(Processus* proc);
int executeExit(Processus* proc, int* exitWanted);
int executeStatus(ProcessusManager* pm, Processus* proc);

int executeAsInternProcessus(ProcessusManager* pm, Processus* proc, int* status, int* exitWanted);
int executeProcessus(ProcessusManager* pm, int* exitWanted);
int prepareProcessus(ProcessusManager* pm);
void printProcessusManager(const ProcessusManager* pm);
int removeFinishedAndTreatedProcessus(ProcessusManager* pm);

#endif
