/* KallistiOS ##version##

   gethostbyname.c
   Copyright (C) 2014 Lawrence Sebald

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netdb.h>

#ifdef __STRICT_ANSI__
/* Newlib doesn't prototype this function in strict standards compliant mode, so
   we'll do it here. It is still provided either way, but it isn't prototyped if
   we use -std=c99 (or any other non-gnuXX value). */
char *strdup(const char *);
#endif

int h_errno = 0;

/* XXXX: Values in here will be leaked on exit. Not that that really matters at
   all. */
static struct hostent he;

static void cleanup_hostent(void) {
    char *i;
    int j;

    if(he.h_aliases) {
        /* Free each entry in the arrays. */
        for(i = he.h_aliases[0], j = 0; i; j++, i = he.h_aliases[j]) {
            free(i);
        }
    }

    if(he.h_addr_list) {
        for(i = he.h_addr_list[0], j = 0; i; j++, i = he.h_addr_list[j]) {
            free(i);
        }
    }

    /* Free the name and the arrays themselves. */
    free(he.h_name);
    free(he.h_aliases);
    free(he.h_addr_list);

    /* Make sure everything's cleaned out properly. */
    memset(&he, 0, sizeof(he));
}

static int fill_hostent(const char *name, struct addrinfo *ai) {
    int addrs = 0, j;
    struct addrinfo *i;

    /* Fill in the basic information. */
    he.h_addrtype = ai->ai_family;

    if(he.h_addrtype == AF_INET)
        he.h_length = 4;
    else if(he.h_addrtype == AF_INET6)
        he.h_length = 16;
    else
        return NO_RECOVERY;

    /* Copy over the name first. */
    if(!(he.h_name = strdup(name)))
        return NO_RECOVERY;

    /* Make a blank list of aliases... */
    if(!(he.h_aliases = (char **)malloc(sizeof(char *))))
        return NO_RECOVERY;

    he.h_aliases[0] = NULL;

    /* Figure out how many addresses we have in the addrinfo chain. */
    for(i = ai; i; i = i->ai_next, ++addrs)
        ;

    if(!(he.h_addr_list = (char **)malloc((addrs + 1) * sizeof(char *))))
        return NO_RECOVERY;

    if(he.h_addrtype == AF_INET) {
        struct sockaddr_in *in;
        in_addr_t *addr;

        for(i = ai, j = 0; i; i = i->ai_next, ++j) {
            /* Make space for the address in question. */
            if(!(he.h_addr_list[j] = (char *)malloc(4))) {
                while(--j >= 0) {
                    free(he.h_addr_list[j]);
                }

                return NO_RECOVERY;
            }

            /* Store the address. */
            in = (struct sockaddr_in *)ai->ai_addr;
            addr = (in_addr_t *)he.h_addr_list[j];
            *addr = in->sin_addr.s_addr;
        }
    }
    else {
        struct sockaddr_in6 *in;

        for(i = ai, j = 0; i; i = i->ai_next, ++j) {
            /* Make space for the address in question. */
            if(!(he.h_addr_list[j] = (char *)malloc(16))) {
                while(--j >= 0) {
                    free(he.h_addr_list[j]);
                }

                return NO_RECOVERY;
            }

            /* Store the address. */
            in = (struct sockaddr_in6 *)ai->ai_addr;
            memcpy(he.h_addr_list[j], &in->sin6_addr.s6_addr, 16);
        }
    }

    he.h_addr_list[addrs] = NULL;

    return 0;
}

struct hostent *gethostbyname2(const char *name, int af) {
    struct addrinfo hints;
    struct addrinfo *rv;
    int err;

    /* Set up a query to getaddrinfo(). */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = af;

    err = getaddrinfo(name, NULL, &hints, &rv);

    switch(err) {
        case 0:
            /* Success. XXXX: Do something with the data. */
            break;

        case EAI_FAIL:
            h_errno = NO_RECOVERY;
            return NULL;

        case EAI_MEMORY:
        case EAI_AGAIN:
        case EAI_SYSTEM:
            h_errno = TRY_AGAIN;
            return NULL;

        case EAI_NONAME:
            h_errno = HOST_NOT_FOUND;
            return NULL;

        default:
            /* None of the other errors should happen, so if they do, then it is
               a bad thing. Assume the worst. */
            h_errno = NO_RECOVERY;
            return NULL;
    }

    /* Clean up any old entry in the static hostent. */
    cleanup_hostent();

    /* Fill in the hostent with the information we got from getaddrinfo() and
       clean up. */
    err = fill_hostent(name, rv);
    freeaddrinfo(rv);

    if(err) {
        h_errno = err;
        return NULL;
    }

    return &he;
}

struct hostent *gethostbyname(const char *name) {
    return gethostbyname2(name, AF_INET);
}
