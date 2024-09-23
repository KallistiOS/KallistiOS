/*
    aica adpcm <-> wave converter;

    Copyright (C) 2002 BERO <bero@geocities.co.jp>
    Copyright (C) 2024 Andress Barajas
    Copyright (C) 2024 Stefanos Kornilios Mitsis Poiitidis

    AICA adpcm seems same as YMZ280B adpcm. The only difference 
    between YMZ280B and AICA adpcm is that the nibbles are swapped.

    The encode and decode algorithms for AICA adpcm - 2019 by superctr.

    This code is for little endian machine.

    Originally modified by Megan Potter to read/write ADPCM WAV files, 
    and to handle stereo (non-interleaved). 
    
    Modified by Andress Barajas to replace the GPL MAME encoding/decoding 
    code with public domain code written by superctr. This version also 
    handles interleaved stereo thanks to SKMP and can output headerless 
    audio data.

    Public domain code source:
    https://github.com/superctr/adpcm/blob/master/ymz_codec.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

/* WAV Header */
typedef struct wavhdr {
    uint8_t hdr1[4];
    uint32_t totalsize;

    uint8_t hdr2[8];
    uint32_t hdrsize;
    uint16_t format;
    uint16_t channels;
    uint32_t freq;
    uint32_t byte_per_sec;
    uint16_t block_align;
    uint16_t bits_per_sample;
} wavhdr_t;

/* Header chunk */
typedef struct wavhdr_chunk {
    char hdr3[4];
    uint32_t datasize;
} wavhdr_chunk_t;

/* Holds flags */
static int interleaved = 0;
static int no_header = 0;

/* Output Formats */
#define WAVE_FMT_PCM                   0x01 /* PCM */
#define WAVE_FMT_YAMAHA_ADPCM_ITU_G723 0x14 /* ITU G.723 Yamaha ADPCM (KallistiOS) */
#define WAVE_FMT_YAMAHA_ADPCM          0x20 /* Yamaha ADPCM (interleaved) */

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

static inline int16_t ymz_step(uint8_t step, int16_t *history, int16_t *step_size) {
    static const int step_table[8] = {
        230, 230, 230, 230, 307, 409, 512, 614
    };

    int sign = step & 8;
    int delta = step & 7;
    int diff = ((1 + (delta << 1)) * *step_size) >> 3;
    int newval = *history;
    int nstep = (step_table[delta] * *step_size) >> 8;

    /* Only found in the official AICA encoder
       but it's possible all chips (including ADPCM-B) does this. */
    diff = CLAMP(diff, 0, 32767);
    if(sign > 0)
        newval -= diff;
    else
        newval += diff;

    *step_size = CLAMP(nstep, 127, 24576);
    *history = newval = CLAMP(newval, -32768, 32767);
    return newval;
}

void adpcm2pcm(int16_t *outbuffer, uint8_t *buffer, size_t bytes) {
    long i;
    int16_t step_size = 127;
    int16_t history = 0;
    uint8_t nibble = 4;
    size_t num_samples = bytes * 2;  /* Each ADPCM byte contains two 4-bit samples */

    for(i = 0; i < num_samples; i++) {
        int8_t step = (*(int8_t *)buffer) << nibble;
        step >>= 4;
        if(!nibble)
            buffer++;
        nibble ^= 4;
        history = history * 254 / 256; // High pass
        *outbuffer++ = ymz_step(step, &history, &step_size);
    }
}

void pcm2adpcm(uint8_t *outbuffer, int16_t *buffer, size_t bytes) {
    long i;
    int16_t step_size = 127;
    int16_t history = 0;
    uint8_t buf_sample = 0, nibble = 0;
    uint32_t adpcm_sample;
    size_t num_samples = bytes / 2; /* Divide by 2 to get the number of 16-bit samples */

    for(i = 0;i < num_samples;i++) {
        /* We remove a few bits_per_sample of accuracy to reduce some noise. */
        int step = ((*buffer++) & -8) - history;
        adpcm_sample = (abs(step) << 16) / (step_size << 14);
        adpcm_sample = CLAMP(adpcm_sample, 0, 7);
        if(step < 0)
            adpcm_sample |= 8;
        if(!nibble)
            *outbuffer++ = buf_sample | (adpcm_sample<<4);
        else
            buf_sample = (adpcm_sample & 15);
        nibble ^= 1;
        ymz_step(adpcm_sample, &history, &step_size);
    }
}

void deinterleave(void *buffer, size_t bytes) {
    uint16_t *buf;
    uint16_t *left, *right;
    int i;

    buf = (uint16_t *)buffer;
    left = malloc(bytes);
    if(!left) {
        fprintf(stderr, "deinterleave: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    right = left + bytes / 4;

    for(i = 0; i < bytes / 4; i++) {
        left[i] = buf[i * 2 + 0];
        right[i] = buf[i * 2 + 1];
    }

    memcpy(buf, left, bytes / 2);
    memcpy(buf + bytes / 4, right, bytes / 2);

    free(left);
}

void deinterleave_adpcm(void *buffer, size_t bytes) {
    uint8_t *buf;
    uint8_t *left, *right;
    int i;

    buf = (uint8_t *)buffer;
    left = malloc(bytes);
    if(!left) {
        fprintf(stderr, "deinterleave_adpcm: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    right = left + bytes / 2;

    for(i = 0; i < bytes; i++) {
        if(i % 2 == 0) { /* Set high nibble */
            left[i / 2] = (buf[i] & 0xF0);
            right[i / 2] = (buf[i] & 0x0F) << 4;
        } else { /* Set low nibble to complete the byte */
            left[i / 2] |= ((buf[i] >> 4) & 0x0F);
            right[i / 2] |= (buf[i] & 0x0F);
        }
    }

    memcpy(buf, left, bytes);

    free(left);
}

void interleave(void *buffer, size_t bytes) {
    uint16_t *buf;
    uint16_t *left, *right;
    int i;

    buf = malloc(bytes);
    if(!buf) {
        fprintf(stderr, "interleave: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    left = (uint16_t *)buffer;
    right = left + bytes / 4;

    for(i = 0; i < bytes / 4; i++) {
        buf[i * 2 + 0] = left[i];
        buf[i * 2 + 1] = right[i];
    }

    memcpy(buffer, buf, bytes);

    free(buf);
}

void interleave_adpcm(void *buffer, size_t bytes) {
    uint8_t *buf;
    uint8_t *left, *right;
    int i;

    buf = malloc(bytes);
    if(!buf) {
        fprintf(stderr, "interleave_adpcm: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    left = (uint8_t *)buffer;
    right = left + bytes / 2;

    for(i = 0; i < bytes; i++) {
        buf[i] = (right[i/2] >> (i%2*4)) & 0xF;
        buf[i] |= ((left[i/2] >> (i%2*4)) & 0XF) << 4;
    }

    memcpy(buffer, buf, bytes);

    free(buf);
}

int validate_wav_header(wavhdr_t *wavhdr, wavhdr_chunk_t *wavhdr3, int format_mask, int bits_per_sample, FILE *in) {
    int result = 0;

    if(memcmp(wavhdr->hdr1, "RIFF", 4)) {
        fprintf(stderr, "Invalid RIFF header.\n");
        result = -1;
    }

    if(memcmp(wavhdr->hdr2, "WAVEfmt ", 8)) {
        fprintf(stderr, "Invalid WAVEfmt header.\n");
        result = -1;
    }

    if(wavhdr->hdrsize < 0x10) {
        fprintf(stderr, "Invalid header size, %d bytes\n", wavhdr->hdrsize);
        result = -1;
    } else if(wavhdr->hdrsize > 0x10) {
        /*fprintf(stderr, "Unique header size, %d bytes\n", wavhdr->hdrsize); */
        fseek(in, wavhdr->hdrsize - 0x10, SEEK_CUR);
    }

    if(!(wavhdr->format & format_mask)) {
        fprintf(stderr, "Unsupported format: %#x\n", wavhdr->format);
        result = -1;
    }

    if(wavhdr->channels != 1 && wavhdr->channels != 2) {
        fprintf(stderr, "Unsupported number of channels: %d\n", wavhdr->channels);
        result = -1;
    }

    if(wavhdr->bits_per_sample != bits_per_sample) {
        fprintf(stderr, "Unsupported bit depth: %d\n", wavhdr->bits_per_sample);
        result = -1;
    }

    for(;;) {
        /* Read the next chunk header */
        if(fread(wavhdr3->hdr3, 1, 4, in) != 4) {
            fprintf(stderr, "Failed to read next chunk header!\n");
            result = -1;
            break;
        }

        /* Read the chunk size */
        if(fread(&wavhdr3->datasize, 1, 4, in) != 4) {
            fprintf(stderr, "Failed to read chunk size!\n");
            result = -1;
            break;
        }

        /* Skip the chunk if it's not the "data" chunk. */
        if(memcmp(wavhdr3->hdr3, "data", 4))
            fseek(in, wavhdr3->datasize, SEEK_CUR);
        else
            break;
    }

    return result;
}

/* Do a straight copy of the input to output file */
int straight_copy(FILE *in, const char *outfile) {
    FILE *out = NULL;
    size_t filesize;
    char *buffer = NULL;
    int result = 0;

    fseek(in, 0, SEEK_END);
    filesize = ftell(in);
    rewind(in);  

    buffer = malloc(filesize);
    if(!buffer) {
        fprintf(stderr, "Memory allocation failed.\n");
        result = -1;
        goto cleanup;
    }

    if(fread(buffer, filesize, 1, in) != 1) {
        fprintf(stderr, "Cannot read file.\n");
        free(buffer);
        result = -1;
        goto cleanup;
    }

    out = fopen(outfile, "wb");
    if(!out) {
        fprintf(stderr, "Cannot open %s for writing.\n", outfile);
        result = -1;
        goto cleanup;
    }

    if(fwrite(buffer, filesize, 1, out) != 1) {
        fprintf(stderr, "Cannot write to output file.\n");
        result = -1;
        goto cleanup;
    }

cleanup:
    if(in) fclose(in);
    if(out) fclose(out);
    if(buffer) free(buffer);

    return result;
}

int wav2adpcm(const char *infile, const char *outfile) {
    wavhdr_t wavhdr;
    wavhdr_chunk_t wavhdr_chunk;
    FILE *in, *out = NULL;
    size_t pcmsize, adpcmsize;
    int16_t *pcmbuf = NULL;
    uint8_t *adpcmbuf = NULL;
    int result = 0;

    in = fopen(infile, "rb");
    if(!in) {
        printf("Cannot open %s\n", infile);
        return -1;
    }

    if(fread(&wavhdr, sizeof(wavhdr), 1, in) != 1) {
        fprintf(stderr, "Cannot read header.\n");
        fclose(in);
        return -1;
    }

    // printf("Header size, %d bytes\n", wavhdr.hdrsize);
    // printf("Format: %#x\n", wavhdr.format);
    // printf("Channels: %d\n", wavhdr.channels);
    // printf("Freq: %d\n", wavhdr.freq);
    // printf("Bit depth: %d\n", wavhdr.bits_per_sample);

    /* If the input is the desired output format, just copy */
    if(wavhdr.format == WAVE_FMT_YAMAHA_ADPCM ||
       wavhdr.format == WAVE_FMT_YAMAHA_ADPCM_ITU_G723) {
        return straight_copy(in, outfile);
    }

    if(validate_wav_header(&wavhdr, &wavhdr_chunk, WAVE_FMT_PCM, 16, in)) {
        fclose(in);
        return -1;
    }

    pcmsize = wavhdr_chunk.datasize;
    adpcmsize = pcmsize / 4;

    pcmbuf = malloc(pcmsize);
    adpcmbuf = malloc(adpcmsize);
    if(!pcmbuf || !adpcmbuf) {
        fprintf(stderr, "Memory allocation failed.\n");
        result = -1;
        goto cleanup;
    }

    if(fread(pcmbuf, pcmsize, 1, in) != 1) {
        fprintf(stderr, "Cannot read data.\n");
        result = -1;
        goto cleanup;
    }
    fclose(in);
    in = NULL;

    if(wavhdr.channels == 1)
        pcm2adpcm(adpcmbuf, pcmbuf, pcmsize);
    else {
        /* For stereo we just deinterleave the input and store the
           left and right channel of the ADPCM data separately. */
        deinterleave(pcmbuf, pcmsize);
        pcm2adpcm(adpcmbuf, pcmbuf, pcmsize / 2);
        pcm2adpcm(adpcmbuf + adpcmsize / 2, pcmbuf + pcmsize / 4,  pcmsize / 2);

        if(interleaved)
            interleave_adpcm(adpcmbuf, adpcmsize);
    }

    out = fopen(outfile, "wb");
    if(!out) {
        fprintf(stderr, "Cannot open output file for writing.\n");
        result = -1;
        goto cleanup;
    }

    if(no_header) {
        /* Just write the body */
        if(fwrite(adpcmbuf, adpcmsize, 1, out) != 1) {
            fprintf(stderr, "Cannot write ADPCM data.\n");
            result = -1;
            goto cleanup;
        }
    }
    else {
        /* Build header */
        wavhdr.hdrsize = 0x10;
        wavhdr.format = interleaved ? WAVE_FMT_YAMAHA_ADPCM : WAVE_FMT_YAMAHA_ADPCM_ITU_G723;
        wavhdr.bits_per_sample = 4;
        wavhdr.block_align = (wavhdr.channels * wavhdr.bits_per_sample) / 8;
        wavhdr.byte_per_sec = (wavhdr.freq * wavhdr.channels * wavhdr.bits_per_sample) / 8;
        wavhdr.totalsize = adpcmsize + sizeof(wavhdr) + sizeof(wavhdr_chunk) - 8;
        
        memcpy(wavhdr_chunk.hdr3, "data", 4);
        wavhdr_chunk.datasize = adpcmsize;

        /* Write the whole file */
        if(fwrite(&wavhdr, sizeof(wavhdr), 1, out) != 1 ||
           fwrite(&wavhdr_chunk, sizeof(wavhdr_chunk), 1, out) != 1 ||
           fwrite(adpcmbuf, adpcmsize, 1, out) != 1) {
            fprintf(stderr, "Cannot write ADPCM data.\n");
            result = -1;
            goto cleanup;
        }
    }

cleanup:
    if(in) fclose(in);
    if(out) fclose(out);
    if(adpcmbuf) free(adpcmbuf);
    if(pcmbuf) free(pcmbuf);

    return result;
}

int adpcm2wav(const char *infile, const char *outfile) {
    wavhdr_t wavhdr;
    wavhdr_chunk_t wavhdr_chunk;
    FILE *in, *out = NULL;
    size_t pcmsize, adpcmsize;
    int16_t *pcmbuf = NULL;
    uint8_t *adpcmbuf = NULL;
    int result = 0;

    in = fopen(infile, "rb");

    if(!in) {
        fprintf(stderr, "Cannot open %s\n", infile);
        return -1;
    }

    if(fread(&wavhdr, sizeof(wavhdr), 1, in) != 1) {
        fprintf(stderr, "Cannot read header.\n");
        fclose(in);
        return -1;
    }

    // printf("Header size, %d bytes\n", wavhdr.hdrsize);
    // printf("Format: %#x\n", wavhdr.format);
    // printf("Channels: %d\n", wavhdr.channels);
    // printf("Freq: %d\n", wavhdr.freq);
    // printf("Bit depth: %d\n", wavhdr.bits_per_sample);

    /* If the input is the desired output format, just copy */
    if(wavhdr.format == WAVE_FMT_PCM)
        return straight_copy(in, outfile);

    if(validate_wav_header(&wavhdr, &wavhdr_chunk, WAVE_FMT_YAMAHA_ADPCM | WAVE_FMT_YAMAHA_ADPCM_ITU_G723, 4, in)) {
        fclose(in);
        return -1;
    }

    adpcmsize = wavhdr_chunk.datasize;
    pcmsize = adpcmsize * 4;

    adpcmbuf = malloc(adpcmsize);
    pcmbuf = malloc(pcmsize);
    if(!adpcmbuf || !pcmbuf) {
        fprintf(stderr, "Memory allocation failed.\n");
        result = -1;
        goto cleanup;
    }

    if(fread(adpcmbuf, adpcmsize, 1, in) != 1) {
        fprintf(stderr, "Cannot read data.\n");
        result = -1;
        goto cleanup;
    }
    fclose(in);
    in = NULL;

    if(wavhdr.channels == 1)
        adpcm2pcm(pcmbuf, adpcmbuf, adpcmsize);
    else {
        if(wavhdr.format == WAVE_FMT_YAMAHA_ADPCM)
            deinterleave_adpcm(adpcmbuf, adpcmsize);

        adpcm2pcm(pcmbuf, adpcmbuf, adpcmsize / 2);
        adpcm2pcm(pcmbuf + pcmsize / 4, adpcmbuf + adpcmsize / 2, adpcmsize / 2);
        interleave(pcmbuf, pcmsize);
    }

    out = fopen(outfile, "wb");
    if(!out) {
        fprintf(stderr, "Cannot open output file for writing.\n");
        result = -1;
        goto cleanup;
    }

    if(no_header) {
        /* Just write the body */
        if(fwrite(pcmbuf, pcmsize, 1, out) != 1) {
            fprintf(stderr, "Cannot write WAV data.\n");
            result = -1;
            goto cleanup;
        }
    }
    else {
        /* Build header */
        wavhdr.hdrsize = 0x10;
        wavhdr.format = WAVE_FMT_PCM;
        wavhdr.bits_per_sample = 16;
        wavhdr.block_align = (wavhdr.channels * wavhdr.bits_per_sample) / 8;
        wavhdr.byte_per_sec = (wavhdr.freq * wavhdr.channels * wavhdr.bits_per_sample) / 8;
        wavhdr.totalsize = pcmsize + sizeof(wavhdr) + sizeof(wavhdr_chunk) - 8;

        memcpy(wavhdr_chunk.hdr3, "data", 4);
        wavhdr_chunk.datasize = pcmsize;

        /* Write the whole file */
        if(fwrite(&wavhdr, sizeof(wavhdr), 1, out) != 1 ||
           fwrite(&wavhdr_chunk, sizeof(wavhdr_chunk), 1, out) != 1 ||
           fwrite(pcmbuf, pcmsize, 1, out) != 1) {
            fprintf(stderr, "Cannot write WAV data.\n");
            result = -1;
            goto cleanup;
        }
    }

cleanup:
    if(in) fclose(in);
    if(out) fclose(out);
    if(adpcmbuf) free(adpcmbuf);
    if(pcmbuf) free(pcmbuf);

    return result;
}

void usage() {
    printf("wav2adpcm: Convert 16-bit WAV to AICA ADPCM and vice-versa\n"
           "Copyright (C) 2002 BERO\n"
           "Copyright (C) 2024 Andress Barajas\n"
           "Copyright (C) 2024 SKMP\n\n"
           
           "Usage:\n"
           "    wav2adpcm -t <infile.wav> <outfile.wav>       (To ADPCM)\n"
           "    wav2adpcm -i -t <infile.wav> <outfile.wav>    (To ADPCM with interleaved data)\n"
           "    wav2adpcm -f <infile.wav> <outfile.wav>       (From ADPCM)\n"
           "    wav2adpcm -n -i -t <infile.wav> <outfile.wav> (To ADPCM interleaved without a header)\n"
           "    wav2adpcm -n -f <infile.wav> <outfile.wav>    (From ADPCM without a header)\n"
           "\n"
           "Options:\n"
           "    -t    Convert 16-bit WAV to AICA ADPCM.\n"
           "    -f    Convert AICA ADPCM back to 16-bit WAV.\n"
           "    -i    Optional parameter to output interleaved adpcm data (use with -t).\n"
           "    -n    Optional parameter to output headerless pcm/adpcm data (use with -t or -f).\n"
           "    -h    Prints this usage information.\n"
           "\n"
           "Note:\n"
           "If you are having trouble with your input WAV file, you can preprocess it using ffmpeg:\n"
           "    ffmpeg -i input.wav -ac 1 -acodec pcm_s16le output.wav\n"
          );
}

int main(int argc, char **argv) {
    int t_flag_pos = 0;

    /* Check for help flag first */
    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "-h")) {
            usage();
            return 0;
        }
    }

    /* Parse flags */
    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "-n")) {
            if(t_flag_pos) {
                fprintf(stderr, "-n flag must come before -t or -f\n");
                usage();
                return -1;
            }
            no_header = 1;
        }
        else if(!strcmp(argv[i], "-i")) {
            if(t_flag_pos) {
                fprintf(stderr, "-i flag must come before -t\n");
                usage();
                return -1;
            }
            interleaved = 1;
        }
        else if(!strcmp(argv[i], "-t") || !strcmp(argv[i], "-f")) {
            if(t_flag_pos) {
                fprintf(stderr, "Only one of -t or -f is allowed\n");
                usage();
                return -1;
            }
            t_flag_pos = i;
        }
    }

    /* Check for required number of arguments */
    if(t_flag_pos == 0 || argc < t_flag_pos + 3) {
        usage();
        return -1;
    }

    /* Ensure -i is only used with -t */
    if(interleaved && strcmp(argv[t_flag_pos], "-t") != 0) {
        fprintf(stderr, "-i flag can only be used with -t\n");
        usage();
        return -1;
    }

    /* Handle conversion based on -t or -f */
    if(!strcmp(argv[t_flag_pos], "-t")) {
        /* Convert WAV to ADPCM */
        return wav2adpcm(argv[t_flag_pos + 1], argv[t_flag_pos + 2]);
    }
    else if(!strcmp(argv[t_flag_pos], "-f")) {
        /* Convert ADPCM to WAV */
        return adpcm2wav(argv[t_flag_pos + 1], argv[t_flag_pos + 2]);
    }
    else {
        usage();
        return -1;
    }
}
