
#ifndef QBN_STD_H
#define QBN_STD_H

#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

char* get_program_path(char** argv) {
    if (argv[0][0] == '/') {
        return argv[0];
    }
    char* path = malloc(PATH_MAX);
    char* res = getcwd(path, PATH_MAX);
    int wd_len = strlen(path);
    if (res == NULL) {
        return NULL;
    }
    char* rel_program_path = argv[0];
    while (rel_program_path[0] == '/') {
        rel_program_path++;
    }
    path[wd_len] = '/';
    strcpy(path + wd_len + 1, rel_program_path);
    return path;
}

#endif //QBN_STD_H
