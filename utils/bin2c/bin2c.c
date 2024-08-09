/* Converts a binary file into a C integer array (for inclusion in
   a source file)

   (c)2000 Megan Potter

*/

#include <stdio.h>

void convert(char *ifn, char *ofn, char *prefix) {
    FILE *i, *o;
    unsigned char buffer[2048];
    int red, left, lc, q;
    char buf[BUFSIZ];

    i = fopen(ifn, "rb");
    o = fopen(ofn, "w");

    if(!i || !o) {
        printf("error: can't open input or output file\n");
        return;
    }

    fseek(i, 0, SEEK_END);
    left = ftell(i);
    fseek(i, 0, SEEK_SET);
    setbuf(o, buf);

    fprintf(o, "#ifdef __cplusplus\nextern \"C\"\n#endif\n");
    fprintf(o, "const int %s_size = %d;\n", prefix, left);

    fprintf(o, "#ifdef __cplusplus\nextern \"C\"\n#endif\n");
    fprintf(o, "const unsigned char %s_data[%d] =", prefix, left);
    fprintf(o, "{\n\t");

    lc = 0;

    while(left > 0) {
        red = fread(buffer, 1, 2048, i);
        left -= red;

        for(q = 0; q < red; q++) {
            fprintf(o, "0x%02x, ", buffer[q]);

            if((++lc) >= 8) {
                lc = 0;
                fprintf(o, "\n\t");
            }
        }
    }

    fprintf(o, "\n};\n");
    fclose(i);
    fclose(o);
}

int main(int argc, char **argv) {

    char *prefix;

    argc--;

    if(argc != 2 && argc != 3) {
        printf("usage: bin2c <input> <output> [prefix]\n");
        return 0;
    }

    prefix = (argc == 3) ? argv[3] : "file";
    convert(argv[1], argv[2], prefix);
    return 0;
}

