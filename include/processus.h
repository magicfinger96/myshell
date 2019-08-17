#ifndef __PROCESSUS__
#define __PROCESSUS__

enum ArgumentType
  {
    COMMAND,
    OPTION,
    NAME,
    NOT_AN_ARG
  };

enum ConditionGoOn
  {
    COND_AND,
    COND_OR,
    COND_NONE,
    COND_BG
  };

enum RedirectionType
  {
    ERASE_STANDARD,
    KEEP_STANDARD,
    ERASE_ERROR,
    KEEP_ERROR,
    ERASE_STANDARD_ERROR,
    KEEP_STANDARD_ERROR,
    FROM_FILE,
    NONE_REDIRECTION
  };

struct ProcessusArgument
{
  char* value;
  enum ArgumentType type;
};

struct Processus
{
  char* bruteLine;
  
  int maxArg;
  int currentArg;
  struct ProcessusArgument** args;

  int currentRealArg;
  char** realArgs;
  
  int pid;
  int jobNumber;

  int hasBeenTreatedAtExec;
  int hasBeenPrepare;
  int isExecuting;
  int hasFinish;
  int success;
  int backgroundCondition;
  int isStopped;

  int shouldBePipedIn;
  struct Processus* processusToPipeIn;
  int shouldBePipedOut;
  struct Processus* processusToPipeOut;

  int shouldBePipedRedirection;
  enum RedirectionType typeRedirection;
  char* redirectionFile;
  
  int hasConditionGoOn;
  enum ConditionGoOn conditionGoOn;
  struct Processus* processusConditionGoOn;
};

typedef enum RedirectionType RedirectionType;
typedef enum ConditionGoOn ConditionGoOn;
typedef enum ArgumentType ArgumentType;
typedef struct ProcessusArgument ProcessusArgument;
typedef struct Processus Processus;

/*====================*/
/* Argument Processus */
/*====================*/

ProcessusArgument* constructProcessusArgument(const char* value, ArgumentType type, int* status);
int initProcessusArgument(ProcessusArgument* procArg, const char* value, ArgumentType type);
void freeProcessusArgument(ProcessusArgument* procArg);


/*====================*/
/*      Processus     */
/*====================*/

Processus* constructProcessus(int* status, char* line);
int initProcessus(Processus* proc, char* line);
void freeProcessus(Processus* proc);

int addArgumentToProcessus(Processus* proc, const char* value, ArgumentType type);
void setConditionOnGoProcessus(Processus* proc, ConditionGoOn cond, Processus* condProc);

int loadoptions(char* opts, int len, int* hide, int* rec);
int compstr(void const* a, void const* b);
void parsePath(char* deb, char* path, int posDirec,char**res, int* nbRes,int includeHidden,int forMyPs);
int buildArgumentsAndOptions(Processus* proc);
void printProcessus(const Processus* proc);

int buildProperProcessusOption(Processus* proc);
void printProcessus(const Processus* proc);

#endif
