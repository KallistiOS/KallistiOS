/* KallistiOS ##version##

   kernel/net/net_ipv4_frag.c
   Copyright (C) 2009, 2013 Lawrence Sebald

*/

#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include <kos/net.h>
#include <kos/mutex.h>
#include <arch/timer.h>
#include <arch/irq.h>

#include "net_ipv4.h"
#include "net_thd.h"

#define MAX(a, b) a > b ? a : b;

struct ip_frag {
    TAILQ_ENTRY(ip_frag) listhnd;

    uint32_t src;
    uint32_t dst;
    uint16_t ident;
    uint8_t proto;

    ip_hdr_t hdr;
    uint8_t *data;
    uint8_t bitfield[8192];
    int cur_length;
    int total_length;
    uint64_t death_time;
};

TAILQ_HEAD(ip_frag_list, ip_frag);

static struct ip_frag_list frags;
static mutex_t frag_mutex = MUTEX_INITIALIZER;
static int cbid = -1;
static int initted = 0;

/* IP fragment "thread" -- this thread is set up to delete fragments for which
   the "death_time" has passed. This is run approximately once every two
   seconds (since death_time is always on the order of seconds). */
static void frag_thd_cb(void *data) {
    struct ip_frag *f, *n;
    uint64_t now = timer_ms_gettime64();

    (void)data;

    mutex_lock_scoped(&frag_mutex);

    /* Look at each fragment item, and see if the timer has expired. If so,
       remove it. */
    f = TAILQ_FIRST(&frags);

    while(f) {
        n = TAILQ_NEXT(f, listhnd);

        if(f->death_time < now) {
            TAILQ_REMOVE(&frags, f, listhnd);
            free(f->data);
            free(f);
        }

        f = n;
    }
}

/* Set the bits in the bitfield for the given set of fragment blocks. */
static inline void set_bits(uint8_t *bitfield, int start, int end) {
    /* Use a slightly more efficient method when dealing with an end that is not
       in the same byte as the start. */
    if((end >> 3) > (start >> 3)) {
        int bits = start & 0x07;

        /* Finish off anything in the start byte... */
        while(bits && bits < 8) {
            bitfield[start >> 3] |= (1 << bits);
            ++bits;
        }

        /* ...and the start of the end byte. */
        bits = end & 0x07;
        bitfield[end >> 3] |= (1 << bits) - 1;

        /* Now, fill anything in in the middle. */
        bits = (end >> 3) - ((start + 1) >> 3);
        memset(bitfield + ((start + 1) >> 3), 0xFF, bits);
    }
    /* Fall back to brute-forcing it for when the two are in the same byte. */
    else {
        --end;

        while(end >= start) {
            bitfield[end >> 3] |= (1 << (end & 0x07));
            --end;
        }
    }
}

/* Check if all bits in the bitfield that should be set are set. */
static inline int __pure all_bits_set(const uint8_t *bitfield, int end) {
    int i;

    /* Make sure each of the beginning bytes are fully set. */
    for(i = 0; i < (end >> 3); ++i) {
        if(bitfield[i] != 0xFF) {
            return 0;
        }
    }

    /* Check the last byte to make sure it has the right number of bits set. */
    if(bitfield[(end >> 3)] != ((1 << (end & 0x07)) - 1)) {
        return 0;
    }

    return 1;
}

/* Import the data for a fragment, potentially passing it onward in processing,
   if the whole datagram has arrived. */
static int frag_import(netif_t *src, const ip_hdr_t *hdr, const uint8_t *data,
                       size_t size, uint16_t flags, struct ip_frag *frag) {
    void *tmp;
    int fo = flags & 0x1FFF;
    int tl = ntohs(hdr->length);
    int start = (fo << 3);
    int ihl = (hdr->version_ihl & 0x0F) << 2;
    int end = start + tl - ihl;
    int rv = 0;
    uint64_t now = timer_ms_gettime64();

    (void)size;

    /* Reallocate space for the data buffer, if needed. */
    if(end > frag->cur_length) {
        tmp = realloc(frag->data, end);

        if(!tmp) {
            errno = ENOMEM;
            rv = -1;
            goto out;
        }

        frag->data = tmp;
    }

    memcpy(frag->data + start, data, end - start);
    set_bits(frag->bitfield, fo, fo + (((tl - ihl) + 7) >> 3));

    /* If the MF flag is not set, set the data length. */
    if(!(flags & 0x2000)) {
        frag->total_length = end;
    }

    /* If the fragment offset is zero, store the header. */
    if(!fo) {
        frag->hdr = *hdr;
    }

    /* If the total length is not zero, and all the bits in the bitfield are
       set, we continue on. */
    if(frag->total_length &&
            all_bits_set(frag->bitfield, frag->total_length >> 3)) {
        /* Set the right length. Don't worry about updating the checksum, since
           net_ipv4_input_proto doesn't check it anyway. */
        frag->hdr.length = htons(frag->total_length +
                                 ((frag->hdr.version_ihl & 0x0F) << 2));

        rv = net_ipv4_input_proto(src, &frag->hdr, frag->data);

        /* Remove the fragment from our buffer. */
        TAILQ_REMOVE(&frags, frag, listhnd);
        free(frag->data);
        free(frag);

        goto out;
    }

    /* Update the timer. */
    frag->death_time = MAX(frag->death_time, (now + hdr->ttl * 1000));

out:
    mutex_unlock(&frag_mutex);
    return rv;
}

/* IPv4 fragmentation procedure. This is basically a direct implementation of
   the example IP fragmentation procedure on pages 26-27 of RFC 791. */
int net_ipv4_frag_send(netif_t *net, ip_hdr_t *hdr, const uint8_t *data,
                       size_t size) {
    int ihl = (hdr->version_ihl & 0x0f) << 2;
    int total = size + ihl;
    uint16_t flags = ntohs(hdr->flags_frag_offs);
    ip_hdr_t newhdr;
    int nfb, ds;

    if(net == NULL)
        net = net_default_dev;

    /* If the packet doesn't need to be fragmented, send it away as is. */
    if(total < net->mtu) {
        return net_ipv4_send_packet(net, hdr, data, size);
    }
    /* If it needs to be fragmented and the DF flag is set, return error. */
    else if(flags & 0x4000) {
        errno = EMSGSIZE;
        return -1;
    }

    /* Copy over the old header, and set up things for fragment processing. */
    memcpy(&newhdr, hdr, ihl);
    nfb = ((net->mtu - ihl) >> 3);
    ds = nfb << 3;
    newhdr.flags_frag_offs = htons(flags | 0x2000);
    newhdr.length = htons(ihl + ds);

    /* Recompute the checksum. */
    newhdr.checksum = 0;
    newhdr.checksum = net_ipv4_checksum((uint8 *)&newhdr, sizeof(ip_hdr_t), 0);

    if(net_ipv4_send_packet(net, &newhdr, data, ds)) {
        return -1;
    }

    /* We don't deal with options right now, so dealing with the rest of the
       fragments is pretty easy. Fix the header, and recursively call this
       function to finish things off. */
    hdr->length = htons(ihl + size - ds);
    hdr->flags_frag_offs = htons((flags & 0xE000) | ((flags & 0x1FFF) + nfb));
    hdr->checksum = 0;
    hdr->checksum = net_ipv4_checksum((uint8 *)hdr, sizeof(ip_hdr_t), 0);

    return net_ipv4_frag_send(net, hdr, data + ds, size - ds);
}

/* IPv4 fragment reassembly procedure. This (along with the frag_import function
   above are basically a direct implementation of the example IP reassembly
   routine on pages 27-29 of RFC 791. */
int net_ipv4_reassemble(netif_t *src, const ip_hdr_t *hdr, const uint8_t *data,
                        size_t size) {
    uint16_t flags = ntohs(hdr->flags_frag_offs);
    struct ip_frag *f;

    /* If the fragment offset is zero and the MF flag is 0, this is the whole
       packet. Treat it as such. */
    if(!(flags & 0x2000) && (flags & 0x1FFF) == 0) {
        return net_ipv4_input_proto(src, hdr, data);
    }

    /* This is usually called inside an interrupt, so try to safely lock the
       mutex, and bail if we can't. */
    if(mutex_lock_irqsafe(&frag_mutex))
        return -1;

    /* Find the packet if we already have this one in our data buffer. */
    TAILQ_FOREACH(f, &frags, listhnd) {
        if(f->src == hdr->src && f->dst == hdr->dest &&
           f->ident == hdr->packet_id && f->proto == hdr->protocol) {
            /* We've got it, import the data (this function handles unlocking
               the mutex when its done). */
            return frag_import(src, hdr, data, size, flags, f);
        }
    }

    /* We don't have a fragment with that identifier, so make one. */
    f = (struct ip_frag *)malloc(sizeof(struct ip_frag));

    if(!f) {
        errno = ENOMEM;
        return -1;
    }

    f->src = hdr->src;
    f->dst = hdr->dest;
    f->ident = hdr->packet_id;
    f->proto = hdr->protocol;
    f->data = NULL;
    f->cur_length = 0;
    f->total_length = 0;
    memset(f->bitfield, 0, sizeof(f->bitfield));

    TAILQ_INSERT_TAIL(&frags, f, listhnd);

    return frag_import(src, hdr, data, size, flags, f);
}

int net_ipv4_frag_init(void) {
    if(!initted) {
        cbid = net_thd_add_callback(&frag_thd_cb, NULL, 2000);
        TAILQ_INIT(&frags);
    }

    initted = 1;
    return 0;
}

void net_ipv4_frag_shutdown(void) {
    struct ip_frag *c, *n;

    if(initted) {
        if(cbid != -1)
            net_thd_del_callback(cbid);

        c = TAILQ_FIRST(&frags);

        while(c) {
            n = TAILQ_NEXT(c, listhnd);
            free(c->data);
            free(c->bitfield);
            c = n;
        }
    }

    cbid = -1;
    initted = 0;
    TAILQ_INIT(&frags);
}
