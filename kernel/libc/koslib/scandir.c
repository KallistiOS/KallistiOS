/* KallistiOS ##version##

   scandir.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2024 Falco Girgis
*/

#include <kos/dbglog.h>
#include <sys/dirent.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* Comparator used to sort two directory entries alphabetically. */
int alphasort(const struct dirent **a, const struct dirent **b) {
   assert(a && b);
    /* Simply use strcoll() to determine lexical order. */
   return strcoll((*a)->d_name, (*b)->d_name);
}

/* Static utility function used to add an entry to our current list of entries.
   The list's capacity doubles every time its size reaches its capacity,
   similarly to a C++ std::vector's allocation behavior.

   The only real difference here is that we must gracefully handle
   out-of-memory situations (malloc() returns NULL), in which case we clean up
   the vector and its entries, returning 0 to denote failure. */
static int push_back(struct dirent ***list, int *size, int *capacity,
                      const struct dirent *dir) {
    struct dirent *new_dir;
    int entry_size;
    struct dirent ***list_tmp = list;

    /* Check if the "vector" needs to resize */
    if(*size == *capacity) {
        /* Capacity should be powers-of-2 */
        if(*capacity)
            *capacity *= 2;
        else
            *capacity = 1;

        /* Resize our list */
        *list_tmp = realloc(*list, *capacity * sizeof(struct dirent*));

        /* Handle out-of-memory in case realloc() failed. */
        if(!*list_tmp)
            goto out_of_memory;
        else
            list = list_tmp;
    }

    /* Allocate space for the new directory entry on the heap.

       NOTE: We only allocate EXACTLY as much space as is needed
             for the directory entry, based on the length of its name,
             using our flexible array member, dirent::d_name */
    entry_size = sizeof(struct dirent) + strlen(dir->d_name) + 1;
    new_dir = malloc(entry_size);

    /* Check for malloc() failure of the single entry */
    if(!new_dir)
        goto out_of_memory;

    /* Copy the directory entry into its new heap-allocated storage. */
    memcpy(new_dir, dir, entry_size);

    /* Add it to the list, incrementing its size. */
    (*list)[(*size)++] = new_dir;

    return 1;

/* Handle out-of-memory failure: */
out_of_memory:
    /* If this is our first loop and we're OOM, just bail. */
    if(!*list)
        return 0;

    /* Free each individual directory entry. */
    for(int e = 0; e < *size; ++e)
        free((*list)[e]);

    /* Free the overall directory entry list */
    free(*list);

    /* Reset our list variables */
    *list = NULL;

    return 0;
}

/* Implementation of the POSIX scandir() function within dirent.h. */
int scandir(const char *dirname, struct dirent ***namelist, 
            int(*filter)(const struct dirent *), 
            int(*compar)(const struct dirent ** , const struct dirent **)) {

    DIR* dir;
    struct dirent *dirent;
    struct stat stat;
    int size = 0, capacity = 0;

    /* There's no standard way to validate these, so we'll assert(). */
    assert(dirname && namelist);

    /* Initialize namelist to its initial value */
    *namelist = NULL;

    /* First we simply attempt to open the directory using the POSIX API. */
    if(!(dir = opendir(dirname))) {

        /* If we cannot open the directory, we attempt to stat it for more
           detailed information on why it failed */
        if(fs_stat(dirname, &stat, 0) < 0) {
            /* If stat itself failes, there's no directory entry found. */
            errno = ENOENT;
            return -1;
        }

        /* Check whether the file type is even a directory. */
        if(stat.st_mode != S_IFDIR) {
            errno = ENOTDIR;
            return -1;
        }
        else
        {
            /* If it's a directory according to stat, yet we failed to open it,
               this could potentially be a KOS issue or some unknown failure.
               assert() and return a generic error. Not much we can do. */
            assert(0);
            errno = ENOENT;
            return -1;
        }
    }

    /* Iterate over each file within the directory. */
    while((dirent = readdir(dir))) {
        /* If we provided no filter, or the file is within the filter,
           attempt to add it to the namelist.*/
        if(!filter || filter(dirent))
            /* If we successfully added dirent to the namelist, continue. */
            if(!push_back(namelist, &size, &capacity, dirent)) {
                /* If push_back() returned 0, we ran out of memory.
                   Close the directory and return the appropriate error. */
                closedir(dir);
                errno = ENOMEM;
                return -1;
            }
    }

    /* If namelist has any entries and we were passed a comparison callback,
       use qsort() to sort namelist with the callback. */
    if(size > 0 && compar)
        qsort(*namelist, size, sizeof(struct dirent*), (void *)compar);

    /* Close the directory and release resources when we're done. */
    closedir(dir);

    /* Return the number of entries stored within namelist. */
    return size;
}
