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

static bool index = false;
static bool longList = false;
static bool recur = false;

int main(int argc, char** args)
{
    if (argc < 1){
        exit(EXIT_FAILURE);
    }

    char** fileList = malloc(sizeof(char*)*argc);
    int fileCount = 0;

    for(int i=1; i<argc; i++){
        if(args[i][0] == '-'){
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
        }
    }
    
    if(fileCount == 0){
        fileList[0] = ".";
        fileCount++;
    }

    char* pathname = NULL;
    for(int i=0; i<fileCount; i++){
        pathname = fileList[i];
        if(fileCount > 1){
            printf( (i==0) ? "" : "\n");
            printf("%s:\n", pathname);
        }
        DIR* dirp = NULL;
        dirp = opendir(pathname);
        if(dirp == NULL){
            fprintf(stderr, "cannot find directory %s: ", pathname);
            perror("");
            continue;
        }
        struct dirent* dp = NULL;
        while ((dp = readdir(dirp)) != NULL){
            if(dp->d_name[0] != '.'){
                if(index){
                    printf("%-30ld", dp->d_ino);
                }
                if(longList){
                    struct stat* statbuf = malloc(10000);
                    // printf("size of stat: %ld\n", sizeof(stat));
                    // statbuf = realloc(statbuf, 1000000);
                    char pathForLongList[PATH_MAX];
                    snprintf(pathForLongList, sizeof(pathForLongList), "%s/%s", pathname, dp->d_name);
                    lstat(pathForLongList, statbuf);

                    // if(S_ISREG(statbuf->st_mode)){
                    //     printf("-");
                    // }else if(S_ISDIR(statbuf->st_mode)){
                    //     printf("d");
                    // }else if(S_ISLNK(statbuf->st_mode)){
                    //     printf("l");
                    // }
                    printf( (S_ISREG(statbuf->st_mode)) ? "-" : "");
                    printf( (S_ISDIR(statbuf->st_mode)) ? "d" : "");
                    printf( (S_ISLNK(statbuf->st_mode)) ? "l" : "");

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

                    printf("%-10ld", statbuf->st_nlink);
                    
                    struct passwd* pw = getpwuid(statbuf->st_uid);
                    if(pw){
                        printf("%-15s", pw->pw_name);
                    }
                
                    struct group* grp = getgrgid(statbuf->st_gid);
                    printf("%-20s", grp->gr_name);

                    printf("%-15ld", statbuf->st_size);

                    char* mt = ctime(&statbuf->st_mtim.tv_sec);
                    printf("%-7.*s%-5.*s%-10.*s", 6, mt+4, 4, mt+20, 5, mt+11);

                    free(statbuf);
                }
                printf("%s\n", dp->d_name);
            }
        }
        closedir(dirp);
    }

    free(fileList);
    
    return 0;
}