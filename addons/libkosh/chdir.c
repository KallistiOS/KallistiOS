/*
 * chdir.c - chdir stuff
 *
 * (C) 2000 Jordan DeLong
 */

#include <kos/fs.h>
#include <kos/limits.h>
#include <string.h>
#include "kosh.h"

/* the current directory */
static char cwd[NAME_MAX];

/* read a pathname based on the current directory and turn it into an abs one, we
   don't check for validity, or really do anything except handle ..'s and .'s */
int makeabspath(char *buff, char *path, size_t size) {
    int numtxts;
    int i;
    char *txts[32];     /* max of 32...should be nuff */
    char *rslash;

    numtxts = 0;

    /* check if path is already absolute */
    if (path[0] == '/') {
        strncpy(buff, path, size);
        return 0;
    }

    /* split the path into tokens */
    for (numtxts = 0; numtxts < 32;) {
        if ((txts[numtxts] = strsep(&path, "/")) == NULL)
            break;
        if (*txts[numtxts] != '\0')
            numtxts++;
    }

    /* start from the current directory */
    strncpy(buff, cwd, size);

    for (i = 0; i < numtxts; i++) {
        if (strcmp(txts[i], "..") == 0) {
            if ((rslash = strrchr(buff, '/')) != NULL)
                *rslash = '\0';
        } else if (strcmp(txts[i], ".") == 0) {
            /* do nothing */
        } else {
            if (buff[strlen(buff) - 1] != '/')
                strncat(buff, "/", size - 1 - strlen(buff));
            strncat(buff, txts[i], size - 1 - strlen(buff));
        }
    }

    /* make sure it's not empty */
    if (buff[0] == '\0') {
        buff[0] = '/';
        buff[1] = '\0';
    }

    return 0;
}

/* change the current directory (dir is an absolute path for now) */
int kosh_chdir(char *dir) {
    char buff[NAME_MAX];

    makeabspath(buff, dir, NAME_MAX);
    strncpy(cwd, buff, NAME_MAX);

    fs_chdir(cwd);

    return 0;
}

