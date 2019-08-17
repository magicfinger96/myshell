#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>

#define NOT_ENOUGH_ARGS 1
#define TOO_MUCH_ARG 2
#define WRONG_ARG_TYPE 3

#define MAX_THINKING 3
#define MIN_THINKING 1

#define MAX_EATING 2
#define MIN_EATING 2

#define couleurnormale()  printf("%s","\033[0m")

/* ==================================== */
/*               Structures             */
/* ==================================== */

/* Philosophe */
struct Philosophe
{
  int isThinking;
  int isEating;
  int isHunger;
  int position;
  char color[20];
};

int initPhilo(struct Philosophe* philo, int position)
{
  philo->isThinking = 0;
  philo->isEating = 0;
  philo->isHunger = 0;
  philo->position = position;
  return 0;
}

struct PhilosopheList
{
  int nbPhilosophe;
  struct Philosophe* philosophes;
};

typedef struct Philosophe Philosophe;
typedef struct PhilosopheList PhilosopheList;

/* ==================================== */
/*           Global variables           */
/* ==================================== */

PhilosopheList listPhilo;
int nbPhilo;
pthread_mutex_t* eachFork;
pthread_mutex_t forkMutex;

/* ==================================== */
/*           Argument Checking          */
/* ==================================== */

int argumentIsDigital(char* arg)
{
  unsigned int i;
  for(i = 0; i < strlen(arg); i++)
    {
      if(!isdigit(arg[i]))
	{
	  return 0;
	}
    }
  return 1;
}

int argumentsChecking(int argc, char* argv[])
{
  if(argc == 1)
    {
      return NOT_ENOUGH_ARGS;
    }
  else if(argc > 2)
    {
      return TOO_MUCH_ARG;
    }
  else
    {
      if(!argumentIsDigital(argv[1]))
	{
	  return WRONG_ARG_TYPE;
	}
    }

  return 0;
}

void test(int position)
{
  Philosophe* left;
  Philosophe* right;
  Philosophe* current;
		      

  /* Assigning right */
  if(position == nbPhilo - 1)
    {
      right = &listPhilo.philosophes[0];
    }
  else
    {
      right = &listPhilo.philosophes[position + 1];
    }

  /* Assigning left */
  if(position == 0)
    {
      left = &listPhilo.philosophes[nbPhilo - 1];
    }
  else
    {
      left = &listPhilo.philosophes[position - 1];
    }

  /* Assigning current */
  current = &listPhilo.philosophes[position];

  /* Testing */
  if(!left->isEating && !right->isEating && current->isHunger)
    {
      current->isHunger = 0;
      current->isEating = 1;
      printf("%s",current->color);printf("Philosophe %d : MANGE\n",position);couleurnormale();
      if(pthread_mutex_unlock(&eachFork[position]) != 0) perror("Error unlock 1");
    }
}

void takeFork(int position)
{
  Philosophe* philo = &listPhilo.philosophes[position];
  
  if(pthread_mutex_lock(&forkMutex) != 0) perror("Error lock 2");
  listPhilo.philosophes[position].isThinking = 0;
  listPhilo.philosophes[position].isHunger = 1;
  printf("%s",philo->color);printf("Philosophe %d : FAIM\n",position);couleurnormale();

  test(position);

  if(pthread_mutex_unlock(&forkMutex) != 0) perror("Error unlock 2");
  if(pthread_mutex_lock(&eachFork[position]) != 0) perror("Error lock 1");
}

void leaveFork(int position)
{
  int left;
  int right;
  Philosophe* philo = &listPhilo.philosophes[position];
    
  if(pthread_mutex_lock(&forkMutex) != 0) perror("Error lock 3\n");
  
  listPhilo.philosophes[position].isEating = 0;
  listPhilo.philosophes[position].isThinking = 1;
  printf("%s",philo->color);printf("Philosophe %d : PENSE\n",position);couleurnormale();

  if(position == 0)
    left = nbPhilo - 1;
  else
    left = position - 1;
  
  if(position == nbPhilo - 1)
    right = 0;
  else
    right = position + 1;

  test(left);
  test(right);
  
  if(pthread_mutex_unlock(&forkMutex) != 0) perror("Error unlock 3");
}

void* philosopheThread(void* arg)
{
  Philosophe* philo = (Philosophe*)arg;
  int position = philo->position;
  unsigned int i;
  
  for(i = 0; i < 2; i++)
    { 
      int randomTimeThinking = rand() % (MAX_THINKING + 1 - MIN_THINKING) + MIN_THINKING;
      int randomTimeEating = rand() % (MAX_EATING + 1 - MIN_EATING) + MIN_EATING;
	    
      /* Thinking time */
      if(i == 0)
	{
	  printf("%s",philo->color);printf("Philosophe %d : SE MET A TABLE ET PENSE\n",position);couleurnormale();
	}
      
      philo->isThinking = 1;
      sleep(randomTimeThinking);

      takeFork(position);
      sleep(randomTimeEating);
      
      leaveFork(position);

    }

  return 0;
}

int main(int argc, char* argv[])
{
  int resultArgChecking;
  pthread_t* threads;
  unsigned int i;
  char* colors[] = {"\033[01;31m","\033[01;32m","\033[01;33m","\033[01;34m","\033[01;35m","\033[01;36m","\033[0m"};

  srand(time(NULL));

  /* ==================================== */
  /*          Arguments checking          */
  /* ==================================== */
  resultArgChecking = argumentsChecking(argc, argv);
  switch(resultArgChecking)
    {
    case NOT_ENOUGH_ARGS:
      fprintf(stderr,"-- Not enough argument --\n");
      return 1;
      break;
    case TOO_MUCH_ARG:
      fprintf(stderr,"-- Too much arguments --\n");
      return 1;
      break;
    case WRONG_ARG_TYPE:
      fprintf(stderr,"-- Wrong argument, please enter integer --\n");
      return 1;
      break;
    }

  /* ==================================== */
  /*      Constructing philosophe list    */
  /* ==================================== */
  nbPhilo = atoi(argv[1]);
  listPhilo.nbPhilosophe = nbPhilo;
  
  listPhilo.philosophes = calloc(nbPhilo, sizeof(Philosophe));
  for(i = 0; i < (unsigned int) nbPhilo; i++)
    {
      initPhilo(&listPhilo.philosophes[i], i);
      if(i <= 5)
	{
	  strcpy(listPhilo.philosophes[i].color,colors[i]);
	}
      else
	{
	  strcpy(listPhilo.philosophes[i].color,colors[6]);
	}
    }
  
  eachFork = calloc(nbPhilo,sizeof(pthread_mutex_t));
  for(i = 0; i < (unsigned int) nbPhilo; i++)
    {
      pthread_mutex_init(&eachFork[i],NULL);
      pthread_mutex_lock(&eachFork[i]);
    }
  pthread_mutex_init(&forkMutex,NULL);
  
  /* ==================================== */
  /*         Lauching philosophes         */
  /* ==================================== */
  threads = calloc(nbPhilo, sizeof(pthread_t));
  for(i = 0; i < (unsigned int) nbPhilo; i++)
    {
      if(pthread_create(&threads[i],NULL,philosopheThread,&listPhilo.philosophes[i]) != 0)
	{
	  fprintf(stderr,"Error at pthread_create of philosophe %d\n",i);
	}
    }

  /* ==================================== */
  /*          Waiting philosophes         */
  /* ==================================== */
  for(i = 0; i < (unsigned int) nbPhilo; i++)
    {
      Philosophe* philo = &listPhilo.philosophes[i];
      if(pthread_join(threads[i],0) != 0)
	{
	  fprintf(stderr,"Error at pthread_join of philosophe %d\n",i);
	}
      printf("%s",philo->color);printf("Philosophe %d : QUITTE LA TABLE\n",i);couleurnormale();
    }

  /* ==================================== */
  /*       Freeing philosophes list       */
  /* ==================================== */
  free(eachFork);
  free(listPhilo.philosophes);
  free(threads);
  return 0;
}
