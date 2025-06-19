/* KallistiOS ##version##

   httpd.c
   Copyright (C)2003 Megan Potter
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/queue.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#include <kos/fs.h>
#include <kos/thread.h>
#include <kos/mutex.h>

struct http_state;
typedef TAILQ_HEAD(http_state_list, http_state) http_state_list_t;

typedef struct http_state {
    TAILQ_ENTRY(http_state)     list;

    int         socket;
    struct sockaddr_in  client;
    socklen_t       client_size;

    kthread_t       * thd;
} http_state_t;

http_state_list_t states;
#define st_foreach(var) TAILQ_FOREACH(var, &states, list)
mutex_t list_mutex = MUTEX_INITIALIZER;

int st_init(void) {
    TAILQ_INIT(&states);
    return 0;
}

http_state_t * st_create(void) {
    http_state_t * ns;

    ns = calloc(1, sizeof(http_state_t));
    mutex_lock(&list_mutex);
    TAILQ_INSERT_TAIL(&states, ns, list);
    mutex_unlock(&list_mutex);

    return ns;
}

void st_destroy(http_state_t *st) {
    mutex_lock(&list_mutex);
    TAILQ_REMOVE(&states, st, list);
    mutex_unlock(&list_mutex);
    free(st);
}

int st_add_fds(fd_set * fds, int maxfd) {
    http_state_t * st;

    mutex_lock(&list_mutex);
    st_foreach(st) {
        FD_SET(st->socket, fds);

        if(maxfd < (st->socket + 1))
            maxfd = st->socket + 1;
    }
    mutex_unlock(&list_mutex);
    return maxfd;
}


/**********************************************************************/

// This is undoubtedly very slow
int readline(int sock, char *buf, int bufsize) {
    int r, rt;
    char c;

    rt = 0;

    do {
        r = read(sock, &c, 1);

        if(r == 0)
            return -1;

        if(rt < bufsize)
            buf[rt++] = c;
    }
    while(c != '\n');

    buf[rt - 1] = 0;

    if(buf[rt - 2] == '\r')
        buf[rt - 2] = 0;

    return 0;
}

int read_headers(http_state_t * hs, char * buffer, int bufsize) {
    char fn[256];
    int i, j;

    if(buffer == NULL)
        return -2;

    for(i = 0; ; i++) {
        if(readline(hs->socket, buffer, bufsize) < 0) {
            if(i > 0)
                return 0;
            else
                return -1;
        }

        if(strlen(buffer) == 0)
            break;

        // printf("httpd: read header '%s'\n", buffer);

        if(i == 0) {
            if(!strncmp(buffer, "GET", 3)) {
                for(j = 4; buffer[j] && buffer[j] != 32 && j < 256; j++) {
                    fn[j - 4] = buffer[j];
                }

                fn[j - 4] = 0;
            }
        }
    }

    strcpy(buffer, fn);
    // printf("httpd: read headers ok\n");

    return 0;
}

/**********************************************************************/

static const char * errmsg1 = "<html><head><title>";
static const char * errmsg2 = "</title></head><body bgcolor=\"white\"><h4>";
static const char * errmsg3 = "</h4>\n<hr>\nKOSHttp/1.0 server\n</body></html>";

int send_error(http_state_t * hs, int errcode, const char * str) {
    char * buffer = malloc(65536);

    if(buffer == NULL)
        return -1;

    sprintf(buffer, "HTTP/1.0 %d %s\r\nContent-type: text/html\r\n\r\n", errcode, str);
    write(hs->socket, buffer, strlen(buffer));

    write(hs->socket, errmsg1, strlen(errmsg1));

    sprintf(buffer, "%d %s", errcode, str);
    write(hs->socket, buffer, strlen(buffer));

    write(hs->socket, errmsg2, strlen(errmsg2));

    sprintf(buffer, "%d %s", errcode, str);
    write(hs->socket, buffer, strlen(buffer));

    write(hs->socket, errmsg3, strlen(errmsg3));

    free(buffer);

    return 0;
}

int send_ok(http_state_t * hs, const char * ct) {
    char buffer[512];

    sprintf(buffer, "HTTP/1.0 200 OK\r\nContent-type: %s\r\nConnection: close\r\n\r\n", ct);
    write(hs->socket, buffer, strlen(buffer));

    return 0;
}

/**********************************************************************/

int do_dirlist(const char * name, http_state_t * hs, file_t f) {
    char * dl, *dlout;
    dirent_t * d;
    int dlsize, r;

    dl = malloc(65536);
    dlout = dl;
    
    if(dl == NULL)
        return -1;

    sprintf(dlout, "<html><head><title>Listing of %s</title></head></html>\n<body bgcolor=\"white\">\n", name);
    dlout += strlen(dlout);

    sprintf(dlout, "<h4>Listing of %s</h4>\n<hr>\n<table>\n", name);
    dlout += strlen(dlout);

    while((d = fs_readdir(f))) {
        if(d->size >= 0) {
            sprintf(dlout, "<tr><td><a href=\"%s\">%s</a></td><td>%d</td></tr>\n", d->name, d->name, d->size);
            dlout += strlen(dlout);
        }
        else {
            sprintf(dlout, "<tr><td><a href=\"%s/\">%s/</a></td><td>%d</td></tr>\n", d->name, d->name, d->size);
            dlout += strlen(dlout);
        }
    }

    sprintf(dlout, "</table>\n<hr>\nKOSHttp/1.0 server\n</body></html>\n");
    dlout += strlen(dlout);

    dlsize = strlen(dl);

    send_ok(hs, "text/html");
    dlout = dl;

    while(dlsize > 0) {
        r = write(hs->socket, dlout, dlsize);

        if(r <= 0) {
            free(dl);
            return -1;
        }

        dlsize -= r;
        dlout += r;
    }

    free(dl);

    return 0;
}

/**********************************************************************/

#define BUFSIZE (256*1024)

void *client_thread(void *p) {
    http_state_t * hs = (http_state_t *)p;
    char * buf, * ext;
    const char * ct;
    file_t f = -1;
    int r, o, cnt;

    printf("httpd: client thread started, sock %d\n", hs->socket);

    buf = malloc(BUFSIZE);

    if(read_headers(hs, buf, BUFSIZE) < 0) {
        goto out;
    }

    printf("httpd: client requested '%s'\n", buf);

    // Is it a directory or a file?
    f = fs_open(buf, O_RDONLY | O_DIR);

    if(f >= 0) {
        do_dirlist(buf, hs, f);
    }
    else {
        f = fs_open(buf, O_RDONLY);

        if(f < 0) {
            if(send_error(hs, 404, "File not found or unreadable") < 0)
                printf("Error sending 404\n");
            goto out;
        }

        ext = strrchr(buf, '.');
        ct = "application/octet-stream";

        if(ext) {
            ext++;

            if(!strcasecmp(ext, "jpg"))
                ct = "image/jpeg";
            else if(!strcasecmp(ext, "png"))
                ct = "image/png";
            else if(!strcasecmp(ext, "gif"))
                ct = "image/gif";
            else if(!strcasecmp(ext, "txt"))
                ct = "text/plain";
            else if(!strcasecmp(ext, "mp3"))
                ct = "audio/mpeg";
            else if(!strcasecmp(ext, "ogg"))
                ct = "application/ogg";
            else if(!strcasecmp(ext, "html"))
                ct = "text/html";
        }

        send_ok(hs, ct);

        while((cnt = fs_read(f, buf, BUFSIZE)) != 0) {
            o = 0;

            while(cnt > 0) {
                r = write(hs->socket, buf + o, cnt);

                if(r <= 0)
                    goto out;

                cnt -= r;
                o += r;
            }
        }
    }

    fs_close(f);

out:
    free(buf);
    printf("httpd: closed client connection %d\n", hs->socket);
    close(hs->socket);
    st_destroy(hs);

    if(f >= 0)
        fs_close(f);

    return NULL;
}

/**********************************************************************/

/*
int handle_read(http_state_t * hs) {
    char buffer[80];
    int rc;

    for ( ; ; ) {
        rc = read(hs->socket, buffer, 80);
        if (rc == 0)
            return -1;
        write(hs->socket, buffer, rc);
        if (rc < 80)
            return 0;
    }
}
*/

/**********************************************************************/

void httpd(void) {
    int listenfd;
    struct sockaddr_in saddr;
    fd_set readset;
    fd_set writeset;
    int i, maxfdp1;
    http_state_t *hs;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if(listenfd < 0) {
        printf("httpd: socket create failed\n");
        return;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(80);

    if(bind(listenfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        printf("httpd: bind failed\n");
        close(listenfd);
        return;
    }

    if(listen(listenfd, 10) < 0) {
        printf("httpd: listen failed\n");
        close(listenfd);
        return;
    }

    st_init();
    printf("httpd: listening for connections on socket %d\n", listenfd);

    for(; ;) {
        maxfdp1 = listenfd + 1;

        FD_ZERO(&readset);
        FD_ZERO(&writeset);
        FD_SET(listenfd, &readset);
        // maxfdp1 = st_add_fds(&readset, maxfdp1);
        // st_add_fds(&writeset);

        i = select(maxfdp1, &readset, &writeset, 0, 0);

        if(i == 0)
            continue;

        // Check for new incoming connections
        if(FD_ISSET(listenfd, &readset)) {
            hs = st_create();
            hs->client_size = sizeof(hs->client);
            hs->socket = accept(listenfd,
                                (struct sockaddr *)&hs->client,
                                &hs->client_size);
            printf("httpd: connect from %08lx, port %d, socket %d\n",
                   hs->client.sin_addr.s_addr, hs->client.sin_port, hs->socket);

            if(hs->socket < 0) {
                st_destroy(hs);
            }
            else {
                hs->thd = thd_create(1, client_thread, hs);
                // hs->thd = sys_thread_new(client_thread, hs, 0);
            }

            /* else if (ioctl(hs->socket, FIONBIO, &tmp) < 0) {
                printf("httpd: failed to set non-blocking\n");
                st_destroy(hs);
            } */
        }

#if 0
        // Process data from connected clients
        st_foreach(hs) {
            if(FD_ISSET(hs->socket, &readset)) {
                if(handle_read(hs) < 0) {
                    printf("httpd: disconnected socket %d\n", hs->socket);
                    close(hs->socket);
                    st_destroy(hs);
                    break;
                }
            }

            /* if (FD_ISSET(hs->socket, &writeset)) {
            } */
        }
#endif
    }
}
