#include <stdio.h>
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
#include <errno.h>
#include <time.h>
#include <sys/times.h>
#include <pwd.h>
#include <grp.h>
#include <features.h>
#include <linux/limits.h>
#include <limits.h>

#define SIZE_START 100
#define ERR -1
#define errsys(m,n) perror(m),exit(n)
#define SIZE_PID 7
#define LENGTH_COLOR_NAME 8
#define MY_RIGHTS_MAX 12
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\033[0m"

struct tm timeNow,*tm;

char* displayTime(int sec){
    /* affiche la date ou l'heure à laquelle a commencé un processus */
    time_t t;
    struct tm timeStart;
    char *display;
    const char * months[12] = {"jan.", "feb.", "mar.", "apr.", "may", "jun.", "jul.", "aug.", "sept.", "oct.", "nov.", "dec."};
    t = sec;
    display=malloc(sizeof(char)*SIZE_START);
    if ((tm = localtime (&t)) == NULL) errsys("Can't get the last modified time !",4);
    memcpy(&timeStart,tm,sizeof(*tm));
    if((abs(timeStart.tm_mon-timeNow.tm_mon) <= 6) && timeStart.tm_year == timeNow.tm_year){
        sprintf(display,"%-5s %2d %02d:%02d",months[timeStart.tm_mon],timeStart.tm_mday,timeStart.tm_hour,timeStart.tm_min);
    } else {
        sprintf(display,"%-5s %2d %5d",months[timeStart.tm_mon],timeStart.tm_mday,timeStart.tm_year+1900);
    }
    
    return display;
}

static char firstRight(mode_t bits){
  /* Retourne le caractère reservé qui précède les droits */
  if (S_ISREG(bits))
    return '-';
  if (S_ISDIR(bits))
    return 'd';
  if (S_ISBLK(bits))
    return 'b';
  if (S_ISCHR(bits))
    return 'c';
  if (S_ISLNK(bits))
    return 'l';
  if (S_ISFIFO(bits))
    return 'p';
  if (S_ISSOCK(bits))
    return 's';
  return '?';
}

int gettotal(char ** filenames,char *path,int len,int *lenghtsize,int *lenghtlnk){
    /* récupère le total de blocs alloués d'un répertoire */
    struct stat bufstat;
    int i,total=0;
    long lenghts=(long)*lenghtsize,lenghtl = (long)*lenghtlnk;
    char *fullpath,size[8],lnk[8];
    for(i=0;i<len;i++){
        fullpath = malloc(sizeof(char)*(strlen(path)+strlen(filenames[i])+2));
        sprintf(fullpath,"%s%s",path,filenames[i]);
        if (lstat(fullpath,&bufstat)==-1) errsys("lstat failed",1);
        total+=bufstat.st_blocks/2;
        if((unsigned)lenghts < bufstat.st_size) lenghts = bufstat.st_size;
        if((unsigned)lenghtl < bufstat.st_nlink) lenghtl = bufstat.st_nlink;
        free(fullpath);
    }
    sprintf(size,"%ld",lenghts);
    sprintf(lnk,"%ld",lenghtl);
    *lenghtsize = strlen(size);
    *lenghtlnk = strlen(lnk);
    return total;
}

void strmode(mode_t mode, char *str){
    /* convertis mode_t en données lisibles */
    str[0] = firstRight(mode);
    str[1] = mode & S_IRUSR ? 'r' : '-';
    str[2] = mode & S_IWUSR ? 'w' : '-';
    str[3] = (mode & S_ISUID? (mode & S_IXUSR ? 's' : 'S'): (mode & S_IXUSR ? 'x' : '-'));
    str[4] = mode & S_IRGRP ? 'r' : '-';
    str[5] = mode & S_IWGRP ? 'w' : '-';
    str[6] = (mode & S_ISGID? (mode & S_IXGRP ? 's' : 'S'): (mode & S_IXGRP ? 'x' : '-'));
    str[7] = mode & S_IROTH ? 'r' : '-';
    str[8] = mode & S_IWOTH ? 'w' : '-';
    str[9] = (mode & S_ISVTX? (mode & S_IXOTH ? 't' : 'T'): (mode & S_IXOTH ? 'x' : '-'));
    str[10] = ' ';
    str[11] = '\0';
}


void savecolor(char * pathname,char color[LENGTH_COLOR_NAME], struct stat *bufstat, char rights[12],char filename[256]){
    int unsigned rightspos;
    int signed ret = 0;

    if (lstat(pathname,bufstat)==-1) errsys("lstat failed",1);
    
    /* récupère les droits de façon lisible */    
    strmode(bufstat->st_mode,rights);

    /* vérifie si le fichier est un executable */
    if(strlen(rights)>0 && rights[0]=='-'){
        rightspos=0;
        while (rightspos<strlen(rights)){
            if (rights[rightspos]=='x'){
                sprintf(color,"%s",COLOR_GREEN);
                break;

           }
           rightspos++;
        }
    }

    /* vérifie si le fichier a un format image ou audio */
    if(strlen(filename)>=4 && rights[0]=='-'){
        ret = strlen(filename)-4;
        if ((ret>=0 && (strcmp(filename+ret,".jpg")== 0 || strcmp(filename+ret,".gif")== 0 || strcmp(filename+ret,".png")== 0 || strcmp(filename+ret,".mp3")== 0 || strcmp(filename+ret,".ogg")== 0 || strcmp(filename+ret,".wav")== 0)) || ((ret>=1) && (strcmp(filename+(ret-1),".tiff") == 0 || strcmp(filename+ret,".jpeg") == 0)) ){
            sprintf(color,"%s",COLOR_MAGENTA);
        }
    }

    /* vérifie si le fichier est une archive */
    if(strlen(filename)>=4 && rights[0]=='-'){
        ret = strlen(filename)-4;
        if (ret>=0 && (strcmp(filename+ret,".tar")== 0 || strcmp(filename+ret,".zip")== 0 || strcmp(filename+ret,".deb")== 0 || strcmp(filename+ret,".rpm")== 0 || strcmp(filename+ret,".rar")== 0 || strcmp(filename+ret,".targz")== 0 || strcmp(filename+ret,".jar")== 0 || strcmp(filename+ret,".gzip")== 0 )){
            sprintf(color,"%s",COLOR_RED);
        }
    }
    if(!strlen(color)){
        switch (bufstat->st_mode & S_IFMT) {
        case S_IFBLK:
        case S_IFIFO:
            sprintf(color,"%s",COLOR_YELLOW);
            break;
        case S_IFDIR:
            sprintf(color,"%s",COLOR_BLUE);
            break;
        case S_IFREG:
            sprintf(color,"%s",COLOR_RESET);
            break;
        case S_IFSOCK:
            sprintf(color,"%s",COLOR_MAGENTA);
            break;
        default:
            sprintf(color,"%s",COLOR_RESET);
            break;
        }
    }
}


void displayInfos(char * thefilepath, int lenghtsize, int lenghtlnk){
    /* display the informations of a file with the pathname: thefilepath */
    struct stat bufstat,tmplnk;
    char *lastmodif,rights[MY_RIGHTS_MAX],tmprights[MY_RIGHTS_MAX],thefilepathtok[PATH_MAX],*token,*therealpath;
    struct passwd * pw;
    struct group *g;
    int nb=0;
    char color[LENGTH_COLOR_NAME],filename[256], targetlink[PATH_MAX],targfinal[PATH_MAX];
    time_t now = time(0);
    targetlink[0]='\0';
    targfinal[0]='\0';
    color[0]='\0';
    filename[0]='\0';
    strcpy(thefilepathtok,thefilepath);
    token = strtok(thefilepathtok,"/");
    while(token != NULL){
        sprintf(filename,"%s",token);
        token= strtok(NULL,"/");
    }

    savecolor(thefilepath,color,&bufstat,rights,filename);

    if ((tm = localtime (&now)) == NULL) errsys("Can't get the current time !",5);
    memcpy(&timeNow,tm,sizeof(*tm));
    lastmodif = displayTime(bufstat.st_mtime);
    pw = getpwuid(bufstat.st_uid);
    if(pw == NULL) errsys("Pwuid not found !",2);
    g = getgrgid(bufstat.st_gid);
    if(g == NULL) errsys("gid not found !",3);

    if(strlen(pw->pw_name)>8){
        pw->pw_name[7] = '+';
        pw->pw_name[8] = '\0';
    }

    if(strlen(g->gr_name)>8){
        g->gr_name[7] = '+';
        g->gr_name[8] = '\0';
    }

    if((bufstat.st_mode & S_IFMT) != S_IFLNK){
        printf("%s %*ld %-8s %-8s %*ld  %s %s%s%s\n",rights,lenghtlnk,(long) bufstat.st_nlink,pw->pw_name,g->gr_name,lenghtsize,(long) bufstat.st_size,lastmodif,color,filename,COLOR_RESET);
    } else {
        /* si le fichier est un lien */
        therealpath = realpath(thefilepath,NULL);
        sprintf(targetlink,"%s%c",therealpath,'\0');
        /* si la cible existe */
        if(targetlink != NULL && strcmp(targetlink,"(null)")!=0){
            savecolor(targetlink,color,&tmplnk,tmprights,filename);
            printf("%s %*ld %-8s %-8s %*ld  %s %s%s%s -> %s%s%s\n",rights,lenghtlnk,(long) bufstat.st_nlink,pw->pw_name,g->gr_name,lenghtsize,(long) bufstat.st_size,lastmodif,COLOR_CYAN,filename,COLOR_RESET,color,targetlink,COLOR_RESET);
        /* si la cible n'existe pas */
        } else {
            nb = readlink(thefilepath,targfinal,PATH_MAX);
            targfinal[nb]='\0';
            printf("%s %*ld %-8s %-8s %*ld  %s %s%s%s -> %s%s%s\n",rights,lenghtlnk,(long) bufstat.st_nlink,pw->pw_name,g->gr_name,lenghtsize,(long) bufstat.st_size,lastmodif,COLOR_RED,filename,COLOR_RESET,COLOR_RED,targfinal,COLOR_RESET);
        }
        free(therealpath);
    }
    free(lastmodif);
}

int compstr(void const *a,void const *b){
    const char *aa = *(const char **)a;
    const char *bb = *(const char **)b;
    return strcasecmp(aa,bb);
}

void freetabname( char **sortedname,int postab){
    /* libere la liste des noms de fichier triés */
    int i;
    for(i=0;i<postab;i++){
        free(sortedname[i]);
    }
    free(sortedname);
}

void displaydirectory(char *pathname,int hidden,int rec){
    /* affiche le contenu du directory avec un certain path name, fichiers cachés et récursivité traités */
    DIR *dir;
    struct dirent *dr;
    struct stat bufstat;
    char * wholefilename;
    char **sortedname;
    int sizesortedtab = 10,postab=0,i2,lenghtsize,lenghtlnk, *posdir,iposdir=0;

    sortedname = malloc(sizesortedtab*sizeof(char*));
    if ((dir = opendir(pathname)) == NULL) errsys("Can't open proc directory !",7);
    while((dr=readdir(dir)) != NULL){
        if((hidden == 0 && dr->d_name[0] != '.') || hidden == 1){
            if(postab >= sizesortedtab){
                sizesortedtab+=10;
                sortedname = realloc(sortedname,sizesortedtab*sizeof(char*));
            }
            sortedname[postab++] = malloc(sizeof(char)*(strlen(dr->d_name)+1));
            sprintf(sortedname[postab-1],"%s",dr->d_name);
        }
    }
    qsort(sortedname,postab,sizeof(char*),compstr);

    lenghtsize=0;
    lenghtlnk=0;
    printf("total %d\n",gettotal(sortedname,pathname,postab,&lenghtsize,&lenghtlnk));
    
    if(rec && postab>0) posdir = malloc(sizeof(int)*postab);

    for(i2=0;i2<postab;i2++){
        wholefilename = malloc(sizeof(char)*(strlen(pathname)+strlen(sortedname[i2])+2));
        sprintf(wholefilename,"%s%s",pathname,sortedname[i2]);
        displayInfos(wholefilename,lenghtsize,lenghtlnk);
        if(rec){
            if (lstat(wholefilename,&bufstat)==-1) errsys("lstat failed",1);
            if((bufstat.st_mode & S_IFMT) == S_IFDIR){
                posdir[iposdir++]=i2;
            }
        }
        free(wholefilename);
    }
    printf("\n");

    if(rec && postab>0){
        for(i2=0;i2<iposdir;i2++){
            if(strcmp(sortedname[posdir[i2]],".")!=0 && strcmp(sortedname[posdir[i2]],"..")!=0){
                wholefilename = malloc(sizeof(char)*(strlen(pathname)+strlen(sortedname[posdir[i2]])+2));
                sprintf(wholefilename,"%s%s%c",pathname,sortedname[posdir[i2]],'/');
                printf("%s\n",wholefilename);
                displaydirectory(wholefilename,hidden,rec);
                free(wholefilename);
            }
        }
        free(posdir);
    }

    freetabname(sortedname,postab);
    closedir(dir);
}

int main (int argc, char * argv[]){
    /* arg 0: /myls arg1: récursif ? arg2: caché ? arg3: path courant arg4 et +: noms de chemin */
    struct stat bufstat;
    int i=4;
    char **filenames;
    if(argc<4) errsys("Not enough args for ./myls",6);
    filenames = malloc(sizeof(char*)*(argc-4));
    i=4;
    while(i<argc){
        if(strlen(argv[i]) > 0 && argv[i][0]=='/'){
            filenames[i-4]= malloc(sizeof(char)*(strlen(argv[i])+1));
            strcpy(filenames[i-4],argv[i]);
        } else {
            filenames[i-4]= malloc(sizeof(char)*(strlen(argv[i])+strlen(argv[3])+2));
            strcpy(filenames[i-4],argv[3]);
            if(strlen(argv[3])>0 && argv[3][strlen(argv[3])-1] != '/'){
                strcat(filenames[i-4],"/");
            }
            strcat(filenames[i-4],argv[i]);
        }
        i++;
    }
    qsort(filenames,argc-4,sizeof(char*),compstr);

    i=0;
    while(i < argc-4){
        if (lstat(filenames[i],&bufstat)==-1) errsys("lstat failed",1);
        if((bufstat.st_mode & S_IFMT) != S_IFDIR){
            displayInfos(filenames[i],0,0);
        }
        i++;
    }
    i = 0;
    while(i < argc-4){
        if (lstat(filenames[i],&bufstat)==-1) errsys("lstat failed",1);
        if((bufstat.st_mode & S_IFMT) == S_IFDIR){
            printf("\n%s:\n",filenames[i]);
            displaydirectory(filenames[i],atoi(argv[2]),atoi(argv[1]));
        }
        i++;
    }
	return 0;
}
