#ifndef __ERROR__
#define __ERROR__

/*=============================*/
/*            MEMORY           */
/*=============================*/

#define ALLOCATION_ERROR 1
#define REALLOCATION_ERROR 2
#define FORK_ERROR 3
#define EXEC_ERROR 4
#define WAIT_ERROR 5
#define CHILD_END_ERROR 6
#define KILL_ERROR 7
#define DUP_ERROR 8

/*=============================*/
/*           AUTOMATA          */
/*=============================*/

#define BUILD_AUTOMATA_ERROR 10
#define MALFORMED_RE_ERROR 10

/*=============================*/
/*            MYSHELL          */
/*=============================*/

#define INIT_MYSHELL_ERROR 20

/*=============================*/
/*             OTHER           */
/*=============================*/

#define GET_CWD_ERROR 40
#define READING_USER_INPUT_ERROR 41
#define ELEMENT_NOT_IN_ARRAY 42
#define INTERN_COMMAND_ERROR 43
#define PIPE_ERROR 44
#define OPEN_ERROR 45

/*=============================*/
/*         MEMORY DATAS        */
/*=============================*/

#define NOT_VALID_COMMAND 60
#define NEED_KEY_PARAMETER 61
#define WRONG_PARAMETERS 62
#define WRONG_IDENTIFIER 63
#define FTOK_ERROR 64
#define SHMGET_ERROR 65
#define SHMAT_ERROR 66
#define SHMDT_ERROR 67
#define SHMCTL_ERROR 68
#define NO_PLACE_ANYMORE 69
#define TOO_HEAVY_DATA 70
#define SEMGET_ERROR 71
#define SEMCTL_ERROR 72

/*============================*/
/*      COMMAND EXECUTION     */
/*============================*/

#define WRONG_COMMAND_PARAMETERS 80
#define WRONG_REDIRECTION 81
#define BACKGROUND_AT_END 82
#define NO_2_SPECIAL_FOLLOWING 83
#define NEED_COMMAND_BEFORE_SPECIAL 84
#define CONDITIONNAL_CANT_END_CMD 85
#define NEED_FILE_REDIRECTION 86

#endif
