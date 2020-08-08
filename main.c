#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <stdbool.h>
#include <libgen.h>
#include <ctype.h>
#include <math.h>

static bool index = false;
static bool longList = false;
static bool recur = false;

static char** fileList = NULL;
static int fileCount = 0;

struct maxlengths
{
    int inoMaxLen;
    int nlinkMaxLen;
    int uidMaxLen;
    int gidMaxLen;
    int sizeMaxLen;
};


void setOptionsFiles(int argc, char** args);
bool isDir(char* path);
void listDir(char* path);
void printStat(struct stat* statbuf, char* filename, char* path,struct maxlengths* maxlenbuf);
void lexicalSort(char** arr, int start, int end);
void updateMaxLen(char* path, struct maxlengths* maxlenbuf);
int numLen(__uintmax_t num);
bool ifContain(char* string);

int main(int argc, char** args)
{
    if (argc < 1){
        exit(EXIT_FAILURE);
    }

    fileList = malloc((sizeof(char*))*argc);

    setOptionsFiles(argc, args);
    lexicalSort(fileList, 0, fileCount-1);

    bool filesInArgs = false;
    char* pathname = NULL;
    struct maxlengths* maxlenbuf = malloc(sizeof(struct maxlengths));
    memset(maxlenbuf, 0, sizeof(struct maxlengths));

    for(int i=0; i<fileCount; i++){
        pathname = fileList[i];
        if(!isDir(pathname)){
            updateMaxLen(pathname, maxlenbuf);
            filesInArgs = true;
        }
    }

    for(int i=0; i<fileCount; i++){
        pathname = fileList[i];
        if(!isDir(pathname)){
            filesInArgs = true;
            char* filename = basename(pathname);
            if(filename[0] != '.'){
                struct stat* statbuf = malloc(sizeof(struct stat));
                lstat(pathname, statbuf);
                printStat(statbuf, pathname, pathname, maxlenbuf);
                free(statbuf);
            }
        }
    }

    free(maxlenbuf);

    for(int i=0; i<fileCount; i++){
        pathname = fileList[i];
        if(isDir(pathname)){
            if(fileCount > 1 || recur){
                printf( (i==0 && !filesInArgs) ? "" : "\n");
                printf("%s:\n", pathname);
            }
            listDir(pathname);
        }
    }

    free(fileList);
    
    return 0;
}

void lexicalSort(char** arr, int start, int end)
{
    char* tmp = NULL;
    for(int i=start; i<=end; i++){
        for(int j=i+1; j<=end; j++){
            if(strcmp(arr[i], arr[j]) > 0){
                tmp = arr[i];
                arr[i] = arr[j];
                arr[j] = tmp;
            }
        }
    }
}

void setOptionsFiles(int argc, char** args)
{
    bool optionListEnded = false;
    for(int i=1; i<argc; i++){
        if(args[i][0] == '-' && !optionListEnded){
            for(int j=1; j<strlen(args[i]); j++){
                if(args[i][j] == 'i'){
                    index = true;
                }else if(args[i][j] == 'l'){
                    longList = true;
                }else if(args[i][j] == 'R'){
                    recur = true;
                }else{
                    puts("error: unsupported options");
                    exit(EXIT_FAILURE);
                }
            }
        }
        else{
            fileList[fileCount] = args[i];
            fileCount++;
            optionListEnded = true;
        }
    }
    
    if(fileCount == 0){
        fileList[0] = ".";
        fileCount++;
    }
}

bool isDir(char* path)
{
    struct stat* filesb = malloc(sizeof(struct stat));
    if (lstat(path, filesb) < 0){
        perror(path);
        exit(EXIT_FAILURE);
    }
    if(S_ISDIR(filesb->st_mode)){
        free(filesb);
        return true;
    }else{
        free(filesb);
        return false;
    }
}

void listDir(char* path)
{
    char* direntList[10000];
    int direntCount = 0;
    char* subDirList[10000];
    int subDirCount = 0;

    DIR* dirp = NULL;
    dirp = opendir(path);

    if(dirp == NULL){
        perror("cannot open directory");
        return;
    }

    struct maxlengths* maxlenbuf;
    maxlenbuf = malloc(sizeof(struct maxlengths));
    memset(maxlenbuf, 0, sizeof(struct maxlengths));

    struct dirent* de = NULL;

    while ((de = readdir(dirp)) != NULL){

        if(de->d_name[0] != '.'){
            char pathForLongList[PATH_MAX];
            snprintf(pathForLongList, sizeof(pathForLongList), "%s/%s", path, de->d_name);

            updateMaxLen(pathForLongList, maxlenbuf);

            char* direntPath = malloc(PATH_MAX);
            strncpy(direntPath, pathForLongList, sizeof(pathForLongList));
            direntList[direntCount] = direntPath;
            direntCount++;
        }
    }
    closedir(dirp);


    lexicalSort(direntList, 0, direntCount-1);
    for(int i=0; i<direntCount; i++){
        struct stat* statbuf = malloc(sizeof(struct stat));
        lstat(direntList[i], statbuf);
        printStat(statbuf, basename(direntList[i]), direntList[i], maxlenbuf);

        if(recur){
            if(S_ISDIR(statbuf->st_mode)){
                char* subDirPath = malloc(PATH_MAX);
                strncpy(subDirPath, direntList[i], PATH_MAX);
                subDirList[subDirCount] = subDirPath;
                subDirCount++;
            }
        }

        free(statbuf);
    }
    free(maxlenbuf);
    for(int i=0; i<direntCount; i++){
        free(direntList[i]);
    }

    if(recur){
        lexicalSort(subDirList, 0, subDirCount-1);
        for(int i=0; i<subDirCount; i++){
            printf("\n%s:\n", subDirList[i]);
            listDir(subDirList[i]);
        }
        for(int i=0; i<subDirCount; i++){
            free(subDirList[i]);
        }
    }
}

void printStat(struct stat* statbuf, char* filename, char* path,struct maxlengths* maxlenbuf)
{
    if(index){
        printf("%*ju ", maxlenbuf->inoMaxLen, statbuf->st_ino);
    }
    if(longList){
        printf( (S_ISREG(statbuf->st_mode)) ? "-" : "");
        printf( (S_ISDIR(statbuf->st_mode)) ? "d" : "");
        printf( (S_ISLNK(statbuf->st_mode)) ? "l" : "");
        printf( (S_ISCHR(statbuf->st_mode)) ? "c" : "");
        printf( (S_ISBLK(statbuf->st_mode)) ? "b" : "");
        printf( (S_ISSOCK(statbuf->st_mode)) ? "s" : "");
        printf( (S_ISFIFO(statbuf->st_mode)) ? "p" : "");

        printf( (statbuf->st_mode & S_IRUSR) ? "r" : "-");
        printf( (statbuf->st_mode & S_IWUSR) ? "w" : "-");
        printf( (statbuf->st_mode & S_IXUSR) ? "x" : "-");
        printf( (statbuf->st_mode & S_IRGRP) ? "r" : "-");
        printf( (statbuf->st_mode & S_IWGRP) ? "w" : "-");
        printf( (statbuf->st_mode & S_IXGRP) ? "x" : "-");
        printf( (statbuf->st_mode & S_IROTH) ? "r" : "-");
        printf( (statbuf->st_mode & S_IWOTH) ? "w" : "-");
        printf( (statbuf->st_mode & S_IXOTH) ? "x" : "-");
        printf(" ");

        printf("%*ju ",maxlenbuf->nlinkMaxLen, statbuf->st_nlink);
        
        struct passwd* pw = getpwuid(statbuf->st_uid);
        if(pw){
            printf("%-*s ",maxlenbuf->uidMaxLen, pw->pw_name);
        }

        struct group* grp = getgrgid(statbuf->st_gid);
        if(grp){
            printf("%-*s ",maxlenbuf->gidMaxLen, grp->gr_name);
        }
        printf("%*ju ",maxlenbuf->sizeMaxLen, statbuf->st_size);

        char* mt = ctime(&statbuf->st_mtim.tv_sec);
        printf("%-7.*s%-5.*s%-6.*s", 6, mt+4, 4, mt+20, 5, mt+11);
    }
    if(ifContain(filename))
    {
        printf("'%s'", filename);
    }
    else
    {
    printf("%s", filename);
    }
    
    if(longList){
        if(S_ISLNK(statbuf->st_mode)){
            char* linkbuf = malloc(PATH_MAX+1);
            ssize_t linklen = readlink(path, linkbuf, PATH_MAX);
            if (linklen != -1){
                linkbuf[linklen] = '\0';
            }else{
                perror("readlink failed");
            }
            printf(" -> %s", linkbuf);
            free(linkbuf);
        }
    }
    printf("\n");
}

void updateMaxLen(char* path, struct maxlengths* maxlenbuf)
{
    struct stat* statbuf = malloc(sizeof(struct stat));
    lstat(path, statbuf);

    if (numLen(statbuf->st_ino) > maxlenbuf->inoMaxLen)
    {
        maxlenbuf->inoMaxLen = numLen(statbuf->st_ino);
    }
    if (numLen(statbuf->st_nlink) >  maxlenbuf->nlinkMaxLen)
    {
        maxlenbuf->nlinkMaxLen = numLen(statbuf->st_nlink);
    }

    struct passwd* pw = getpwuid(statbuf->st_uid);
    if(pw){
        if(strlen(pw->pw_name) > maxlenbuf->uidMaxLen){
            maxlenbuf->uidMaxLen = strlen(pw->pw_name);
        }
    }
    struct group* grp = getgrgid(statbuf->st_gid);
    if(grp){
        if(strlen(grp->gr_name) > maxlenbuf->gidMaxLen){
            maxlenbuf->gidMaxLen = strlen(grp->gr_name);
        }
    }
    if (numLen(statbuf->st_size) > maxlenbuf->sizeMaxLen)
    {
        maxlenbuf->sizeMaxLen = numLen(statbuf->st_size);
    }
    free(statbuf);
}

int numLen(__uintmax_t num)
{
    if (num == 0){
        return 1;
    }
    else{
        return (int)(floor(log10(num)) + 1);
    }
}

bool ifContain(char* string)
{
    if(strchr(string, ' ')!=NULL || 
        strchr(string, '!')!=NULL || 
        strchr(string, '$')!=NULL || 
        strchr(string, '&')!=NULL || 
        strchr(string, '^')!=NULL || 
        strchr(string, '(')!=NULL || 
        strchr(string, ')')!=NULL)
    {
        return true;
    }
    else
    {
        return false;
    }
}