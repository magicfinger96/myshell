#ifndef MYJOB_H
#define MYJOB_H
#define ERR -1
#define errsys(m,n) perror(m),exit(n)
#define MAX_SIZE_JOBS 10
#define SIZE_STATUS_NAME 30
#define MAX_CHARAC_READER_PROC 100
#define SIZE_STATE 22

void getStateCmd(char state[SIZE_STATE],char **cmd,int pid);
void myjobs();

#endif
