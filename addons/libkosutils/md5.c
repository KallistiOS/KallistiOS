/* KallistiOS ##version##

   md5.c
   Copyright (C) 2010 Lawrence Sebald
*/

#include <string.h>
#include <kos/md5.h>

/* Initial values used in starting the MD5 checksum */
static const uint32_t md5initial[4] = {
    0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476
};

/* We will append somewhere between 1 and 64 bytes of padding to every message,
   depending on its length. This is the padding that is to be used. */
static const uint8_t md5padding[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* MD5 "magic" values */
static const uint32_t md5tab[64] = {
    0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE,
    0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
    0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE,
    0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
    0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
    0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
    0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED,
    0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
    0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C,
    0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
    0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
    0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
    0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039,
    0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
    0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1,
    0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391
};

/* Code to generate the above table (in case anyone is interested):

#include <stdio.h>
#include <math.h>

int main(int argc, char *argv[]) {
    int i;
    double tmp;

    printf("static const uint32_t md5tab[64] = {");

    for(i = 0; i < 64; ++i) {
        if((i % 4) == 0)
            printf("\n   ");

        tmp = sin(i + 1) * 4294967296.0;
        if(tmp < 0)
            tmp = -tmp;

        printf(" 0x%08X", (unsigned long) tmp);

        if(i == 63)
            break;

        printf(",");
    }

    printf("\n};\n");
    return 0;
}
*/

#define MD5_PAD_LEN(mod) (((mod) < 56) ? 56 - (mod) : 120 - (mod))

#define MD5_ROT(x, y)  ((x << y) | ((x & 0xFFFFFFFF) >> (32 - y)))

/* Basic MD5 functions */
#define MD5_F(x, y, z) (z ^ (x & (y ^ z)))
#define MD5_G(x, y, z) (y ^ (z & (x ^ y)))
#define MD5_H(x, y, z) (x ^ y ^ z)
#define MD5_I(x, y, z) (y ^ (x | (~z)))

/* Compound operations, consisting of a F, G, H, or I operation and a rotate */
#define MD5_FH(a, b, c, d, w, s, t) { \
        (a) += MD5_F((b), (c), (d)) + (w) + (t); \
        (a) = MD5_ROT((a), (s)); \
        (a) += (b); \
    }

#define MD5_GH(a, b, c, d, w, s, t) { \
        (a) += MD5_G((b), (c), (d)) + (w) + (t); \
        (a) = MD5_ROT((a), (s)); \
        (a) += (b); \
    }

#define MD5_HH(a, b, c, d, w, s, t) { \
        (a) += MD5_H((b), (c), (d)) + (w) + (t); \
        (a) = MD5_ROT((a), (s)); \
        (a) += (b); \
    }

#define MD5_IH(a, b, c, d, w, s, t) { \
        (a) += MD5_I((b), (c), (d)) + (w) + (t); \
        (a) = MD5_ROT((a), (s)); \
        (a) += (b); \
    }

void kos_md5_start(kos_md5_cxt_t *cxt) {
    cxt->size = 0;
    cxt->hash[0] = md5initial[0];
    cxt->hash[1] = md5initial[1];
    cxt->hash[2] = md5initial[2];
    cxt->hash[3] = md5initial[3];
}

/* input must be at least 64 bytes long */
static void kos_md5_process(kos_md5_cxt_t *cxt, const uint8_t *input) {
    uint32_t a, b, c, d, w[16];
    int i;

    /* Read in what we're starting with */
    a = cxt->hash[0];
    b = cxt->hash[1];
    c = cxt->hash[2];
    d = cxt->hash[3];

    /* Read the input into our buffer */
    for(i = 0; i < 16; ++i) {
        w[i] = input[(i << 2)] | (input[(i << 2) + 1] << 8) |
               (input[(i << 2) + 2] << 16) | (input[(i << 2) + 3] << 24);
    }

    /* First Round */
    MD5_FH(a, b, c, d, w[ 0],  7, md5tab[ 0]);
    MD5_FH(d, a, b, c, w[ 1], 12, md5tab[ 1]);
    MD5_FH(c, d, a, b, w[ 2], 17, md5tab[ 2]);
    MD5_FH(b, c, d, a, w[ 3], 22, md5tab[ 3]);
    MD5_FH(a, b, c, d, w[ 4],  7, md5tab[ 4]);
    MD5_FH(d, a, b, c, w[ 5], 12, md5tab[ 5]);
    MD5_FH(c, d, a, b, w[ 6], 17, md5tab[ 6]);
    MD5_FH(b, c, d, a, w[ 7], 22, md5tab[ 7]);
    MD5_FH(a, b, c, d, w[ 8],  7, md5tab[ 8]);
    MD5_FH(d, a, b, c, w[ 9], 12, md5tab[ 9]);
    MD5_FH(c, d, a, b, w[10], 17, md5tab[10]);
    MD5_FH(b, c, d, a, w[11], 22, md5tab[11]);
    MD5_FH(a, b, c, d, w[12],  7, md5tab[12]);
    MD5_FH(d, a, b, c, w[13], 12, md5tab[13]);
    MD5_FH(c, d, a, b, w[14], 17, md5tab[14]);
    MD5_FH(b, c, d, a, w[15], 22, md5tab[15]);

    /* Second Round */
    MD5_GH(a, b, c, d, w[ 1],  5, md5tab[16]);
    MD5_GH(d, a, b, c, w[ 6],  9, md5tab[17]);
    MD5_GH(c, d, a, b, w[11], 14, md5tab[18]);
    MD5_GH(b, c, d, a, w[ 0], 20, md5tab[19]);
    MD5_GH(a, b, c, d, w[ 5],  5, md5tab[20]);
    MD5_GH(d, a, b, c, w[10],  9, md5tab[21]);
    MD5_GH(c, d, a, b, w[15], 14, md5tab[22]);
    MD5_GH(b, c, d, a, w[ 4], 20, md5tab[23]);
    MD5_GH(a, b, c, d, w[ 9],  5, md5tab[24]);
    MD5_GH(d, a, b, c, w[14],  9, md5tab[25]);
    MD5_GH(c, d, a, b, w[ 3], 14, md5tab[26]);
    MD5_GH(b, c, d, a, w[ 8], 20, md5tab[27]);
    MD5_GH(a, b, c, d, w[13],  5, md5tab[28]);
    MD5_GH(d, a, b, c, w[ 2],  9, md5tab[29]);
    MD5_GH(c, d, a, b, w[ 7], 14, md5tab[30]);
    MD5_GH(b, c, d, a, w[12], 20, md5tab[31]);

    /* Third Round */
    MD5_HH(a, b, c, d, w[ 5],  4, md5tab[32]);
    MD5_HH(d, a, b, c, w[ 8], 11, md5tab[33]);
    MD5_HH(c, d, a, b, w[11], 16, md5tab[34]);
    MD5_HH(b, c, d, a, w[14], 23, md5tab[35]);
    MD5_HH(a, b, c, d, w[ 1],  4, md5tab[36]);
    MD5_HH(d, a, b, c, w[ 4], 11, md5tab[37]);
    MD5_HH(c, d, a, b, w[ 7], 16, md5tab[38]);
    MD5_HH(b, c, d, a, w[10], 23, md5tab[39]);
    MD5_HH(a, b, c, d, w[13],  4, md5tab[40]);
    MD5_HH(d, a, b, c, w[ 0], 11, md5tab[41]);
    MD5_HH(c, d, a, b, w[ 3], 16, md5tab[42]);
    MD5_HH(b, c, d, a, w[ 6], 23, md5tab[43]);
    MD5_HH(a, b, c, d, w[ 9],  4, md5tab[44]);
    MD5_HH(d, a, b, c, w[12], 11, md5tab[45]);
    MD5_HH(c, d, a, b, w[15], 16, md5tab[46]);
    MD5_HH(b, c, d, a, w[ 2], 23, md5tab[47]);

    /* Last Round */
    MD5_IH(a, b, c, d, w[ 0],  6, md5tab[48]);
    MD5_IH(d, a, b, c, w[ 7], 10, md5tab[49]);
    MD5_IH(c, d, a, b, w[14], 15, md5tab[50]);
    MD5_IH(b, c, d, a, w[ 5], 21, md5tab[51]);
    MD5_IH(a, b, c, d, w[12],  6, md5tab[52]);
    MD5_IH(d, a, b, c, w[ 3], 10, md5tab[53]);
    MD5_IH(c, d, a, b, w[10], 15, md5tab[54]);
    MD5_IH(b, c, d, a, w[ 1], 21, md5tab[55]);
    MD5_IH(a, b, c, d, w[ 8],  6, md5tab[56]);
    MD5_IH(d, a, b, c, w[15], 10, md5tab[57]);
    MD5_IH(c, d, a, b, w[ 6], 15, md5tab[58]);
    MD5_IH(b, c, d, a, w[13], 21, md5tab[59]);
    MD5_IH(a, b, c, d, w[ 4],  6, md5tab[60]);
    MD5_IH(d, a, b, c, w[11], 10, md5tab[61]);
    MD5_IH(c, d, a, b, w[ 2], 15, md5tab[62]);
    MD5_IH(b, c, d, a, w[ 9], 21, md5tab[63]);

    /* Save out what we have */
    cxt->hash[0] += a;
    cxt->hash[1] += b;
    cxt->hash[2] += c;
    cxt->hash[3] += d;
}

void kos_md5_hash_block(kos_md5_cxt_t *cxt, const uint8_t *input, uint32_t size) {
    uint32_t left, copy;

    /* Figure out what we had left over from last time (if anything) */
    left = (uint32_t)((cxt->size >> 3) & 0x3F);
    copy = 64 - left;

    /* Update the size */
    cxt->size += (size << 3);

    /* Deal with what was left over, if we have enough data to do so */
    if(left && size >= copy) {
        memcpy(&cxt->buf[left], input, copy);
        kos_md5_process(cxt, cxt->buf);
        left = 0;
        size -= copy;
        input += copy;
    }

    /* Hash each block of the data */
    while(size >= 64) {
        kos_md5_process(cxt, input);
        input += 64;
        size -= 64;
    }

    /* Buffer anything left over */
    if(size) {
        memcpy(&cxt->buf[left], input, size);
    }
}

void kos_md5_finish(kos_md5_cxt_t *cxt, uint8_t output[16]) {
    uint64_t len = cxt->size;
    uint32_t blen = (cxt->size >> 3) & 0x3F;
    uint32_t plen = MD5_PAD_LEN(blen);
    int i;
    uint8_t len_bytes[8];

    /* Add in the padding */
    kos_md5_hash_block(cxt, md5padding, plen);

    /* Hash in the length -- this will finish the last 64-byte block */
    len_bytes[0] = (uint8_t)(len);
    len_bytes[1] = (uint8_t)(len >> 8);
    len_bytes[2] = (uint8_t)(len >> 16);
    len_bytes[3] = (uint8_t)(len >> 24);
    len_bytes[4] = (uint8_t)(len >> 32);
    len_bytes[5] = (uint8_t)(len >> 40);
    len_bytes[6] = (uint8_t)(len >> 48);
    len_bytes[7] = (uint8_t)(len >> 56);
    kos_md5_hash_block(cxt, len_bytes, 8);

    /* Copy out the hash, since we're done */
    for(i = 0; i < 16; ++i) {
        output[i] = (uint8_t)(cxt->hash[i >> 2] >> ((i & 0x03) << 3));
    }
}

/* Convenience function for computing an MD5 of a complete block. */
void kos_md5(const uint8_t *input, uint32_t size, uint8_t output[16]) {
    kos_md5_cxt_t cxt;

    kos_md5_start(&cxt);
    kos_md5_hash_block(&cxt, input, size);
    kos_md5_finish(&cxt, output);
}
