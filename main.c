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

    char* pathname;
        struct dirent* dp;
        DIR* dirp;
    for(int i=0; i<fileCount; i++){
        pathname = fileList[i];
        dirp = opendir(pathname);
        if(dirp == NULL){
            fprintf(stderr, "cannot find directory %s: ", pathname);
            perror("");
            continue;
        }
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

                    if(S_ISREG(statbuf->st_mode)){
                        printf("-");
                    }else if(S_ISDIR(statbuf->st_mode)){
                        printf("d");
                    }

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

                    // char* t = ctime(&statbuf->st_mtim.tv_sec);
                    // char mmm[3];
                    // char dd[2];
                    // char yyyy[4];
                    // char hh[2];
                    // char mm[2];
                    // strncpy(mmm, t+4, 3);
                    // strncpy(dd, t+8, 2);
                    // strncpy(hh, t+11, 2);
                    // strncpy(mm, t+14, 2);
                    // strncpy(yyyy, t+20, 4);

                    // // printf("%-30s", t);
                    // printf("%-30s", mmm);
                    // // printf("%-20s%-20s%-20s%-20s:%-20s", mmm, dd, yyyy, hh, mm);


                    free(statbuf);
                }
                // printf("\n");
                printf("%s\n", dp->d_name);
            }
        }
        closedir(dirp);
    }

    free(fileList);
    
    // printf("index: %d, longList: %d, recur: %d\n", index, longList, recur);



        // printf("d_ino: %ld\n", dp->d_ino);

    // struct stat* statbuf = malloc(sizeof(stat));
    // stat(pathname, statbuf);
    
    // printf("st_ino: %ld\n", statbuf->st_ino);
    // printf("st_mode: %d\n", statbuf->st_mode);
    // printf("st_uid: %d\n", statbuf->st_uid);
    // printf("st_gid: %d\n", statbuf->st_gid);
    // printf("st_size: %ld\n", statbuf->st_size);
    // printf("st_atime: %ld\n", statbuf->st_atime);


    return 0;
}