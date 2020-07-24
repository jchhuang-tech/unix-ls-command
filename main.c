#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

int main(int argc, char** args)
{
    char* pathname;
    if (argc < 1){
        exit(EXIT_FAILURE);
    }else if(argc == 1){
        // pathname = malloc(sizeof(char));
        pathname = ".";
    }else if(argc == 2){
        pathname = args[1];
    }

    DIR* dirp = opendir(pathname);
    struct dirent* dirpent;
    while ((dirpent = readdir(dirp)) != NULL){
        // printf("d_ino: %ld\n", dirpent->d_ino);
        puts(dirpent->d_name);
    }

    closedir(dirp);

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