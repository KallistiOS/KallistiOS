/* KallistiOS ##version##

   pty.c
   Copyright (C) 2024 Andress Barajas
*/

#include <kos/fs_pty.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc, char* argv[]) {
    /* Create a PTY pair */
    file_t master_fd, slave_fd;
    if(fs_pty_create(NULL, 0, &master_fd, &slave_fd) < 0) {
        fprintf(stderr, "Error creating PTY pair");
        return 1;
    }

    /* Set non-blocking mode on the slave_fd for testing */
    int flags = fcntl(slave_fd, F_GETFL, 0);
    if(fcntl(slave_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        fprintf(stderr, "Error setting O_NONBLOCK");
        fs_close(master_fd);
        fs_close(slave_fd);
        return 1;
    }

    /* Write to the master end of the PTY */
    const char *msg = "Hello from master!";
    if(write(master_fd, msg, strlen(msg)) < 0) {
        fprintf(stderr, "Error writing to master PTY");
        fs_close(master_fd);
        fs_close(slave_fd);
        return 1;
    }

    /* Read from the slave end of the PTY */
    char buffer[128];
    ssize_t bytes_read = read(slave_fd, buffer, sizeof(buffer) - 1);
    if(bytes_read < 0) {
        if(errno == EAGAIN) {
            printf("No data available (non-blocking mode)\n");
        } else {
            fprintf(stderr, "Error reading from slave PTY");
            fs_close(master_fd);
            fs_close(slave_fd);
            return 1;
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
            fs_close(master_fd);
            fs_close(slave_fd);
            return 1;
        }
    } else if (bytes_read == 0) {
        printf("No more data to read (EOF)\n");
    } else {
        /* Null-terminate and print the received message */
        buffer[bytes_read] = '\0';
        printf("Received 2nd message: %s\n", buffer);
    }

    /* Clean up resources */
    fs_close(master_fd);
    fs_close(slave_fd);

    printf("DONE!\n");

    return 0;
}
