/* KallistiOS ##version##

   ser_console.c
   (c)2001 Megan Potter

 */

#include <stdio.h>
#include <string.h>
#include <arch/dbgio.h>
#include <arch/irq.h>
#include <kos/thread.h>
#include <kos/sem.h>

/* This is a little thing I ported over from KOS-MMU which turned out
   to be extremely useful during debugging. It basically sets up a
   simple serial console on the DC serial port which you can use
   to view process listings, etc. It has to be manually enabled,
   though, and it will interfere with dc-load-serial, so don't try
   to use both at once. */

static char buffer[256];
static semaphore_t *chr_ready;

static vuint16 * const SCSCR2 = (vuint16*)0xffe80008;

int thd_pslist();

static void ser_irq(irq_t source, irq_context_t *context) {
    *SCSCR2 &= ~(1 << 6);
    *SCSCR2 |= (1 << 6);

    sem_signal(chr_ready);
}

static char *read_line(void) {
    int q = 0, ch;

    while(1) {
        while((ch = dbgio_read()) == -1)
            /* sem_wait(chr_ready); */
            thd_pass();

        if(ch == '\r') {
            buffer[q] = 0;
            dbgio_write_str("\n");
            return buffer;
        }

        if(ch == '\x08') {
            q--;
            dbgio_write_str("\x08 \x08");
            continue;
        }

        buffer[q++] = ch;
        dbgio_write(ch);
    }
}

static char curdir[256];

static void interact(void) {
    char tmp[16];
    char *buf;
    int cnt = 0;

    curdir[0] = '/';
    curdir[1] = 0;

    while(1) {
        sprintf(tmp, "%d", cnt);
        dbgio_write_str(tmp);
        dbgio_write(' ');
        dbgio_write_str(curdir);
        dbgio_write('>');
        /*write_str("%d %s> ", cnt, curdir); */
        buf = read_line();

        if(buf[0] == 0) {
            cnt++;
            continue;
        }

        if(!strcmp(buf, "quit") || !strcmp(buf, "exit")) {
            break;
        }
        else if(!strcmp(buf, "die")) {
            arch_exit();
        }
        else if(!strcmp(buf, "ps")) {
            thd_pslist();
        }
        else if(!strcmp(buf, "reboot")) {
            arch_reboot();
        }
        else if(!strcmp(buf, "menu")) {
            (*(void(**)())0x8c0000e0)(1);
        }
        else {
            dbgio_write_str("commands: die quit/exit reboot menu ps\n");
        }

        cnt++;
    }
}

static void real_start(void *param) {
    dbgio_printk_func old;

    /* Set debug output to the serial console again */
    old = dbgio_set_printk(dbgio_write_str);

    /* Make a semaphore */
    /* chr_ready = sem_create(0); */

    /* Hook the serial IRQ */
    /* irq_set_handler(EXC_SCIF_RXI, ser_irq);
    *SCSCR2 |= 1 << 6;
    irq_set_priority(IRQ_SRC_SCIF, 14); */

    /* Do the shell */
    interact();
    dbgio_write_str("shell exiting\n");

    /* Unhook serial IRQ */
    /* irq_set_priority(IRQ_SRC_SCIF, IRQ_PRIO_MASKED);
    *SCSCR2 &= ~(1 << 6);
    irq_set_handler(EXC_SCIF_RXI, NULL); */

    /* Destroy the semaphore */
    /* sem_destroy(chr_ready); */

    dbgio_set_printk(old);
}

void ser_console_init(void) {
    thd_create(real_start, NULL);
}

