/* KallistiOS ##version##

   exec.c
   (c)2002 Megan Potter
*/

#include <kos.h>
#include <assert.h>

int main(int argc, char **argv) {
    file_t f;
    void *subelf;

    /* Print a hello */
    printf("\n\nHello world from the exec.elf process\n");

    /* Map the sub-elf */
    f = fs_open("/rd/sub.bin", O_RDONLY);
    assert(f);
    subelf = fs_mmap(f);
    assert(subelf);

    /* Tell exec to replace us */
    printf("sub.bin mapped at %08x, jumping to it!\n\n\n", (unsigned int)subelf);
    arch_exec(subelf, fs_total(f));

    /* Shouldn't get here */
    assert_msg(false, "exec call failed");

    return 0;
}


