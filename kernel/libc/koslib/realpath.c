/* KallistiOS ##version##

   realpath.c

   Copyright (C) 2024 Andress Barajas
*/

/*
    The support for handling removing symlink components in this function is 
    currently disabled because readlink implementations don't exist for all 
    but one file system (ext2fs).
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

char *realpath(const char *__restrict path, char *__restrict resolved) {
    char temp_path[PATH_MAX];
    //char link_target[PATH_MAX];
    char *token;
    char *last_slash;
    ssize_t len;
    struct stat statbuf;

    /* Check for invalid params. */
    if(path == NULL || resolved == NULL) {
        errno = EINVAL;
        return NULL;
    }

    /* Too big of a path? */
    len = strlen(path);
    if(len >= PATH_MAX) {
        errno = ENAMETOOLONG;
        return NULL;
    }

    /* Handle absolute path. */
    if(path[0] == '/') {
        strncpy(temp_path, path, len);
        temp_path[len] = '\0';
    } 
    else {
        /* Handle relative path: prepend current working directory. */
        if(!getcwd(temp_path, PATH_MAX))
            return NULL;

        /* Check if appending will send us over. */
        if(strlen(temp_path) + len + 1 >= PATH_MAX) {
            errno = ENAMETOOLONG;
            return NULL;
        }

        /* Now append relative path. */
        strcat(temp_path, "/");
        strcat(temp_path, path);
    }

    /* Initialize the resolved path. */
    resolved[0] = '/';
    resolved[1] = '\0';

    /* Tokenize and look at each token. temp_path has the unprocessed path. */
    token = strtok(temp_path, "/");
    while(token != NULL) {
        /* Check for overlong path */
        if(strlen(resolved) + strlen(token) + 1 >= PATH_MAX) {
            errno = ENAMETOOLONG;
            return NULL;
        }

        if(strcmp(token, ".") == 0) {
            /* Ignore "." */
        } 
        else if(strcmp(token, "..") == 0) {
            /* Remove the last component from resolved path. */
            last_slash = strrchr(resolved, '/');

            if(last_slash != NULL && last_slash != resolved)
                *last_slash = '\0';
            else
                /* If there's no previous component, we stay at root. */
                resolved[1] = '\0';
        }
        else {
            /* Append a '/' if we don't already have one. */
            if(resolved[strlen(resolved) - 1] != '/')
                strcat(resolved, "/");

            /* Append the token to the resolved path. */
            strcat(resolved, token);

            /* Check if the current resolved path exists or is a symlink. */
            if(lstat(resolved, &statbuf) == -1)
                return NULL;

            // if(S_ISLNK(statbuf.st_mode)) {
            //     len = readlink(resolved, link_target, PATH_MAX - 1);
            //     if(len == -1)
            //         return NULL;

            //     /* Handle readlink silently truncating. */
            //     if(len == PATH_MAX - 1) {
            //         errno = ENAMETOOLONG;
            //         return NULL;
            //     }

            //     /* NULL terminate */
            //     link_target[len] = '\0';

            //     /* Handle replacing symlink with an absolute path. */
            //     if(link_target[0] == '/') {
            //         strncpy(resolved, link_target, len);
            //         resolved[len] = '\0';
            //     } else {
            //         /* Handle symlink that points to a relative path. */
            //         last_slash = strrchr(resolved, '/');
            //         if(last_slash != NULL) {
            //             /* Remove the last component from resolved path. */
            //             *(last_slash + 1) = '\0';

            //             if(strlen(resolved) + strlen(link_target) >= PATH_MAX) {
            //                 errno = ENAMETOOLONG;
            //                 return NULL;
            //             }

            //             strcat(resolved, link_target);
            //         } else {
            //             /* We got here because the initial path is a symbolic 
            //               link pointing to a relative path or the link target 
            //               itself does not contain any directory separators and 
            //               resolves to a path with no slashes. */
            //             return NULL;
            //         }
            //     }
            // }
        }

        token = strtok(NULL, "/");
    }

    return resolved;
}
