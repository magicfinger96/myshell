#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>


#include "memory.h"
#include "error.h"
#include "tool.h"

/*========================*/
/*         Defines        */
/*========================*/

#define MAX_BASE_VARIABLES_INTERN 10
#define MAX_BASE_VARIABLES_EXTERN 10

/*========================*/
/*        Variable        */
/*========================*/


Variable* createVariable(int* status, char* key, char* value)
{
  Variable* variable;

  /* Instanciation */
  variable = calloc(1,sizeof(Variable));
  variable->key = NULL;
  variable->value = NULL;
  
  if(variable == NULL)
    {
      *status = ALLOCATION_ERROR;
      return NULL;
    }

  /* Building key */
  variable->key = calloc(strlen(key) + 1, sizeof(char));
  if(variable->key == NULL)
    {
      *status = ALLOCATION_ERROR;
      freeVariable(variable);
      return NULL;
    }
  strcpy(variable->key,key);
  

  /* Building value */
  variable->value = calloc(strlen(value) + 1, sizeof(char));
  if(variable->value == NULL)
    {
      *status = ALLOCATION_ERROR;
      freeVariable(variable);
      return NULL;
    }
  strcpy(variable->value,value);
  
  *status = 0;
  return variable;
}

void freeVariable(Variable* variable)
{
  if(variable != NULL)
    {
      if(variable->key != NULL)
	{
	  free(variable->key);
	}
      
      if(variable->value != NULL)
	{
	  free(variable->value);
	}
      
      free(variable);
    }
}

/*========================*/
/*      Intern Memory     */
/*========================*/

InternMemory* createInternMemory(int* status)
{
  InternMemory* memory;

  /* Instanciation */
  memory = calloc(1,sizeof(InternMemory));
  if(memory == NULL)
    {
      *status = ALLOCATION_ERROR;
      return NULL;
    }

  memory->maxVariables = MAX_BASE_VARIABLES_INTERN;
  memory->curVariables = 0;
  memory->variables = calloc(memory->maxVariables,sizeof(Variable*));

  *status = 0;
  return memory;
}

void freeInternMemory(InternMemory* memory)
{
  if(memory != NULL)
    {
      int i;
      for(i = 0; i < memory->curVariables; i++)
	{
	  freeVariable(memory->variables[i]);
	}
      free(memory->variables);
      
      free(memory);
    }
}

void printInternMemory(const InternMemory* memory)
{
  int i;
  for(i = 0; i < memory->curVariables; i++)
    {
      printf("%s=%s\n",memory->variables[i]->key,memory->variables[i]->value);
    }
}

/*========================*/
/*      Extern Memory     */
/*========================*/

int createSemaphore(Memory* memory, int value)
{
  ExternMemory* mem;
  key_t key;

  mem = memory->externMem;
  /* Clé */
  if((key = ftok(mem->pathMemory,mem->projectIDSem)) == -1)
    {
      return FTOK_ERROR;
    }

  if((mem->IDSem = semget(key,1,IPC_CREAT|IPC_EXCL|0660)) < 0)
    {
      if((mem->IDSem = semget(key,1,0660)) < 0)
	{
	  return SEMGET_ERROR;
	}
    }

  if(semctl(mem->IDSem,0,SETVAL,value) < 0)
    {
      return SEMCTL_ERROR;
    }

  return 0;
}

int destroySemaphore(Memory* memory)
{
  /* Verifier que le semaphore peut etre détruit si c'est le dernier à l'avoir */
  if(semctl(memory->externMem->IDSem, 0, IPC_RMID) < 0)
    {
      return SEMCTL_ERROR;
    }

  return 0;
}

void* createSharedMemory(Memory* memory, int* status, int keyGen, int* IDShm, int sizeMem)
{
  ExternMemory* mem;
  key_t key;
  void* attachMem;
  int isCreator = 0;
  
  mem = memory->externMem;

  /* Création de la clé */
  if((key = ftok(mem->pathMemory,keyGen)) == -1)
    {
      *status = FTOK_ERROR;
      return NULL;
    }

  /* Création du segment mémoire */
  if((*IDShm=shmget(key,sizeMem,IPC_CREAT | IPC_EXCL | 0660)) < 0)
    {
      if((*IDShm=shmget(key,sizeMem, 0660)) < 0)
	{
	  *status = SHMGET_ERROR;
	  return NULL;
	}
    }
  else
    {
      isCreator = 1;
    }

  /* Attachement segment mémoire */
  if((attachMem=shmat(*IDShm,NULL,0)) == (char*)-1)
    {
      *status = SHMAT_ERROR;
      return NULL;
    }

  if(isCreator)
    {
      if(keyGen == 666) /* Control */
	{
	  int i;
	  ((ControlMemory*) attachMem)->nbElem = 1;
	  for(i = 0; i < SIZE_MEMORY; i++)
	    {
	      ((ControlMemory*)attachMem)->space[i].size = -1;
	      ((ControlMemory*)attachMem)->space[i].isOccupied = 0;
	    }
	  ((ControlMemory*)attachMem)->space[0].size = REAL_SIZE_MEMORY;
	  ((ControlMemory*)attachMem)->space[0].isOccupied = 0;
	  
	}
      else
	{
	  ((DataMemory*)attachMem)->nbElem = 0;
	}
    }
  
  *status = 0;
  return attachMem;
}

void destroySharedMemory(int* status, void* attachMem, int IDShm)
{
  struct shmid_ds datas;
  
  /* Détacher le segment */
  if(shmdt(attachMem) < 0)
    {
      *status = SHMDT_ERROR;
      return;
    }

  /* Suppression segment 
     On va supprimer seulement si il ne reste qu'une seul 
     zone attaché à la mémoire partagée */
  if(shmctl(IDShm,IPC_STAT,&datas) < 0)
    {
      *status = SHMCTL_ERROR;
      return;
    }

  if(datas.shm_nattch == 0)
    {
      if(shmctl(IDShm,IPC_RMID,NULL) < 0)
	{
	  *status = SHMCTL_ERROR;
	  return;
	}
    }
  
  *status = 0;
}

ExternMemory* createExternMemory(int* status)
{
  ExternMemory* memory;

  /* Instanciation */
  memory = calloc(1,sizeof(ExternMemory));
  if(memory == NULL)
    {
      *status = ALLOCATION_ERROR;
      return NULL;
    }

  memory->attachMemoryMem = NULL;
  memory->attachMemoryControl = NULL;
  
  memory->sizeSharedMemory = sizeof(DataMemory);
  memory->sizeSharedControl = sizeof(ControlMemory);
  
  memory->projectIDControl = 666;
  memory->projectIDMem = 667;
  memory->projectIDSem = 668;
  
  strcpy(memory->pathMemory,"/bin/ls");
  
  *status = 0;
  return memory;
}

void freeExternMemory(ExternMemory* memory)
{
  if(memory != NULL)
    {   
      free(memory);
    }
}

void printExternMemory(const ExternMemory* memory)
{
  int i;

  for(i = 0; i < memory->attachMemoryMem->nbElem; i++)
    {
      printf("%s\n",memory->attachMemoryMem->datas[i]);
    }
}

/*========================*/
/*         Memory         */
/*========================*/

Memory* createMemory(int* status)
{
  Memory* memory;
  ExternMemory* ext;

  /* Instanciation */
  memory = calloc(1,sizeof(Memory));
  if(memory == NULL)
    {
      *status = ALLOCATION_ERROR;
      return NULL;
    }

  /* Create intern memory */
  if((memory->internMem = createInternMemory(status)) == NULL)
    {
      return NULL;
    }

  /* Create extern memory */
  if((memory->externMem = createExternMemory(status)) == NULL)
    {
      return NULL;
    }

  /* Create shared memory */
  ext = memory->externMem;
  
  ext->attachMemoryMem = (DataMemory*)createSharedMemory(memory,status,ext->projectIDMem,&ext->IDShmMemory,ext->sizeSharedMemory);
  ext->attachMemoryControl = (ControlMemory*) createSharedMemory(memory,status, ext->projectIDControl,&ext->IDShmControl,ext->sizeSharedControl);
  if(createSemaphore(memory,1) != 0)
    {
      printf("Error at creating semaphore\n");
    }

  if(*status != 0)
    {
      printf("Error creating shared memory\n");
    }
  
  *status = 0;
  return memory;
}

void freeMemory(Memory* memory)
{
  if(memory != NULL)
    {
      int status;
      ExternMemory* ext = memory->externMem;
      destroySharedMemory(&status,ext->attachMemoryMem,ext->IDShmMemory);
      if(status != 0)
	{
	  printf("Error destroying shared memory 1\n");
	}

      destroySharedMemory(&status,ext->attachMemoryControl,ext->IDShmControl);
      if(status != 0)
	{
	  printf("Error destroying shared memory 2\n");
	}
      
      if(destroySemaphore(memory) != 0)
	{
	  printf("Error at destroying semaphore\n");
	}
      

      if(memory->internMem != NULL)
	{
	  freeInternMemory(memory->internMem);
	}
      if(memory->externMem != NULL)
	{
	  freeExternMemory(memory->externMem);
	}

      free(memory);
    }
}

int isAssociatedToMemory(char* line)
{
  unsigned int i;
  int j;

  char* elements[] = {"set","setenv","unset","unsetenv"};
  int nbElement = 4;
  
  char firstElem[500];
  int curElem = 0;

  /* Constructing the first element */
  for(i = 0; i < strlen(line); i++)
    {
      if(line[i] == ' ')
	{
	  break;
	}
      else
	{
	  firstElem[curElem++] = line[i];
	}
    }
  firstElem[curElem++] = '\0';

  /* Element is associated to memory ? */
  for(j = 0; j < nbElement; j++)
    {
      if(strcmp(elements[j],firstElem) == 0)
	{
	  return 1;
	}
    }
  
  return 0;
}

int getPositionFromKey(Memory* memory, char* key, MemoryType type)
{
  if(type == INTERN)
    {
      int i;
      for(i = 0; i < memory->internMem->curVariables; i++)
	{
	  if(strcmp(memory->internMem->variables[i]->key,key) == 0)
	    {
	      return i;
	    }
	}
      
      return -1;
    }

  return -1;
}

char* getValueFromKey(Memory* memory, char* key)
{
  /* First check in local memory then environement memory */
  int i;

  for(i = 0; i < memory->internMem->curVariables; i++)
    {
      if(strcmp(memory->internMem->variables[i]->key,key) == 0)
	{
	  return memory->internMem->variables[i]->value;
	}
    }

  for(i = 0; i < memory->externMem->attachMemoryMem->nbElem; i++)
    {
      /* Get key */
      char* equal = strchr(memory->externMem->attachMemoryMem->datas[i],'=');
      int indexEqual = (int)(equal-memory->externMem->attachMemoryMem->datas[i]);
      char* keyMem = calloc(indexEqual + 5,sizeof(char));
      strncpy(keyMem,memory->externMem->attachMemoryMem->datas[i],indexEqual);

      if(strcmp(keyMem,key) == 0)
	{
	  free(keyMem);
	  return (memory->externMem->attachMemoryMem->datas[i]+indexEqual+1);
	}
      else
	{
	  free(keyMem);
	}
    }
  
  return NULL;
}

char* getValueFromKeyExtern(Memory* memory, char* key)
{
  int i;

  for(i = 0; i < memory->externMem->attachMemoryMem->nbElem; i++)
    {
      /* Get key */
      char* equal = strchr(memory->externMem->attachMemoryMem->datas[i],'=');
      int indexEqual = (int)(equal-memory->externMem->attachMemoryMem->datas[i]);
      char* keyMem = calloc(indexEqual + 5,sizeof(char));
      strncpy(keyMem,memory->externMem->attachMemoryMem->datas[i],indexEqual);

      if(strcmp(keyMem,key) == 0)
	{
	  free(keyMem);
	  return (memory->externMem->attachMemoryMem->datas[i]+indexEqual+1);
	}
      else
	{
	  free(keyMem);
	}
    }
  
  return NULL;
}

int executeCommandInMemory(Memory* memory, char* line)
{
  char* copy = NULL;
  char* firstElem = NULL;
  char* datas = NULL;
  char* realDatas = NULL;
  
  copy = calloc(strlen(line) + 1, sizeof(char));
  strcpy(copy,line);
  datas = strchr(line,' ');
  firstElem = strtok(copy," ");

  if(datas != NULL)
    {
      realDatas = calloc(strlen(datas) + 1, sizeof(char));
      strcpy(realDatas,datas+1);
    }
  else
    {
      realDatas = NULL;
    }
  
  /* Test des possibilités */
  if(strcmp(firstElem,"set") == 0)
    {
      int res;
      res = addVariableToMemory(memory,realDatas,INTERN);
      free(copy);
      if(realDatas != NULL) free(realDatas);
      return res;
    }
  else if(strcmp(firstElem,"setenv") == 0)
    {
      int res;
      res = addVariableToMemory(memory,realDatas,EXTERN);
      free(copy);
      if(realDatas != NULL) free(realDatas);
      return res;
    }
  else if(strcmp(firstElem,"unset") == 0)
    {
      int res;
      res = removeVariableFromMemory(memory,realDatas,INTERN);
      free(copy);
      if(realDatas != NULL) free(realDatas);
      return res;
    }
  else if(strcmp(firstElem,"unsetenv") == 0)
    {
      int res;
      res = removeVariableFromMemory(memory,realDatas,EXTERN);
      free(copy);
      if(realDatas != NULL) free(realDatas);
      return res;
    }
  else
    {
      return NOT_VALID_COMMAND;
    }
}

int addVariableToMemory(Memory* memory, char* sentence, MemoryType type)
{
  /* On ne veut faire qu'un affichage des variables */
  if(sentence == NULL)
    {
      if(type == INTERN)
	{
	  printInternMemory(memory->internMem);
	}
      else
	{
	  printExternMemory(memory->externMem);
	}

      return 0;
    }
  else
    {
      char* pair = NULL;

      char* equal;
      int indexEqual;
      int sizeSentence;
      int sizeKey;
      int sizeValue;

      char* key = NULL;
      char* value = NULL;
      
      /* Reconstruct pair value */
      pair = calloc(strlen(sentence) + 1, sizeof(char));
      if(pair == NULL)
	{
	  return ALLOCATION_ERROR;
	}
      strcpy(pair,sentence);

      /* On doit récupérer ce qu'il y a avant le premier = et aprés le premier =
	 Ce qu'il y a avant le = c'est la clé,
	 Ce qu'il y a aprés le = c'est la valeur
      */
      
      equal = strchr(sentence,'=');
      indexEqual = (int)(equal-sentence);

      sizeSentence = strlen(sentence) + 1;
      sizeValue = sizeSentence - indexEqual;
      sizeKey = sizeSentence - sizeValue;
      
      key = calloc(sizeKey + 10,sizeof(char));
      value = calloc(sizeValue + 10, sizeof(char));
      strncpy(key,sentence,sizeKey);
      strcpy(value,sentence+indexEqual+1);
      
      /* On regarde si le split c'est passé correctement */
      if(key == NULL || value == NULL || strcmp(key,"") == 0 || strcmp(value,"") == 0)
	{
	  if(pair != NULL) free(pair);
	  if(key != NULL) free(key);
	  if(value != NULL) free(value);
	  
	  return WRONG_PARAMETERS;
	}
      else
	{
	  /* On regarde si la valeur qu'on veut ajouter n'est pas une référence à
	     une autre variable, dans ce cas ou dit donner la valeur de la variable */
	  if(value[0] == '$')
	    {
	      char* valueFromKey;
	      char* newValue;
	      
	      newValue = calloc(strlen(value) + 1, sizeof(char));
	      if(newValue == NULL)
		{
		  if(pair != NULL) free(pair);
		  if(key != NULL) free(key);
		  if(value != NULL) free(value);
		  return ALLOCATION_ERROR;
		}
	      strcpy(newValue,value+1);

	      valueFromKey = getValueFromKey(memory,newValue);
	      if(valueFromKey != NULL)
		{
		  if(value != NULL) free(value);

		  value = calloc(strlen(valueFromKey) + 1, sizeof(char));
		  strcpy(value,valueFromKey);

		  if(newValue != NULL) free(newValue);
		}
	      else
		{
		  if(newValue != NULL) free(newValue);
		  if(pair != NULL) free(pair);
		  if(key != NULL) free(key);
		  if(value != NULL) free(value);
		  return WRONG_IDENTIFIER;
		}
	    }
	  
	  if(type == INTERN)
	    {
	      int res = addVariableToInternMemory(memory,key,value);
	      if(pair != NULL) free(pair);
	      if(key != NULL) free(key);
	      if(value != NULL) free(value);
	      return res;
	    }
	  else
	    {
	      int res = addVariableToExternMemory(memory, key, value);
	      if(pair != NULL) free(pair);
	      if(key != NULL) free(key);
	      if(value != NULL) free(value);
	      return res;
	    }
	  
	  return 1;
	}
    }
}

int addVariableToInternMemory(Memory* memory, char* key, char* value)
{
  InternMemory* intern = memory->internMem;
  int status = 0;

  removeUselessSpace(key);
  removeUselessSpace(value);
  
  if(key != NULL && value != NULL)
    {
      /* On regarde que si la clé existe, on la remplace juste */
      int positionKey = -1;
      positionKey = getPositionFromKey(memory,key,INTERN);
      if(positionKey != -1)
	{
	  free(intern->variables[positionKey]->value);
	  intern->variables[positionKey]->value = calloc(strlen(value) + 1, sizeof(char));
	  strcpy(intern->variables[positionKey]->value,value);
	  return 0;
	}
      
      /* Elle n'existe pas, on peut l'ajouter */
      if(intern->maxVariables == intern->curVariables)
	{
	  intern->maxVariables *= 2;
	  intern->variables = realloc(intern->variables, intern->maxVariables * sizeof(Variable*));
	  if(intern->variables == NULL)
	    {
	      return REALLOCATION_ERROR;
	    }
	}
      
      intern->variables[intern->curVariables++] = createVariable(&status,key,value);
      return status;
    }
  else
    {
      return WRONG_PARAMETERS;
    }
}

int addUpdateControlMemory(Memory* memory, int sizeWanted)
{
  ExternMemory* ext = memory->externMem;
  ControlMemory* ctrl = ext->attachMemoryControl;
  int i,j;
 
  /* On regarde tout d'abord si il existe une case qui pourrait contenir */
  for(i = 0; i < ctrl->nbElem; i++)
    {
      if(ctrl->space[i].size == sizeWanted && !ctrl->space[i].isOccupied)
	{
	  ctrl->space[i].isOccupied = 1;
	  return 1;
	}
    }

  /* Rien ne pouvait contenir le string, on va donc le essayer de subdiviser */
  /* On va aussi s'assurer qu'on pourra subdiviser en regardant s'il
     reste des cases libres */
  
  for(;;)
    {
      int hasSubdivide = 0;
      if(ctrl->nbElem == SIZE_MEMORY)
	{
	  return 0;
	}
	
      for(i = 0; i < ctrl->nbElem; i++)
	{
	  if(ctrl->space[i].size > sizeWanted && !ctrl->space[i].isOccupied)
	    {
	      for(j = ctrl->nbElem; j > i; j--)
		{
		  ctrl->space[j].isOccupied = ctrl->space[j-1].isOccupied;
		  ctrl->space[j].size = ctrl->space[j-1].size;
		}
	      ctrl->space[i+1].size /= 2;
	      ctrl->space[i].size /= 2;
	      ctrl->nbElem++;
	      hasSubdivide = 1;
	      break;
	    }
	  else if(!ctrl->space[i].isOccupied && ctrl->space[i].size == sizeWanted)
	    {
	      ctrl->space[i].isOccupied = 1;
	      return 1;
	    }
	}

      if(!hasSubdivide)
	{
	  return 0;
	}
    }

  return 0;
}

int compactMemory(Memory* memory, int sizeCompacting)
{
  ExternMemory* ext = memory->externMem;
  ControlMemory* ctrl = ext->attachMemoryControl;
  int i;
  int case1 = -1;
  int case2 = -1;

  for(i = 0; i < ctrl->nbElem; i++)
    {
      if(ctrl->space[i].size == sizeCompacting && !ctrl->space[i].isOccupied)
	{
	  case1 = i;
	}
    }

  for(i = ctrl->nbElem - 1; i >= 0; i--)
    {
      if(ctrl->space[i].size == sizeCompacting && !ctrl->space[i].isOccupied)
        {
	  case2 = i;
	}
    }
  
  if(case1 != -1 && case2 != -1 && case1 != case2)
    {
      int j;

      ctrl->space[case2].size *= 2;
      for(j = case1; j < ctrl->nbElem; j++)
	{
	  ctrl->space[j].isOccupied = ctrl->space[j+1].isOccupied;
	  ctrl->space[j].size = ctrl->space[j+1].size;
	}
      ctrl->nbElem--;
      
      return compactMemory(memory,sizeCompacting*2);
    }
  else
    {
      return 1;
    }
}

int removeUpdateControlMemory(Memory* memory, int sizeWanted)
{
  int i;
  int caseToEmpty = -1;
  ExternMemory* ext = memory->externMem;
  ControlMemory* ctrl = ext->attachMemoryControl;
  
  /* On sait que si i a été libéré, on va essayer de repasser dans la mémoire voir si une autre
     case de même taille peut etre fusionné */

  for(i = 0; i < ctrl->nbElem; i++)
    {
      if(ctrl->space[i].size == sizeWanted && ctrl->space[i].isOccupied)
	{
	  caseToEmpty = i;
	}
    }
  
  if(caseToEmpty == -1)
    {
      return 0;
    }
  else
    {
      ctrl->space[caseToEmpty].isOccupied = 0;
      compactMemory(memory,sizeWanted);

      
      return 1;
    }
     
}

int addVariableToExternMemory(Memory* memory, char* key, char* value)
{
  struct sembuf up = {0,1,0};
  struct sembuf down = {0,-1,0};
  ExternMemory* ext = memory->externMem;

  removeUselessSpace(key);
  removeUselessSpace(value);

  if(key != NULL && value != NULL)
    {
      /* Accés exclusing pour modifier la mémoire */
      /* On commande par le controle de mémoire */
      char* reconstruct = calloc((strlen(key)+1)+(strlen(value)+1) + 2, sizeof(char));
      sprintf(reconstruct,"%s=%s",key,value);
      int sizeRequire = strlen(reconstruct);
      int effective = getMinusPowOf2(sizeRequire);

      /* On regarde si la clé existe déjà */
      if(getValueFromKeyExtern(memory,key) != NULL)
	{
	  removeVariableFromExternMemory(memory,key);
	}

      if(strlen(reconstruct) > SIZE_DATA)
	{
	  return TOO_HEAVY_DATA; 
	}

      semop(ext->IDSem,&down,1);

      if(addUpdateControlMemory(memory,effective))
	{
	  /* Update has worked, we should add our variable */
	  strcpy(ext->attachMemoryMem->datas[ext->attachMemoryMem->nbElem],reconstruct);
	  ext->attachMemoryMem->nbElem++;
	}
      
      free(reconstruct);
      semop(ext->IDSem,&up,1);
      return 0;
    }
  
  return 0;
}

int removeVariableFromMemory(Memory* memory, char* key, MemoryType type)
{
  /* On a une erreur, car unset est toujour suivi de l'identifiant */

  removeUselessSpace(key);
  if(key == NULL)
    {
      return NEED_KEY_PARAMETER;
    }
  else
    {
      if(key[0] != '$' || strcmp(key,"") == 0)
	{
	  return WRONG_PARAMETERS;
	}
      else
	{
	  char* realKey = calloc(strlen(key) + 1, sizeof(char));
	  strcpy(realKey,key+1);
	  
	  if(type == INTERN)
	    {
	      removeVariableFromInternMemory(memory,realKey);
	    }
	  else
	    {
	      removeVariableFromExternMemory(memory,realKey);
	    }
	  
	  free(realKey);
	}
      return 0;
    }
}

int removeVariableFromInternMemory(Memory* memory, char* key)
{
  int i;
  int toRemove = -1;
  Variable* tmp;
  InternMemory* intern = memory->internMem;

  for(i = 0; i < intern->curVariables; i++)
    {
      if(strcmp(intern->variables[i]->key,key) == 0)
	{
	  toRemove = i;
	}
    }

  if(toRemove == -1)
    {
      return WRONG_PARAMETERS;
    }

  tmp = intern->variables[intern->curVariables - 1];
  intern->variables[intern->curVariables - 1] = intern->variables[toRemove];
  intern->variables[toRemove] = tmp;
  
  freeVariable(intern->variables[intern->curVariables - 1]);
  intern->curVariables--;

  return 0;
}

int removeVariableFromExternMemory(Memory* memory, char* key)
{
  /* We should first try to find a case in control memory of the size of key-value
     and after, we can remove the variable from data memory */
  struct sembuf up = {0,1,0};
  struct sembuf down = {0,-1,0};

  DataMemory* data = memory->externMem->attachMemoryMem;
  char* value = getValueFromKeyExtern(memory,key);
  char* reconstruct;
  int effectiveSize;
  
  if(value == NULL)
    {
      return WRONG_PARAMETERS;
    }

  reconstruct = calloc(strlen(value) + strlen(key) + 5, sizeof(char));
  sprintf(reconstruct,"%s=%s",key,value);
  effectiveSize = getMinusPowOf2(strlen(reconstruct));

  semop(memory->externMem->IDSem,&down,1);

  /* Trying to get emplacement in control memory */
  if(removeUpdateControlMemory(memory,effectiveSize))
    {
      int i;
      for(i = 0; i < data->nbElem; i++)
	{
	  if(strcmp(reconstruct,data->datas[i]) == 0)
	    {
	      char* toCopy = calloc(strlen(data->datas[data->nbElem-1])+1, sizeof(char));
	      strcpy(toCopy,data->datas[data->nbElem-1]);
	      strcpy(data->datas[i],toCopy);
	      memory->externMem->attachMemoryMem->nbElem--;
	      free(toCopy);
	      free(reconstruct);
	      semop(memory->externMem->IDSem,&up,1);
	      return 1;
	    }
	}

      free(reconstruct);
      semop(memory->externMem->IDSem,&up,1);
      return 0;
    }
  else
    {
      free(reconstruct);
      semop(memory->externMem->IDSem,&up,1);
      return 0;
    }
  
}
