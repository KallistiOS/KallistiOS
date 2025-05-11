/* KallistiOS ##version##

   getaddrinfo.c

   Copyright (C) 2014 Lawrence Sebald

   Originally:
   lwip/dns.c
   Copyright (C) 2004 Megan Potter
*/

/* The actual code for querying the DNS was taken from the old kos-ports lwIP
   port, and was originally written by Megan. This has been modified/extended a
   bit. The old lwip_gethostbyname() function was reworked a lot to create the
   new getaddrinfo_dns() function. In addition, the dns_parse_response()
   function got a bit of a makeover too.

   The implementations of getaddrinfo() and freeaddrinfo() are new to this
   version of the code though.

   Eventually, I'd like to add some sort of caching of results to this file so
   that if you look something up more than once, it doesn't have to go back out
   to the server to do so. But, for now, there is no local database of cached
   addresses...
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <kos/net.h>
#include <kos/dbglog.h>

/* How many attempts to make at contacting the DNS server before giving up. */
#define DNS_ATTEMPTS    4

/* How long to wait between attempts. */
#define DNS_TIMEOUT     500

/*
   This performs a simple DNS A-record query. It hasn't been tested extensively
   but so far it seems to work fine.

   This relies on the really sketchy UDP support in the KOS lwIP port, so it
   can be cleaned up later once that's improved.

   We really need to be setting errno in here too...
 */



/* Basic query process:

   - Send a DNS message to the server on port 53, with one query payload.
   - Receive DNS message from port 53 with one or more answer payloads (hopefully).
 */

// These structs were all taken from RFC1035, page 26.
typedef struct dnsmsg {
    uint16_t id;         // Can be anything
    uint16_t flags;      // Should have 0x0100 set for query, 0x8000 for response
    uint16_t qdcount, ancount, nscount, arcount;
    uint8_t data[];      // Payload
} dnsmsg_t;

static uint16_t qnum = 0;

#define QTYPE_A         1
#define QTYPE_AAAA      28

/* Flags:
   Query/Response (1 bit) -- 0 = Query, 1 = Response
   Opcode (4 bits) -- 0 = Standard, 1 = Inverse, 2 = Status
   AA (1 bit) -- authoritative answer
   TC (1 bit) -- truncated message
   RD (1 bit) -- recursion desired
   RA (1 bit) -- recursion available
   Z (1 bit) -- zero
   RCODE (4 bits) -- 0 = No Error, >0 = Error

   Generally a query will have 0x0100 here, and a typical
   response will have 0x8180.
*/

/* Query section. A standard DNS query will have one query section
   and no other payloads. There is no padding.

   QNAME: one or more "labels", representing a domain name. For
   example "yuna.dp.allusion.net" is "yuna, dp, allusion, net". Each
   label has one length byte followed by N data bytes. A zero
   length byte terminates.

   QTYPE: two-byte code specifying the RR type of the query. For a
   normal DNS query this should be 0x0001 (A - IPv4) or 0x001C (AAAA - IPv6).

   QCLASS: two-byte code specifying the class of the query. For a
   normal DNS query this should be 0x0001 (IN).

   Common RR types:
     A      1
     NS     2
     CNAME  5
     SOA    6
     PTR    12
     MX     15
     TXT    16
     AAAA   28
 */

// Construct a DNS query for an A record by host name. "buf" should
// be at least 1500 bytes, to make sure there's room.
static size_t dns_make_query(const char *host, dnsmsg_t *buf, int ip4,
                             int ip6) {
    int i, o = 0, ls, t;

    // Build up the header.
    buf->id = htons(qnum++);
    buf->flags = htons(0x0100);
    buf->qdcount = htons(ip4 + ip6);
    buf->ancount = htons(0);
    buf->nscount = htons(0);
    buf->arcount = htons(0);

    /* Fill in the question section(s). */
    ls = 0;

    if(ip4) {
        o = ls + 1;
        t = strlen(host);

        for(i = 0; i <= t; i++) {
            if(host[i] == '.' || i == t) {
                buf->data[ls] = (o - ls) - 1;
                ls = o;
                o++;
            }
            else {
                buf->data[o++] = host[i];
            }
        }

        buf->data[ls] = 0;

        // Might be unaligned now... so just build it by hand.
        buf->data[o++] = (uint8_t)(QTYPE_A >> 8);
        buf->data[o++] = (uint8_t)QTYPE_A;
        buf->data[o++] = 0x00;
        buf->data[o++] = 0x01;
        ls = o;
    }

    if(ip6) {
        o = ls + 1;
        t = strlen(host);

        for(i = 0; i <= t; i++) {
            if(host[i] == '.' || i == t) {
                buf->data[ls] = (o - ls) - 1;
                ls = o;
                o++;
            }
            else {
                buf->data[o++] = host[i];
            }
        }

        buf->data[ls] = 0;

        // Might be unaligned now... so just build it by hand.
        buf->data[o++] = (uint8_t)(QTYPE_AAAA >> 8);
        buf->data[o++] = (uint8_t)QTYPE_AAAA;
        buf->data[o++] = 0x00;
        buf->data[o++] = 0x01;
    }

    // Return the full message size.
    return (size_t)(o + sizeof(dnsmsg_t));
}

/* Resource records. A standard DNS response will have one query
   section (the original one) plus an answer section. It may have
   other sections but these can be ignored.

   NAME: Same as QNAME, with one caveat (see below).
   TYPE: Two-byte RR code (same as QTYPE).
   CLASS: Two-byte class code (same as QCLASS).
   TTL: Four-byte time to live interval in seconds; this entry should
     not be cached longer than this.
   RDLENGTH: Two-byte response length (in bytes).
   RDATA: Response data, size is RDLENGTH.

   For "NAME", note that this may also be a "back pointer". This is
   to save space in DNS queries. Back pointers are 16-bit values with
   the upper two bits set to one, and the lower bits representing an
   offset within the full DNS message. So e.g., 0xc00c for the NAME
   means to look at offset 12 in the full message to find the NAME
   record.

   For A records, RDLENGTH is 4 and RDATA is a 4-byte IP address.

   When doing queries on the internet you may also get back CNAME
   entries. In these responses you may have more than one answer
   section (e.g. a 5 and a 1). The CNAME answer will contain the real
   name, and the A answer contains the address.
 */

// Scans through and skips a label in the data payload, starting
// at the given offset. The new offset (after the label) will be
// returned.
static int dns_skip_label(dnsmsg_t *resp, int o) {
    int cnt;

    // End of the label?
    while(resp->data[o] != 0) {
        // Is it a pointer?
        if((resp->data[o] & 0xc0) == 0xc0)
            return o + 2;

        // Skip this part.
        cnt = resp->data[o++];
        o += cnt;
    }

    // Skip the terminator
    o++;

    return o;
}

// Scans through and copies out a label in the data payload,
// starting at the given offset. The new offset (after the label)
// will be returned as well as the label string.
static int dns_copy_label(dnsmsg_t *resp, int o, char *outbuf) {
    int cnt;
    int rv = -1;

    // Do each part.
    outbuf[0] = 0;

    for(;;) {
        // Is it a pointer?
        if((resp->data[o] & 0xc0) == 0xc0) {
            int offs = ((resp->data[o] & ~0xc0) << 8) | resp->data[o + 1];

            if(rv < 0)
                rv = o + 2;

            o = offs - sizeof(dnsmsg_t);
        }
        else {
            cnt = resp->data[o++];

            if(cnt) {
                if(outbuf[0] != 0)
                    strcat(outbuf, ".");

                strncat(outbuf, (char *)(resp->data + o), cnt);
                o += cnt;
            }
            else {
                break;
            }
        }
    }

    if(rv < 0)
        return o;
    else
        return rv;
}

/* Forward declaration... */
static struct addrinfo *add_ipv4_ai(uint32_t ip, uint16_t port,
                                    struct addrinfo *h, struct addrinfo *tail);
static struct addrinfo *add_ipv6_ai(const struct in6_addr *ip, uint16_t port,
                                    struct addrinfo *h, struct addrinfo *tail);

// Parse a response packet from the DNS server. The IP address
// will be filled in upon a successful return, otherwise the
// return value will be <0.
static int dns_parse_response(dnsmsg_t *resp, struct addrinfo *hints,
                              uint16_t port, struct addrinfo **res) {
    int i, o, arecs;
    uint16_t flags;
    char tmp[64];
    uint16_t ancnt, len;
    struct addrinfo *ptr = NULL;

    /* Check the flags first to see if it was successful. */
    flags = ntohs(resp->flags);

    if(!(flags & 0x8000)) {
        /* Not our response! */
        return EAI_AGAIN;
    }

    /* Did the server report an error? */
    switch(flags & 0x000f) {
        case 0:   /* No error */
            break;

        case 1:   /* Format error */
        case 4:   /* Not implemented */
        case 5:   /* Refused */
        default:
            return EAI_FAIL;

        case 3:   /* Name error */
            return EAI_NONAME;

        case 2:   /* Server failure */
            return EAI_AGAIN;
    }

    /* Getting zero answers is also a failure. */
    ancnt = ntohs(resp->ancount);

    if(ancnt < 1)
        return EAI_NONAME;

    /* If we have any query sections (should have at least one), skip 'em. */
    o = 0;
    len = ntohs(resp->qdcount);

    for(i = 0; i < len; i++) {
        /* Skip the label. */
        o = dns_skip_label(resp, o);

        /* And the two type fields. */
        o += 4;
    }

    /* Ok, now the answer section (what we're interested in). */
    arecs = 0;

    for(i = 0; i < ancnt; i++) {
        /* Copy out the label, in case we need it. */
        o = dns_copy_label(resp, o, tmp);

        /* Get the type code. If it's not A or AAAA, skip it. */
        if(resp->data[o] == 0 && resp->data[o + 1] == 1 &&
           (hints->ai_family == AF_INET || hints->ai_family == AF_UNSPEC)) {
            uint32_t addr;

            o += 8;
            len = (resp->data[o] << 8) | resp->data[o + 1];
            o += 2;

            /* Grab the address from the response. */
            addr = htonl((resp->data[o] << 24) | (resp->data[o + 1] << 16) |
                         (resp->data[o + 2] << 8) | resp->data[o + 3]);

            /* Add this address to the chain. */
            if(!(ptr = add_ipv4_ai(addr, port, hints, ptr)))
                /* If something goes wrong in here, it's in calling malloc, so
                   it is definitely a system error. */
                return EAI_SYSTEM;

            if(!*res)
                *res = ptr;

            o += len;
            arecs++;
        }
        else if(resp->data[o] == 0 && resp->data[o + 1] == 28 &&
                (hints->ai_family == AF_INET6 || hints->ai_family == AF_UNSPEC)) {
            struct in6_addr addr;

            o += 8;
            len = (resp->data[o] << 8) | resp->data[o + 1];
            o += 2;

            /* Grab the address from the response. */
            memcpy(addr.s6_addr, &resp->data[o], 16);

            /* Add this address to the chain. */
            if(!(ptr = add_ipv6_ai(&addr, port, hints, ptr)))
                /* If something goes wrong in here, it's in calling malloc, so
                   it is definitely a system error. */
                return EAI_SYSTEM;

            if(!*res)
                *res = ptr;

            o += len;
            arecs++;
        }
        else if(resp->data[o] == 0 && resp->data[o + 1] == 5) {
            char tmp2[64];

            o += 8;
            len = (resp->data[o] << 8) | resp->data[o + 1];
            o += 2;
            o = dns_copy_label(resp, o, tmp2);
        }
        else {
            o += 8;
            len = (resp->data[o] << 8) | resp->data[o + 1];
            o += 2 + len;
        }
    }

    /* Did we find something? */
    return arecs > 0 ? 0 : EAI_NONAME;
}

static int getaddrinfo_dns(const char *name, struct addrinfo *hints,
                           uint16_t port, struct addrinfo **res) {
    struct sockaddr_in toaddr;
    uint8_t qb[512];
    size_t size;
    int sock, rv, tries;
    in_addr_t raddr;
    ssize_t rsize = 0;
    struct pollfd pfd;

    /* Make sure we have a network device to communicate on. */
    if(!net_default_dev) {
        errno = ENETDOWN;
        return EAI_SYSTEM;
    }

    /* Do we have a DNS server specified? */
    if(net_default_dev->dns[0] == 0 && net_default_dev->dns[1] == 0 &&
       net_default_dev->dns[2] == 0 && net_default_dev->dns[3] == 0) {
        return EAI_FAIL;
    }

    /* Setup a query */
    if(hints->ai_family == AF_UNSPEC)
        /* Note: This should (in theory) work, but it seems that some resolvers
           cannot handle multiple questions in one query. So... while the code
           here supports multiple questions in one query, this branch will never
           actually be taken -- getaddrinfo() will always make two separate
           calls if we need to do both IPv4 and IPv6. */
        size = dns_make_query(name, (dnsmsg_t *)qb, 1, 1);
    else if(hints->ai_family == AF_INET)
        size = dns_make_query(name, (dnsmsg_t *)qb, 1, 0);
    else if(hints->ai_family == AF_INET6)
        size = dns_make_query(name, (dnsmsg_t *)qb, 0, 1);
    else {
        errno = EAFNOSUPPORT;
        return EAI_SYSTEM;
    }

    /* Make a socket to talk to the DNS server. */
    if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        return EAI_SYSTEM;

    /* "Connect" the socket to the DNS server's address. */
    raddr = (net_default_dev->dns[0] << 24) | (net_default_dev->dns[1] << 16) |
            (net_default_dev->dns[2] << 8) | net_default_dev->dns[3];

    memset(&toaddr, 0, sizeof(toaddr));
    toaddr.sin_family = AF_INET;
    toaddr.sin_port = htons(53);
    toaddr.sin_addr.s_addr = htonl(raddr);

    if(connect(sock, (struct sockaddr *)&toaddr, sizeof(toaddr))) {
        close(sock);
        return EAI_SYSTEM;
    }

    /* Set up the structure we'll use to feed to the poll function. */
    pfd.fd = sock;
    pfd.events = POLLIN;
    pfd.revents = 0;

    for(tries = 0; tries < DNS_ATTEMPTS; ++tries) {
        /* Send the query to the server. */
        if(send(sock, qb, size, 0) < 0) {
            return EAI_SYSTEM;
        }

        /* Wait for the timeout to expire or for us to get the response. */
        if(poll(&pfd, 1, DNS_TIMEOUT) == 1) {
            /* Get the response. */
            if((rsize = recv(sock, qb, 512, 0)) < 0) {
                return EAI_SYSTEM;
            }

            break;
        }
    }

    /* Close the socket */
    close(sock);

    /* If we never actually got a response, then there's probably a problem with
       the server on the other end. I'm not entirely sure what to return in that
       case, to be perfectly honest. I suppose that EAI_SYSTEM + ETIMEDOUT would
       make the most sense, since that's really what happened... */
    if(!rsize) {
        errno = ETIMEDOUT;
        return EAI_SYSTEM;
    }

    /* Parse the response. */
    rv = dns_parse_response((dnsmsg_t *)qb, hints, port, res);

    /* If we got something in the result, but got an error value in the return,
       free whatever we got. */
    if(rv != 0 && *res) {
        freeaddrinfo(*res);
        *res = NULL;
    }

    return rv;
}

/* New stuff below here... */

static struct addrinfo *add_ipv4_ai(uint32_t ip, uint16_t port,
                                    struct addrinfo *h, struct addrinfo *tail) {
    struct addrinfo *result;
    struct sockaddr_in *addr;

    /* Allocate all our space first. */
    if(!(result = (struct addrinfo *)malloc(sizeof(struct addrinfo)))) {
        return NULL;
    }

    if(!(addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in)))) {
        free(result);
        return NULL;
    }

    /* Fill in the sockaddr_in structure */
    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_port = port;
    addr->sin_addr.s_addr = ip;

    /* Fill in the addrinfo structure */
    result->ai_flags = 0;
    result->ai_family = AF_INET;
    result->ai_socktype = h->ai_socktype;
    result->ai_protocol = h->ai_protocol;
    result->ai_addrlen = sizeof(struct sockaddr_in);
    result->ai_addr = (struct sockaddr *)addr;
    result->ai_canonname = NULL; /* XXXX: might actually need this. */
    result->ai_next = NULL;

    /* Add the result to the list (or make it the list head if it is the
    first entry). */
    if(tail)
        tail->ai_next = result;

    return result;
}

static struct addrinfo *add_ipv6_ai(const struct in6_addr *ip, uint16_t port,
                                    struct addrinfo *h, struct addrinfo *tail) {
    struct addrinfo *result;
    struct sockaddr_in6 *addr;

    /* Allocate all our space first. */
    if(!(result = (struct addrinfo *)malloc(sizeof(struct addrinfo)))) {
        return NULL;
    }

    if(!(addr = (struct sockaddr_in6 *)malloc(sizeof(struct sockaddr_in6)))) {
        free(result);
        return NULL;
    }

    /* Fill in the sockaddr_in structure */
    memset(addr, 0, sizeof(struct sockaddr_in6));
    addr->sin6_family = AF_INET6;
    addr->sin6_port = port;
    addr->sin6_addr = *ip;

    /* Fill in the addrinfo structure */
    result->ai_flags = 0;
    result->ai_family = AF_INET6;
    result->ai_socktype = h->ai_socktype;
    result->ai_protocol = h->ai_protocol;
    result->ai_addrlen = sizeof(struct sockaddr_in6);
    result->ai_addr = (struct sockaddr *)addr;
    result->ai_canonname = NULL; /* XXXX: might actually need this. */
    result->ai_next = NULL;

    /* Add the result to the list (or make it the list head if it is the
    first entry). */
    if(tail)
        tail->ai_next = result;

    return result;
}

void freeaddrinfo(struct addrinfo *ai) {
    struct addrinfo *next;

    /* As long as we've still got something, move along the chain. */
    while(ai) {
        next = ai->ai_next;

        /* Free up anything that might have been malloced. */
        free(ai->ai_addr);
        free(ai->ai_canonname);
        free(ai);

        /* Continue to the next entry, if any. */
        ai = next;
    }
}

int getaddrinfo(const char *nodename, const char *servname,
                const struct addrinfo *hints, struct addrinfo **res) {
    in_port_t port = 0;
    unsigned long tmp;
    char *endp;
    int old_errno;
    struct addrinfo ihints;

    (void)hints;

    /* What to do if res is NULL?... I'll assume we should return error... */
    if(!res) {
        errno = EFAULT;
        return EAI_SYSTEM;
    }

    *res = NULL;

    /* Check the input parameters... */
    if(!nodename && !servname)
        return EAI_NONAME;

    /* We don't support service resolution from service name strings, so make
       sure that if servname is specified, that it is a numeric string. */
    if(servname) {
        old_errno = errno;
        tmp = strtoul(servname, &endp, 10);
        errno = old_errno;

        /* If the return value is greater than the maximum value of a 16-bit
           unsigned integer or the character at *endp is not NUL, then we didn't
           have a pure numeric string (or had one that wasn't valid). Return an
           error in either case. */
        if(tmp > 0xffff || *endp)
            return EAI_NONAME;

        /* Go ahead and swap the byte order of the port now, if we need to. */
        port = htons((uint16_t)tmp);
    }

    /* Did the user give us any hints? */
    if(hints)
        memcpy(&ihints, hints, sizeof(ihints));
    else
        memset(&ihints, 0, sizeof(ihints));

    /* Do we want a local address or a remote one? */
    if(!nodename) {
        struct addrinfo *r = NULL;

        /* Is the passive flag set to indicate we want everything set up for a
           bind? */
        if(ihints.ai_flags & AI_PASSIVE) {
            if(ihints.ai_family == AF_INET || ihints.ai_family == AF_UNSPEC) {
                if(!(r = add_ipv4_ai(INADDR_ANY, port, &ihints, r)))
                    return EAI_SYSTEM;

                *res = r;
            }

            if(ihints.ai_family == AF_INET6 || ihints.ai_family == AF_UNSPEC) {
                if(!(r = add_ipv6_ai(&in6addr_any, port, &ihints, r))) {
                    freeaddrinfo(*res);
                    return EAI_SYSTEM;
                }

                if(!*res)
                    *res = r;
            }
        }
        else {
            if(ihints.ai_family == AF_INET || ihints.ai_family == AF_UNSPEC) {
                uint32_t addr = htonl(0x7f000001);

                if(!(r = add_ipv4_ai(addr, port, &ihints, r)))
                    return EAI_SYSTEM;

                *res = r;
            }

            if(ihints.ai_family == AF_INET6 || ihints.ai_family == AF_UNSPEC) {
                if(!(r = add_ipv6_ai(&in6addr_loopback, port, &ihints, r))) {
                    freeaddrinfo(*res);
                    return EAI_SYSTEM;
                }

                if(!*res)
                    *res = r;
            }
        }

        return 0;
    }

    /* Try to handle input as an IPv4 address */
    if(ihints.ai_family == AF_INET || ihints.ai_family == AF_UNSPEC) {
        uint32_t ip4_addr;

        if(inet_pton(AF_INET, nodename, &ip4_addr) > 0) {
            ihints.ai_family = AF_INET;
            *res = add_ipv4_ai(ip4_addr, port, &ihints, NULL);
            return 0;
        }
    }

    /* Try to handle input as an IPv6 address */
    if(ihints.ai_family == AF_INET6 || ihints.ai_family == AF_UNSPEC) {
        struct in6_addr addr;

        if(inet_pton(AF_INET6, nodename, &addr.s6_addr) > 0) {
            ihints.ai_family = AF_INET6;
            *res = add_ipv6_ai(&addr, port, &ihints, NULL);
            return 0;
        }
    }

    /* If we've gotten this far, do the lookup. */
    if(ihints.ai_family == AF_UNSPEC) {
        /* It seems that some resolvers really don't like multi-part questions.
           So, make sure we only ever send one part at a time... */
        struct addrinfo *res1 = NULL, *res2 = NULL;
        int rv;

        ihints.ai_family = AF_INET;
        rv = getaddrinfo_dns(nodename, &ihints, port, &res1);

        if(rv && rv != EAI_NONAME)
            return rv;

        ihints.ai_family = AF_INET6;
        getaddrinfo_dns(nodename, &ihints, port, &res2);

        /* Figure out what to do with the result(s). */
        if(res1 && res2) {
            *res = res1;

            /* Go to the end of the chain. */
            while(res1->ai_next) {
                res1 = res1->ai_next;
            }

            res1->ai_next = res2;
            return 0;
        }
        else if(res1) {
            *res = res1;
            return 0;
        }
        else if(res2) {
            *res = res2;
            return 0;
        }
        else {
            return EAI_NONAME;
        }
    }

    return getaddrinfo_dns(nodename, &ihints, port, res);
}
