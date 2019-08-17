#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "automata.h"
#include "tool.h"
#include "error.h"

/*=============================*/
/*           AUTOMATA          */
/*=============================*/

Automata* constructAutomata(char* descriptor, int* status)
{
  /* Déclaration variables */
  Automata* automata;
  int statusInit;
  int statusBuild;
  char* descriptorCopy;

  /* Recopie du descripteur de l'automate (car on va le modifier */
  descriptorCopy = calloc(strlen(descriptor) + 1, sizeof(char));
  if(descriptorCopy == NULL)
    {
      *status = ALLOCATION_ERROR;
      return NULL;
    }
  strcpy(descriptorCopy,descriptor);
  
  /* Instanciation de l'automate */
  automata = calloc(1,sizeof(Automata));
  if(automata == NULL)
    {
      free(descriptorCopy);
      *status = ALLOCATION_ERROR;
      return NULL;
    }
  
  /* Initialisation de l'automate */
  if((statusInit = initAutomata(automata)) != 0)
    {
      freeAutomata(automata);
      free(descriptorCopy);
      *status = statusInit;
      return NULL;
    }

  /* Construction de l'automate */
  if((statusBuild = buildAutomata(automata,descriptor)) != 0)
    {
      freeAutomata(automata);
      free(descriptorCopy);
      *status = statusBuild;
      return NULL;
    }

  /* Libération des ressources */
  free(descriptorCopy);

  *status = 0;
  return automata;
}

int initAutomata(Automata* a)
{
  a->cursorState = 0;
  a->maxState = MAX_STATE_BASE;
  a->currentState = 0;
  a->states = calloc(a->maxState,sizeof(StateAutomata*));

  return ((a->states == NULL) ? (ALLOCATION_ERROR) : (0)); 
}

void freeAutomata(Automata* a)
{
  int i;
  if(a->states != NULL)
    {
      for(i = 0; i < a->currentState; i++)
	{
	  freeStateAutomata(a->states[i]);
	}
      free(a->states);
    }
  free(a);
}

StateAutomata* createAndAddStateToAutomata(Automata *a, int nextState, int isToken, int isReverse, int* status)
{
  /* Variables */
  StateAutomata* state;
  int stateCreation;
  int stateAdding;
  
  /* Construction */
  state = constructStateAutomata(&stateCreation, nextState, isToken, isReverse);
  if(state == NULL)
    {
      *status = stateCreation;
      return NULL;
    }

  /* Adding state to automata */
  stateAdding = addStateToAutomata(a,state);
  if(stateAdding != 0)
    {
      *status = stateAdding;
      return NULL;
    }

  *status = 0;
  return state;
}

int addStateToAutomata(Automata* a, StateAutomata* state)
{
  if(a->maxState == a->currentState)
    {
      a->maxState *= 2;
      a->states = realloc(a->states, a->maxState * sizeof(StateAutomata*));
      if(a->states == NULL)
	{
	  return REALLOCATION_ERROR;
	}
    }
  a->states[a->currentState++] = state;
  return 0;
}


int buildAutomata(Automata* a, char* description)
{
  char* cursor = description;

  int alreadyStarDetected = 0;

  while(*cursor)
    {
      /* Variables */
      StateAutomata* stateAdded;
      int statusAdded, statusAddCharacter;

      int setDetected = 0, isToken = 0, isReverse = 0, canNextState = 0, mustAdd = 1;
      
      /* On test les différents cas, si on tombe sur '[', 
	 c'est un cas particulier géré en interne, sinon, on va initialiser
	 les différentes variables puis pouvoir construire notre état */
      if(*cursor == '\\')
	{
	  if(*(cursor+1) == '\0')
	    {
	      return MALFORMED_RE_ERROR;
	    }

	  setDetected = 0; alreadyStarDetected = 0; canNextState = 1; isToken = 0;
	  cursor++;
	}
      else if(*cursor == '[')
	{
	  alreadyStarDetected = 0; setDetected = 1; canNextState = 1; isToken = 0;
	  
	  /* Si on trouve un ']', on a bien un ensemble, sinon c'est un caractere simple */
	  if(existCharAfter(cursor,']'))
	    {
	      cursor++;
	      
	      /* On est sur un ensemble vide, l'expression n'est pas correct */
	      /*if(*cursor == ']')
		return MALFORMED_RE_ERROR;*/
	      
	      /* Si l'on tombe en premier sur '^' on doit inverser */
	      if(*cursor == '^')
		isReverse = 1;

	      /* Création de l'état */
	      stateAdded = createAndAddStateToAutomata(a, canNextState, isToken, isReverse,&statusAdded);
	      if(stateAdded == NULL)
		{
		  return statusAdded;
		}

	      /* On va parcourir chaine jusqu'a '[' et ajouter l'état de l'interval */
	      while(*cursor != ']')
		{
		  /* On regarde si l'on a un interval,
		     si oui, on ajoute tout les caracteres reconnaissants */
		  if(*(cursor+1) != '\0' && *(cursor+1) == '-' && *(cursor+2) != '\0' && *(cursor+2) != ']')
		    {
		      char first = *cursor; char second = *(cursor+2);
		      for(;first<=second;first++)
			{
			  if((statusAddCharacter = addRecognizeCharacter(stateAdded,first)) != 0)
			    return statusAddCharacter;
			}

		      cursor += 3;
		    }
		  else
		    {
		      if((statusAddCharacter = addRecognizeCharacter(stateAdded,*cursor)) != 0)
			return statusAddCharacter;
		      cursor++;
		    }
		}
	    }
	  else
	    {
	      setDetected = 0; isToken = 0; isReverse = 0; canNextState = 1;
	    }
	  
	}
      else if(*cursor == '*')
	{
	  if(alreadyStarDetected)
	    mustAdd = 0;
	  alreadyStarDetected = 1; isToken = 1; isReverse = 0; canNextState = 0;
	}
      else if(*cursor == '?')
	{
	  alreadyStarDetected = 0; isToken = 1; isReverse = 0; canNextState = 1;
	}
      else
	{
	  alreadyStarDetected = 0; isToken = 0; isReverse = 0; canNextState = 1;
	}
      
      /* Si l'on ne vient pas de construire un ensemble, alors on va essayer d'ajouter
	 un nouvel état à notre automate avec les valeurs que l'on a renseigner avant */
      if(!setDetected && mustAdd)
	{
	  if((stateAdded = createAndAddStateToAutomata(a,canNextState,isToken,isReverse,&statusAdded)) == NULL)
	    {
	      return statusAdded;
	    }
	  else
	    {
	      if((statusAddCharacter = addRecognizeCharacter(stateAdded,*cursor)) != 0)
		{
		  return statusAddCharacter;
		}
	    }
	}

      /* On incrémente notre curseur */
      cursor++;
    }

  return 0;
}


int isRecognizeByAutomata(Automata* a, char* string)
{
  /* IDEE 
     - On parcours toute la chaine à reconnaitre 'string'
     - A cote de ça, on a le curseur qui pointe sur l'état courant
     - Si on lit un état étoile, on passe au caractere suivant sans changer d'état
     - Si on lit un état étoile et que l'état suivant = caractere, on change d'état
     tout en conservant en mémoire la position de l'ancien état *
     - Lorsqu'on lit un état ? ou état caractere, on avance la position si on match
     - On continue de lire, si l'on rencontre un élément qui ne match pas, on repart en
     état * que l'on était avant
     - Si à la fin, le curseur de l'état est = au nombre d'état alors oK
     - Sinon, on n'est pas arrivé sur un état d'acception, on sort 
  */
  
  char* cursor = string;
  int lastStarStatePosition = -1;
  int isFirst = 1;
  
  while(*cursor)
    {
      StateAutomata* currentState;
      StateAutomata* nextState = NULL;

      /* On doit encore lire des caractere mais l'automate a fini ses états
	 on est donc dans un état de rejet */
      if(a->cursorState == a->currentState)
	{
	  return 0;
	}

      /* On enregistre l'état courant et l'état suivant si celui ci existe */
      currentState = a->states[a->cursorState];
      if(a->cursorState + 1 < a->currentState)
	{
	  nextState = a->states[a->cursorState + 1];
	}

      /* L'état n'est pas reconnu, on regarde si l'on était en état d'étoile avant */
      if(!stateAutomataRecognize(currentState,*cursor,isFirst))
	{
	  if(lastStarStatePosition != -1)
	    {
	      a->cursorState = lastStarStatePosition;
	    }
	  else
	    {
	      return 0;
	    }
	}
      /* L'état est reconnu, on va procéder à l'avancer (ou non) du curseur d'état */
      else
	{
	  /* On doit gérer le cas ou l'on tombe sur une étoile,
	     mais l'état suivant et le dernier diponible
	     dans ce cas, on doit attendre de voir la fin de la chaine pour
	     savoir si il y a un match */
	  if(currentState->isToken && currentState->recognize[0] == '*' && nextState != NULL && a->cursorState + 1 ==  a->currentState -1)
	    {
	      if(*(cursor+1) == '\0' && stateAutomataRecognize(nextState,*cursor,isFirst))
		{
		  return 1;
		}
	    }
	  /* Si l'état suivant reconnu le caractere en cours,
	     On doit augmenter de 2 position notre curseur et aussi
	     mettre notre curseur actuel d'étoile */
	  else if(currentState->isToken && currentState->recognize[0] == '*' && nextState != NULL && stateAutomataRecognize(nextState,*cursor,isFirst))
	    {
	      lastStarStatePosition = a->cursorState;
	      a->cursorState += 2;
	    }

	  /* Cas général */
	  else if(currentState->nextState)
	    {
	       a->cursorState++;
	    }
	}
      
      cursor++;
      isFirst = 0;
    }

  return ( a->cursorState == a->currentState || ((a->cursorState == a->currentState -1) &&(a->states[a->cursorState]->isToken) && (a->states[a->cursorState]->recognize[0] == '*')));
}

/*=============================*/
/*        STATE AUTOMATA       */
/*=============================*/

StateAutomata* constructStateAutomata(int *status, int next, int isToken, int isReverse)
{
  /* Variables */
  StateAutomata* state;
  int statusInit;
  
  /* Instanciation */
  state = calloc(1,sizeof(StateAutomata));
  if(state == NULL)
    {
      *status = ALLOCATION_ERROR;
      return NULL;
    }
  
  /* Initialisation */
  if((statusInit = initStateAutomata(state,next,isToken,isReverse)) != 0)
    {
      freeStateAutomata(state);
      *status = statusInit;
      return NULL;
    }

  *status = 0;
  return state;
}

int initStateAutomata(StateAutomata* state, int next, int isToken, int isReverse)
{
  state->maxRecognize = MAX_RECOGNIZE_SIZE_BASE;
  state->currentRecognize = 0;
  state->recognize = calloc(state->maxRecognize, sizeof(char));
  if(state->recognize == NULL)
    {
      return ALLOCATION_ERROR;
    }
  
  state->inverseCondition = isReverse;
  state->nextState = next;
  state->isToken = isToken;
  return 0;
}

void freeStateAutomata(StateAutomata* state)
{
  if(state->recognize != NULL)
    free(state->recognize);
  
  free(state);
}

int addRecognizeCharacter(StateAutomata* state, char toAdd)
{
  if(state->maxRecognize == state->currentRecognize)
    {
      state->maxRecognize *= 2;
      state->recognize = realloc(state->recognize, sizeof(char) * state->maxRecognize);
      if(state->recognize == NULL)
	{
	  return REALLOCATION_ERROR;
	}
    }
  state->recognize[state->currentRecognize++] = toAdd;
  return 0;
}

void printStateAutomata(StateAutomata* state)
{
  int i;
  printf("State :\n");
  printf("Is token = %d\n",state->isToken);
  printf("Can next = %d\n",state->nextState);
  printf("Recognize : ");
  for(i = 0; i < state->currentRecognize; i++)
    {
      printf("%c , ",state->recognize[i]);
    }
  printf("\n");
}

int stateAutomataRecognize(StateAutomata* state, char token, int isFirst)
{
  if(state->isToken && (state->recognize[0] == '*' || state->recognize[0] == '?'))
    {
        if(token == '.' && isFirst) return 0;
        else return 1;
    }
  else
    {
      int i;
      for(i = 0; i < state->currentRecognize; i++)
	{
	  if(state->recognize[i] == token && state->inverseCondition)
	    return 0;
	  else if(state->recognize[i] == token && !state->inverseCondition)
	    return 1;
	}

      return ((state->inverseCondition) ? (1) : (0));
    }
}

void printAutomata(Automata* a)
{
  int i;
  for(i = 0; i < a->currentState; i++)
    {
      printStateAutomata(a->states[i]);
    }
}

