/*
 * Kallistios 2.0.0
 *
 * txt2bin.c
 * (c)2002, Paul Boese
 *
 * Take the <object>.txt files from nehe lesson26 and convert them into
 * binary files so we can eliminate the use of sscanf from the nehe26 demo.
 *
 * The format of the files:
 * <file.txt> (input file):
 * line 1
 * Vertices: <integer>
 * line 2 thru end. Should have <integer> lines after line 1.
 * <float> <float> <float>
 * <float> <float> <float>
 * ...
 *
 * <file.bin> (output file):
 * First 4 bytes (integer) is number of vertices that follow.
 * Remainder of file 4 byte floats. Read in 3 at a time. We assume
 * litte endian packing.
 */

#include <stdio.h>
#include <stdlib.h>

/* function to read a string from a file */
void readstr(FILE *f, char *string) {
    /* Loop Until End Of Line Is Reached */
    do {
        /* Gets A String Of 255 Chars Max From f (File) */
        fgets(string, 255, f);
    }
    while((string[0] == '/') || (string[0] == '\n'));

    return;
}

unsigned char buffer[4096];
char oneline[255]; /* Holds One Line Of Text (255 Chars Max) */

void convert(char *ifn, char *ofn) {
    FILE *in, *out;
    int num_verts, i;
    float rx, ry, rz;

    in = fopen(ifn, "rt");
    out = fopen(ofn, "w");

    if(!in || !out) {
        printf("error: can't open input or output file\n");
        return;
    }

    /* Jumps To Code That Reads One Line Of Text From The File */
    readstr(in, oneline);
    /* Scans Text For "Vertices: ".  Number After Is Stored In ver */
    sscanf(oneline, "Vertices: %d\n", &num_verts);
    fwrite(&num_verts, sizeof(int), 1, out);

    /* Loops Through The Vertices */
    for(i = 0; i < num_verts; i++) {
        /* Reads In The Next Line Of Text */
        readstr(in, oneline);
        /* Searches For 3 Floating Point Numbers, Store In rx,ry & rz */
        sscanf(oneline, "%f %f %f", &rx, &ry, &rz);
        fwrite(&rx, sizeof(float), 1, out);
        fwrite(&ry, sizeof(float), 1, out);
        fwrite(&rz, sizeof(float), 1, out);
    }

    fclose(in);
    fclose(out);
}

int main(int argc, char **argv) {
    argc--;

    if(argc != 2) {
        printf("usage: txt2bin <input.txt> <output.bin>\n");
        return 1;
    }

    convert(argv[1], argv[2]);

    return 0;
}
