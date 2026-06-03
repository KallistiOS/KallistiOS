/* KallistiOS ##version##

   conio.c
   Copyright (C) 2002 Megan Potter

   Adapted from Kosh, Copyright (C) 2000 Jordan DeLong

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <kos/fs.h>
#include <kos/thread.h>
#include <kos/sem.h>
#include <dc/maple/keyboard.h>
#include <dc/scif.h>
#include <conio/conio.h>

/* the cursor */
conio_cursor_t conio_cursor;

/* the virtual screen */
char conio_virtscr[CONIO_NUM_ROWS][CONIO_NUM_COLS];

/* Our modes */
int conio_ttymode = CONIO_TTY_NONE, conio_inputmode = CONIO_INPUT_NONE;
int conio_theme = CONIO_THEME_PLAIN;

/* freeze/thaw mutex */
static semaphore_t ft_mutex;

/* File handle for serial tty */
file_t conio_serial_fd;

/* scroll everything up a line */
void conio_scroll(void) {
    int i;

    switch (conio_ttymode) {
        case CONIO_TTY_PVR:
            memmove(conio_virtscr, conio_virtscr[1], (CONIO_NUM_ROWS - 1) * CONIO_NUM_COLS);
            for (i = 0; i < CONIO_NUM_COLS; i++)
                conio_virtscr[CONIO_NUM_ROWS - 1][i] = ' ';
            conio_cursor.row--;
            break;
        case CONIO_TTY_SERIAL:
            scif_write_buffer((const uint8_t *)"\x1b[M", 3, 1);
            break;
        case CONIO_TTY_STDIO:
            fs_write(conio_serial_fd, (void *)"\x1b[M", 3);
            break;
        case CONIO_TTY_DBGIO:
            dbgio_write_buffer((const uint8_t *)"\x1b[M", 3);
            break;
    }
}

/* move the cursor back, don't scroll (we can't) */
void conio_deadvance_cursor(void) {
    switch (conio_ttymode) {
        case CONIO_TTY_PVR:
            conio_cursor.col--;
            if (conio_cursor.col < 0) {
                if (conio_cursor.row != 0) {
                    conio_cursor.col = CONIO_NUM_COLS - 1;
                    conio_cursor.row--;
                } else {
                    conio_cursor.col = 0;
                }
            }
            break;
        case CONIO_TTY_SERIAL:
            scif_write(8);
            scif_write(32);
            scif_write(8);
            break;
        case CONIO_TTY_STDIO:
            fs_write(conio_serial_fd, (void *)"\x8\x20\x8", 3);
            break;
        case CONIO_TTY_DBGIO:
            dbgio_write_buffer((const uint8_t *)"\x8\x20\x8", 3);
            break;
    }
}

/* move the cursor ahead, scroll if we need to */
void conio_advance_cursor(void) {
    switch (conio_ttymode) {
        case CONIO_TTY_PVR:
            conio_cursor.col++;
            if (conio_cursor.col >= CONIO_NUM_COLS) {
                conio_cursor.col = 0;
                conio_cursor.row++;
                if (conio_cursor.row >= CONIO_NUM_ROWS)
                    conio_scroll();
            }
            break;
        case CONIO_TTY_SERIAL:
            scif_write_buffer((const uint8_t *)"\x1b[1C", 4, 1);
            break;
        case CONIO_TTY_STDIO:
            fs_write(conio_serial_fd, (void *)"\x1b[1C", 4);
            break;
        case CONIO_TTY_DBGIO:
            dbgio_write_buffer((const uint8_t *)"\x1b[1C", 4);
            break;
    }
}

/* move the cursor */
void conio_gotoxy(int x, int y) {
    switch (conio_ttymode) {
        case CONIO_TTY_PVR:
            conio_cursor.col = x;
            conio_cursor.row = y;
            break;
        case CONIO_TTY_SERIAL: {
            char tmp[256];
            sprintf(tmp, "\x1b[%d;%df", x, y);
            scif_write_buffer((unsigned char *)tmp, strlen(tmp), 1);
            break;
        }
        case CONIO_TTY_STDIO: {
            char tmp[256];
            sprintf(tmp, "\x1b[%d;%df", x, y);
            fs_write(conio_serial_fd, tmp, strlen(tmp));
            break;
        }
        case CONIO_TTY_DBGIO: {
            char tmp[256];
            sprintf(tmp, "\x1b[%d;%df", x, y);
            dbgio_write_buffer((unsigned char *)tmp, strlen(tmp));
            break;
        }
    }
}

/* blocking call for a character */
int conio_getch(void) {
    int key = -1;
    uint8_t b;

    switch (conio_ttymode) {
        case CONIO_TTY_PVR:
#ifdef GFX
            while ((key = kbd_get_key()) == -1) { thd_pass(); }
#endif
            break;
        case CONIO_TTY_SERIAL: {
            while ((key = scif_read()) == -1) { thd_pass(); }

            if (key == 3)
                exit(EXIT_SUCCESS);

            break;
        }
        case CONIO_TTY_STDIO: {
            int i;
            i = fs_read(conio_serial_fd, &b, 1);
            if (i <= 0)
                return -1;
            key = b;
            if (key == '\n')
                return conio_getch();
            break;
        }
        case CONIO_TTY_DBGIO: {
            while (dbgio_read_buffer(&b, 1) == -1) {
                thd_sleep(1000);
            }

            key = b;

            if (key == 3)
                exit(EXIT_SUCCESS);

            break;
        }
    }

    return key;
}

/* Check to see if a key has been pressed */
int conio_check_getch(void) {
    int key = -1;
    uint8_t b;

    switch (conio_ttymode) {
        case CONIO_TTY_PVR:
#ifdef GFX
            key = kbd_get_key();
#endif
            break;
        case CONIO_TTY_SERIAL: {
            key = scif_read();

            if (key == 3)
                exit(EXIT_SUCCESS);

            break;
        }
        case CONIO_TTY_STDIO:
            if (fs_total(conio_serial_fd) > 0) {
                if (fs_read(conio_serial_fd, &b, 1) == 1)
                    key = b;
            }
            if (key == '\n')
                key = -1;
            break;
        case CONIO_TTY_DBGIO: {
            if(dbgio_read_buffer(&b, 1) == 1)
                key = b;

            if (key == 3)
                exit(EXIT_SUCCESS);

            break;
        }
    }

    return key;
}

/* set current char to ch, w/o advancing the cursor */
void conio_setch(int ch) {
    switch (conio_ttymode) {
        case CONIO_TTY_PVR:
            switch (ch) {
                case '\n':
                case '\r':
                    break;
                default:
                    conio_virtscr[conio_cursor.row][conio_cursor.col] = ch;
            }
            break;
        case CONIO_TTY_SERIAL:
        case CONIO_TTY_STDIO:
        case CONIO_TTY_DBGIO:
            conio_deadvance_cursor();
            conio_putch(ch);
            break;
    }
}

/* put a character at the cursor and move the cursor */
void conio_putch(int ch) {
    switch (conio_ttymode) {
        case CONIO_TTY_PVR:
            switch (ch) {
                case '\r':
                    conio_cursor.col = 0;
                    break;
                case '\n':
                    conio_cursor.row++;
                    conio_cursor.col = 0;
                    if (conio_cursor.row >= CONIO_NUM_ROWS)
                        conio_scroll();
                    break;
                default:
                    conio_virtscr[conio_cursor.row][conio_cursor.col] = ch;
                    conio_advance_cursor();
            }
            break;
        case CONIO_TTY_SERIAL:
            if (ch == '\n')
                scif_write('\r');
            else if (ch == '\r')
                break;
            scif_write(ch);
            break;
        case CONIO_TTY_STDIO:
            if (ch == '\n')
                fs_write(conio_serial_fd, "\r", 1);
            else if (ch == '\r')
                break;
            fs_write(conio_serial_fd, &ch, 1);
            break;
        case CONIO_TTY_DBGIO:
            dbgio_write_buffer((uint8_t *)&ch, 1);
            break;
    }
}

/* put a string of characters */
void conio_putstr(char *str) {
    while (*str != '\0') {
        conio_putch(*str++);
    }
}

/* a printfish function */
int conio_printf(const char *fmt, ...) {
    char buff[512]; /* buffer overflow waiting to happen.... I'll add a vsnprintf func later */
    va_list args;
    int i;

    va_start(args, fmt);
    i = vsprintf(buff, fmt, args);

    if (conio_ttymode != CONIO_TTY_NONE)
        conio_putstr(buff);

    va_end(args);

    return i;
}

/* clear the screen */
void conio_clear(void) {
    int row, col;

    switch (conio_ttymode) {
        case CONIO_TTY_PVR:
            /* fill screen with spaces */
            for (row = 0; row < CONIO_NUM_ROWS; row++)
                for (col = 0; col < CONIO_NUM_COLS; col++)
                    conio_virtscr[row][col] = ' ';
            break;
        case CONIO_TTY_SERIAL:
            scif_write_buffer((uint8_t *)"\x1b[2J", 4, 1);
            break;
        case CONIO_TTY_STDIO:
            fs_write(conio_serial_fd, (void *)"\x1b[2J", 4);
            break;
        case CONIO_TTY_DBGIO:
            dbgio_write_buffer((uint8_t *)"\x1b[2J", 4);
            break;
    }
}

/* conio freeze (for sub-process taking over TA) */
void conio_freeze(void) {
    sem_wait(&ft_mutex);
}

/* conio thaw */
void conio_thaw(void) {
    sem_signal(&ft_mutex);
}

/* set theme */
void conio_set_theme(int theme) {
    assert( theme >= CONIO_THEME_PLAIN && theme <= CONIO_THEME_C64 );
    conio_theme = theme;
}

/* ptr to old printf */
/* static int (*oldprintf)(const char *fmt, ...) = NULL; */
static volatile int conio_entered = 0;
static volatile int conio_exit = 0;

/* the drawing/keyboard polling thread */
static void *conio_thread(void *param) {
    (void)param;

    conio_entered = 1;
    while (!conio_exit) {
        sem_wait(&ft_mutex);
        conio_input_frame();
        if (conio_ttymode == CONIO_TTY_PVR) {
#ifdef GFX
            conio_draw_frame();
#endif
        } else {
            if (conio_ttymode == CONIO_TTY_SERIAL)
                scif_flush();
            thd_sleep(1000/60); /* Simulate frame delay */
        }
        sem_signal(&ft_mutex);
    }
    conio_exit = -1;
    return NULL;
}

/* initialize the console I/O stuffs */
int conio_init(int ttymode, int inputmode) {
    conio_ttymode = ttymode;
    conio_inputmode = inputmode;

    switch (conio_ttymode) {
        case CONIO_TTY_PVR:
#ifdef GFX
            conio_draw_init();
#endif
            break;
        case CONIO_TTY_STDIO:
            break;
        case CONIO_TTY_SERIAL:
            conio_serial_fd = 1;
            break;
        case CONIO_TTY_DBGIO:
            break;
        default:
            assert_msg( 0, "Unknown CONIO TTY mode" );
            conio_ttymode = CONIO_TTY_PVR;
            break;
    }
    assert_msg( conio_inputmode == CONIO_INPUT_LINE, "Non-Line input modes not supported yet" );

    conio_input_init();
    if (conio_ttymode == CONIO_TTY_PVR) {
        conio_clear();
        conio_gotoxy(0, 0);
    }

    sem_init(&ft_mutex, 1);

    /* create the conio thread */
    conio_exit = 0;
    if (!thd_create(1, conio_thread, 0))
        return -1;

    /* Wait for it to actually start */
    while (!conio_entered)
        thd_pass();

    return 0;
}

int conio_shutdown(void) {
    /* shutup our thread */
    conio_exit = 1;

    while (conio_exit != -1)
        ;

    /* Delete the sempahore */
    sem_destroy(&ft_mutex);

#ifdef GFX
    if (conio_ttymode == CONIO_TTY_PVR)
        conio_draw_shutdown();
#endif
    conio_input_shutdown();

    conio_ttymode = CONIO_TTY_NONE;
    conio_inputmode = CONIO_INPUT_NONE;

    return 0;
}
