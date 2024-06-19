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
    if (fs_pty_create(NULL, 0, &master_fd, &slave_fd) < 0) {
        fprintf(stderr, "Error creating PTY pair");
        return 1;
    }

    /* Write to the master end of the PTY */
    const char *msg = "Hello from master!";
    if (write(master_fd, msg, strlen(msg)) < 0) {
        fprintf(stderr, "Error writing to master PTY");
        fs_close(master_fd);
        fs_close(slave_fd);
        return 1;
    }

    /* Read from the slave end of the PTY */
    char buffer[128];
    ssize_t bytes_read = read(slave_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        fprintf(stderr, "Error reading from slave PTY");
        fs_close(master_fd);
        fs_close(slave_fd);
        return 1;
    }

    /* Null-terminate and print the received message */
    buffer[bytes_read] = '\0';
    printf("Received message: %s\n", buffer);

    /* Clean up resources */
    fs_close(master_fd);
    fs_close(slave_fd);

    return 0;
}
