#ifndef __AUTOMATA__
#define __AUTOMATA__

#define MAX_STATE_BASE 10
#define MAX_TOKEN_SIZE 20
#define MAX_RECOGNIZE_SIZE_BASE 10

struct StateAutomata
{
  int maxRecognize;
  int currentRecognize;
  char *recognize;

  int nextState;
  int isToken;
  int inverseCondition;
};


struct Automata
{
  int cursorState;

  struct StateAutomata** states;
  int maxState;
  int currentState;
};

typedef struct Automata Automata;
typedef struct StateAutomata StateAutomata;

StateAutomata* constructStateAutomata(int* status, int next, int isToken, int isReverse);
int initStateAutomata(StateAutomata* state, int next, int isToken, int isReverse);
void freeStateAutomata(StateAutomata* state);

int addRecognizeCharacter(StateAutomata* state, char toAdd);
void printStateAutomata(StateAutomata* state);
int stateAutomataRecognize(StateAutomata* state, char token, int isFirst);

/*=============================*/
/*           AUTOMATA          */
/*=============================*/

Automata* constructAutomata(char* descriptor, int* status);
void freeAutomata(Automata* a);
int initAutomata(Automata* a);

StateAutomata* createAndAddStateToAutomata(Automata* a, int nextState, int isToken, int isReverse,int* status);
int handleSetAutomata(Automata* a, char* description);
int buildAutomata(Automata* a, char* description);
int addStateToAutomata(Automata* a, StateAutomata* state);

int isRecognizeByAutomata(Automata* a, char* string);
void printAutomata(Automata* a);

#endif
