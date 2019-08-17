#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "tool.h"
#include "application.h"
#include "memory.h"
#include "automata.h"

#define UNUSED(x) (void)(x)

int main(int argc,char *argv[], char *envp[])
{
  
  /* Variables */
  int status;
  
  struct sigaction interruptHandler;
  struct sigaction alarmHandler;
  struct sigaction stopHandler;

  UNUSED(argc);
  UNUSED(argv);
  
  alarmHandler.sa_handler = alrmHandler;
  alarmHandler.sa_flags = SA_RESTART;
  sigemptyset(&alarmHandler.sa_mask);
  
  interruptHandler.sa_handler = ctrlCHandler;
  interruptHandler.sa_flags = SA_RESTART | SA_NODEFER;
  sigemptyset(&interruptHandler.sa_mask);
  
  stopHandler.sa_handler = sleepHandler;
  stopHandler.sa_flags = SA_RESTART;
  sigemptyset(&stopHandler.sa_mask);
  
  if(sigaction(SIGALRM,&alarmHandler,NULL) < 0)
    {
      printf("Error at sigaction\n");
    }
  
  if(sigaction(SIGINT,&interruptHandler,NULL) < 0)
    {
      printf("Error at sigaction\n");
    }

  if(sigaction(SIGTSTP,&stopHandler,NULL) < 0)
    {
      printf("Error at sigaction\n");
    }

  /*=====================*/
  /*        SHELL        */
  /*=====================*/
  if((status=buildGlobalVariables()) != 0)
    {
      printf("Error initialiazing variables -- error : %d\n",status);
    }
  else
    {
      while (*envp)
	{
	  char* reconstruct = calloc(strlen(*envp) + 25, sizeof(char));
	  sprintf(reconstruct,"setenv %s",*(envp++));
	  executeCommandInMemory(shell->memory,reconstruct);
	  free(reconstruct);
	}      
      
      loopShell(shell);
      freeMyShell(shell);
    }

  return 0;
}
