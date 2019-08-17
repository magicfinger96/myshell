#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>
#include <poll.h>
#include <pwd.h>


#include "tool.h"
#include "Shell.h"
#include "error.h"
#include "processus.h"
#include "automata.h"

MyShell* constructMyShell(int* status)
{
  /* Déclaration variables */
  MyShell* myShell;
  
  /* Instanciation du shell */
  myShell = calloc(1,sizeof(MyShell));
  if(myShell == NULL)
    {
      *status = ALLOCATION_ERROR;
      return NULL;
    }

  /* Initialisation du shell */
  if((*status = initMyShell(myShell)) != 0)
    {
      freeMyShell(myShell);
      return NULL;
    }
  
  *status = 0;
  return myShell;
}

int initMyShell(MyShell* shell)
{
  shell->sizePath = SIZE_PATH_BASE;
  shell->path = NULL;
  shell->basePath = NULL;
  shell->homeDir = NULL;
  shell->pathPrint = NULL;
  shell->path = calloc(shell->sizePath, sizeof(char));
  shell->command = NULL;
  shell->isRunning = 1;
  shell->isRequestingExitValidation = 0;
  shell->hardExit = 0;
  shell->manager = NULL;
  shell->memory = NULL;
 
  if(getenv("HOME") == NULL){
    shell->homeDir = malloc((strlen(getpwuid(getuid())->pw_dir)+1)*sizeof(char));
    strcpy(shell->homeDir,getpwuid(getuid())->pw_dir);
  } else {
    shell->homeDir = malloc((strlen(getenv("HOME"))+1)*sizeof(char));
    strcpy(shell->homeDir,getenv("HOME"));
  }
  
  shell->path = getcwd(shell->path, SIZE_PATH_BASE);
  if(shell->path == NULL)
    return GET_CWD_ERROR;
  shell->basePath = calloc(strlen(shell->path) + 1, sizeof(char));
  strcpy(shell->basePath,shell->path);

  shell->pathPrint = calloc(strlen(shell->path) + 1, sizeof(char));
  strcpy(shell->pathPrint,shell->path);

  /* On va supprimer la partie home */
  
  if(strncmp(shell->path,shell->homeDir,strlen(shell->homeDir)) == 0)
    {
      char* tmp = calloc(strlen(shell->path) + 1,sizeof(char));
      strcpy(tmp,shell->path+strlen(shell->homeDir));
      sprintf(shell->pathPrint,"~%s",tmp);
      free(tmp);
    }
  
  return 0;
}

void freeMyShell(MyShell* shell)
{
  if(shell->basePath != NULL)
    {
      free(shell->basePath);
    }
  if(shell->pathPrint != NULL)
    {
      free(shell->pathPrint);
    }
  if(shell->path != NULL)
    {
      free(shell->path);
    }
  if(shell->manager != NULL)
    {
      freeProcessusManager(shell->manager);
    }
  if(shell->memory != NULL)
    {
      freeMemory(shell->memory);
    }

  if(shell->command != NULL)
    {
      free(shell->command);
    }

  if(shell->homeDir != NULL){
    free(shell->homeDir);
  }

  free(shell);
}

int addElementToContainer(char** container, int* max, int* current, const char* toAdd)
{
  if(*max == *current)
    {
      *max *= 2;
      container = realloc(container, *max * sizeof(char*));
      if(container == NULL)
	{
	  return REALLOCATION_ERROR; 
	}
    }

  container[*current] = calloc(strlen(toAdd)+1,sizeof(char));
  if(container[*current] == NULL)
    {
      return ALLOCATION_ERROR;
    }
  
  strcpy(container[(*current)++],toAdd);
  return 0;
}

int addElementToBuffer(char* buffer, int* max, int* current, char toAdd)
{
  if(*max == *current)
    {
      *max *= 2;
      buffer = realloc(buffer, *max * sizeof(char));
      if(buffer == NULL)
	{
	  return REALLOCATION_ERROR;
	}
    }

  buffer[(*current)++] = toAdd;
  return 0;
}

int isCommandDelimiter(char* element)
{
  char* delimiters[] = {"&&","||",";","&"};
  int nb = 4;
  int i;
  
  for(i = 0; i < nb; i++)
    {
      if(strcmp(delimiters[i],element) == 0)
	{
	  return 1;
	}
    }
  
  return 0;
}

int elementIsSpecialArg(char* line,int* toSkip)
{
  
  if(strncmp(line,"&&",2) == 0)
    {
      *toSkip = 2;
      return 1;
    }
  else if(strncmp(line,"||",2) == 0)
    {
      *toSkip = 2;
      return 1;
    }
  else if(strncmp(line,"|",1) == 0)
    {
      *toSkip = 1;
      return 1;
    }
  else if(strncmp(line,">>",2) == 0)
    {
      *toSkip = 2;
      return 1;
    }
  else if(strncmp(line,"2>",2) == 0)
    {
      *toSkip = 2;
      return 1;
    }
  else if(strncmp(line,"2>>",3) == 0)
    {
      *toSkip = 3;
      return 1;
    }
  else if(strncmp(line,">&",2) == 0)
    {
      *toSkip = 2;
      return 1;
    }
  else if(strncmp(line,">>&",3) == 0)
    {
      *toSkip = 3;
      return 1;
    }
  else if(strncmp(line,">",1) == 0)
    {
      *toSkip = 1;
      return 1;
    }
  else if(strncmp(line,"<",1) == 0)
    {
      *toSkip = 1;
      return 1;
    }
  else
    {
      return 0;
    }
}

int isRedirectArgument(char* line)
{
  if(strncmp(line,">>",2) == 0)
    {
      return 1;
    }
  else if(strncmp(line,"2>",2) == 0)
    {
      return 1;
    }
  else if(strncmp(line,"2>>",3) == 0)
    {
      return 1;
    }
  else if(strncmp(line,">&",2) == 0)
    {
      return 1;
    }
  else if(strncmp(line,">>&",3) == 0)
    {
      return 1;
    }
  else if(strncmp(line,">",1) == 0)
    {
      return 1;
    }
  else if(strncmp(line,"<",1) == 0)
    {
      return 1;
    }
  
  return 0;
}

int isSyntaxllyCorrect(char* command)
{
  /* Variables */
   char* line;
  char* saveLine;

  int specialDetected = 0;
  int commaDotDetected = 0;
  int hasReadAtLeast1 = 0;

  int hasFoundRedirectionSymbol = 0;
  int mustLinkRedirectionSymbol = 0;
  
  /* Allocation variables */
  line = calloc(strlen(command) + 1, sizeof(char));
  if(line == NULL)
    {
      return ALLOCATION_ERROR;
    }
  strcpy(line,command);
  saveLine = line;

  /* Verification */
  removeUselessSpace(line);
  while(*line)
    {
      int hasSkipped = 0;
      int toSkip = 0;
      if(elementIsSpecialArg(line,&toSkip))
	{
	  /* Les element speciaux doivent etre obligatoirement suivi d'au moins un caractere */
	  if(specialDetected || commaDotDetected)
	    {
	      return NO_2_SPECIAL_FOLLOWING;
	    }
	  if(!hasReadAtLeast1)
	    {
	      return NEED_COMMAND_BEFORE_SPECIAL;
	    }

	  if(isRedirectArgument(line))
	    {
	      hasFoundRedirectionSymbol = 1;
	      mustLinkRedirectionSymbol = 1;
	    }
	  
	  
	  specialDetected = 1;
	  hasSkipped = 1;
	  line += toSkip;
	}
      else if(*line == '&')
	{
	  /* Le background doit être le tout dernier element */
	  if(*(line + 1) != '\0')
	    {
	      return BACKGROUND_AT_END;
	    }
	  if(specialDetected || commaDotDetected)
	    {
	      return NO_2_SPECIAL_FOLLOWING;
	    }
	  if(!hasReadAtLeast1)
	    {
	      return NEED_COMMAND_BEFORE_SPECIAL;
	    }

	  hasSkipped = 1;
	  line++;
	}
      else if(*line == ';')
	{
	  /* Le point virgule peut etre tout seul mais ne doit
	     pas etre suivi d'un element special ou d'un & ou d'un ; */
	  if(!hasReadAtLeast1)
	    {
	      return NEED_COMMAND_BEFORE_SPECIAL;
	    }
	  if(specialDetected)
	    {
	      return NO_2_SPECIAL_FOLLOWING;
	    }

	  commaDotDetected = 1;
	  hasSkipped = 1;
	  line++;
	}
      else
	{
	  if(isspace(*line))
	    {
	      line++;
	      continue;
	    }
	  else
	    {
	      if(hasFoundRedirectionSymbol)
		{
		  hasFoundRedirectionSymbol = 0;
		  mustLinkRedirectionSymbol = 0;
		}
	      hasReadAtLeast1 = 1;
	      specialDetected = 0;
	      commaDotDetected = 0;
	    }
	}

      if(!hasSkipped)
	{
	  line++;
	}
    }

  free(saveLine);

  if(specialDetected)
    {
      return CONDITIONNAL_CANT_END_CMD;
    }
  if(hasFoundRedirectionSymbol && mustLinkRedirectionSymbol)
    {
      return NEED_FILE_REDIRECTION;
    }
  
  return 0;
}

int elementIsRedirection(char* line, RedirectionType* type, int* toSkip)
{
  if(strncmp(line,">>",2) == 0)
    {
      *type = KEEP_STANDARD;
      *toSkip = 2;
      return 1;
    }
  else if(strncmp(line,"2>",2) == 0)
    {
      *type = ERASE_ERROR;
      *toSkip = 2;
      return 1;
    }
  else if(strncmp(line,"2>>",3) == 0)
    {
      *type = KEEP_ERROR;
      *toSkip = 3;
      return 1;
    }
  else if(strncmp(line,">&",2) == 0)
    {
      *type = ERASE_STANDARD_ERROR;
      *toSkip = 2;
      return 1;
    }
  else if(strncmp(line,">>&",3) == 0)
    {
      *type = KEEP_STANDARD_ERROR;
      *toSkip = 3;
      return 1;
    }
  else if(strncmp(line,"<",1) == 0)
    {
      *type = FROM_FILE;
      *toSkip = 1;
      return 1;
    }
  else if(strncmp(line,">",1) == 0)
    {
      *type = ERASE_STANDARD;
      *toSkip = 1;
      return 1;
    }

  *toSkip = 0;
  return 0;
}

int analyseCommand(MyShell* shell, char* command)
{
  char* line;
  char* saveLine;

  char* commandCurrent = NULL;
  int maxCurrent = SIZE_ARG_BASE;
  int current = 0;

  char* redirection = NULL;
  int maxRedir = SIZE_ARG_BASE;
  int curRedir = 0;
  
  int it = 0;
  int background = 0;
  int nextProcessus = 0;

  ConditionGoOn oldCondition = COND_NONE;
  ConditionGoOn newCondition = COND_NONE;

  int redirectionCondition = 0;
  RedirectionType redirectionType;
  
  int pipeCondition = 0;
  int hasAddedPipe = 0;
  int hasAddedPipeIt = -1;
  
  /* Allocation variables */
  line = calloc(strlen(command) + 50, sizeof(char));
  if(line == NULL)
    {
      return ALLOCATION_ERROR;
    }
  strcpy(line,command);
  saveLine = line;

  commandCurrent = calloc(maxCurrent, sizeof(char));
  if(commandCurrent == NULL)
    {
      return ALLOCATION_ERROR;
    }

  redirection = calloc(maxRedir,sizeof(char));
  if(redirection == NULL)
    {
      return ALLOCATION_ERROR;
    }
  
  /* On va analyser la ligne compléte */
  removeUselessSpace(line);
  while(*line != '\0')
    {
      int shouldSkip = 0;
      int toSkipRedirection;
      
      /* Gestion des différents cas pour passer à la commande suivante */
      if(elementIsRedirection(line,&redirectionType,&toSkipRedirection))
	{
	  redirectionCondition = 1;
	  line += toSkipRedirection;
	}
      if(*line == ';')
	{
	  shouldSkip = 1;
	  nextProcessus = 1;
	  line++;
	  oldCondition = newCondition;
	  newCondition = COND_NONE;
	}      
      if(*line == '&' && *(line+1) != '\0' && *(line+1) == '&')
	{
	  shouldSkip = 1;
	  nextProcessus = 1;
	  line+=2;
	  oldCondition = newCondition;
	  newCondition = COND_AND;
	}
      if(*line == '&' && *(line+1) == '\0')
	{
	  shouldSkip = 1;
	  nextProcessus = 1;
	  background = 1;
	  line++;
	}
      if(*line == '|' && *(line+1) != '\0' && *(line+1) == '|')
	{
	  shouldSkip = 1;
	  nextProcessus = 1;
	  line+=2;
	  oldCondition = newCondition;
	  newCondition = COND_OR;
	}
      if(*line == '|' && *(line+1) != '|')
	{
	  shouldSkip = 1;
	  nextProcessus = 1;
	  pipeCondition = 1;
	  line++;
	}

      /* Ajout */
      if(!nextProcessus)
	{
	  if(strcmp(redirection,"") != 0 && *line == ' ')
	    {
	      redirectionCondition = 0;
	    }
	  
	  if(redirectionCondition && *line != ' ')
	    {
	      if(curRedir == maxRedir)
		{
		  maxRedir *= 2;
		  redirection = realloc(redirection,maxRedir * sizeof(char));
		  if(redirection == NULL)
		    {
		      return REALLOCATION_ERROR;
		    }
		}
	      redirection[curRedir++] = *line;
	    }
	  else
	    {
	      if(current == maxCurrent)
		{
		  maxCurrent *= 2;
		  commandCurrent = realloc(commandCurrent, maxCurrent * sizeof(char));
		  if(commandCurrent == NULL)
		    {
		      return REALLOCATION_ERROR;
		    }
		}
	      commandCurrent[current++] = *line;
	    }
	}

      /* Si on doit passer au prochain processus, on crée celui ci d'abord */
      if(nextProcessus || *(line+1) == '\0')
	{
	  Processus* previous = NULL;
	  Processus* actual = NULL;
	  
	  if(*(line+1) == '\0')
	    {
	      oldCondition = newCondition;
	    }
	  
	  /* Ajout d'un '\0' pour finir la commande */
	  if(current == maxCurrent)
	    {
	      maxCurrent *= 2;
	      commandCurrent = realloc(commandCurrent, maxCurrent * sizeof(char));
	      if(commandCurrent == NULL)
		{
		  return REALLOCATION_ERROR;
		}
	    }
	  commandCurrent[current++] = '\0';

	  buildProcessus(shell,commandCurrent);
	  actual = shell->manager->processus[shell->manager->currentProc - 1];
	  
	  actual->backgroundCondition = background;
	  /* Gestion de si l'on doit executer selon une condition */
	  if(oldCondition != COND_NONE && it > 0)
	    {
	      previous = shell->manager->processus[shell->manager->currentProc - 2];
	      actual->hasConditionGoOn = 1;
	      actual->conditionGoOn = oldCondition;
	      actual->processusConditionGoOn = previous;
	    }
	  
	  /* Gestion de si l'on doit executer via un pipe */
	  if(pipeCondition)
	    {
	      actual->shouldBePipedOut = 1;
	      /* If hasAddedPipe, it come from the previous one */
	      if(hasAddedPipe)
		{
		  previous = shell->manager->processus[shell->manager->currentProc - 2];
		  actual->shouldBePipedIn = 1;
		  actual->processusToPipeIn = previous;
		  previous->processusToPipeOut = actual;
		}
	      hasAddedPipe = 1;
	      hasAddedPipeIt = it;
	      pipeCondition = 0;
	    }
	  
	  if(hasAddedPipe && hasAddedPipeIt != it)
	    {
	      previous = shell->manager->processus[shell->manager->currentProc - 2];
	      actual->shouldBePipedIn = 1;
	      actual->processusToPipeIn = previous;
	      previous->processusToPipeOut = actual;
	      hasAddedPipe = 0;
	    }

	  /* Gestion de si l'on doit gérer une redirection */
	  if(redirection != NULL && strcmp(redirection,"") != 0)
	    {
	      /* Ajout d'un '\0' pour finir la redirection */
	      if(curRedir == maxRedir)
		{
		  maxRedir *= 2;
		  redirection = realloc(redirection, maxRedir * sizeof(char));
		  if(redirection == NULL)
		    {
		      return REALLOCATION_ERROR;
		    }
		}
	      redirection[curRedir++] = '\0';
	      /* Ajout de la redirection puis reinitialisation */
	      actual->shouldBePipedRedirection = 1;
	      actual->typeRedirection = redirectionType;
	      actual->redirectionFile = calloc(strlen(redirection) + 1,sizeof(char));
	      strcpy(actual->redirectionFile,redirection);
	      
	      redirection[0] = '\0';
	      curRedir = 0;
	    }

	  /* Mise a 0 des variables */
	  background = 0;
	  nextProcessus = 0;
	  current = 0;
	  it++;
	}

      /* Next caracter only with we haven't do it */
      if(!shouldSkip)
	{
	  line++;
	}
    }

  free(redirection);
  free(commandCurrent);
  free(saveLine);
  return 0;
}

int buildProcessus(MyShell* shell, char* original)
{
  char* saveLine;
  char* line;
  
  int nextArgument = 0;
  
  ArgumentType type = COMMAND;
  int maxCurrent = SIZE_ARG_BASE; 
  int current = 0;
  char* argCurrent = NULL;

  Processus* processus;
  int status;

  /* Removing useless space and test if there no empty string */
  removeUselessSpace(original);
  if(strcmp(original,"") == 0 || strcmp(original," ") == 0)
    {
      return 1;
    }

  /* On construit le processus et l'argument */
  line = calloc(strlen(original) + 1, sizeof(char));
  if(line == NULL)
    {
      return ALLOCATION_ERROR;
    }
  strcpy(line,original);
  saveLine = line;
  
  argCurrent = calloc(maxCurrent, sizeof(char));
  if(argCurrent == NULL)
    {
      return ALLOCATION_ERROR;
    }
  
  processus = constructProcessus(&status, original);
  if(processus == NULL)
    {
      return status;
    }
  
  while(*line != '\0')
    {
      /* Gestion des cas speciaux, on va en avoir ici plusieurs
	 - ' ' = on doit passer à l'argument suivant de la commande 
	 - '-' = on doit préciser que le prochain argument est une option
      */
      if(*line == ' ')
	{
	  nextArgument = 1;
	}
      if(*line == '-')
	{
	  type = OPTION;
	}
      
      /* On construit l'argument courant en utilisant le caractere */
      if(!nextArgument)
	{
	  if(maxCurrent == current)
	    {
	      maxCurrent *= 2;
	      argCurrent = realloc(argCurrent, maxCurrent * sizeof(char));
	      if(argCurrent == NULL)
		{
		  return REALLOCATION_ERROR;
		}
	    }
	  argCurrent[current++] = *line;
	}
      
      /* Si on doit passer à l'argument suivant, on ajoute l'argument au processus */
      if(nextArgument || *(line+1) == '\0')
	{
	  /* Ajout du dernier caracter \0' */
	  if(maxCurrent == current)
	    {
	      maxCurrent += 2;
	      argCurrent = realloc(argCurrent, maxCurrent * sizeof(char));
	      if(argCurrent == NULL)
		{
		  return REALLOCATION_ERROR;
		}
	    }
	  argCurrent[current++] = '\0';

	  /* Ajout de l'arguemnt au processus et réinitialisation */
	  addArgumentToProcessus(processus,argCurrent,type);

	  type = NAME;
	  current = 0;
	  nextArgument = 0;
	}
      
      line++;
    }

  addProcessusToManager(shell->manager,processus);
  free(argCurrent);
  free(saveLine);
  return 0;
}

void loopShell(MyShell* shell)
{
  displayMyShell(shell);
  
  while(shell->isRunning)
    {
      struct pollfd ufds[1];

      ufds[0].fd = STDIN_FILENO;
      ufds[0].events = POLLIN;
      
      if(poll(ufds,1,500))
	{
	  if(!shell->isRequestingExitValidation)
	    {
	      readEntire(&shell->command);
	      if(shell->command != NULL)
		{
		  int analyseResult = 0;
		  
		  /* Analysing to check if correct */
		  if((analyseResult=isSyntaxllyCorrect(shell->command)) == 0)
		    {
		      int exitWanted = 0;
		      int stdinCopy = dup(STDIN_FILENO);

		      /* Constructing all differents processus */
		      analyseCommand(shell, shell->command);
		      /* Prepare processus to be executed */
		      prepareProcessus(shell->manager);
		      /* Launch processus line */
		      executeProcessus(shell->manager,&exitWanted);
		      /* Remove all ended procesus */
		      removeFinishedAndTreatedProcessus(shell->manager);
		      shell->manager->isExecutingForegroundTask = 0;
		      
		      if(dup2(stdinCopy,STDIN_FILENO) < 0) perror("DUP ERROR");
		      if(exitWanted)
			{
			  exitAsked(shell,0);
			}

		      free(shell->command);
		      shell->command = NULL;

		      if(shell->isRunning)
			{
			  displayMyShell(shell);
			}
		    }
		  else
		    {
		      fprintf(stderr,"Erreur dans la syntaxe de la commande !\n");
		      if(shell->isRunning)
			{
			  displayMyShell(shell);
			}
		    }
		}
	      else
		{
		  emptyBuffer();
		}
	    }
	  else
	    {
	      shell->isRequestingExitValidation = 0;
	    }
	}
    }
  if(shell->hardExit)
    {
      exitAsked(shell,1);
    }
}

void displayMyShell(MyShell* shell)
{
  fprintf(stdout, "%s>",shell->pathPrint);
  fflush(stdout);
}

int exitAsked(MyShell* shell, int totalExit)
{
  if(totalExit)
    {
      int i;
      for(i = 0; i < shell->manager->currentProc; i++)
	{
	  if(kill(shell->manager->processus[i]->pid,SIGKILL) < 0)
	    {
	      return KILL_ERROR;
	    }
	}
    }
  else
    {
      shell->isRunning = 0;
    }

  return 0;
}
