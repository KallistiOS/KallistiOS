// Copyright (c) 2024 Eric Fradella
// Copyright (c) 2024 Andress Barajas

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <kos/thread.h>

#include "speedtest.h"

/* Number of connections allowed on the incoming queue */
#define BACKLOG         1
#define HTTP_PORT       80

void *server_thread(void *p) {
    (void) p;
    int server_socket;
    struct sockaddr_in server_addr;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0) {
        printf("server_thread: socket create failed\n");
        return NULL;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(HTTP_PORT);

    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("server_thread: bind failed\n");
        goto server_cleanup;
    }

    if(listen(server_socket, BACKLOG) < 0) {
        printf("server_thread: listen failed\n");
        goto server_cleanup;
    }

    /* Loop here listening for new connections from one client */
    while(true) {
        http_state_t *hr = calloc(1, sizeof(http_state_t));
        if(!hr) {
            fprintf(stderr, "calloc failed\n");
            goto server_cleanup;
        }

        socklen_t client_len = sizeof(hr->client);
        hr->socket = accept(server_socket, (struct sockaddr *)&hr->client, &client_len);
        if(hr->socket < 0) {
            fprintf(stderr, "accept failed\n");
            free(hr);
            goto server_cleanup;
        }

        uint32_t new_buf_sz = 65535;
        setsockopt(hr->socket, SOL_SOCKET, SO_SNDBUF, &new_buf_sz, sizeof(new_buf_sz));
        setsockopt(hr->socket, SOL_SOCKET, SO_RCVBUF, &new_buf_sz, sizeof(new_buf_sz));

        /* Create thread for new client */
        thd_create(DETACHED_THREAD, handle_request, hr);
    }

server_cleanup:
    /* Close all connections with the client */
    close(server_socket);

    return NULL;
}
