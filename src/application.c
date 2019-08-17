#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <poll.h>

#include "processus.h"
#include "processusManager.h"
#include "application.h"
#include "error.h"
#include "tool.h"

int buildGlobalVariables()
{
  int status;
  
  shell = constructMyShell(&status);
  if(shell == NULL)
    {
      return status;
    }

  manager = constructProcessusManager(&status);
  if(manager == NULL)
  {
    freeMyShell(shell);
    return status;
  }

  shell->manager = manager;

  memory = createMemory(&status);
  if(memory == NULL)
    {
      freeMyShell(shell);
      return status;
    }

  shell->memory = memory;

  shouldRespondeCtrlC = 0;

  return 0;
}

void ctrlCHandler()
{
  if(shell != NULL)
    {
      if(shell->manager->isExecutingForegroundTask)
	{
	  kill(shell->manager->pidForegroundTask,SIGKILL);
	}
      else
	{
	  if(!shouldRespondeCtrlC)
	    {
	      shouldRespondeCtrlC = 1;
	      shell->isRequestingExitValidation = 1;
	      fprintf(stderr,"\nVoulez vous vraiment quitter ? (o/n) ");
	      while(shouldRespondeCtrlC)
		{
		  char* answer;
		  answer = readRestrictedSize(3);
		  if(answer == NULL)
		    {
		      printf("Réponse incorrecte !\n");
		      printf("\nVoulez vous vraiment quitter ? (o/n) ");
		    }
		  else
		    {
		      if(strcmp(answer,"o") == 0)
			{
			  shell->hardExit = 1;
			  shell->isRunning = 0;
			  shouldRespondeCtrlC = 0;
			  free(answer);
			}
		      else if(strcmp(answer,"n") == 0)
			{
			  shouldRespondeCtrlC = 0;
			  free(answer);
			  displayMyShell(shell);
			}
		      else
			{
			  free(answer);
			  printf("Réponse incorrecte !\n");
			  printf("\nVoulez vous vraiment quitter ? (o/n) ");
			}
		    }
		}
	    }
	}
    }
}

void alrmHandler()
{
  return;
}

void childEndHandler()
{
  int status;
  int id;

  id = waitpid(-1,&status,WNOHANG);
  if(id == -1)
    {
    }
  else if(id > 0)
    {
      Processus* proc = getProcessusFromPid(manager,id);
      if(proc != NULL)
	{
	  if(WIFEXITED(status))
	    {
	      printf("\n%s (jobs=[%d],pid=%d) terminé avec status=%d\n",proc->bruteLine,proc->jobNumber,proc->pid,WEXITSTATUS(status));
	      if(WEXITSTATUS(status) == 0)
		{
		  proc->success = 1;
		}
	      else
		{
		  proc->success = 0;
		}

	      proc->isExecuting = 0;
	      proc->hasFinish = 1;
	    }
	  else
	    {
	      printf("\n%s (jobs=[%d],pid=%d) terminé avec status=%d\n",proc->bruteLine,proc->jobNumber,proc->pid,-1);
	      proc->isExecuting = 0;
	      proc->hasFinish = 1;
	    }
	  
	  removeProcessusFromManager(manager,proc->pid);
	}
    }
}

void sleepHandler()
{
  if(shell->manager->isExecutingForegroundTask)
    {
      Processus* proc;

      proc = getProcessusFromPid(shell->manager,shell->manager->pidForegroundTask);
      if(proc != NULL)
	{
	  kill(shell->manager->pidForegroundTask,SIGTSTP);
	  proc->isStopped = 1;
	}
    }
  return;
}
