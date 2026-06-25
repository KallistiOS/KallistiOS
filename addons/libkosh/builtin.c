/*
 * builtin.c - shell builtin commands
 *
 * (C) 2000 Jordan DeLong
 */

#include <ctype.h>
#include <dirent.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/queue.h>

#include <arch/arch.h>
#include <dc/video.h>
#include <kos/fs_romdisk.h>
#include <kos/thread.h>
#include "builtin.h"
#include "chdir.h"
#include "kosh.h"

typedef struct builtin {
    SLIST_ENTRY(builtin)    list;

    const char *command;
    const char *desc;

    void (*handler)(int argc, char *argv[]);
} builtin_t;

static SLIST_HEAD(builtin_list, builtin) builtins;

/* print command list/desc */
static void builtin_help(int argc, char *argv[]) {
    builtin_t *b;

    if (argc > 1) {
        SLIST_FOREACH(b, &builtins, list) {
            if (strcmp(argv[1], b->command) == 0) {
                conio_printf("%s - %s\n", b->command, b->desc);
                return;
            }
        }
    } else {
        conio_printf("KOSH commands:\n  ");

        SLIST_FOREACH(b, &builtins, list) {
            conio_printf("%s ", b->command);
        }

        conio_printf("\nType 'help command' for a description.\n");
    }
}

/* exit the shell */
static void builtin_exit(int argc, char *argv[]) {
    if (argc > 1)
        conio_printf("Superfluous arguments to exit\n");
    else
        kosh_exit = 1;
}

/* list the contents of the directory */
static void builtin_ls(int argc, char *argv[]) {
    const dirent_t *ent;
    uint32_t hnd;
    char dir[NAME_MAX];
    int lflag;

    lflag = 0;

    if (argc > 1) {
        if (strcmp(argv[1], "-l") == 0)
            lflag = 1;
    }

    getcwd(dir, NAME_MAX);

    conio_printf("Reading %s\n", dir);

    hnd = fs_open(dir, O_RDONLY | O_DIR);
    if (!hnd) {
        conio_printf("Error opening %s\n", dir);
        return;
    }

    while ((ent = fs_readdir(hnd)) != NULL) {
        if (lflag)
            conio_printf("%12d bytes %s\n", ent->size, ent->name);
        else
            conio_printf("%s\n", ent->name);
    }

    fs_close(hnd);
}

/* change the current directory */
static void builtin_cd(int argc, char *argv[]) {
    if (argc != 2) {
        conio_printf("usage: cd dir\n");
        return;
    }

    kosh_chdir(argv[1]);
}

/* print the current directory */
static void builtin_pwd(int argc, char *argv[]) {
    char buff[NAME_MAX];

    conio_printf("%s\n", getcwd(buff, NAME_MAX));
}

/* clear the screen */
static void builtin_clear(int argc, char *argv[]) {
    conio_clear();
    conio_gotoxy(0, 0);
}

/* echo, heh */
static void builtin_echo(int argc, char *argv[]) {
    int i;

    for (i = 1; i < argc; i++)
        conio_printf("%s ", argv[i]);
    conio_printf("\n");
}

/* cat text files to the screen */
static void builtin_cat(int argc, char *argv[]) {
    uint32_t hnd;
    char buff[256];
    char fn[NAME_MAX];
    int cnt;

    if (argc != 2) {
        conio_printf("usage: cat filename\n");
        return;
    }

    /* the the abs path */
    makeabspath(fn, argv[1], NAME_MAX);

    hnd = fs_open(fn, O_RDONLY);
    if (!hnd) {
        conio_printf("Error opening %s\n", fn);
        return;
    }

    while ((cnt = fs_read(hnd, buff, 256)) != 0) {
        int i;

        for (i = 0; i < cnt; i++) {
            if (buff[i] == '\r') {
                if (buff[i + 1] != '\0')
                    break;
                if (buff[++i] != '\n')
                    conio_putch('\r');
            }
            if (isascii(buff[i]))
                conio_putch(buff[i]);
        }
    }

    fs_close(hnd);
}

/* dump files as hex */
static void builtin_hd(int argc, char *argv[]) {
    char fn[NAME_MAX];
    char buff[10];
    uint32_t hnd;
    int cnt;

    if (argc != 2) {
        conio_printf("usage: hd file\n");
        return;
    }

    makeabspath(fn, argv[1], NAME_MAX);

    hnd = fs_open(fn, O_RDONLY);
    if (!hnd) {
        conio_printf("Error opening %s\n", fn);
        return;
    }

    /* do the hexdump */
    while ((cnt = fs_read(hnd, buff, 10)) != 0) {
        int i;

        for (i = 0; i < cnt; i++)
            conio_printf("%02x ", (unsigned char) buff[i]);
        conio_deadvance_cursor();
        //cursor.col = 10 * 3 + 2;
        conio_putch('|');
        for (i = 0; i < cnt; i++) {
            if (isprint((int)buff[i]))
                conio_putch(buff[i]);
            else
                conio_putch('.');
        }
        conio_printf("|\n");
    }

    fs_close(hnd);
}

/* copy files */
static void builtin_cp(int argc, char *argv[]) {
    char buff[256];
    char srcfn[NAME_MAX];
    char destfn[NAME_MAX];
    uint32_t src;
    uint32_t dest;
    int cnt;

    if (argc != 3) {
        conio_printf("usage: cp src dest\n");
        return;
    }

    /* get abs paths */
    makeabspath(srcfn, argv[1], NAME_MAX);
    makeabspath(destfn, argv[2], NAME_MAX);

    src = fs_open(srcfn, O_RDONLY);
    if (!src) { conio_printf("Error opening %s\n", srcfn); return; }

    dest = fs_open(destfn, O_WRONLY);
    if (!dest) { conio_printf("Error opening %s\n", destfn); fs_close(src); return; }

    while ((cnt = fs_read(src, buff, 256)) != 0)
        fs_write(dest, buff, cnt);

    fs_close(src);
    fs_close(dest);
}

/* unlink files */
static void builtin_rm(int argc, char *argv[]) {
    char fn[NAME_MAX];

    if (argc != 2) {
        conio_printf("usage: rm file\n");
        return;
    }

    /* get the abs path for the fn to rem */
    makeabspath(fn, argv[1], NAME_MAX);

    if (fs_unlink(fn) != 0)
        conio_printf("Error unlinking %s.\n", fn);
}

/* create a directory */
static void builtin_mkdir(int argc, char *argv[]) {
    char fn[NAME_MAX];

    if (argc != 2) {
        conio_printf("usage: mkdir dirname\n");
        return;
    }

    /* get the abs path for the dir to create */
    makeabspath(fn, argv[1], NAME_MAX);

    if (fs_mkdir(fn) != 0)
        conio_printf("Error making directory %s.\n", fn);
}

/* delete a directory */
static void builtin_rmdir(int argc, char *argv[]) {
    char fn[NAME_MAX];

    if (argc != 2) {
        conio_printf("usage: rmdir dirname\n");
        return;
    }

    /* get the abs path for the dir to delete */
    makeabspath(fn, argv[1], NAME_MAX);

    if (fs_rmdir(fn) != 0)
        conio_printf("Error deleting directory %s.\n", fn);
}

/* change console colors */
static void builtin_theme(int argc, char *argv[]) {
    if (argc != 2) {
        conio_printf("usage: theme themename\nchoices are 'matrix', 'c64' and 'plain'\n");
        return;
    }

    if (strcmp(argv[1], "matrix") == 0)
        conio_set_theme(CONIO_THEME_MATRIX);
    else if (strcmp(argv[1], "c64") == 0)
        conio_set_theme(CONIO_THEME_C64);
    else if (strcmp(argv[1], "plain") == 0)
        conio_set_theme(CONIO_THEME_PLAIN);
    else
        conio_printf("unknown theme\n");
}

/* exit to the DC menus */
static void builtin_menu(int argc, char *argv[]) {
    conio_printf("bye!\n");
    arch_menu();
}

/* Mount a romdisk image */
static void builtin_mount_romdisk(int argc, char *argv[]) {
    void *data;

    if (argc != 3) {
        conio_printf("usage: mount_romdisk srcimg mountpoint\n");
        return;
    }

    /* Load up the romdisk image */
    conio_printf("Loading the romdisk image...\n");
    if (fs_load(argv[1], &data) <= 0) {
        conio_printf("error: invalid romdisk source image\n");
        if (data != NULL)
            free(data);
        return;
    }

    /* Attempt to mount it */
    conio_printf("Mounting on %s...\n", argv[2]);
    if (fs_romdisk_mount(argv[2], (const uint8_t *)data, 1) < 0) {
        conio_printf("error: can't mount romdisk image\n");
        free(data);
        return;
    }

    conio_printf("Success!\n");
    return;
}

/* Print out a list of running threads */
static void builtin_threads(int argc, char *argv[]) {
    thd_pslist(conio_printf);
}

/* Print out memory usage statistics */
static void builtin_mstats(int argc, char *argv[]) {
    struct mallinfo mi = mallinfo();

    conio_printf("Memory usage:\n");
    conio_printf("  Max system bytes = %10lu\n",
        (unsigned long)(mi.usmblks));
    conio_printf("  System bytes = %10lu\n",
        (unsigned long)(mi.arena + mi.hblkhd));
    conio_printf("  In use bytes = %10lu\n",
        (unsigned long)(mi.uordblks + mi.hblkhd));
    conio_printf("  Current sbrk = %08lx\n",
        (unsigned long)(sbrk(0)));
}

/* Kill the whole KOS program */
static void builtin_die(int argc, char *argv[]) {
    conio_printf("Goodbye cruel world...\n");
    arch_exit();
}

#ifdef _arch_dreamcast
/* Do a screen dump -- note, won't always work 100% due to threading stuff */
static void builtin_sshot(int argc, char *argv[]) {
    char fn[NAME_MAX];

    if (argc != 2) {
        conio_printf("usage: sshot outfile.ppm\n");
        return;
    }

    /* get the abs path for the dir to create */
    makeabspath(fn, argv[1], NAME_MAX);

    conio_printf("Doing a screen shot to %s\n", fn);
    vid_screen_shot(fn);
    conio_printf("Done!\n");
}
#endif


/* try to run a builtin command, return 0 if there is no such builtin */
int kosh_builtin_command(int argc, char *argv[]) {
    builtin_t *b;

    SLIST_FOREACH(b, &builtins, list) {
        if (strcmp(argv[0], b->command) == 0) {
            b->handler(argc, argv);
            return 1;
        }
    }

    return 0;
}

void kosh_builtin_add(const char *cmd, const char *helpmsg, void (*handler)(int argc, char *argv[])) {
    builtin_t *b;

    b = (builtin_t *)calloc(1, sizeof(builtin_t));
    b->command = cmd;
    b->desc = helpmsg;
    b->handler = handler;

    SLIST_INSERT_HEAD(&builtins, b, list);
}

void kosh_builtin_remove(const char *cmd) {
    builtin_t *b;

    SLIST_FOREACH(b, &builtins, list) {
        if(strcmp(cmd, b->command) == 0) {
            SLIST_REMOVE(&builtins, b, builtin, list);
            free(b);
        }
    }
}

/* Setup all our builtins */
void kosh_builtins_init(void) {
    SLIST_INIT(&builtins);

    /* table of the builtin commands, their help mesg,  and their handler funcs */
    kosh_builtin_add("help",    "Hmm...",                           builtin_help);
    kosh_builtin_add("exit",    "Exit kosh",                        builtin_exit);
    kosh_builtin_add("ls",      "List contents of directories",     builtin_ls);
    kosh_builtin_add("cd",      "Change directory",                 builtin_cd);
    kosh_builtin_add("pwd",     "Print the current directory",      builtin_pwd);
    kosh_builtin_add("clear",   "Clear the screen",                 builtin_clear);
    kosh_builtin_add("echo",    "Echo text to the console",         builtin_echo);
    kosh_builtin_add("cat",     "Display text files to the console",    builtin_cat);
    kosh_builtin_add("hd",      "Dump files as hex to the console", builtin_hd);
    kosh_builtin_add("cp",      "Copy files",                       builtin_cp);
    kosh_builtin_add("rm",      "Remove files",                     builtin_rm);
    kosh_builtin_add("mkdir",   "Create a directory",               builtin_mkdir);
    kosh_builtin_add("rmdir",   "Delete a directory",               builtin_rmdir);
    kosh_builtin_add("theme",   "Change the console color theme",   builtin_theme);
    kosh_builtin_add("menu",    "Exit KOS to the bios menus",       builtin_menu);
    kosh_builtin_add("mount_romdisk", "Mount a romdisk image",      builtin_mount_romdisk);
    kosh_builtin_add("threads", "Get a list of running threads",    builtin_threads);
    kosh_builtin_add("mstats",  "Get memory statistics",            builtin_mstats);
    kosh_builtin_add("die",     "Call arch_exit()",                 builtin_die);
#ifdef _arch_dreamcast
    kosh_builtin_add("sshot",   "Make a screen shot",               builtin_sshot);
#endif
}

/* Kill our list */
void kosh_builtins_shutdown(void) {
    builtin_t *c, *n;

    SLIST_FOREACH_SAFE(c, &builtins, list, n) {
        SLIST_REMOVE(&builtins, c, builtin, list);
        free(c);
    }

    SLIST_INIT(&builtins);
}
