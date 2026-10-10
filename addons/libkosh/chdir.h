/*
 * chdir.h - current directory stuff
 *
 * (C) 2000 Jordan DeLong
 */
#ifndef _KOSH_CHDIR_H
#define _KOSH_CHDIR_H

#include <stdint.h>

int makeabspath(char *buff, char *path, size_t size);
int kosh_chdir(char *dir);

#endif  /* _KOSH_CHDIR_H */

