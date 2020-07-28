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

static bool index = false;
static bool longList = false;
static bool recur = false;

static char** fileList = NULL;
static int fileCount = 0;

void setOptionsFiles(int argc, char** args);
bool isDir(char* path);
void listDir(char* path);
void listNonDir(char* path);
void printStat(struct stat* statbuf, char* filename, char* path);
void lexicalSort(char** arr, int start, int end);

// i am thinking of using a struct to record the max length of the -l info
// so that we can minimize the space between the columns.
// but maybe the format doesn't matter anyways, so we will wait for the clarification.

int main(int argc, char** args)
{
    if (argc < 1){
        exit(EXIT_FAILURE);
    }

    fileList = malloc((sizeof(char*))*argc);
    fileCount = 0;

    setOptionsFiles(argc, args);
    // we need to sort the fileList before using any items in it
    lexicalSort(fileList, 0, fileCount-1);

    // we need to modify the printing function to ensure there is space ('\n's) between files and directories
    bool filesInArgs = false;
    char* pathname = NULL;
    for(int i=0; i<fileCount; i++){
        pathname = fileList[i];
        if(!isDir(pathname)){
            listNonDir(pathname);
            filesInArgs = true;
        }
    }

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
    // this is a case-insensitive sorting
    // i am not sure if i should change it back to case-sensitive sorting
    // we will wait for further clarification
    char* lowercase[end-start+1];
    for(int i=0; i<end-start+1; i++){
        lowercase[i] = malloc(PATH_MAX);
        strncpy(lowercase[i], arr[start+i], PATH_MAX);
    }
    for(int i=0; i<end-start+1; i++){
        for(int j=0; j<strlen(lowercase[i]); j++){
            lowercase[i][j] = tolower(lowercase[i][j]);
        }
    }
    char* tmp = NULL;
    for(int i=start; i<=end; i++){
        for(int j=i+1; j<=end; j++){
            if(strcmp(lowercase[i-start], lowercase[j-start]) > 0){
                tmp = arr[i];
                arr[i] = arr[j];
                arr[j] = tmp;
                tmp = lowercase[i-start];
                lowercase[i-start] = lowercase[j-start];
                lowercase[j-start] = tmp;
            }
        }
    }
    for(int i=0; i<end-start+1; i++){
        free(lowercase[i]);
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
    struct dirent* de = NULL;
    while ((de = readdir(dirp)) != NULL){
        if(de->d_name[0] != '.'){
            char pathForLongList[PATH_MAX];
            snprintf(pathForLongList, sizeof(pathForLongList), "%s/%s", path, de->d_name);
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
        printStat(statbuf, basename(direntList[i]), direntList[i]);
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

void listNonDir(char* path)
{
    char* filename = basename(path);
    if(filename[0] != '.'){
        struct stat* statbuf = malloc(sizeof(struct stat));
        lstat(path, statbuf);
        printStat(statbuf, filename, path);
        free(statbuf);
    }
}

void printStat(struct stat* statbuf, char* filename, char* path)
{
    if(index){
        printf("%-30ju", statbuf->st_ino);
    }
    if(longList){
        printf( (S_ISREG(statbuf->st_mode)) ? "-" : "");
        printf( (S_ISDIR(statbuf->st_mode)) ? "d" : "");
        printf( (S_ISLNK(statbuf->st_mode)) ? "l" : "");
        printf( (S_ISCHR(statbuf->st_mode)) ? "c" : "");
        printf( (S_ISBLK(statbuf->st_mode)) ? "b" : "");
        printf( (S_ISSOCK(statbuf->st_mode)) ? "s" : "");
        printf( (S_ISFIFO(statbuf->st_mode)) ? "p" : "");
        // if(!S_ISLNK(statbuf->st_mode) && 
        //     !S_ISDIR(statbuf->st_mode) && 
        //     !S_ISREG(statbuf->st_mode) && 
        //     !S_ISCHR(statbuf->st_mode) && 
        //     !S_ISBLK(statbuf->st_mode) && 
        //     !S_ISSOCK(statbuf->st_mode) && 
        //     !S_ISFIFO(statbuf->st_mode)){
        //     puts("\nSPECIAL FILE!");
        //     exit(EXIT_FAILURE);
        // }

        printf( (statbuf->st_mode & S_IRUSR) ? "r" : "-");
        printf( (statbuf->st_mode & S_IWUSR) ? "w" : "-");
        printf( (statbuf->st_mode & S_IXUSR) ? "x" : "-");
        printf( (statbuf->st_mode & S_IRGRP) ? "r" : "-");
        printf( (statbuf->st_mode & S_IWGRP) ? "w" : "-");
        printf( (statbuf->st_mode & S_IXGRP) ? "x" : "-");
        printf( (statbuf->st_mode & S_IROTH) ? "r" : "-");
        printf( (statbuf->st_mode & S_IWOTH) ? "w" : "-");
        printf( (statbuf->st_mode & S_IXOTH) ? "x" : "-");
        printf("     ");

        printf("%-7ju", statbuf->st_nlink);
        
        struct passwd* pw = getpwuid(statbuf->st_uid);
        if(pw){
            printf("%-15s", pw->pw_name);
        }

        struct group* grp = getgrgid(statbuf->st_gid);
        if(grp){
            printf("%-20s", grp->gr_name);
        }

        printf("%-12ju", statbuf->st_size);

        char* mt = ctime(&statbuf->st_mtim.tv_sec);
        printf("%-7.*s%-5.*s%-10.*s", 6, mt+4, 4, mt+20, 5, mt+11);
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