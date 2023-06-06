/* KallistiOS ##version##

   inet_ntop.c
   Copyright (C) 2007, 2010, 2011 Lawrence Sebald

*/

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

static const char *inet_ntop4(const void *src, char *dst, socklen_t size) {
    char tmp[3];
    int i, part;
    char *ch = tmp;
    char *ch2 = dst;
    struct in_addr *addr = (struct in_addr *)src;

    /* Parse each 8 bits individually. */
    for(i = 0; i < 4; ++i) {
        /* Treat the 32-bit address value as if it were an array of 8-bit
           values. This works regardless of the endianness of the host system
           because the specs require the address passed in here to be in
           network byte order (big endian). */
        part = ((uint8_t *) &addr->s_addr)[i];

        do {
            *ch++ = '0' + (char)(part % 10);
            part /= 10;
        }
        while(part);

        /* tmp now contains the inverse of the number that is in the given
           8 bits. Reverse it for the final result, rewinding ch to the
           beginning of tmp in the process. */
        while(ch != tmp && size) {
            *ch2++ = *--ch;
            --size;
        }

        if(!size) {
            dst[0] = 0;
            errno = ENOSPC;
            return NULL;
        }

        *ch2++ = '.';
        --size;
    }

    /* There's a trailing '.' at the end of the address, change it to the
       required NUL character */
    *--ch2 = 0;

    return dst;
}

static const char *inet_ntop6(const void *src, char *dst, socklen_t size) {
    int tmp[8] = { 0 };
    int runstart = -1, maxzero = 0;
    socklen_t i, dcs = (socklen_t)-1;
    char tmpstr[4];
    char *ch = tmpstr, *ch2 = dst;
    int part;
    struct in6_addr addr;

    /* Copy the address, just in case the original was misaligned */
    memcpy(&addr, src, sizeof(struct in6_addr));

    /* Handle the special cases of IPv4 Mapped and Compatibility addresses */
    if(IN6_IS_ADDR_V4MAPPED(&addr)) {
        if(size > 7) {
            dst[0] = dst[1] = dst[6] = ':';
            dst[2] = dst[3] = dst[4] = dst[5] = 'f';

            /* Parse the IPv4 address at the end */
            if(!inet_ntop4(&addr.__s6_addr.__s6_addr32[3], dst + 7, size - 7))
                goto err;

            return dst;
        }
        else {
            goto err;
        }
    }
    else if(IN6_IS_ADDR_V4COMPAT(&addr)) {
        if(size > 2) {
            dst[0] = dst[1] = ':';

            /* Parse the IPv4 address at the end */
            if(!inet_ntop4(&addr.__s6_addr.__s6_addr32[3], dst + 2, size - 2))
                goto err;

            return dst;
        }
        else {
            goto err;
        }
    }

    /* Figure out if we have any use for double colons in the address or not */
    for(i = 0; i < 8; ++i) {
        if(addr.__s6_addr.__s6_addr16[i] == 0) {
            if(runstart != -1) {
                ++tmp[runstart];
            }
            else {
                runstart = i;
                tmp[i] = 1;
            }

            if(tmp[runstart] > maxzero) {
                maxzero = tmp[runstart];
                dcs = runstart;
            }
        }
        else {
            runstart = -1;
        }
    }

    /* We should now know where the double colons will be, and how many zeroes
       they will replace, the rest is pretty easy. */
    i = 0;

    if(dcs == 0) {
        if(size > 2) {
            *ch2++ = ':';
            *ch2++ = ':';
            size -= 2;
            i = maxzero;
        }
        else {
            goto err;
        }
    }

    while(i < 8) {
        if(i == dcs) {
            if(size > 1) {
                *ch2++ = ':';
                --size;
                i += maxzero;
            }
            else {
                goto err;
            }
        }
        else {
            part = ntohs(addr.__s6_addr.__s6_addr16[i]);

            do {
                *ch = (char)(part & 0x0f) + '0';
                part >>= 4;

                /* Deal with digits greater than 9 */
                if(*ch > '9') {
                    *ch = *ch - ':' + 'a';
                }

                ch++;
            }
            while(part);

            /* tmp now contains the inverse of the number that is in the given
               16 bits. Reverse it for the final result, rewinding ch to the
               beginning of tmpstr in the process. */
            while(ch != tmpstr && size) {
                *ch2++ = *--ch;
                --size;
            }

            if(size < 1) {
                goto err;
            }

            *ch2++ = ':';
            --size;
            ++i;
        }
    }

    /* Change the last : to a NUL terminator, unless the last set was where we
       had a run of zeroes replaced by a :: */
    if(dcs + maxzero != 8) {
        *(ch2 - 1) = 0;
    }
    else if(size > 0) {
        *ch2 = 0;
    }
    else {
        goto err;
    }

    return dst;

err:

    /* In the event of an error, clear whatever we may have done */
    for(i = 0; i < size; ++i) {
        dst[i] = 0;
    }

    errno = ENOSPC;
    return NULL;
}

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size) {
    if(size < 1) {
        errno = ENOSPC;
        return NULL;
    }

    switch(af) {
        case AF_INET:
            return inet_ntop4(src, dst, size);

        case AF_INET6:
            return inet_ntop6(src, dst, size);

        default:
            errno = EAFNOSUPPORT;
            return NULL;
    }
}
