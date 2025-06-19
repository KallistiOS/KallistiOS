/* KallistiOS ##version##

   fs_utils.c
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2014 Lawrence Sebald

*/

/*

A couple of helpful utility functions for VFS usage.
XXX This probably belongs in something like libc...

*/

#include <kos/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* Copies a file from 'src' to 'dst'. The amount of the file
   actually copied without error is returned. */
ssize_t fs_copy(const char * src, const char * dst) {
    char    *buff;
    ssize_t left, total, r;
    file_t  fs, fd;

    /* Try to open both files */
    fs = fs_open(src, O_RDONLY);

    if(fs == FILEHND_INVALID) {
        return -1;
    }

    fd = fs_open(dst, O_WRONLY | O_TRUNC | O_CREAT);

    if(fd == FILEHND_INVALID) {
        fs_close(fs);
        return -2;
    }

    /* Get the source size */
    left = fs_total(fs);
    total = 0;

    /* Allocate a buffer */
    buff = malloc(65536);

    /* Do the copy */
    while(left > 0) {
        r = fs_read(fs, buff, 65536);

        if(r <= 0)
            break;

        fs_write(fd, buff, r);
        left -= r;
        total += r;
    }

    /* Free the buffer */
    free(buff);

    /* Close both files */
    fs_close(fs);
    fs_close(fd);

    return total;
}

/* Opens a file, allocates enough RAM to hold the whole thing,
   reads it into RAM, and closes it. The caller owns the allocated
   memory (and must free it). The file size is returned, or -1
   on failure; on success, out_ptr is filled with the address
   of the loaded buffer. */
ssize_t fs_load(const char * src, void ** out_ptr) {
    file_t  f;
    void    * data;
    void    * new_data;
    uint8   * out;
    ssize_t total, left, r;

    assert(out_ptr != NULL);
    *out_ptr = NULL;

    /* Try to open the file */
    f = fs_open(src, O_RDONLY);

    if(f == FILEHND_INVALID)
        return -1;

    /* Get the size and alloc a buffer */
    left = fs_total(f);
    total = 0;
    data = malloc(left);
    if(data == NULL) {
        fs_close(f);
        return -1;
    }

    out = (uint8 *)data;

    /* Load the data */
    while(left > 0) {
        r = fs_read(f, out, left);

        if(r <= 0)
            break;

        left -= r;
        total += r;
        out += r;
    }

    /* Did we get it all? If not, realloc the buffer */
    if(left > 0) {
        new_data = realloc(data, total);
        if(new_data != NULL) {
            data = new_data;
        }
    }

    *out_ptr = data;

    fs_close(f);

    return total;
}

/* Basically, this is like strcat, but for path components. It'll automatically
   put a / in the middle if there isn't one there. */
ssize_t fs_path_append(char *dst, const char *src, size_t len) {
    size_t dlen, slen;

    /* Verify the arguments passed in. */
    if(!src || !dst) {
        errno = EFAULT;
        return -1;
    }

    if(!len) {
        errno = EINVAL;
        return -1;
    }

    /* Find the lengths of the strings. */
    dlen = strlen(dst);
    slen = strlen(src);

    /* Dont do anything if dst and src are empty. Return 1 for NUL terminator */
    if(dlen == 0 && slen == 0)
        return 1;

    /* Will we run out of space? */
    if(dlen == 0 || dst[dlen - 1] != '/') {
        if(dlen + slen + 2 > len) {
            errno = ENAMETOOLONG;
            return -1;
        }

        /* If src doesn't start with '/', add one to dst */
        if(src[0] != '/')
            dst[dlen++] = '/';
    }
    else if(dlen + slen + 1 > len) {
        errno = ENAMETOOLONG;
        return -1;
    }

    /* If dst ends with '/' and src starts with '/', ignore '/' from dst */
    if(dlen > 0 && dst[dlen - 1] == '/' && src[0] == '/')
        --dlen;

    /* Concatenate the src string on the dst, copying the NUL terminator while
       we are at it. */
    memcpy(dst + dlen, src, slen + 1);

    /* Return the current length of the string, including the NUL terminator. */
    return (ssize_t)(dlen + slen + 1);
}

char *fs_normalize_path(const char *__restrict path, char *__restrict resolved) {
    char temp_path[PATH_MAX];
    char *token;
    char *last_slash;
    ssize_t len;

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
        }

        token = strtok(NULL, "/");
    }

    return resolved;
}
