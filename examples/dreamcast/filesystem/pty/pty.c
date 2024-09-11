/* KallistiOS ##version##

   pty.c
   Copyright (C) 2024 Andress Barajas

   This program demonstrates the creation and usage of a pseudo-terminal (PTY) 
   pair. It involves creating a master and a slave PTY, writing a message to 
   the master PTY, and reading the message from the slave PTY. It also 
   demonstrates handling non-blocking read operations and performs clean-up of 
   resources before exiting.
*/

#include <kos/fs_pty.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    /* Create a PTY pair */
    file_t master_fd = -1, slave_fd = -1;
    int retval = EXIT_SUCCESS;

    if(fs_pty_create(NULL, 0, &master_fd, &slave_fd) < 0) {
        fprintf(stderr, "Error creating PTY pair");
        goto failure;
    }

    /* Set non-blocking mode on the slave_fd for testing */
    int flags = fcntl(slave_fd, F_GETFL, 0);
    if(fcntl(slave_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        fprintf(stderr, "Error setting O_NONBLOCK");
        goto failure;
    }

    /* Write to the master end of the PTY */
    const char *msg = "Hello from master!";
    if(write(master_fd, msg, strlen(msg)) < 0) {
        fprintf(stderr, "Error writing to master PTY");
        goto failure;
    }

    /* Read from the slave end of the PTY */
    char buffer[128];
    ssize_t bytes_read = read(slave_fd, buffer, sizeof(buffer) - 1);
    if(bytes_read < 0) {
        if(errno == EAGAIN) {
            printf("No data available (non-blocking mode)\n");
        } else {
            fprintf(stderr, "Error reading from slave PTY");
            goto failure;
        }
    } else {
        /* Null-terminate and print the received message */
        buffer[bytes_read] = '\0';
        printf("Received message: %s\n", buffer);
    }

    /* Try and read again */
    bytes_read = read(slave_fd, buffer, sizeof(buffer) - 1);
    if(bytes_read < 0) {
        if(errno == EAGAIN) {
            printf("No more data available (non-blocking mode)\n");
        } else {
            fprintf(stderr, "Error reading from slave PTY");
            goto failure;
        }
    } else if (bytes_read == 0) {
        printf("No more data to read (EOF)\n");
    } else {
        /* Null-terminate and print the received message */
        buffer[bytes_read] = '\0';
        printf("Received 2nd message: %s\n", buffer);
    }
    
    /* Jump over failure and only do clean-up */
    goto cleanup;
    
    /* Set exit code and clean up */
failure:
    retval = EXIT_FAILURE;

    /* Clean up resources */
cleanup:
    if(master_fd) fs_close(master_fd);
    if(slave_fd) fs_close(slave_fd);

    printf("DONE!\n");

    return retval;
}
