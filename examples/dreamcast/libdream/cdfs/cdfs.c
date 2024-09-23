/* Small demonstration of using the CD file system; note that this
   applies pretty equally to all the other file systems. */

#include <kos.h>


char buffer[2048];
void cdfs_test(void) {
    file_t fd;
    file_t d;
    dirent_t *de;
    int amt;

    printf("Reading directory from CD-Rom:\r\n");

    /* Read and print the root directory */
    d = fs_open("/cd", O_RDONLY | O_DIR);

    if(d < 0) {
        printf("Can't open root!\r\n");
        return;
    }

    while((de = fs_readdir(d))) {
        printf("%s  /  ", de->name);

        if(de->size >= 0) {
            printf("%d\r\n", de->size);
        }
        else {
            printf("DIR\r\n");
        }
    }

    fs_close(d);

    /* Read and print a file called README.TXT (if any) */
    fd = fs_open("/cd/readme.txt", O_RDONLY);

    if(fd < 0) {
        printf("Can't open file README.TXT for reading.\r\n");
        return;
    }

    while((amt = fs_read(fd, buffer, 2048))) {
        int i;

        for(i = 0; i < amt; i++) printf("%c", buffer[i]);
    }

    fs_close(fd);
}

int main(int argc, char **argv) {
    cdfs_test();
    return 0;
}
