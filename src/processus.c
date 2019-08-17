#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include "processus.h"
#include "error.h"
#include "application.h"
#include "automata.h"

/* Global */
int totalMatches;
/*====================*/
/*        Define      */
/*====================*/

#define NB_MATCHES 100
#define SIZE_PATH 100
#define MAX_ARG_PROCESSUS_BASE 5

/*====================*/
/* Argument Processus */
/*====================*/

ProcessusArgument* constructProcessusArgument(const char* value, ArgumentType type, int* status)
{
  /* Variables */
  ProcessusArgument* processusArgument;
  
  /* Instanciation */
  processusArgument = calloc(1,sizeof(ProcessusArgument));
  if(processusArgument == NULL)
    {
      *status = ALLOCATION_ERROR;
      return NULL;
    }

  /* Initialisation */
  if((*status=initProcessusArgument(processusArgument, value, type)) != 0)
    {
      freeProcessusArgument(processusArgument);
      return NULL;
    }

  *status = 0;
  return processusArgument;
}

int initProcessusArgument(ProcessusArgument* procArg, const char* value, ArgumentType type)
{
  procArg->type = type;
  procArg->value = calloc(strlen(value) + 1, sizeof(char));
  if(procArg->value == NULL)
    {
      return ALLOCATION_ERROR;
    }
  strcpy(procArg->value,value);
  return 0;
}

void freeProcessusArgument(ProcessusArgument* procArg)
{
  if(procArg->value != NULL)
    {
      free(procArg->value);
    }
  free(procArg);
}

/*====================*/
/*      Processus     */
/*====================*/

Processus* constructProcessus(int* status, char* line)
{
  /* Variables */
  Processus* processus;
  
  /* Instanciation */
  processus = calloc(1,sizeof(Processus));
  if(processus == NULL)
    {
      *status = ALLOCATION_ERROR;
      return NULL;
    }

  /* Initialisation */
  if((*status=initProcessus(processus,line)) != 0)
    {
      freeProcessus(processus);
      return NULL;
    }
		     
  *status = 0;
  return processus;
}

int initProcessus(Processus* proc, char* line)
{
  proc->bruteLine = calloc(strlen(line) + 1, sizeof(char));
  if(proc->bruteLine == NULL)
    {
      return ALLOCATION_ERROR;
    }
  strcpy(proc->bruteLine, line);
  
  proc->maxArg = MAX_ARG_PROCESSUS_BASE;
  proc->currentArg = 0;
  proc->args = calloc(proc->maxArg,sizeof(ProcessusArgument*));
  
  if(proc->args == NULL)
    {
      return ALLOCATION_ERROR;
    }
     
  proc->pid = -1;
  proc->jobNumber = -1;
  
  proc->realArgs = NULL;
  proc->currentRealArg = 0;

  proc->hasBeenTreatedAtExec = 0;
  proc->hasBeenPrepare = 0;

  proc->backgroundCondition = 0;
  proc->isExecuting = 0;
  proc->success = 0;
  proc->hasFinish = 0;
  proc->isStopped = 0;
  
  proc->hasConditionGoOn = 0;
  proc->conditionGoOn = COND_NONE;
  proc->processusConditionGoOn = NULL;

  proc->shouldBePipedRedirection = 0;
  proc->redirectionFile = NULL;
  
  return 0;
}

void freeProcessus(Processus* proc)
{
  int i;
  if(proc->args != NULL)
    {
      for(i = 0; i < proc->currentArg; i++)
	{
	  freeProcessusArgument(proc->args[i]);
	}
      free(proc->args);
    }

  if(proc->realArgs != NULL)
    {
      for(i = 0; i < proc->currentRealArg; i++)
	{
	  if(proc->realArgs[i] != NULL)
	    {
	      free(proc->realArgs[i]);
	    }
	}

      free(proc->realArgs);
    }

  if(proc->bruteLine != NULL)
    {
      free(proc->bruteLine);
    }

  if(proc->redirectionFile != NULL)
    {
      free(proc->redirectionFile);
    }
  
  free(proc);
}

int addArgumentToProcessus(Processus* proc, const char* value, ArgumentType type)
{
  int status;
  
  if(proc->maxArg == proc->currentArg)
    {
      proc->maxArg *= 2;
      proc->args = realloc(proc->args, proc->maxArg * sizeof(ProcessusArgument*));
      if(proc->args == NULL)
	{
	  return REALLOCATION_ERROR;
	}
    }

  proc->args[proc->currentArg++] = constructProcessusArgument(value,type,&status);
  if(status != 0)
    {
      proc->currentArg--;
      return status;
    }
  
  return 0;
}

void parsePath(char* deb, char* path, int posDirec, char**res, int* nbRes,int includeHidden,int forMyLs)
{
  /* récupère fichiers concernés par le path (en cas de wildcard notamment) */
  char *request,*tmpNext;
  int i,i2=0;
  char *actualPath,*tmp;

  if(path == NULL){
    return;
  }

  tmp = calloc(strlen(path)+1,sizeof(char));
  strcpy(tmp,path);
  request = strtok(tmp,"/");

  /* mettre dans actualpath tous le chemin jusqu'avant le nom qu'on test */
  if(deb != NULL && strcmp(deb,"")){
    actualPath = calloc(strlen(path)+strlen(deb)+3,sizeof(char));
    actualPath[0] = '\0';
    sprintf(actualPath,"%s",deb);
    if(request != NULL && posDirec > i2){
      if(actualPath[strlen(actualPath)-1] == '/'){
	      actualPath[strlen(actualPath)-1] = '\0';
      }
    }
  } else {
    actualPath = calloc(strlen(path)+3,sizeof(char));
    actualPath[0] = '\0';
  }

  while(request != NULL && i2<posDirec){
    strcat(actualPath,"/");
    strcat(actualPath,request);
    request = strtok(NULL,"/");
    i2++;
  }

  /* si on est pas a la fin */
  if(request != NULL){
    char *nextPath,*next;
    char **matches = calloc(NB_MATCHES,sizeof(char*));
    int nbMatches = 0,status,finished = 0;
    DIR *dir;
    struct dirent *dr;
    if ((dir = opendir(actualPath)) == NULL){
      free(actualPath);
      free(matches);
      free(tmp);
      return;
    }
    Automata* automata = constructAutomata(request,&status);
    /* regarde tous les fichiers du actualpath pour voir si ca match avec celui qu'on test */
    while((dr=readdir(dir)) != NULL){
      if(isRecognizeByAutomata(automata,dr->d_name)){
	matches[nbMatches] = calloc(strlen(dr->d_name)+2,sizeof(char));
	sprintf(matches[nbMatches++],"%s%c",dr->d_name,'\0'); 
      }
      automata->cursorState =0;
    }
    closedir(dir);
    free(dr);
    freeAutomata(automata);

    nextPath = calloc(2,sizeof(char));
    /* on ecrit dans nextPath la suite du path d'actualpath + request */
    request = strtok(NULL,"/");
    if(request == NULL){
      finished = 1;
    }

    while(request != NULL){
      nextPath = realloc(nextPath,(strlen(request)+strlen(nextPath)+2)*sizeof(char));
      strcat(nextPath,"/");
      strcat(nextPath,request);
      request = strtok(NULL,"/");
    }

    /* pour tous les match, on va le repasser dans la fonction pour trouver les résultats 
       finaux */
    for(i=0;i<nbMatches;i++){
      struct stat bufstat;
      
      next = calloc(strlen(actualPath)+strlen(matches[i])+strlen(nextPath)+10,sizeof(char));
      strcat(next,actualPath);
      if(strlen(actualPath) > 0 && actualPath[strlen(actualPath)-1] != '/'){
	strcat(next,"/");
      }
      strcat(next,matches[i]);
      strcat(next,nextPath);
      tmpNext = malloc(sizeof(char)*(strlen(next)+1));
      if(strlen(deb) > 0 && next[strlen(deb)] == '/'){
        strcpy(tmpNext,next+strlen(deb)+1);
      } else {
        strcpy(tmpNext,next+strlen(deb));
      }

      strcpy(next,tmpNext);
      free(tmpNext);
      if(strlen(next)>0 && next[strlen(next)-1]== '/') next[strlen(next)-1]='\0';
      if(!finished){
	      parsePath(deb,next,posDirec+1,res,nbRes,includeHidden,forMyLs);
      } else {
        if((strlen(matches[i]) > 0 && matches[i][0]!='.' && !includeHidden) || includeHidden){
	        if(*nbRes == totalMatches){
	          totalMatches += NB_MATCHES;
	          res = realloc(res,totalMatches);
	        }
	        res[*nbRes] = malloc((strlen(next)+3)*sizeof(char));
            res[*nbRes][0]='\0';

            if(!strcmp(deb,"/") && path != NULL){
                char *tmpNext2 = malloc(sizeof(char)*(2+strlen(next)));
                tmpNext2[0]='\0';
                strcat(tmpNext2,"/");
                strcat(tmpNext2,next);
                strcpy(next,tmpNext2);
                free(tmpNext2);
            }

	        if(lstat(next,&bufstat) == -1)
	        {
	          perror("lstate failed");
	        }
            strcat(res[*nbRes],next);
            if(forMyLs && (bufstat.st_mode & S_IFMT)== S_IFDIR){
	            strcat(res[*nbRes],"/");
            }
	        
	        *nbRes = *nbRes+1;
        }
      }
      free(next);
    }
    free(nextPath);                     

    for(i=0;i<nbMatches;i++){
      free(matches[i]);
    }
    free(matches);
  }
  free(actualPath);
  free(tmp);
}

int loadoptions(char *opts ,int len,int *hide,int *rec){
    int pos=1;
    while(pos<len){
        if(opts[pos] == 'a'){
            *hide = 1;
        } else if(opts[pos] == 'R'){
            *rec = 1;
        } else {
            return -1;
        }
        pos++;
    }
    return 1;
}

int compstr(void const *a,void const *b){
    const char *aa = *(const char **)a;
    const char *bb = *(const char **)b;
    return strcasecmp(aa,bb);
}

void launchParsingPath(char *originalPath,char **res,int *nbRes,int wantOriginal,int forMyLs){
    int saveNbRes;
    char *path = malloc(sizeof(char) * SIZE_PATH);
	  path[0] = '\0';
	  if(strlen(originalPath) >= 2 && originalPath[0] == '/')
	    {
	      path[0] = '/';
	      path[1] = '\0';
	    }
    else if(strlen(originalPath) >= 2 && originalPath[0] == '~'){
            char *tmpPth;
            tmpPth = malloc(sizeof(char)*(strlen(shell->homeDir)+1+strlen(originalPath)));
            tmpPth[0]='\0';
            strcpy(tmpPth,shell->homeDir);
            strcat(tmpPth,originalPath+1);

            if(originalPath !=NULL) free(originalPath);
            originalPath = malloc((strlen(tmpPth)+1)*sizeof(char));
            originalPath[0] = '\0';

            strcpy(originalPath,tmpPth);
            free(tmpPth);
            sprintf(path,"%c",'/');

        } else
	    {
	      sprintf(path,"%s",shell->path);
	    }

    if(strlen(originalPath) == 1 && (originalPath[0] == '~' || originalPath[0] == '/')){
        if(originalPath[0] == '/'){
            if(*nbRes == totalMatches){
	          totalMatches += NB_MATCHES;
	          res = realloc(res,totalMatches);
	        }
            res[*nbRes]=malloc(sizeof(char)*2);
            res[*nbRes][0] ='\0';
            strcat(res[*nbRes],"/");
            *nbRes = *nbRes+1;
        } else if(originalPath[0] == '~'){
            if(*nbRes == totalMatches){
	          totalMatches += NB_MATCHES;
	          res = realloc(res,totalMatches);
	        }
            res[*nbRes]=malloc(sizeof(char)*(5+strlen(shell->homeDir)));
            res[*nbRes][0] ='\0';
            strcat(res[*nbRes],shell->homeDir);
            if(strlen(res[*nbRes]) > 0 && res[*nbRes][strlen(res[*nbRes])-1] != '/'){
                strcat(res[*nbRes],"/");
            }
            *nbRes = *nbRes+1;
        }
    } else {
        saveNbRes = *nbRes;
        parsePath(path,originalPath,0,res,nbRes,0,forMyLs);
      
        if(*nbRes == saveNbRes && wantOriginal){
            if(*nbRes == totalMatches){
	          totalMatches += NB_MATCHES;
	          res = realloc(res,totalMatches*sizeof(char));
	        }
            res[*nbRes]=malloc(sizeof(char)*(5+strlen(originalPath)));
            res[*nbRes][0] ='\0';
            strcat(res[*nbRes],originalPath);
            *nbRes= *nbRes +1;
        }
    }
    free(path);
    if(originalPath != NULL) free(originalPath);
}

int buildArgumentsAndOptions(Processus* proc)
{
  int i;
  int nbArgName = 0;

  char** res;
  int nbRes = 0;
  int hide = 0; int rec = 0;

  int hasAddedAtLeast1 = 0;

  res = calloc(NB_MATCHES,sizeof(char*));
  totalMatches = NB_MATCHES;
  nbRes=0;

  /* On gere le cas ou l'on a un fichier pour la redirection, on doit gérer le wildcard */
  if(proc->shouldBePipedRedirection)
    {
      int k;
      char** res2;
      int nbRes2 = 0;

      char* copy;
      copy = calloc(strlen(proc->redirectionFile)+5,sizeof(char));
      strcpy(copy,proc->redirectionFile);
      
      res2 = calloc(NB_MATCHES,sizeof(char*));

      
      launchParsingPath(copy,res2,&nbRes2,1,0);
      qsort(res2,nbRes2,sizeof(char*),compstr);

      if(proc->redirectionFile != NULL) free(proc->redirectionFile);
      proc->redirectionFile = calloc(strlen(res2[0])+5,sizeof(char));
      strcpy(proc->redirectionFile,res2[0]);
      
      for(k=0;k<nbRes2;k++){
	free(res2[k]);
      }
      free(res2);
    }

  totalMatches = NB_MATCHES;
  
  /* Gestion des options pour le LS */
  if(strcmp(proc->args[0]->value,"myls") == 0)
    {
      for(i = 0; i < proc->currentArg; i++)
	{
	  if(proc->args[i]->type == OPTION)
	    {
	      if(loadoptions(proc->args[i]->value,strlen(proc->args[i]->value),&hide,&rec) == -1)
		{
		  fprintf(stderr,"Option(s) invalide(s)\n");
		  proc->hasBeenTreatedAtExec = 1;
	 	  proc->isExecuting = 0;
		  proc->hasFinish = 1;
		  proc->success = 0;
		  free(res);
		  return 1;
		}
	    }
	}
    }

  if(isAssociatedToMemory(proc->args[0]->value))
    {
      
    }
  else
    {
      /* Gestion des name pour le remplacement des wildcard */
      for(i = 0; i < proc->currentArg; i++)
	{	  
	  if(proc->args[i]->type == NAME)
	    {
	      char* copy;
	      if(proc->args[i]->value[0] == '$')
		{
		  unsigned int p;
		  char* newWord = calloc(1000,sizeof(char));
		  int maxSize = 1000;
		  int itNewWord = 0;

		  char var[500];
		  int itVar = 0;
		  
		  int isReadingVar = 0;
		  int shouldAddPlus = 0;
		  int newVar = 0;
		  
		  newWord[0] = '\0';
		  for(p = 0; p < strlen(proc->args[i]->value); p++)
		    {
		      if(proc->args[i]->value[p] == '+')
			{
			  shouldAddPlus = 1;
			  isReadingVar = 0;
			}
		      else if(proc->args[i]->value[p] == '$')
			{
			  isReadingVar = 1;
			  newVar = 1;
			}
		      else if(isReadingVar)
			{
			  var[itVar++] = proc->args[i]->value[p];
			}

		      if(proc->args[i]->value[p+1] == '\0')
			{
			  isReadingVar = 0;
			}

		      if(newVar)
			{
			  var[itVar++] = '\0';
			  char* res = getValueFromKey(memory,var);
			  if(res != NULL)
			    {
			      if(strlen(res)+itNewWord+1 > 1000)
				{
				  maxSize *= 2;
				  newWord = realloc(newWord,maxSize*sizeof(char));
				}
			      strcat(newWord,res);
			      if(shouldAddPlus)
				{
				  strcat(newWord,"+");
				  shouldAddPlus = 0;
				}
			    }
			  
			  newVar = 0;
			  itVar = 0;
			  var[itVar] = '\0';
			}
		      else if(!isReadingVar)
			{
			  var[itVar++] = '\0';
			  char* res = getValueFromKey(memory,var);
			  if(res != NULL)
			    {
			      if(strlen(res)+itNewWord+1 > 1000)
				{
				  maxSize *= 2;
				  newWord = realloc(newWord,maxSize*sizeof(char));
				}
			      strcat(newWord,res);
			      if(shouldAddPlus)
				{
				  strcat(newWord,"+");
				  shouldAddPlus = 0;
				}
			    }
			  itVar = 0;
			  var[itVar] = '\0';
			}
		    }
		  

		  if(strcmp(newWord,"") != 0)
		    {
		      free(proc->args[i]->value);
		      proc->args[i]->value = calloc(strlen(newWord)+10,sizeof(char));
		      sprintf(proc->args[i]->value,"%s",newWord);
		    }
		  else
		    {
		      continue;
		    }
		  free(newWord);
		}
		  
	      copy = calloc(strlen(proc->args[i]->value) + 20,sizeof(char));
	      strcpy(copy,proc->args[i]->value);
	      nbArgName++;
	      if(strcmp(proc->args[0]->value,"myls") == 0){
		launchParsingPath(copy,res,&nbRes,0,1);
	      } else {
                launchParsingPath(copy,res,&nbRes,1,0);
	      }
	    }
	}
    }
  
  qsort(res,nbRes,sizeof(char*),compstr);

  
  /* Ajout des options  
     Si on est dans myls, les arguments doivent etre recursif puis cache
     Pour les autres on récupére */
  proc->realArgs = calloc(nbRes + 30 + 5, sizeof(char*));

  
  /* Gestion du cas de myps et myls, ceux ci doivent avoir leur chemin en "./myls/myLs" */
  if(strcmp(proc->args[0]->value,"myls") == 0)
    {
      proc->realArgs[proc->currentRealArg] = calloc(strlen(proc->args[0]->value)+strlen(shell->basePath) + 10, sizeof(char));
      sprintf(proc->realArgs[proc->currentRealArg++],"%s/myLs/%s",shell->basePath,proc->args[0]->value);
    }
  else if(strcmp(proc->args[0]->value,"myps") == 0)
    {
      proc->realArgs[proc->currentRealArg] = calloc(strlen(proc->args[0]->value)+strlen(shell->basePath) + 10, sizeof(char));
      sprintf(proc->realArgs[proc->currentRealArg++],"%s/myPs/%s",shell->basePath,proc->args[0]->value);
    }
  else
    {
      proc->realArgs[proc->currentRealArg] = calloc(strlen(proc->args[0]->value)+5, sizeof(char));
      strcpy(proc->realArgs[proc->currentRealArg++],proc->args[0]->value);
    }
		 
  if(strcmp(proc->args[0]->value,"myls") == 0)
    {
      proc->realArgs[proc->currentRealArg] = calloc(5,sizeof(char));
      sprintf(proc->realArgs[proc->currentRealArg++],"%d",rec);
      proc->realArgs[proc->currentRealArg] = calloc(5,sizeof(char));
      sprintf(proc->realArgs[proc->currentRealArg++],"%d",hide);
      proc->realArgs[proc->currentRealArg] = calloc(strlen(shell->path)+50,sizeof(char));
      sprintf(proc->realArgs[proc->currentRealArg++],"%s",shell->path);
    }
  else
    {
      for(i = 0; i < proc->currentArg; i++)
	{
	  if(proc->args[i]->type == OPTION)
	    {
	      proc->realArgs[proc->currentRealArg] = calloc(strlen(proc->args[i]->value) + 1, sizeof(char));
	      strcpy(proc->realArgs[proc->currentRealArg++],proc->args[i]->value);
							   
	    }
	}
    }

  /* Action des noms */
  /* On gére pour myls l'ajout du répertoire courant */
  
  for(i = 0; i < nbRes; i++)
    {      
      proc->realArgs[proc->currentRealArg] = calloc(strlen(res[i]) + 5 ,sizeof(char));
      strcpy(proc->realArgs[proc->currentRealArg++],res[i]);
      hasAddedAtLeast1 = 1;
    }
  if(!hasAddedAtLeast1 && strcmp(proc->args[0]->value,"myls") == 0)
    {
      proc->realArgs[proc->currentRealArg] = calloc(strlen(shell->path)+5 ,sizeof(char));
      sprintf(proc->realArgs[proc->currentRealArg++],"%s/",shell->path);
    }
  
  proc->realArgs[proc->currentRealArg++] = NULL;
  for(i=0;i<nbRes;i++){
    free(res[i]);
  }
  free(res);

  if(nbArgName > 0 && nbRes == 0)
    {
      printf("Aucun fichier ou dossier de ce type\n");
      proc->hasBeenTreatedAtExec = 1;
      proc->hasFinish = 1;
      proc->success = 0;
    }
  
  return 0;
}

void printProcessus(const Processus* proc)
{
  int i;
  printf("-- Processus PID = %d\n",proc->pid);
  printf("-- Processus JOB = %d\n",proc->jobNumber);
  printf("-- Gonna be redirected = %d\n",proc->shouldBePipedRedirection);
  printf("-- In background = %d\n",proc->backgroundCondition);
  printf("-- Arguments: \n");
  printf("------ ");
  for(i = 0; i < proc->currentArg; i++)
    {
      if(proc->args[i]->type == COMMAND)
	{
	  printf("[%s] ",proc->args[i]->value);
	}
      else if(proc->args[i]->type == OPTION)
	{
	  printf("{%s} ",proc->args[i]->value);
	}
      else if(proc->args[i]->type == NAME)
	{
	  printf("(%s) ",proc->args[i]->value);
	}
    }
  printf("\n");
  printf("-- As executable arguments: \n");
  printf("------ ");
  for(i = 0; i < proc->currentRealArg; i++)
    {
      printf("%s ",proc->realArgs[i]);
    }
  printf("\n");
}
