#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>

#include "myJobs.h"
#include "Shell.h"

void getStateCmd(char state[SIZE_STATE],char **cmd,int pid){
  /* Récupère l'état et la commande lancée d'un processus d'un certain pid */
  int fd,ret=0;
  char c;
  char word[MAX_CHARAC_READER_PROC];
  char filename[SIZE_STATUS_NAME];
  int posW =0;
  sprintf(filename, "/proc/%d/status",pid);
  if ((fd = open(filename, O_RDONLY)) == ERR) errsys("Can't open file !",1);
  while((ret = read(fd,&c,1))){
    if(ret == ERR) errsys("Reading problem",3);
    posW =0;
    while(c!=EOF && c!= ' ' && c!='\t'){
      word[posW++]=c;
      ret = read(fd,&c,1);
    }
    word[posW++]='\0';
    if(!strcmp("Name:",word)){
      posW =0;
      while(c!=EOF && (c== ' ' || c=='\t')) ret = read(fd,&c,1);
      while(c!=EOF && (c!= ' ' && c!='\n' && c!='\t')){
	word[posW++]=c;
	ret = read(fd,&c,1);
      }
      word[posW++]='\0';
      *cmd = malloc(sizeof(char)*posW);
      strcpy (*cmd,word);
    } else if(!strcmp("State:",word)){
      posW =0;
      while(c!=EOF && (c== ' ' || c=='\t')) ret = read(fd,&c,1);
      while(c!=EOF && (c!= ' ' && c!='\t' && c!='\n')){
	word[posW++]=c;
	ret = read(fd,&c,1);
      }
      word[posW++]='\0';
      strcpy (state,word);
      if(!strcmp("T",word)){
	strcpy (state,"Stoppé");
      } else{
	strcpy (state,"En cours d'exécution");
      } 
    }
       
    while(c!=EOF && c!= '\n') ret=read(fd,&c,1);
  }
  close (fd);
}

void myjobs(const MyShell* mySh){
  /* Affiche la liste des processus en arrière plan.
     (numero de job, pid, état du processus, la commande lancée) */
  int tmpPid,i;
  char state[SIZE_STATE];
  char *cmd = NULL;
  printf("%-3s  %-6s  %-36s %s \n","N°","Pid","État","Commande");
  for(i=0;i<mySh->manager->currentProc;i++)
    {
      if(mySh->manager->processus[i]->backgroundCondition)
	{
	  tmpPid = mySh->manager->processus[i]->pid;
	  getStateCmd(state,&cmd,tmpPid);
	  printf("%-3d %-6d  %-36s %s \n",mySh->manager->processus[i]->jobNumber,tmpPid,state,cmd);
	  if (cmd != NULL) free(cmd);
	}
    }
}
