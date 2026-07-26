/* KallistiOS ##version##

   examples/xbox/hello/hello.c
   Copyright (C) 2026 Andress Barajas
*/

/* "Hello, Xbox!" — a runnable console program for the original Microsoft Xbox,
   loaded over the network by kos-tool's xbox-load-ip.

   There is no KOS Xbox arch yet (no libc, no video/console driver), so this is
   a freestanding guest that talks straight to the loader. xbox-load-ip
   publishes a small header at XBOX_KOSLOAD_BASE -- a magic value plus a syscall
   trampoline -- and the guest calls syscall(WRITE, 1, ...) to print to the host
   console and syscall(EXIT) to hand control back. This is the same guest ABI
   kos-tool's own console-test / xbox-video-test examples use.

   Run:  kos-tool -x hello.elf      (see the Makefile 'run' target) */

/* Loader header: magic at XBOX_KOSLOAD_BASE+0, syscall fn ptr at +4. The build
   passes -DXBOX_KOSLOAD_BASE; default to the conventional 0x00011000 (loader
   XBE base 0x10000 + a 0x1000 header page). */
#ifndef XBOX_KOSLOAD_BASE
#define XBOX_KOSLOAD_BASE 0x00011000u
#endif

#define KOSLOAD_MAGIC  0xdeadbeefu
#define SYSCALL_WRITE  1
#define SYSCALL_EXIT   15

#define KOSLOAD_MAGIC_PTR   (*(volatile unsigned int *)(XBOX_KOSLOAD_BASE + 0))
#define KOSLOAD_SYSCALL_PTR (*(volatile unsigned int *)(XBOX_KOSLOAD_BASE + 4))

typedef int (*kosload_syscall_fn)(int nr, int arg1, int arg2, int arg3);

static int slen(const char *s) {
    int n = 0;
    while(s[n])
        n++;
    return n;
}

/* Entry point. The loader jumps here (-e _start; the PE mangles C 'start' to
   the '_start' symbol). Placed in .text.start so it lands at the image base. */
void start(void) __attribute__((section(".text.start")));
void start(void) {
    const char *msg = "Hello, Xbox!\n";
    kosload_syscall_fn syscall;

    /* Bail out cleanly if we are not actually running under kosload. */
    if(KOSLOAD_MAGIC_PTR != KOSLOAD_MAGIC)
        return;

    syscall = (kosload_syscall_fn)KOSLOAD_SYSCALL_PTR;

    syscall(SYSCALL_WRITE, 1, (int)msg, slen(msg));
    syscall(SYSCALL_EXIT, 0, 0, 0);
}
