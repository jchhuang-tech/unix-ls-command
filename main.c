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
void listNonDir(char* path);
void printStat(struct stat* statbuf, char* filename, char* path,struct maxlengths* maxlenbuf);
void lexicalSort(char** arr, int start, int end);
void updateMaxLen(char* path, struct maxlengths* maxlenbuf);
int numLen(__uintmax_t num);

//------changed here----------------//
//must be global for updateMaxlen()
// struct maxlengths* maxlenbuf;

//update: fixed
// i am thinking of using a struct to record the max length of the -l info
// so that we can minimize the space between the columns.
// but maybe the format doesn't matter anyways, so we will wait for the clarification.
// we also need one more function to get the max length info
// we pass a struct and an array of files' pathnames to it and it will fill the struct for us
// the struct should have the following variables: inoMaxLen, nlinkMaxLen, uidMaxLen, gidMaxLen and sizeMaxLen

// other notes: check each string to see if they are null terminated
// check return values

int main(int argc, char** args)
{
    // printf("0");
    if (argc < 1){
        exit(EXIT_FAILURE);
    }

    fileList = malloc((sizeof(char*))*argc);
    

    setOptionsFiles(argc, args);
    // we need to sort the fileList before using any items in it
    lexicalSort(fileList, 0, fileCount-1);

    // we need to modify the printing function to ensure there is space ('\n's) between files and directories
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
            // listNonDir(pathname);
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
    // free(maxlenbuf);
    
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
    // we must allow a file or directory to start with '-' if it's listed after another file/directory
    // we must treat every argument after the first file/dir argument as a file/dir argument instead of an option
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
    //------changed here----------------//
    //set maxlenbuf to global

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
        // we need to sort all the entries before printing the info
        // one way to do it is we put them in an array, sort them after the loop ends, and print their info
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
        // we need to sort subDirList before recursing on the items
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

//update: fixed
// this function needs modification
// we cannot print them one by one
// instead we have to put all the files in a list,
// use a struct to record their info's max length and then print them
void listNonDir(char* path)
{
    // char* filename = basename(path);

    // // struct maxlengths* maxlenbuf;
    // // maxlenbuf = malloc(sizeof(struct maxlengths));
    // // memset(maxlenbuf, 0, sizeof(struct maxlengths));

    // if(filename[0] != '.'){
    //     // updateMaxLen(pathForLongList, maxlenbuf);
    //     struct stat* statbuf = malloc(sizeof(struct stat));
    //     lstat(path, statbuf);
    //     printStat(statbuf, path, path, maxlenbuf);
    //     // free(maxlenbuf);
    //     free(statbuf);
    // }
}

// we need to pass a struct minLens to the printStat function
void printStat(struct stat* statbuf, char* filename, char* path,struct maxlengths* maxlenbuf)
{
    if(index){
        //---------changed to maxlength---------------------------//
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

        // ---------changed to maxlength---------------------------//
        printf("%*ju ",maxlenbuf->nlinkMaxLen, statbuf->st_nlink);
        
        struct passwd* pw = getpwuid(statbuf->st_uid);
        if(pw){
            //---------changed to maxlength---------------------------//
            printf("%-*s ",maxlenbuf->uidMaxLen, pw->pw_name);
        }

        struct group* grp = getgrgid(statbuf->st_gid);
        if(grp){
            //---------changed to maxlength---------------------------//
            printf("%-*s ",maxlenbuf->gidMaxLen, grp->gr_name);
        }
        // ---------changed to maxlength---------------------------//
        printf("%*ju ",maxlenbuf->sizeMaxLen, statbuf->st_size);

        char* mt = ctime(&statbuf->st_mtim.tv_sec);
        printf("%-7.*s%-5.*s%-6.*s", 6, mt+4, 4, mt+20, 5, mt+11);
    }
    printf("%s", filename);
    if(longList){
        if(S_ISLNK(statbuf->st_mode)){
            char* linkbuf = malloc(PATH_MAX+1);
            ssize_t linklen = readlink(path, linkbuf, PATH_MAX);
            if (linklen != -1){
                linkbuf[linklen] = '\0';
            }else{
                perror("readlink failed");
            }
            // strange characters printed after the link string
            // if you do "./myls -l /bin" you will see the problem
            // update: now fixed
            printf(" -> %s", linkbuf);
            free(linkbuf);
        }
    }
    printf("\n");
}

//update: fixed
    // this part needs modification
    // we need to add a loop here
    // where we go through the direntList and
    // use a struct to record the max length of the info in each column

void updateMaxLen(char* path, struct maxlengths* maxlenbuf)
{
    struct stat* statbuf = malloc(sizeof(struct stat));
    lstat(path, statbuf);

    // inoMaxLen
    //---------update---------------------------//
    // printf("1");
    if (numLen(statbuf->st_ino) > maxlenbuf->inoMaxLen)
    {
        maxlenbuf->inoMaxLen = numLen(statbuf->st_ino);
        // printf("%ju\n", statbuf->st_ino);

        // printf("%f", (floor(log10(statbuf->st_ino)) + 1));
    }
    //  printf("2");
    // nlinkMaxLen
    //---------update---------------------------//
    if (numLen(statbuf->st_nlink) >  maxlenbuf->nlinkMaxLen)
    {
        maxlenbuf->nlinkMaxLen = numLen(statbuf->st_nlink);
        // printf("nlinklen: %d of nlink: %ju", floor(log10(statbuf->st_size)) + 1, )
    }

    struct passwd* pw = getpwuid(statbuf->st_uid);
    if(pw){
        if(strlen(pw->pw_name) > maxlenbuf->uidMaxLen){
            maxlenbuf->uidMaxLen = strlen(pw->pw_name);
            // printf("Len: %ld for name: %s\n", strlen(grp->gr_name),grp->gr_name);
        }
    }
    //  printf("4");
    struct group* grp = getgrgid(statbuf->st_gid);
    if(grp){
        if(strlen(grp->gr_name) > maxlenbuf->gidMaxLen){
            maxlenbuf->gidMaxLen = strlen(grp->gr_name);
            // printf("Len: %ld for gr_name: %s\n", strlen(grp->gr_name),grp->gr_name);
        }
    }
    //  printf("5");
    // sizeMaxLen   
    //---------update---------------------------//
    if (numLen(statbuf->st_size) > maxlenbuf->sizeMaxLen)
    {
        maxlenbuf->sizeMaxLen = numLen(statbuf->st_size);
    }
    //  printf("6");
    free(statbuf);
}

int numLen(__uintmax_t num)
{
    if (num == 0){
        return 1;
    }
    else{
        // printf("leng of %lu: %f\n", num, floor(log10(num)) + 1);
        return (int)(floor(log10(num)) + 1);
    }
}