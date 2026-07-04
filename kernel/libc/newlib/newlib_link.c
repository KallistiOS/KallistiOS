/* KallistiOS ##version##

   newlib_link.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2013 Lawrence Sebald

*/

#include <kos/fs.h>
#include <reent.h>

int _link_r(struct _reent *reent, const char *oldf, const char *newf) {
    (void)reent;
    return fs_link(oldf, newf);
}
