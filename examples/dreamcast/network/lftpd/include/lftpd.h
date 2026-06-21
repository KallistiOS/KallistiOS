#pragma once

typedef struct {
    char *directory;
    int socket;
    int data_socket;
} lftpd_client_t;

typedef struct {
    const char *directory;
    int port;
    int server_socket;

    lftpd_client_t *client;
} lftpd_t;

/**
 * @brief Create a server on port and start listening for client
 * connections. This function blocks for the life of the server and
 * only returns when lftpd_stop() is called with the same lftpd_t.
 */
int lftpd_start(const char *directory, int port, lftpd_t *lftpd);

/**
 * @brief Stop a previously started server. This kills any active client
 * connections, shuts down the listener, and returns. After this
 * function returns, lftpd_start() will also return.
 */
int lftpd_stop(lftpd_t *lftpd);
