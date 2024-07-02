/* KallistiOS ##version##

   pipe.c
   Copyright (C) 2024 Andress Barajas

*/

#include <kos/fs_pty.h>
#include <kos/fs.h>
#include <errno.h>

int pipe(int pipefd[2]) {
    int master_fd, slave_fd;

    if(pipefd == NULL) {
        errno = EFAULT;
        return -1;
    }

    /* Create a PTY master/slave pair */
    if(fs_pty_create(NULL, 0, &master_fd, &slave_fd) < 0)
        return -1;

    /* Set the file descriptors for the pipe */
    pipefd[0] = master_fd;  /* Reading end */
    pipefd[1] = slave_fd;   /* Writing end */

    return 0;
}
