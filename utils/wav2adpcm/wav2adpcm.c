/*
    aica adpcm <-> wave converter;

    (c) 2002 BERO <bero@geocities.co.jp>
    under GPL or notify me

    aica adpcm seems same as YMZ280B adpcm
    adpcm->pcm algorithm can found MAME/src/sound/ymz280b.c by Aaron Giles

    this code is for little endian machine

    Modified by Megan Potter to read/write ADPCM WAV files, and to
    handle stereo (though the stereo is very likely KOS specific
    since we make no effort to interleave it). Please see README.GPL
    in the KOS docs dir for more info on the GPL license.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static int diff_lookup[16] = {
    1, 3, 5, 7, 9, 11, 13, 15,
    -1, -3, -5, -7, -9, -11, -13, -15,
};

static int index_scale[16] = {
    0x0e6, 0x0e6, 0x0e6, 0x0e6, 0x133, 0x199, 0x200, 0x266,
    0x0e6, 0x0e6, 0x0e6, 0x0e6, 0x133, 0x199, 0x200, 0x266 /* same value for speedup */
};

static inline int limit(int val, int min, int max) {
    if(val < min) return min;
    else if(val > max) return max;
    else return val;
}

void pcm2adpcm(unsigned char *dst, const short *src, size_t length) {
    int signal, step;
    signal = 0;
    step = 0x7f;

    /* length /= 4; */
    length = (length + 3) / 4;

    do {
        int data, val, diff;

        /* hign nibble */
        diff = *src++ - signal;
        diff = (diff * 8) / step;

        val = abs(diff) / 2;

        if(val > 7) val = 7;

        if(diff < 0) val += 8;

        signal += (step * diff_lookup[val]) / 8;
        signal = limit(signal, -32768, 32767);

        step = (step * index_scale[val]) >> 8;
        step = limit(step, 0x7f, 0x6000);

        data = val;

        /* low nibble */
        diff = *src++ - signal;
        diff = (diff * 8) / step;

        val = (abs(diff)) / 2;

        if(val > 7) val = 7;

        if(diff < 0) val += 8;

        signal += (step * diff_lookup[val]) / 8;
        signal = limit(signal, -32768, 32767);

        step = (step * index_scale[val]) >> 8;
        step = limit(step, 0x7f, 0x6000);

        data |= val << 4;

        *dst++ = data;

    }
    while(--length);
}

void adpcm2pcm(short *dst, const unsigned char *src, size_t length) {
    int signal, step;
    signal = 0;
    step = 0x7f;

    do {
        int data, val;

        data = *src++;

        /* low nibble */
        val = data & 15;

        signal += (step * diff_lookup[val]) / 8;
        signal = limit(signal, -32768, 32767);

        step = (step * index_scale[val & 7]) >> 8;
        step = limit(step, 0x7f, 0x6000);

        *dst++ = signal;

        /* high nibble */
        val = (data >> 4) & 15;

        signal += (step * diff_lookup[val]) / 8;
        signal = limit(signal, -32768, 32767);

        step = (step * index_scale[val & 7]) >> 8;
        step = limit(step, 0x7f, 0x6000);

        *dst++ = signal;

    }
    while(--length);
}

void deinterleave(void *buffer, size_t size) {
    short * buf;
    short * buf1, * buf2;
    int i;

    buf = (short *)buffer;
    buf1 = malloc(size / 2);
    buf2 = malloc(size / 2);

    for(i = 0; i < size / 4; i++) {
        buf1[i] = buf[i * 2 + 0];
        buf2[i] = buf[i * 2 + 1];
    }

    memcpy(buf, buf1, size / 2);
    memcpy(buf + size / 4, buf2, size / 2);

    free(buf1);
    free(buf2);
}

void interleave(void *buffer, size_t size) {
    short * buf;
    short * buf1, * buf2;
    int i;

    buf = malloc(size);
    buf1 = (short *)buffer;
    buf2 = buf1 + size / 4;

    for(i = 0; i < size / 4; i++) {
        buf[i * 2 + 0] = buf1[i];
        buf[i * 2 + 1] = buf2[i];
    }

    memcpy(buffer, buf, size);

    free(buf);
}

struct wavhdr_t {
    char hdr1[4];
    int32_t totalsize;

    char hdr2[8];
    int32_t hdrsize;
    short format;
    short channels;
    int32_t freq;
    int32_t byte_per_sec;
    short blocksize;
    short bits;

    char hdr3[4];
    int32_t datasize;
};

int wav2adpcm(const char *infile, const char *outfile) {
    struct wavhdr_t wavhdr;
    FILE *in, *out;
    size_t pcmsize, adpcmsize;
    short *pcmbuf;
    unsigned char *adpcmbuf;

    in = fopen(infile, "rb");

    if(!in)  {
        printf("can't open %s\n", infile);
        return -1;
    }

    if(fread(&wavhdr, sizeof(wavhdr), 1, in) != 1) {
        fprintf(stderr, "Cannot read header.\n");
        fclose(in);
        return -1;
    }

    if(memcmp(wavhdr.hdr1, "RIFF", 4)
            || memcmp(wavhdr.hdr2, "WAVEfmt ", 8)
            || memcmp(wavhdr.hdr3, "data", 4)
            || wavhdr.hdrsize != 0x10
            || wavhdr.format != 1
            || (wavhdr.channels != 1 && wavhdr.channels != 2)
            || wavhdr.bits != 16) {
        fprintf(stderr, "Unsupported format.\n");
        fclose(in);
        return -1;
    }

    pcmsize = wavhdr.datasize;

    adpcmsize = pcmsize / 4;
    pcmbuf = malloc(pcmsize);
    adpcmbuf = malloc(adpcmsize);

    if(fread(pcmbuf, pcmsize, 1, in) != 1) {
        fprintf(stderr, "Cannot read data.\n");
        fclose(in);
        return -1;
    }
    fclose(in);

    if(wavhdr.channels == 1) {
        pcm2adpcm(adpcmbuf, pcmbuf, pcmsize);
    }
    else {
        /* For stereo we just deinterleave the input and store the
           left and right channel of the ADPCM data separately. */
        deinterleave(pcmbuf, pcmsize);
        pcm2adpcm(adpcmbuf, pcmbuf, pcmsize / 2);
        pcm2adpcm(adpcmbuf + adpcmsize / 2, pcmbuf + pcmsize / 4, pcmsize / 2);
    }

    wavhdr.datasize = adpcmsize;
    wavhdr.format = 20; /* ITU G.723 ADPCM (Yamaha) */
    wavhdr.bits = 4;
    wavhdr.totalsize = wavhdr.datasize + sizeof(wavhdr) - 8;

    out = fopen(outfile, "wb");
    if(fwrite(&wavhdr, sizeof(wavhdr), 1, out) != 1
            || fwrite(adpcmbuf, adpcmsize, 1, out) != 1) {
        fprintf(stderr, "Cannot write ADPCM data.\n");
        fclose(out);
        return -1;
    }
    fclose(out);

    return 0;
}

int adpcm2wav(const char *infile, const char *outfile) {
    struct wavhdr_t wavhdr;
    FILE *in, *out;
    size_t pcmsize, adpcmsize;
    short *pcmbuf;
    unsigned char *adpcmbuf;

    in = fopen(infile, "rb");

    if(!in)  {
        fprintf(stderr, "Cannot open %s\n", infile);
        return -1;
    }

    if(fread(&wavhdr, sizeof(wavhdr), 1, in) != 1) {
        fprintf(stderr, "Cannot read header.\n");
        fclose(in);
        return -1;
    }

    if(memcmp(wavhdr.hdr1, "RIFF", 4)
            || memcmp(wavhdr.hdr2, "WAVEfmt ", 8)
            || memcmp(wavhdr.hdr3, "data", 4)
            || wavhdr.hdrsize != 0x10
            || wavhdr.format != 20
            || (wavhdr.channels != 1 && wavhdr.channels != 2)
            || wavhdr.bits != 4) {
        fprintf(stderr, "Unsupported format.\n");
        fclose(in);
        return -1;
    }

    adpcmsize = wavhdr.datasize;
    pcmsize = adpcmsize * 4;
    adpcmbuf = malloc(adpcmsize);
    pcmbuf = malloc(pcmsize);

    if(fread(adpcmbuf, adpcmsize, 1, in) != 1) {
        fprintf(stderr, "Cannot read data.\n");
        fclose(in);
        return -1;
    }
    fclose(in);

    if(wavhdr.channels == 1) {
        adpcm2pcm(pcmbuf, adpcmbuf, adpcmsize);
    }
    else {
        adpcm2pcm(pcmbuf, adpcmbuf, adpcmsize / 2);
        adpcm2pcm(pcmbuf + pcmsize / 4, adpcmbuf + adpcmsize / 2, adpcmsize / 2);
        interleave(pcmbuf, pcmsize);
    }

    wavhdr.blocksize = wavhdr.channels * sizeof(short);
    wavhdr.byte_per_sec = wavhdr.freq * wavhdr.blocksize;
    wavhdr.datasize = pcmsize;
    wavhdr.totalsize = wavhdr.datasize + sizeof(wavhdr) - 8;
    wavhdr.format = 1;
    wavhdr.bits = 16;

    out = fopen(outfile, "wb");
    if(fwrite(&wavhdr, sizeof(wavhdr), 1, out) != 1
            || fwrite(pcmbuf, pcmsize, 1, out) != 1) {
        fprintf(stderr, "Cannot write WAV data.\n");
        fclose(out);
        return -1;
    }

    fclose(out);

    return 0;
}

void usage() {
    printf("wav2adpcm: 16bit mono wav to aica adpcm and vice-versa (c)2002 BERO\n"
           " wav2adpcm -t <infile.wav> <outfile.wav>   (To adpcm)\n"
           " wav2adpcm -f <infile.wav> <outfile.wav>   (From adpcm)\n"
          );
}

int main(int argc, char **argv) {
    if(argc == 4) {
        if(!strcmp(argv[1], "-t")) {
            return wav2adpcm(argv[2], argv[3]);
        }
        else if(!strcmp(argv[1], "-f")) {
            return adpcm2wav(argv[2], argv[3]);
        }
        else {
            usage();
            return -1;
        }
    }
    else {
        usage();
        return -1;
    }
}
