#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <time.h>
#include <sys/times.h>
#include <pwd.h>

#define ERR -1
#define errsys(m,n) perror(m),exit(n)
#define SIZE_PID 7
#define SIZE_STATUS_NAME 50
#define SIZE_COMMAND_NAME 20
#define SIZE_CMDLINE 1000
#define SIZE_START 100
#define MAX_CHARAC_READER_PROC 100

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_DARK    "\x1B[30m"
#define COLOR_RESET   "\033[0m"

int uptime;
struct tm timeNow,*tm;

int isNumber(char *c){
    /* indique si une chaine de caractère est un nombre */
    unsigned int i;
    for(i=0;i<strlen(c);i++){
        if(!isdigit(c[i])){
            return 0;
        }
    }
    return 1;
}

char* displayStart(int sec){
    /* affiche la date ou l'heure à laquelle a commencé un processus */
    time_t start;
    struct tm timeStart;
    char *display = malloc(sizeof(char)*SIZE_START);
    const char * months[12] = {"jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sept", "oct", "nov", "dec"};
    start = time(0)-sec;

    if ((tm = localtime (&start)) == NULL) errsys("Can't get the start time !",5);
    memcpy(&timeStart,tm,sizeof(*tm));
    if(timeStart.tm_year != timeNow.tm_year){
        sprintf(display,"%4d",timeStart.tm_year+1900);
    } else if(timeStart.tm_mon != timeNow.tm_mon || timeStart.tm_mday != timeNow.tm_mday){
        sprintf(display,"%s.%02d",months[timeStart.tm_mon],timeStart.tm_mday);
    } else {
        sprintf(display,"%02d:%02d",timeStart.tm_hour,timeStart.tm_min);
    }
    
    return display;
}

void parseStatusFile(char *pid,int *uid,int *rss){
    /* Lis le fichier status */
    int fd,ret=0;
    char c;
    char word[MAX_CHARAC_READER_PROC];
    char filename[SIZE_STATUS_NAME];
    int posW =0;
    *rss = 0;
    sprintf(filename, "/proc/%s/status",pid);
    if ((fd = open(filename, O_RDONLY)) == ERR) errsys("Status file open error !",6);
    while((ret = read(fd,&c,1))){
		if(ret == ERR) errsys("Reading status file problem",7);
        posW =0;
        while(c!=EOF && c!= ' ' && c!='\t'){
            word[posW++]=c;
            ret = read(fd,&c,1);
        }
        word[posW++]='\0';
        if(!strcmp("Uid:",word)){
            posW =0;
            while(c!=EOF && (c== ' ' || c=='\t')) ret = read(fd,&c,1);
            while(c!=EOF && (c!= ' ' && c!='\n' && c!='\t')){
                word[posW++]=c;
                ret = read(fd,&c,1);
            }
            word[posW++]='\0';
            if(!strcmp("",word)) errsys("No Uid found !",8);
            *uid = atoi(word);
        
        } else if(!strcmp("VmRSS:",word)){
            posW =0;
            while(c!=EOF && (c== ' ' || c=='\t')) ret = read(fd,&c,1);
            while(c!=EOF && (c!= ' ' && c!='\n' && c!='\t')){
                word[posW++]=c;
                ret = read(fd,&c,1);
            }
            word[posW++]='\0';
            if(!strcmp("",word)){
                *rss=0;
            } else {
                *rss = atoi(word);
            }
        }
       
        while(c!=EOF && c!= '\n') ret=read(fd,&c,1);
	}
    if (uid == NULL) errsys("No Uid found !",12);
    close (fd);
}

void displayInfos(char *pid, int maxCaracInLine){
    /* affiche les informations du processus d'un certain pid */
    int fd,starttime,nothing,elapsed,uid,ret,i,stime,utime,vsz,memtotal,rss,timems[2],ttynr;
    float cpu,mem;
    char state,command[SIZE_COMMAND_NAME],*start,*cmdline,c,timec[8],tty[8],color[8];
    char filename[SIZE_STATUS_NAME],buf[1500],*left,*word,*right;
    struct stat bufstat;
    FILE * file;
    DIR *dir;
    struct dirent *dr;
    struct passwd * pw;
    *command = '\0';
    *timec = '\0';
    *tty = '\0';
    cmdline = malloc((maxCaracInLine-78)*sizeof(char));
    *cmdline = '\0';

    /* Lecture du fichier stat */
    sprintf(filename, "/proc/%s/stat",pid);
    if ((fd = open(filename, O_RDONLY)) == ERR) errsys("Can't open file 'stat' (open()) !",2);
    file = fdopen(fd,"r");
    if(file == NULL) errsys("Can't open file (fdopen()) !",3);
    fgets(buf,1499,file);
    left = buf;
    word = strtok(buf,"(");
    word = strtok(NULL,"(");
    word = strtok(word,")");
    right = strtok(NULL,")");
    sscanf(left, "%d ",&starttime);
    strcpy(command,word);
    sscanf(right, " %c %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",&state, &nothing, &nothing, &nothing,&ttynr, &nothing,&nothing, &nothing,&nothing, &nothing,&nothing, &utime,&stime, &nothing,&nothing, &nothing,&nothing, &nothing,&nothing, &starttime,&vsz, &nothing,&nothing, &nothing,&nothing, &nothing,&nothing, &nothing,&nothing,&nothing,&nothing,&nothing,&nothing,&nothing,&nothing,&nothing,&nothing,&nothing,&nothing,&nothing,&nothing,&nothing,&nothing,&nothing,&nothing,&nothing,&nothing,&nothing,&nothing,&nothing);
    fclose(file);
    close(fd);    

    /* Définition de la couleur de la ligne */
    /* D R S T t X Z */
    switch(state){
        case 'D':
            sprintf(color,"%s",COLOR_YELLOW);
            break;
        case 'R':
            sprintf(color,"%s",COLOR_MAGENTA);
            break;
        case 'S':
            sprintf(color,"%s",COLOR_GREEN);
            break;
        case 'T':
            sprintf(color,"%s",COLOR_CYAN);
            break;
        case 't':
            sprintf(color,"%s",COLOR_BLUE);
            break;
        case 'X':
            sprintf(color,"%s",COLOR_RED);
            break;
        case 'Z':
            sprintf(color,"%s",COLOR_DARK);
            break;
        default:
           sprintf(color,"%s",COLOR_RESET);
           break;
    }

    /* Récupère START */    
    elapsed = uptime-(starttime/sysconf(_SC_CLK_TCK));
    start = displayStart(elapsed);

    /* Calcule %CPU */
    cpu = (stime+utime)/(float)uptime;
    parseStatusFile(pid,&uid,&rss);
    pw = getpwuid(uid);
    if(pw == NULL) errsys("Pwuid not found !",8);
    if(strlen(pw->pw_name)>8){
        pw->pw_name[7] = '+';
        pw->pw_name[8] = '\0';
    }

    /* Lecture de la mémoire total */
    if ((fd = open("/proc/meminfo", O_RDONLY)) == ERR) errsys("Can't open file 'meminfo' !",10);
    file = fdopen(fd,"r");
    if(file == NULL) errsys("Can't open 'meminfo' (fdopen()) !",11);
    fscanf(file, "MemTotal: %d",&memtotal);
    fclose(file);
    close(fd);

    /* ouverture du dossier /dev/pts pour récuperer tty */
    if ((dir = opendir("/dev/pts/")) == NULL) errsys("Can't open /dev/pts directory !",11);
    while((dr=readdir(dir)) != NULL){
        if(isNumber(dr->d_name)){
            sprintf(filename,"/dev/pts/%s",dr->d_name);
            stat(filename,&bufstat);
            if((signed)bufstat.st_rdev == ttynr){
                sprintf(tty,"pts/%s",dr->d_name);
            }
        }
    }
    closedir(dir);
    dir=NULL;

    /* Ouverture du dossier /dev pour récupérer tty si pas trouvé dans /dev/pts */
    if(strlen(tty) == 0){
        if ((dir = opendir("/dev")) == NULL) errsys("Can't open dev directory !",12);
        while((dr=readdir(dir)) != NULL){
            if(strlen(dr->d_name) >= 3 && !strncmp (dr->d_name,"tty",3) && isNumber(dr->d_name+3)){
                sprintf(filename,"/dev/%s",dr->d_name);
                stat(filename,&bufstat);
                if((signed)bufstat.st_rdev == ttynr){
                    sprintf(tty,"%s",dr->d_name);
                }
            }
        }
        closedir(dir);
        dir=NULL;
    }

    if(strlen(tty) == 0) sprintf(tty,"?");

    /* Réouverture du dossier /proc/pid */
    sprintf(filename,"/proc/%s",pid);
    if ((dir = opendir(filename)) == NULL) errsys("Can't open /proc/pid directory !",12);
    dr=readdir(dir);

    /* Écriture de VSZ et %MEM */
    vsz = vsz / 1024.0;
    mem = ((float)rss*100.0)/((float)memtotal);
    if(mem > 99) mem=99;

    /* Écriture de TIME */
    timems[1] = (utime+stime)/sysconf(_SC_CLK_TCK);
    timems[0] = timems[1]/60;
    timems[1] = timems[1]%60;
    if (timems[0] > 9999){
        sprintf(timec, ">9999:%02d",timems[1]);
    } else {
        sprintf(timec, "%4d:%02d",timems[0],timems[1]);
    }

    /* Récupere cmdline afin d'afficher COMMAND */
    sprintf(filename, "/proc/%s/cmdline",pid);
    
    if ((fd = open(filename, O_RDONLY)) == ERR) errsys("Can't open file 'cmdline' (open()) !",9);
    i=0;
    while((ret = read(fd,&c,1)) && i< (maxCaracInLine-78)){
	    if(c == '\00'){
            cmdline[i++]=' ';
        } else {
            cmdline[i++]=c;
        }
	}
    if (i != 0) cmdline[i-1]='\0';

    /* Affiche les informations d'un processus avec PID pid */
    if(cmdline != NULL && strcmp(cmdline,"")!= 0){
        printf("%s%-8s %8s %5.1f %5.1f %8d %8d %-8s %c    %6s %8s %s\n%s",color,pw->pw_name,pid,cpu,mem,vsz,rss,tty,state,start,timec,cmdline,COLOR_RESET);
    } else {
        if(command[0]=='(' && command[strlen(command)-1]==')'){
            command[0]='[';
            command[strlen(command)-1]=']';
        }
        printf("%s%-8s %8s %5.1f %5.1f %8d %8d %-8s %c    %6s %8s %s\n%s",color,pw->pw_name,pid,cpu,mem,vsz,rss,tty,state,start,timec,command,COLOR_RESET);
    }
    closedir(dir);
    close(fd);
    free(start);
    free(cmdline);
   
}

void myps(){
    /* affiche les processus en cours et leurs informations */
    DIR *dir;
    struct dirent *dr;
    FILE * file;
    int fd,nothing;
    struct winsize w;
    time_t now = time(0);
    char namepid[8];

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    if ((tm = localtime (&now)) == NULL) errsys("Can't get the current time !",5);
    memcpy(&timeNow,tm,sizeof(*tm));
    if ((fd = open("/proc/uptime", O_RDONLY)) == ERR) errsys("Can't open file (open()) !",2);
    file = fdopen(fd,"r");
    if(file == NULL) errsys("Can't open file 'uptime' (fdopen()) !",4);
    fscanf(file, "%d %d",&uptime,&nothing);
    fclose(file);
    close(fd);

    if ((dir = opendir("/proc")) == NULL) errsys("Can't open proc directory !",1);
    printf("%-8s %8s  %-5s %-4s %8s %8s %-8s %-4s  %s %8s %7s\n","USER","PID","%CPU","%MEM","VSZ","RSS","TTY","STAT","START","TIME","COMMAND");
    while((dr=readdir(dir)) != NULL){
        if(dr->d_type == DT_DIR && isNumber(dr->d_name)){
            memcpy(namepid,dr->d_name,strlen(dr->d_name));
            displayInfos(dr->d_name,w.ws_col);
        }
    }
    closedir(dir);
}

int main (){
    myps();
	return 0;
}
