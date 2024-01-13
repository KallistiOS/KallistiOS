/* KallistiOS ##version##

   Sample failure with socket-based writes.

   Copyright (c) 2023 Donald Haase
   built from:
   httpd.c
   Copyright (C)2003 Megan Potter
*/

/* The example arose as an attempt to do a speed test. My thought was 'whatever you 'GET', 
   send a file of that size. This would be a way to test transmit speed in the bba driver.
   
   What ends up happening though is that anything over a few loops of socket writes will just 
   never end for some reason. Colton on discord was able to take a similar example and 
   demonstrate some stack corruption. It most certainly has something to do with the socket 
   write command being too fast, as putting any sleep in that loop fixes the issue.
   
   In addition to that, for reasons I'm not able to immediately understand, if the main do_http 
   function is run from main instead of as a thread (with no empty for loop at all) then nothing 
   works at all.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#include <kos/thread.h>
#include <kos/mutex.h>

#include <kos/thread.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

#include <arch/arch.h>
#include <arch/timer.h>

/* This first chunk is all identical to the httpd example */
/**********************************************************************/
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

int st_init() {
    TAILQ_INIT(&states);
    return 0;
}

http_state_t * st_create() {
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

/**********************************************************************/

// This is undoubtedly very slow
int readline(int sock, char *buf, int bufsize) {
    int r, rt =0;
    char c;

    if(buf == NULL) return -1;

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

    for(i = 0; ; i++) {
        if(readline(hs->socket, buffer, bufsize) < 0) {
            if(i > 0)
                return 0;
            else
                return -1;
        }

        if(strlen(buffer) == 0)
            break;
        
        if(i == 0) {
            if(!strncmp(buffer, "GET", 3)) {
                printf("httpd: read GET header: '%s'\n", buffer);
                for(j = 4; buffer[j] && buffer[j] != 32 && j < 256; j++) {
                    fn[j - 4] = buffer[j];
                }
                
                fn[j - 4] = 0;
            }
        }
    }

    strcpy(buffer, fn);
    
    return 0;
}

/**********************************************************************/

int send_ok(http_state_t * hs, const char * ct) {
    char buffer[512];

    sprintf(buffer, "HTTP/1.0 200 OK\r\nContent-type: %s\r\nConnection: close\r\n\r\n", ct);
    write(hs->socket, buffer, strlen(buffer));

    return 0;
}
/* From here on things differ from the httpd example */
/**********************************************************************/

int do_dirlist(const char * name, http_state_t * hs) {
    char * dl, *dlout;
    int dlsize, r;

    dl = malloc(65536);
    dlout = dl;

    sprintf(dlout, "<html><head><title>Listing of %s</title></head></html>\n<body bgcolor=\"white\">\n", name);
    dlout += strlen(dlout);

    sprintf(dlout, "<tr><td><a href=\"500.txt\">Click to download a 500B file.</a></td></tr>\n");
    dlout += strlen(dlout);
    sprintf(dlout, "<tr><td><a href=\"10000.txt\">Click to download a 10KB file.</a></td></tr>\n");
    dlout += strlen(dlout);

    dlsize = strlen(dl);
    dlout = dl;

    send_ok(hs, "text/html");
    

    while(dlsize > 0) {
        r = write(hs->socket, dlout, dlsize);

        if(r <= 0)
            return -1;

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
    char * buf;    
    const char * sampletext = kos_get_banner();

    printf("httpd: client thread started, sock %d\n", hs->socket);

    buf = calloc(BUFSIZE, 1);

    if(read_headers(hs, buf, BUFSIZE) < 0) {
        goto out;
    }

    printf("httpd: client requested '%s'\n", buf);

    /* Check for invalid or just '/' */
    if(strlen(buf) <=1) {
        do_dirlist(buf, hs);
    }
    else {
        /* Starting at the second character, try to read as a dec number and put the end to leftover */
        unsigned int size = strtol(buf + 1, NULL, 10);
        
        /* Couldn't pull a number */
        if(size == 0) size=1000;

        unsigned int o = size;

        send_ok(hs, "application/download");
        
        printf("Sending (%i bytes) of data\n", o);
        
        uint64 time = timer_ms_gettime64();

        /* If this goes for any more than a few loops, it just never stops */
        while(o > 0) {
            int r = write(hs->socket, sampletext, strlen(sampletext));

            if(r <= 0)
                break;

            o -= r;
            /* Uncommenting either of the below will allow this to work right 
               my assumption is that there's some timing issue that is not 
               being properly addressed in the way the socket write works. */
            //printf("Transferred %lli bytes, %lli left\n", r, o);
            //thd_sleep(5);
        }

        time = timer_ms_gettime64() - time;
        printf("Sent %i + (%i bytes) of data in %f seconds\n", 
            size, (o * -1), (time / 1000.00));
    }

out:
    free(buf);
    printf("httpd: closed client connection %d\n", hs->socket);
    close(hs->socket);
    st_destroy(hs);

    return NULL;
}

void *do_httpd(void* foo) {
    int listenfd;
    struct sockaddr_in saddr;
    fd_set readset;
    fd_set writeset;
    int i, maxfdp1;
    http_state_t *hs;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if(listenfd < 0) {
        printf("httpd: socket create failed\n");
        return NULL;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(80);

    if(bind(listenfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        printf("httpd: bind failed\n");
        close(listenfd);
        return NULL;
    }

    if(listen(listenfd, 10) < 0) {
        printf("httpd: listen failed\n");
        close(listenfd);
        return NULL;
    }

    st_init();
    printf("httpd: listening for connections on socket %d\n", listenfd);

    for(; ;) {
        thd_sleep(50);
        
        maxfdp1 = listenfd + 1;

        FD_ZERO(&readset);
        FD_ZERO(&writeset);
        FD_SET(listenfd, &readset);

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
                /* Changing this to not be a thread makes no difference 
                   I've defaulted it to just call the function to reduce 
                   the change that the problem is with threading. */
                //hs->thd = thd_create(1, client_thread, hs);
                client_thread((void*) hs);
            }
        }
    }
}

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_NET);

int main(int argc, char **argv) {

    /* Press Start to exit */
    cont_btn_callback(0, CONT_START, (cont_btn_callback_t)arch_exit);
    
    /* For whatever reason, if the do_httpd function is made the main loop, nothing works */
    thd_create(1, do_httpd, NULL);

    for(; ;) {
        thd_sleep(50);
    }

    return 0;
}
