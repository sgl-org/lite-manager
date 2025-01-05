#include "lm_cmd.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#if ( __linux__)
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif


int lm_rmfile(const char *path) {
    return remove(path);
}


int lm_rmdir(const char *dir_name)
{
    DIR *dir;
    struct dirent *entry;
    char path[1024];
    
    dir = opendir(dir_name);
    if (!dir) {
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue; 
        }
        
        snprintf(path, sizeof(path), "%s/%s", dir_name, entry->d_name);
        lm_rmfile(path);
    }
    closedir(dir);
    
    if (rmdir(dir_name) != 0) {
        return -1;
    }
    
    return 0;
}


int lm_mkdir(const char *dir_name)
{
#if (_WIN32)
    return mkdir(dir_name);
#elif ( __linux__)
    return mkdir(dir_name, 0777);
#endif
}


void lm_echo(const char *info)
{
    printf("%s\n", info);
}


void lm_echo_red(const char *info)
{
    printf("\x1b[31m%s\x1b[0m\n", info);
}


void lm_echo_green(const char *info)
{
    printf("\x1b[32m%s\x1b[0m\n", info);
}


void lm_echo_blue(const char *info)
{
    printf("\x1b[34m%s\x1b[0m\n", info);
}
