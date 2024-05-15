/* KallistiOS 2.0.0

   gba-crcfix.c
   (c)2002 Gil Megidish

   Calculate the header checksum (complement check)

*/

#include <stdio.h>
#include <string.h>
#include <errno.h>

/* We do this instead of stdint.h since that (ironically) seems to be
   non-portable */
#define uint8_t unsigned char

#define HEADER_SIZE 0xc0

static void fix_header(uint8_t *header) {

    uint8_t sum = 0;
    uint8_t count = 0xbc - 0xa0;

    /* complement check of 0xa0 - 0xbc */
    header = header + 0xa0;

    while(count-- > 0) {
        sum = sum - *header++;
    }

    sum = (sum - 0x19);
    *header = sum;
}

static int process(const char *filename) {

    FILE *fp;
    uint8_t header[HEADER_SIZE];

    fp = fopen(filename, "r+b");

    if(fp == NULL) {
        fprintf(stderr, "Failed to open %s for reading/writing\n", filename);
        return -errno;
    }

    if(fread(header, 1, HEADER_SIZE, fp) != HEADER_SIZE) {
        fclose(fp);
        fprintf(stderr, "Error reading from %s\n", filename);
        return -errno;
    }

    fix_header(header);

    if(fseek(fp, 0, SEEK_SET) < 0) {
        fclose(fp);
        fprintf(stderr, "Failed to rewind file %s\n", filename);
        return -errno;
    }

    if(fwrite(header, 1, HEADER_SIZE, fp) != HEADER_SIZE) {
        fclose(fp);
        fprintf(stderr, "Error writing header back to file %s\n", filename);
        return -errno;
    }

    fclose(fp);
    return 0;
}

static void usage(const char *ego) {
    const char *name;

    name = strrchr(ego, '/');

    if(name == NULL)
        name = ego;
    else
        name++;

    printf("GBA Complement Check fix\n");
    printf("Usage: %s file1 [file2...]\n", name);
}

int main(int argc, char *argv[]) {

    int i;

    if(argc < 2) {
        usage(argv[0]);
        return 0;
    }

    for(i = 1; i < argc; i++) {
        int ok = process(argv[i]);

        if(ok != 0)
            return ok;
    }

    return 0;
}
