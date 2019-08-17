#ifndef __MEMORY__
#define __MEMORY__

/*========================*/
/*         Define         */
/*========================*/

#define REAL_SIZE_MEMORY 8192
#define SIZE_MEMORY (REAL_SIZE_MEMORY / 2)
#define SIZE_DATA 1500


/*========================*/
/*        Structures      */
/*========================*/

enum MemoryType
  {
    INTERN,
    EXTERN
  };
typedef enum MemoryType MemoryType;

struct Variable
{
  char* key;
  char* value;
};
typedef struct Variable Variable;

struct DataMemory
{
  int nbElem;
  char datas[SIZE_MEMORY][SIZE_DATA];
};
typedef struct DataMemory DataMemory;

struct SpaceControlMemory
{
  int size;
  int isOccupied;
};
typedef struct SpaceControlMemory SpaceControlMemory;

struct ControlMemory
{
  int nbElem;
  SpaceControlMemory space[REAL_SIZE_MEMORY];
};
typedef struct ControlMemory ControlMemory;

struct InternMemory
{
  Variable** variables;
  int maxVariables;
  int curVariables;
};
typedef struct InternMemory InternMemory;

struct ExternMemory
{
  char pathMemory[100];
  
  int sizeSharedMemory;
  int sizeSharedControl;

  int IDShmMemory;
  int IDShmControl;
  int IDSem;
  
  int projectIDControl;
  int projectIDMem;
  int projectIDSem;

  DataMemory* attachMemoryMem;
  ControlMemory* attachMemoryControl;
};
typedef struct ExternMemory ExternMemory;


struct Memory
{
  InternMemory* internMem;
  ExternMemory* externMem;
};
typedef struct Memory Memory;

/*========================*/
/*         Variable       */
/*========================*/

Variable* createVariable(int* status, char* key, char* value);
void freeVariable(Variable* value);

/*========================*/
/*      Intern Memory     */
/*========================*/

InternMemory* createInternMemory(int* status);
void freeInternMemory(InternMemory* memory);

void printInternMemory(const InternMemory* memory);

/*========================*/
/*      Extern Memory     */
/*========================*/

void* createSharedMemory(Memory* memory, int* status, int keyGen, int* IDShm, int sizeMem);
void destroySharedMemory(int* status, void* attachMem, int IDShm);
int createSemaphore(Memory* memory, int value);
int destroySemaphore(Memory* memory);

ExternMemory* createExternMemory(int* status);
void freeExternMemory(ExternMemory* memory);

void printExternMemory(const ExternMemory* memory);

/*========================*/
/*         Memory         */
/*========================*/

Memory* createMemory(int* status);
void freeMemory(Memory* memory);

int isAssociatedToMemory(char* line);
char* getValueFromKey(Memory* memory, char* key);
char* getValueFromKeyExtern(Memory* memory, char* key);
int getPositionFromKey(Memory* memory, char* key, MemoryType type);

int executeCommandInMemory(Memory* memory, char* line);

int compactMemory(Memory* memory, int sizeCompacting);
int addUpdateControlMmeory(Memory* memory, int sizeWanted);
int removeUpdateControlMemory(Memory* memory, int sizeWanted);

int addVariableToMemory(Memory* memory, char* sentence, MemoryType type);
int addVariableToInternMemory(Memory* memory, char* key, char* value);
int addVariableToExternMemory(Memory* memory, char* key, char* value);

int removeVariableFromMemory(Memory* memory, char* key, MemoryType type);
int removeVariableFromInternMemory(Memory* memory, char* key);
int removeVariableFromExternMemory(Memory* memory, char* key);

#endif
