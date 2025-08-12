/* KallistiOS ##version##

   exec.c
   (c)2002 Megan Potter
*/

#include <kos.h>
#include <assert.h>

int main(int argc, char **argv) {
    file_t f;
    size_t s;
    ssize_t rv;
    void *subbin;

    /* Print a hello */
    printf("\n\nHello world from the exec.elf process\n");

    /* Open the sub-bin. Note normally this wouldn't be from a
       romdisk as that means that you'd already have it loaded
       in memory.
    */
    f = fs_open("/rd/sub.bin", O_RDONLY);
    assert(f);

    /* Get the size of sub.bin */
    s = fs_total(f);
    assert(s);

    /* Allocate space for it */
    subbin = malloc(s);
    assert(subbin);

    /* Copy in the sub.bin */
    rv = fs_read(f, subbin, s);
    assert(rv == s);

    /* Lets be nice and tidy up after ourselves */
    fs_close(f);

    /* Tell exec to replace us */
    printf("sub.bin loaded at %08x, jumping to it!\n\n\n", (uintptr_t)subbin);
    arch_exec(subbin, s);

    /* Shouldn't get here */
    assert_msg(false, "exec call failed");

    return 0;
}


