#ifndef __SHELL__
#define __SHELL__

#define SIZE_PATH_BASE 1024
#define SIZE_ARG_BASE 20
#define MAX_ELEMENT_COMMAND_BASE 20

#include "processusManager.h"
#include "memory.h"

struct MyShell{
  /* Command */
  char* command;
  int isRunning;
  int isRequestingExitValidation;
  int hardExit;
  
  /* Path datas */
  int sizePath;
  char* basePath;
  char* path;
  char* homeDir;
  char* pathPrint;

  /* Processus */
  ProcessusManager* manager;
  Memory* memory;
  
};

enum SpecialArg
  {
    AND,
    OR,
    SEQ,
    PIPE,
    NONE
  };

typedef struct MyShell MyShell;
typedef enum SpecialArg SpecialArg;

MyShell* constructMyShell(int* status);
void freeMyShell(MyShell* shell);
int initMyShell(MyShell* shell);

int isRedirectElement();
int elementIsSpecialArg(char* line, int* toSkip);

void displayMyShell(MyShell* shell);
int analyseCommand(MyShell* shell, char* command);
int buildProcessus(MyShell* shell, char* original);
void loopShell(MyShell* shell);

void loopMyShell(MyShell* shell);
int exitAsked(MyShell* shell, int totalExit);

int addElementToContainer(char** container, int* max, int* current, const char* toAdd);
int addElementToBuffer(char* buffer, int* max, int* current, char toAdd);

#endif
