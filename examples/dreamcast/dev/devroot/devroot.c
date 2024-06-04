/* KallistiOS ##version##

   devroot.c
   Copyright (C) 2024 Donald Haase

   This example demonstrates the expected behavior of reading from the 
   / and /dev dirs, as well as attempting to open a non-existant device 
   for the same.
*/

#include <kos.h>
#include <errno.h>
#include <dirent.h>

KOS_INIT_FLAGS(INIT_DEFAULT);

/* This tests readdir and rewinddir by printing a list of all entries, 
then rewinding and counting that the same number of entries are present */
void printdir(char* fn) {
    DIR *d;
    struct dirent *entry;
    int cnt1 = 0, cnt2 = 0;

    if(!(d = opendir(fn))) {
        printf("Could not open %s: %s\n", fn, strerror(errno));
    }
    else {
        printf("Opened %s and found these: \n", fn);
        while((entry = readdir(d))) {
            printf("    %s\n", entry->d_name);
            cnt1++;
        }

        printf("Rewinding %s to loop again.\n", fn);
        rewinddir(d);

        while((entry = readdir(d))) {
            cnt2++;
        }

        if(cnt1 == cnt2) {
            printf("PASS: Counted %i entries both times.\n", cnt1);
        }
        else
            printf("FAIL: Counted %i entries the first time and %i the second.\n", cnt1, cnt2);

        closedir(d);
    }
}

int main(int argc, char **argv) {

    /* Open the root of dev's filesystem and list the contents. */
    /* This should *not* show /dev subdirs */
    printdir("/");

    /* Now do the same with /dev. This should list out the devices under it. */
    printdir("/dev");

    /* Now try the same with a fake dev. It should fail */
    printdir("/dev/quzar");

    return 0;
}
