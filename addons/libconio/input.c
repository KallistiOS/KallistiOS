/* KallistiOS ##version##

   input.c
   Copyright (C) 2002 Megan Potter

   Adapted from Kosh, Copyright (C) 2000 Jordan DeLong

*/

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <kos/limits.h>
#include <kos/thread.h>
#include <kos/sem.h>
#include <dc/maple/keyboard.h>
#include <conio/conio.h>

/* This module defines a conio input system, if you want to use it. */

/* the buffer for input */
static struct {
    char text[CONIO_INPUT_BUFFSIZE];
    unsigned int pos;   /* pos in the buffer, not the screen */
} input_buffer;

/* the state var */
enum {
    INPUT_PROMPT,       /* print the prompt, etc */
    INPUT_READCOMM,     /* read and edit a command line */
    INPUT_COMMAND       /* call a command based on input_buffer.text */
} input_state = INPUT_PROMPT;

/***********************************************************************/
/* Default input callback system: queue up lines of text
   and return them using a semaphore for flow control. */

typedef struct cb_sem_data_str {
    char            line[256];
    struct cb_sem_data_str  *next;
} cb_sem_data_t;
static volatile cb_sem_data_t   *cb_queue;
static semaphore_t      cb_sem, cb_mutex;
static volatile int     cb_dead;

static void input_cb_init(void) {
    sem_init(&cb_sem, 0);
    sem_init(&cb_mutex, 1);
    cb_queue = NULL;
    cb_dead = 0;
}

static void cb_default(const char *str) {
    cb_sem_data_t *t;

    t = malloc(sizeof(cb_sem_data_t));
    sem_wait(&cb_mutex);
    strncpy(t->line, str, 255); t->line[255] = '\0';
    t->next = (cb_sem_data_t *)cb_queue;
    cb_queue = t;
    sem_signal(&cb_mutex);

    sem_signal(&cb_sem);
}

static void input_cb_shutdown(void) {
    cb_sem_data_t *t, *n;
    int i;

    /* Make sure any clients waiting on getline go through */
    cb_dead = 1;
    cb_default("");

    /* This is pretty damned hacky but it should do the job */
    for (i=0; i<5; i++)
        thd_pass();

    sem_destroy(&cb_mutex);
    sem_destroy(&cb_sem);

    for (t=(cb_sem_data_t *)cb_queue; t; t = n) {
        n = t->next;
        free(t);
    }
    cb_queue = NULL;
}

int conio_input_getline(int block, char *dst, int dstcnt) {
    cb_sem_data_t *t, *l;

    /* assert_msg( block != 0, "non-blocking I/O not supported yet" ); */
    if (!block && cb_queue == NULL)
        return -1;

    /* Did we quit already? */
    if (cb_dead)
        return -1;

    /* Wait for some input to be ready */
    if (block > 0) {
        if (sem_wait_timed(&cb_sem, block) < 0)
            return -1;
    } else {
        if (sem_wait(&cb_sem) < 0)
            return -1;
    }

    /* Grab the mutex and retrieve the line */
    sem_wait(&cb_mutex);
    assert( cb_queue != NULL );
    for (l=NULL, t=(cb_sem_data_t *)cb_queue; t->next; t=t->next)
        l=t;
    assert( t->next == NULL );
    if (l != NULL) {
        assert( l->next == t );
        l->next = NULL;
    } else {
        cb_queue = NULL;
    }
    sem_signal(&cb_mutex);

    strncpy(dst, t->line, dstcnt-1);
    dst[dstcnt-1] = '\0';
    free(t);

    return 0;
}

/***********************************************************************/
/* Input callbacks */
static conio_input_callback_t input_cb = NULL;
void conio_input_callback(conio_input_callback_t cb) {
    if (cb == NULL)
        input_cb = cb_default;
    else
        input_cb = cb;
}

/***********************************************************************/
/* add text to the input buff and putch it */
static void input_insertbuff(int ch) {
    int len;

    len = strlen(input_buffer.text);

    if (len >= CONIO_INPUT_BUFFSIZE - 1)
        return;

    /* our str */
    memmove(&input_buffer.text[input_buffer.pos + 1], &input_buffer.text[input_buffer.pos],
        len - input_buffer.pos + 1);
    /* the virtscr */
    if (conio_cursor.row * CONIO_NUM_COLS + conio_cursor.col + len - input_buffer.pos + 1
            >= CONIO_NUM_COLS * CONIO_NUM_ROWS)
        conio_scroll();
    memmove(&conio_virtscr[conio_cursor.row][conio_cursor.col + 1],
        &conio_virtscr[conio_cursor.row][conio_cursor.col],
        len - input_buffer.pos + 1);
    input_buffer.text[input_buffer.pos] = ch;
    input_buffer.pos++;

    if (conio_ttymode != CONIO_TTY_STDIO)
        conio_putch(ch);

    return;
}

/* remove the char at input_buffer.pos from the buffer, and reflect the changes on the virtscr */
static void input_delchar_buff(void) {
    int len;

    len = strlen(input_buffer.text);

    if (len < 1)
        return;

    /* our str */
    memmove(&input_buffer.text[input_buffer.pos], &input_buffer.text[input_buffer.pos + 1],
        len - input_buffer.pos + 1);
    input_buffer.text[len] = '\0';
    /* the virtscr */
    memmove(&conio_virtscr[conio_cursor.row][conio_cursor.col],
        &conio_virtscr[conio_cursor.row][conio_cursor.col + 1],
        len - input_buffer.pos + 1);
}

/* print the prompt out, clear input buffer */
static void input_prompt(void) {
    input_buffer.pos = 0;
    *input_buffer.text = '\0';
    /* conio_putstr("> "); */
    input_state = INPUT_READCOMM;
}

/* reading commands from the prompt */
static void input_readcomm(void) {
    int key;

    key = conio_check_getch();

    switch (key) {
        case -1:
            return;
        case KBD_KEY_DEL << 8:
            input_delchar_buff();
            break;
        case 8:     /* backspace */
            if (input_buffer.pos > 0) {
                conio_deadvance_cursor();
                input_buffer.pos--;
                input_delchar_buff();
            }
            break;
        case '\r':
            input_state = INPUT_COMMAND;
            conio_putch('\n');
            break;
        case KBD_KEY_LEFT << 8: /* left */
            if (input_buffer.pos > 0) {
                input_buffer.pos--;
                conio_deadvance_cursor();
            }
            break;
        case KBD_KEY_RIGHT << 8:
            if (input_buffer.pos < strlen(input_buffer.text)) {
                input_buffer.pos++;
                conio_advance_cursor();
            }
            break;
        case KBD_KEY_HOME << 8:
            for (; input_buffer.pos > 0; input_buffer.pos--)
                conio_deadvance_cursor();
            break;
        case KBD_KEY_END << 8:
            for (; input_buffer.pos < strlen(input_buffer.text); input_buffer.pos++)
                conio_advance_cursor();
            break;
        default:
            if (isascii(key))
                input_insertbuff(key);
    }
}

/* read command line, try to execute builtin commands, otherwise externals (this coming soon) */
static void input_command(void) {
    input_state = INPUT_PROMPT;

    if (input_cb)
        input_cb(input_buffer.text);
}

void conio_input_init(void) {
    input_cb_init();
    input_cb = cb_default;
}

void conio_input_shutdown(void) {
    input_cb = NULL;
    input_cb_shutdown();
}

/* our exported input function, called once per frame. */
void conio_input_frame(void) {
    switch (input_state) {
        case INPUT_PROMPT:  input_prompt();     break;
        case INPUT_READCOMM:    input_readcomm();   break;
        case INPUT_COMMAND: input_command();    break;
    }
}
