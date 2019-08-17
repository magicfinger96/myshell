#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "tool.h"
#include "error.h"
#include "processusManager.h"
#include "application.h"
#include "myJobs.h"
#include "processus.h"

#define MAX_PROC_BASE 5
#define MAX_TO_DELETE_BASE 5
typedef void (*sighandler_t)(int);

int asExecMyBgMyFg = 0;

ProcessusManager* constructProcessusManager(int* status)
{
  /* Variables */
  ProcessusManager* pm;
  
  /* Instanciation */
  pm = calloc(1,sizeof(ProcessusManager));
  if(pm == NULL)
    {
      *status = ALLOCATION_ERROR;
      return NULL;
    }

  /* Initialisation */
  if((*status=initProcessusManager(pm)) != 0)
    {
      return NULL;
    }
  
  *status = 0;
  return pm;
}

int initProcessusManager(ProcessusManager* pm)
{
  pm->maxProc = MAX_PROC_BASE;
  pm->currentProc = 0;
  pm->processus = calloc(pm->maxProc, sizeof(Processus*));
  pm->nbJobs = 1;
  pm->cmdLastForeground = NULL;
  pm->lastForegroundJobNumber = -1;
  pm->lastForegroundJobStatus = -1;
  pm->lastForegroundJobNormallyFinished = 0;
  pm->isExecutingForegroundTask = 0;
  pm->pidForegroundTask = -1;
  if(pm->processus == NULL)
    {
      return ALLOCATION_ERROR;
    }
  return 0;
}

void freeProcessusManager(ProcessusManager* pm)
{
  int i;
  if(pm->processus != NULL)
    {
      for(i = 0; i < pm->currentProc; i++)
	{
	  freeProcessus(pm->processus[i]);
	}
      free(pm->processus);
    }

  if(pm->cmdLastForeground != NULL)
    {
      free(pm->cmdLastForeground);
    }

  free(pm);
}

int isStillProcessusInBackground(const ProcessusManager* pm)
{
  int i;
  for(i = 0; i < pm->currentProc; i++)
    {
      Processus* proc = pm->processus[i];
      if(proc->backgroundCondition && proc->isExecuting)
	{
	  return 1;
	}
    }
  
  return 0;
}

Processus* getProcessusFromHigherJobBackground(const ProcessusManager* pm)
{
  int i;
  int maxJob = -1;
  Processus* proc = NULL;
  
  for(i = 0; i < pm->currentProc; i++)
    {
      Processus* cur = pm->processus[i];
      if((cur->isStopped || (cur->backgroundCondition && cur->isExecuting)) && cur->jobNumber > maxJob)
	{
	  maxJob = cur->jobNumber;
	  proc = cur;
	}
    }
  
  return proc;
}
Processus* getProcessusFromHigherJobForeground(const ProcessusManager* pm)
{
  int i;
  int maxJob = -1;
  Processus* proc = NULL;
  
  for(i = 0; i < pm->currentProc; i++)
    {
      Processus* cur = pm->processus[i];
      if((cur->isStopped || (!cur->backgroundCondition && cur->isExecuting)) && cur->jobNumber > maxJob)
	{
	  maxJob = cur->jobNumber;
	  proc = cur;
	}
    }
  
  return proc;
}

Processus* getProcessusFromPid(ProcessusManager* pm, int pid)
{
  int i;
  for(i = 0; i < pm->currentProc; i++)
    {
      if(pm->processus[i]->pid == pid)
	{
	  return pm->processus[i];
	}
    }
  return NULL;
}

Processus* getProcessusFromJob(ProcessusManager* pm, int job)
{
  int i;
  for(i = 0; i < pm->currentProc; i++)
    {
      if(pm->processus[i]->jobNumber == job)
	{
	  return pm->processus[i];
	}
    }
  return NULL;
}

Processus* getProcessusFromJobAndBackground(ProcessusManager* pm, int job)
{
  int i;
  for(i = 0; i < pm->currentProc; i++)
    {
      Processus* cur = pm->processus[i];
      if(cur->jobNumber == job && (cur->isStopped || (cur->isExecuting && cur->backgroundCondition)))
	{
	  return pm->processus[i];
	}
    }
  return NULL;
}

Processus* getProcessusFromJobAndForeground(ProcessusManager* pm, int job)
{
  int i;
  for(i = 0; i < pm->currentProc; i++)
    {
      Processus* cur = pm->processus[i];
      if(cur->jobNumber == job && (cur->isStopped || (cur->isExecuting && !cur->backgroundCondition)))
	{
	  return pm->processus[i];
	}
    }
  return NULL;
}

int getIndexProcessusFromPid(ProcessusManager* pm, int pid)
{
  int i;
  if(pm->processus != NULL)
    {
      for(i = 0; i < pm->currentProc; i++)
	{
	  if(pm->processus[i]->pid == pid)
	    {
	      return i;
	    }
	}
    }
  
  return -1;
}

int removeProcessusFromManager(ProcessusManager* pm, int pid)
{
  int indexRemove = getIndexProcessusFromPid(pm,pid);
  if(indexRemove == -1)
    {
      return ELEMENT_NOT_IN_ARRAY;
    }

  freeProcessus(pm->processus[indexRemove]);
  pm->processus[indexRemove] = pm->processus[pm->currentProc - 1];

  pm->currentProc--;
    
  return 0;
}

int addProcessusToManager(ProcessusManager* pm, Processus* proc)
{
  if(pm->maxProc == pm->currentProc)
    {
      pm->maxProc *= 2;
      pm->processus = realloc(pm->processus,pm->maxProc * sizeof(Processus*));
      if(pm->processus == NULL)
	{
	  return REALLOCATION_ERROR;
	}
    }

  pm->processus[pm->currentProc++] = proc;
  return 0;
}

int removeFinishedAndTreatedProcessus(ProcessusManager* pm)
{
  int i;
  int status;
  
  /* On crée la liste de tout les pid à supprimer */
  int max = MAX_TO_DELETE_BASE;
  int current = 0;
  int* toDelete = calloc(max,sizeof(int));
  if(toDelete == NULL)
    {
      return ALLOCATION_ERROR;
    }
  
  for(i = 0; i < pm->currentProc; i++)
    {
      Processus* proc = pm->processus[i];
      if(proc->hasFinish || (!proc->isExecuting && proc->hasBeenTreatedAtExec))
	{
	  if(max == current)
	    {
	      max *= 2;
	      toDelete = realloc(toDelete, max * sizeof(int));
	      if(toDelete == NULL)
		{
		  free(toDelete);
		  return REALLOCATION_ERROR;
		}
	    }
	  toDelete[current++] = proc->pid;
	}
    }
  
  /* On supprime tout les elements  */
  for(i = 0; i < current; i++)
    {
      if((status=removeProcessusFromManager(pm,toDelete[i])) != 0)
	{
	  free(toDelete);
	  return status;
	}
    }

  free(toDelete);
  return 0;
}

int executeMyFg(ProcessusManager* pm, Processus* proc)
{
  Processus* target;
  sighandler_t previousHandler;
  
  if(proc->realArgs[1] == NULL)
    {
      if((target = getProcessusFromHigherJobBackground(pm)) == NULL)
	{
	  return 0;
	}
    }
  else
    {
      if(proc->realArgs[2] == NULL)
	{
	  if(!stringIsInteger(proc->realArgs[1]))
	    {
	      return 0;
	    }
	  
	  if((target = getProcessusFromJobAndBackground(pm,atoi(proc->realArgs[1]))) == NULL)
	    {
	      return 0;
	    }
	}
      else
	{
	  return 0;
	}
    }

  /* Tout est ok */
  /* On va peut etre faire venir en premier plan un processus
     qui avait été lancé sans fils, on doit interrompre le handler SIGCHLD */
  asExecMyBgMyFg = 1;
  previousHandler = signal(SIGCHLD,SIG_DFL);
  target->backgroundCondition = 0;
  if(target->isStopped)
    {
      target->isStopped = 0;
      kill(target->pid,SIGCONT);
    }
  pm->isExecutingForegroundTask = 1;
  pm->pidForegroundTask = target->pid;
  waitFromChild(pm,target->pid);
  signal(SIGCHLD,previousHandler);
  return 1;
}

int executeMyBg(ProcessusManager* pm, Processus* proc)
{
  Processus* target;
  
  if(proc->realArgs[1] == NULL)
    {
      if((target = getProcessusFromHigherJobForeground(pm)) == NULL)
	{
	  return 0;
	}
    }
  else
    {
      if(proc->realArgs[2] == NULL)
	{
	  if(!stringIsInteger(proc->realArgs[1]))
	    {
	      return 0;
	    }
	  
	  if((target = getProcessusFromJobAndForeground(pm,atoi(proc->realArgs[1]))) == NULL)
	    {
	      return 0;
	    }
	}
      else
	{
	  return 0;
	}
    }

  asExecMyBgMyFg = 1;
  target->backgroundCondition = 1;
  pm->isExecutingForegroundTask = 0;
  if(target->isStopped)
    {
      target->isStopped = 0;
      kill(target->pid,SIGCONT);
    }
  signal(SIGCHLD,childEndHandler);
  return 1;
}

int executeMyJobs(Processus* proc)
{
  if(proc->realArgs[1] != NULL)
    {
      return 0;
    }
  else
    {
      myjobs(shell);
      return 1;
    }
}

int executeExit(Processus* proc, int* exitWanted)
{
  if(proc->realArgs[1] != NULL)
    {
      return 0;
    }
  else
    {
      *exitWanted = 1;
      return 1;
    }
}

int executeStatus(ProcessusManager* pm, Processus* proc)
{
  if(proc->realArgs[1] != NULL)
    {
      return 0;
    }
  else
    {
      if(pm->lastForegroundJobNumber != -1)
	{
	  if(pm->lastForegroundJobNormallyFinished)
	    {
	      printf("%s terminé avec comme code de retour %d\n",pm->cmdLastForeground,pm->lastForegroundJobStatus);
	    }
	  else
	    {
	      printf("%s terminé anormalement\n",pm->cmdLastForeground);
	    }
	}
      return 1;
    }
}

int executeCd(Processus *proc){
  char *buf;

  if(proc->currentArg > 1){
    if(proc->realArgs[1] != NULL){
      if((chdir(proc->realArgs[1])) == -1){
            printf("Aucun dossier de ce type\n");
            return 0;
       }
      buf = malloc(sizeof(char)*400);
      getcwd(buf,400);
      free(shell->path);
      free(shell->pathPrint);
      shell->path = malloc(sizeof(char)*(strlen(buf)+1));
      strcpy(shell->path,buf);
      shell->pathPrint = calloc(strlen(shell->path)+1,sizeof(char));
      strcpy(shell->pathPrint,shell->path);
      
      if(strncmp(shell->path,shell->homeDir,strlen(shell->homeDir)) == 0)
	{
	  char* tmp = calloc(strlen(shell->path) + 1,sizeof(char));
	  strcpy(tmp,shell->path+strlen(shell->homeDir));
	  sprintf(shell->pathPrint,"~%s",tmp);
	  free(tmp);
	}
      
      free(buf);
    } else {
      printf("Aucun fichier ou dossier de ce type\n");
      return 0;
    }
  } else {
    /* pas de path */
    if(shell->homeDir != NULL){
      if((chdir(shell->homeDir)) == -1){
            printf("Aucun dossier de ce type\n");
            return 0;
      }
      buf = malloc(sizeof(char)*400);
      getcwd(buf,400);
      free(shell->path);
      shell->path = malloc(sizeof(char)*(strlen(buf)+1));
      strcpy(shell->path,buf);
      shell->pathPrint = calloc(strlen(shell->path)+1,sizeof(char));
      strcpy(shell->pathPrint,shell->path);
      
      if(strncmp(shell->path,shell->homeDir,strlen(shell->homeDir)) == 0)
	{
	  char* tmp = calloc(strlen(shell->path) + 1,sizeof(char));
	  strcpy(tmp,shell->path+strlen(shell->homeDir));
	  sprintf(shell->pathPrint,"~%s",tmp);
	  free(tmp);
	}

      free(buf);
    } else {
      printf("Aucun fichier ou dossier de ce type\n");
      return 0;
    }
  }
  return 1;
}

int executeAsInternProcessus(ProcessusManager* pm, Processus* proc, int* status, int* exitWanted)
{
  if(strcmp(proc->realArgs[0],"myjobs") == 0)
    {
      *status = 0;
      if(!executeMyJobs(proc))
	{
	  *status = WRONG_COMMAND_PARAMETERS;
	  printf("Erreur myjobs, respecter la syntaxe, pas d'argument complétementaire!\n");
	}
      return 1;
    }
  else if(strcmp(proc->realArgs[0],"status") == 0)
    {
      *status = 0;
      if(!executeStatus(pm,proc))
	{
	  *status = WRONG_COMMAND_PARAMETERS;
	  printf("Erreur status, respecter la syntaxe, pas d'argument supplémentaire!\n");
	}
      return 1;
    }
  else if(strcmp(proc->realArgs[0],"exit") == 0)
    {
      *status = 0;
      if(!executeExit(proc,exitWanted))
	{
	  *status = WRONG_COMMAND_PARAMETERS;
	  printf("Erreur exit, respecter la syntaxe, pas d'argument supplémentaire !\n");
	}
      return 1;
    }
  else if(strcmp(proc->realArgs[0],"mybg") == 0)
    {
      *status = 0;
      if(!executeMyBg(pm,proc))
	{
	  *status = WRONG_COMMAND_PARAMETERS;
	  printf("Erreur mybg, le job n'existe pas ou est déjà en arriére plan!\n");
	}
      return 1;
    }
  else if(strcmp(proc->realArgs[0],"myfg") == 0)
    {
      *status = 0;
      if(!executeMyFg(pm,proc))
	{
	  *status = WRONG_COMMAND_PARAMETERS;
	  printf("Erreur myfg, le job n'existe pas ou n'a pas les conditions requises!\n");
	}
      return 1;
    }
    else if(strcmp(proc->realArgs[0],"cd") == 0)
    {
      *status = 0;
      if(!executeCd(proc))
	{
	  *status = WRONG_COMMAND_PARAMETERS;
	}
      return 1;
    }

  return 0;
}

int executeAsMemoryCommand(Processus* proc, int* status)
{
  if(!isAssociatedToMemory(proc->realArgs[0]))
    {
      return 0;
    }
  else
    {
      if(executeCommandInMemory(shell->memory,proc->bruteLine) != 0)
	{
	  *status = WRONG_COMMAND_PARAMETERS;
	  printf("Erreur dans l'execution de la commande -- Plus de place ou syntaxe erronée\n");
	}
      else
	{
	  *status = 0;
	}
      
      return 1;
    }
}

int waitFromChild(ProcessusManager* pm, int pidToWait)
{
  Processus* proc = getProcessusFromPid(pm,pidToWait);
  if(proc == NULL)
    {
      return 1;
    }
  else
    {
      int status;

      if(waitpid(pidToWait,&status,WUNTRACED) == -1)
	{
	  return WAIT_ERROR;
	}
      else
	{
	  if(WIFSTOPPED(status))
	    {
	      printf("La commande \"%s\" devient le job %d et est stoppé\n",proc->bruteLine,proc->jobNumber+1);
	      proc->isStopped = 1;
	      proc->backgroundCondition = 1;
	      proc->jobNumber = pm->nbJobs++;
	      return 0;
	    }
	  else 
	    {
	      pm->lastForegroundJobNumber = proc->jobNumber;
	      if(pm->cmdLastForeground != NULL) free(pm->cmdLastForeground);
	      pm->cmdLastForeground = calloc(strlen(proc->bruteLine) + 5,sizeof(char));
	      strcpy(pm->cmdLastForeground,proc->bruteLine);
	      
	      if(WIFEXITED(status))
		{
		  pm->lastForegroundJobNormallyFinished = 1;
		  pm->lastForegroundJobStatus = WEXITSTATUS(status);
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
		  pm->lastForegroundJobNormallyFinished = 0;
		  proc->isExecuting = 0;
		  proc->hasFinish = 1;
		  return CHILD_END_ERROR;
		}
	    }
	}

      return 0;
    }
}

int prepareForRedirection(Processus* proc)
{
  int standard = 0;
  int error = 0;
  int in = 0;
  int FDFile = -1;
  char* path;
  
  path = calloc(strlen(shell->path) + strlen(proc->redirectionFile) + 5,sizeof(char));
  if(path == NULL)
    {
      return ALLOCATION_ERROR;
    }
  
  sprintf(path,"%s/%s",shell->path,proc->redirectionFile);
  
  if(proc->typeRedirection == ERASE_STANDARD)
    {
      standard = 1;
      FDFile = open(path,O_CREAT|O_WRONLY|O_TRUNC,0770);
    }
  else if(proc->typeRedirection == KEEP_STANDARD)
    {
      standard = 1;
      FDFile = open(path,O_CREAT|O_WRONLY|O_APPEND,0770);
    }
  else if(proc->typeRedirection == ERASE_ERROR)
    {
      error = 1;
      FDFile = open(path,O_CREAT|O_WRONLY|O_TRUNC,0770);
    }
  else if(proc->typeRedirection == KEEP_ERROR)
    {
      error = 1;
      FDFile = open(path,O_CREAT|O_WRONLY|O_APPEND,0770);
    }
  else if(proc->typeRedirection == ERASE_STANDARD_ERROR)
    {
      error = 1;
      standard = 1;
      FDFile = open(path,O_CREAT|O_WRONLY|O_TRUNC,0770);
    }
  else if(proc->typeRedirection == KEEP_STANDARD_ERROR)
    {
      error = 1;
      standard = 1;
      FDFile = open(path,O_CREAT|O_WRONLY|O_APPEND,0770);
    }
  else if(proc->typeRedirection == FROM_FILE)
    {
      FDFile = open(path,O_RDONLY,0660);
      in = 1;
    }

  if(FDFile == -1)
    {
      fprintf(stderr,"Aucun fichier ou dossier de ce type\n");
      return OPEN_ERROR;
    }
  else
    {
      if(error)
	{
	  if(dup2(FDFile,STDERR_FILENO) < 0)
	    {
	      perror("DUP error");
	      return DUP_ERROR;
	    }
	}
      if(standard)
	{
	  if(dup2(FDFile,STDOUT_FILENO) < 0)
	    {
	      perror("DUP error");
	      return DUP_ERROR;
	    }
	}
      if(in)
	{
	  if(dup2(FDFile,STDIN_FILENO) < 0)
	    {
	      perror("DUP error");
	      return DUP_ERROR;
	    }
	}
      
      return 0;
    }
}

int executeProcessus(ProcessusManager* pm, int* exitWanted)
{
  int i;
  
  for(i = 0; i < pm->currentProc; i++)
    {
      Processus* proc = pm->processus[i];
      int pipeFD[2];

      if(pipe(pipeFD) < 0) return PIPE_ERROR;

      if(!proc->hasBeenTreatedAtExec)
	{
	  int canContinue = 1;
	  
	  proc->hasBeenTreatedAtExec = 1;
	  /* Check is processus can go on */
	  if(proc->isExecuting)
	    {
	      canContinue = 0;
	    }
	  if(proc->hasConditionGoOn)
	    {
	      if(proc->conditionGoOn == COND_AND)
		{
		  canContinue = proc->processusConditionGoOn->success;
		}
	      else if(proc->conditionGoOn == COND_OR)
		{
		  canContinue = !proc->processusConditionGoOn->success;
		}
	    }

	  /* Can go on, so build fork */
	  if(canContinue)
	    {
	      asExecMyBgMyFg = 0;
	      int status;
	      int stdoutCopy = dup(STDOUT_FILENO);
	      
	      /* Check if processus hasn't be treated as intern command */
	      if(executeAsInternProcessus(pm,proc,&status,exitWanted))
		{
		  proc->isExecuting = 0;
		  proc->hasFinish = 1;
		  proc->success = (status == 0);

		}
	      else if(executeAsMemoryCommand(proc,&status))
		{
		  proc->isExecuting = 0;
		  proc->hasFinish = 1;
		  proc->success = (status == 0);
		}
	      else
		{
		  pid_t pid;
		  if((pid=fork()) < 0)
		    {
		      return FORK_ERROR;
		    }
		  else if(pid == 0)
		    {
		      close(stdoutCopy);
		      
		      /* Geston du piping */
		      if(proc->shouldBePipedOut)
			{
			  close(pipeFD[0]);
			  if(dup2(pipeFD[1],STDOUT_FILENO) < 0) perror("DUP error");
			}
		      /* Gestion de la redirection */
		      if(proc->shouldBePipedRedirection)
			{
			  if(prepareForRedirection(proc) != 0)
			    {
			      exit(1);
			    }
			}

		      execvp(proc->realArgs[0],proc->realArgs);
		      fprintf(stderr,"La commande \"%s\" est introuvable.\n",proc->realArgs[0]);
		      exit(1);
		    }
		  else
		    { 
		      proc->pid = pid;
		      proc->isExecuting = 1;

		      close(pipeFD[1]);

		      if(proc->shouldBePipedOut)
			{
			  close(stdoutCopy);
			  dup2(pipeFD[0],STDIN_FILENO);
			}
		      else
			{
			  if(dup2(stdoutCopy,STDOUT_FILENO) < 0) perror("DUP error 2");
			  close(stdoutCopy);
			  close(pipeFD[0]);
			}
		      
		      if(proc->backgroundCondition)
			{
			  int status;
			  signal(SIGCHLD,childEndHandler);
			  if(!proc->shouldBePipedOut)
			    {
			      printf("[%d] %d\n",proc->jobNumber,proc->pid);
			    }
			  if(waitpid(pid,&status,WNOHANG) > 0)
			    {
			      proc->isExecuting = 0;
			      proc->hasFinish = 1;
			      
			      if(WIFEXITED(status))
				{
				  printf("\n%s (jobs=[%d],pid=%d) terminé avec status=%d\n",proc->bruteLine,proc->jobNumber,proc->pid,WEXITSTATUS(status));
				  if(WEXITSTATUS(status) == 0)
				    proc->success = 1;
				  else
				    proc->success = 0;
				}
			      else
				{
				  printf("\n%s (jobs=[%d],pid=%d) terminé avec status=%d\n",proc->bruteLine,proc->jobNumber,proc->pid,-1);
				  proc->success = 0;
				}
			    }
			  
			}
		      else
			{
			  sighandler_t previousHandler;
			  pm->isExecutingForegroundTask = 1;
			  pm->pidForegroundTask = pid;
			  previousHandler = signal(SIGCHLD,SIG_DFL);
			  waitFromChild(pm,pid);
			  signal(SIGCHLD,previousHandler);
			}
		    }
		}
	    }
	}
      else
	{
	  proc->success = 0;
	}
    }

  
  return 0;
}

int prepareProcessus(ProcessusManager* pm)
{
  int i;
  int status;
  
  for(i = 0; i < pm->currentProc; i++)
    {
      if(!pm->processus[i]->hasBeenPrepare)
	{
	  if((status=buildArgumentsAndOptions(pm->processus[i])) != 0)
	    {
	      return status;
	    }
	  
	  pm->processus[i]->hasBeenPrepare = 1;
	  if(!isStillProcessusInBackground(pm))
	    {
	      pm->nbJobs = 0;
	    }
	  pm->processus[i]->jobNumber = pm->nbJobs++;
	} 
    }

  return 0;
}

void printProcessusManager(const ProcessusManager* pm)
{
  int i;
  printf("====================\n");
  printf("Processus Manager : \n");
  printf("====================\n");
  for(i = 0; i < pm->currentProc; i++)
    {
      printf("=======================================\n");
      printProcessus(pm->processus[i]);
      printf("=======================================\n");
    }
}
