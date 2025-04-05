/* KallistiOS ##version##

   keyboard.c
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2012 Lawrence Sebald
   Copyright (C) 2018 Donald Haase
   Copyright (C) 2024 Paul Cercueil
   Copyright (C) 2024, 2025 Falco Girgis
*/

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <arch/timer.h>
#include <dc/maple.h>
#include <dc/maple/keyboard.h>

/*

This module is an (almost) complete keyboard system. It handles key
debouncing and queueing so you don't miss any pressed keys as long
as you poll often enough. The only thing missing currently is key
repeat handling.

*/

/*  Size of a keyboard queue.

    Each keyboard queue will hold this many elements. Once the queue fills, no
    new elements will be placed on the queue. As long as you check the queue
    relatively frequently, the default of 16 should be plenty.

    This MUST be a power of two.
*/
#define KBD_QUEUE_SIZE 16

/* Keyboard raw condition structure.

    This structure is what the keyboard responds with as its current status.
*/
typedef struct kbd_cond {
    kbd_mods_t modifiers;    /**< \brief Bitmask of set modifiers. */
    kbd_leds_t leds;         /**< \brief Bitmask of set LEDs. */
    kbd_key_t  keys[KBD_MAX_PRESSED_KEYS];      /**< \brief Key codes for currently pressed keys. */
} kbd_cond_t;

typedef struct kbd_state_private {
    kbd_state_t base;

    /* Repeated key press state data. */
    struct {
        kbd_key_t key;    /* Last key held which will repeat. */
        uint64_t timeout; /* Time the next repeat will trigger. */
    } repeater;

    uint32_t key_queue[KBD_QUEUE_SIZE];
    size_t queue_tail;          /* Key queue tail. */
    size_t queue_head;          /* Key queue head. */
    volatile size_t queue_len;  /* Current length of queue. */
} kbd_state_private_t;

static struct {
    kbd_event_handler_t cb;
    void               *ud;
} event_handler = {
    NULL, NULL
};

void kbd_set_event_handler(kbd_event_handler_t callback, void *user_data) {
    event_handler.cb = callback;
    event_handler.ud = user_data;
}

void kbd_get_event_handler(kbd_event_handler_t *callback, void **user_data) {
    *callback = event_handler.cb;
    *user_data = event_handler.ud;
}

/* These are global timings for key repeat. It would be possible to put
    them in the state, but I don't see a reason to.
    It seems unreasonable that one might want different repeat
    timings set on each keyboard.
    The values are arbitrary based off a survey of common values. */
static struct {
    uint16_t start;
    uint16_t interval;
} repeat_timing = {
    600, 20
};

void kbd_set_repeat_timing(uint16_t start, uint16_t interval) {
    repeat_timing.start    = start;
    repeat_timing.interval = interval;
}

/** \brief   Keyboard keymap
    \ingroup kbd

    This structure represents a mapping from raw key values to ASCII values, if
    appropriate. This handles base values as well as shifted ("Shift" and "Alt"
    keys) values.

    \headerfile dc/maple/keyboard.h
*/
typedef struct kbd_keymap {
    uint8_t base[KBD_MAX_KEYS];
    uint8_t shifted[KBD_MAX_KEYS];
    uint8_t alt[KBD_MAX_KEYS];
} kbd_keymap_t;

/* Built-in keymaps. */
#define KBD_NUM_KEYMAPS (sizeof(keymaps) / sizeof(keymaps[0]))
static const kbd_keymap_t keymaps[] = {
    {
        /* Japanese keyboard */
        {
            /* Base values */
            0, 0, 0, 0, 'a', 'b', 'c', 'd',                 /* 0x00 - 0x07 */
            'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',         /* 0x08 - 0x0F */
            'm', 'n', 'o', 'p', 'q', 'r', 's', 't',         /* 0x10 - 0x17 */
            'u', 'v', 'w', 'x', 'y', 'z', '1', '2',         /* 0x18 - 0x1F */
            '3', '4', '5', '6', '7', '8', '9', '0',         /* 0x20 - 0x27 */
            10, 27, 8, 9, ' ', '-', '^', '@',               /* 0x28 - 0x2F */
            '[', 0, ']', ';', ':', 0, ',', '.',             /* 0x30 - 0x37 */
            '/', 0, 0, 0, 0, 0, 0, 0,                       /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x50 - 0x57 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x58 - 0x5F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x60 - 0x67 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x68 - 0x6F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x70 - 0x77 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x78 - 0x7F */
            0, 0, 0, 0, 0, 0, 0, '\\',                      /* 0x80 - 0x87 */
            0, 165, 0, 0                                    /* 0x88 - 0x8A */
            /* All the rest are unused, and will be 0. */
        },
        {
            /* Shifted values */
            0, 0, 0, 0, 'A', 'B', 'C', 'D',                 /* 0x00 - 0x07 */
            'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',         /* 0x08 - 0x0F */
            'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',         /* 0x10 - 0x17 */
            'U', 'V', 'W', 'X', 'Y', 'Z', '!', '"',         /* 0x18 - 0x1F */
            '#', '$', '%', '&', '\'', '(', ')', '~',        /* 0x20 - 0x27 */
            10, 27, 8, 9, ' ', '=', 175, '`',               /* 0x28 - 0x2F */
            '{', 0, '}', '+', '*', 0, '<', '>',             /* 0x30 - 0x37 */
            '?', 0, 0, 0, 0, 0, 0, 0,                       /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x50 - 0x57 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x58 - 0x5F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x60 - 0x67 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x68 - 0x6F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x70 - 0x77 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x78 - 0x7F */
            0, 0, 0, 0, 0, 0, 0, '_',                       /* 0x80 - 0x87 */
            0, '|', 0, 0                                    /* 0x88 - 0x8A */
            /* All the rest are unused, and will be 0. */
        },
        {
            /* no "Alt" shifted values */
        }
    },
    {
        /* US/QWERTY keyboard */
        {
            /* Base values */
            0, 0, 0, 0, 'a', 'b', 'c', 'd',                 /* 0x00 - 0x07 */
            'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',         /* 0x08 - 0x0F */
            'm', 'n', 'o', 'p', 'q', 'r', 's', 't',         /* 0x10 - 0x17 */
            'u', 'v', 'w', 'x', 'y', 'z', '1', '2',         /* 0x18 - 0x1F */
            '3', '4', '5', '6', '7', '8', '9', '0',         /* 0x20 - 0x27 */
            10, 27, 8, 9, ' ', '-', '=', '[',               /* 0x28 - 0x2F */
            ']', '\\', 0, ';', '\'', '`', ',', '.',         /* 0x30 - 0x37 */
            '/', 0, 0, 0, 0, 0, 0, 0,                       /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, '/', '*', '-', '+',                 /* 0x50 - 0x57 */
            13, '1', '2', '3', '4', '5', '6', '7',          /* 0x58 - 0x5F */
            '8', '9', '0', '.', 0, 0                        /* 0x60 - 0x65 */
            /* All the rest are unused, and will be 0. */
        },
        {
            /* Shifted values */
            0, 0, 0, 0, 'A', 'B', 'C', 'D',                 /* 0x00 - 0x07 */
            'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',         /* 0x08 - 0x0F */
            'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',         /* 0x10 - 0x17 */
            'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@',         /* 0x18 - 0x1F */
            '#', '$', '%', '^', '&', '*', '(', ')',         /* 0x20 - 0x27 */
            10, 27, 8, 9, ' ', '_', '+', '{',               /* 0x28 - 0x2F */
            '}', '|', 0, ':', '"', '~', '<', '>',           /* 0x30 - 0x37 */
            '?', 0, 0, 0, 0, 0, 0, 0,                       /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, '/', '*', '-', '+',                 /* 0x50 - 0x57 */
            13, '1', '2', '3', '4', '5', '6', '7',          /* 0x58 - 0x5F */
            '8', '9', '0', '.', 0, 0                        /* 0x60 - 0x65 */
            /* All the rest are unused, and will be 0. */
        },
        {
            /* no "Alt" shifted values */
        }
    },
    {
        /* UK/QWERTY keyboard */
        {
            /* Base values */
            0, 0, 0, 0, 'a', 'b', 'c', 'd',                 /* 0x00 - 0x07 */
            'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',         /* 0x08 - 0x0F */
            'm', 'n', 'o', 'p', 'q', 'r', 's', 't',         /* 0x10 - 0x17 */
            'u', 'v', 'w', 'x', 'y', 'z', '1', '2',         /* 0x18 - 0x1F */
            '3', '4', '5', '6', '7', '8', '9', '0',         /* 0x20 - 0x27 */
            10, 27, 8, 9, ' ', '-', '=', '[',               /* 0x28 - 0x2F */
            ']', '\\', '#', ';', '\'', '`', ',', '.',       /* 0x30 - 0x37 */
            '/', 0, 0, 0, 0, 0, 0, 0,                       /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, '/', '*', '-', '+',                 /* 0x50 - 0x57 */
            13, '1', '2', '3', '4', '5', '6', '7',          /* 0x58 - 0x5F */
            '8', '9', '0', '.', '\\', 0                     /* 0x60 - 0x65 */
            /* All the rest are unused, and will be 0. */
        },
        {
            /* Shifted values */
            0, 0, 0, 0, 'A', 'B', 'C', 'D',                 /* 0x00 - 0x07 */
            'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',         /* 0x08 - 0x0F */
            'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',         /* 0x10 - 0x17 */
            'U', 'V', 'W', 'X', 'Y', 'Z', '!', '"',         /* 0x18 - 0x1F */
            0xa3, '$', '%', '^', '&', '*', '(', ')',        /* 0x20 - 0x27 */
            10, 27, 8, 9, ' ', '_', '+', '{',               /* 0x28 - 0x2F */
            '}', '|', '~', ':', '@', '|', '<', '>',         /* 0x30 - 0x37 */
            '?', 0, 0, 0, 0, 0, 0, 0,                       /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, '/', '*', '-', '+',                 /* 0x50 - 0x57 */
            13, '1', '2', '3', '4', '5', '6', '7',          /* 0x58 - 0x5F */
            '8', '9', '0', '.', '|', 0                      /* 0x60 - 0x65 */
            /* All the rest are unused, and will be 0. */
        },
        {
            /* "Alt" shifted values */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x00 - 0x07 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x08 - 0x0F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x10 - 0x17 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x18 - 0x1F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x20 - 0x27 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x28 - 0x2F */
            0, 0, 0, 0, 0, '|', 0, 0,                       /* 0x30 - 0x37 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x50 - 0x57 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x58 - 0x5F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x60 - 0x65 */
            /* All the rest are unused, and will be 0. */
        }
    },
    {
        /* German/QWERTZ keyboard */
        /* The hex values in the tables are the ISO-8859-15 representation of the
           German special chars. */
        {
            /* Base values */
            0, 0, 0, 0, 'a', 'b', 'c', 'd',                 /* 0x00 - 0x07 */
            'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',         /* 0x08 - 0x0F */
            'm', 'n', 'o', 'p', 'q', 'r', 's', 't',         /* 0x10 - 0x17 */
            'u', 'v', 'w', 'x', 'z', 'y', '1', '2',         /* 0x18 - 0x1F */
            '3', '4', '5', '6', '7', '8', '9', '0',         /* 0x20 - 0x27 */
            10, 27, 8, 9, ' ', 0xdf, '\'', 0xfc,            /* 0x28 - 0x2F */
            '+', '\\', '#', 0xf6, 0xe4, '^', ',', '.',      /* 0x30 - 0x37 */
            '-', 0, 0, 0, 0, 0, 0, 0,                       /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, '/', '*', '-', '+',                 /* 0x50 - 0x57 */
            13, '1', '2', '3', '4', '5', '6', '7',          /* 0x58 - 0x5F */
            '8', '9', '0', '.', '<', 0                      /* 0x60 - 0x65 */
            /* All the rest are unused, and will be 0. */
        },
        {
            /* Shifted values */
            0, 0, 0, 0, 'A', 'B', 'C', 'D',                 /* 0x00 - 0x07 */
            'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',         /* 0x08 - 0x0F */
            'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',         /* 0x10 - 0x17 */
            'U', 'V', 'W', 'X', 'Z', 'Y', '!', '"',         /* 0x18 - 0x1F */
            0xa7, '$', '%', '&', '/', '(', ')', '=',        /* 0x20 - 0x27 */
            10, 27, 8, 9, ' ', '?', '`', 0xdc,              /* 0x28 - 0x2F */
            '*', '|', '\'', 0xd6, 0xc4, 0xb0, ';', ':',     /* 0x30 - 0x37 */
            '_', 0, 0, 0, 0, 0, 0, 0,                       /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, '/', '*', '-', '+',                 /* 0x50 - 0x57 */
            13, '1', '2', '3', '4', '5', '6', '7',          /* 0x58 - 0x5F */
            '8', '9', '0', '.', '>', 0                      /* 0x60 - 0x65 */
            /* All the rest are unused, and will be 0. */
        },
        {
            /* "Alt" shifted values */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x00 - 0x07 */
            0xa4, 0, 0, 0, 0, 0, 0, 0,                      /* 0x08 - 0x0F */
            0xb5, 0, 0, 0, 0, 0, 0, 0,                      /* 0x10 - 0x17 */
            0, 0, 0, 0, 0, 0, 0, 0xb2,                      /* 0x18 - 0x1F */
            0xb3, 0, 0, 0, '{', '[', ']', '}',              /* 0x20 - 0x27 */
            0, 0, 0, 0, 0, '\\', 0, 0,                      /* 0x28 - 0x2F */
            '~', 0, 0, 0, 0, 0, 0, 0,                       /* 0x30 - 0x37 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x50 - 0x57 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x58 - 0x5F */
            0, 0, 0, 0, '|', 0, 0, 0,                       /* 0x60 - 0x65 */
            /* All the rest are unused, and will be 0. */
        }
    },
    {
        /* French/AZERTY keyboard */
        /* The hex values in the tables are the ISO-8859-15 representation of the
           French special chars. */
        {
            /* Base values */
            0, 0, 0, 0, 'q', 'b', 'c', 'd',                 /* 0x00 - 0x07 */
            'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',         /* 0x08 - 0x0F */
            ',', 'n', 'o', 'p', 'a', 'r', 's', 't',         /* 0x10 - 0x17 */
            'u', 'v', 'z', 'x', 'y', 'w', '&', 0xe9,        /* 0x18 - 0x1F */
            '\"', '\'', '(', '-', 0xe8, '_', 0xe7, 0xe0,    /* 0x20 - 0x27 */
            10, 27, 8, 9, ' ', ')', '=', '^',               /* 0x28 - 0x2F */
            '$', 0, '*', 'm', 0xf9, 0xb2, ';', ':',         /* 0x30 - 0x37 */
            '!', 0, 0, 0, 0, 0, 0, 0,                       /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, '/', '*', '-', '+',                 /* 0x50 - 0x57 */
            13, '1', '2', '3', '4', '5', '6', '7',          /* 0x58 - 0x5F */
            '8', '9', '0', '.', 0, 0                        /* 0x60 - 0x65 */
            /* All the rest are unused, and will be 0. */
        },
        {
            /* Shifted values */
            0, 0, 0, 0, 'Q', 'B', 'C', 'D',                 /* 0x00 - 0x07 */
            'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',         /* 0x08 - 0x0F */
            '?', 'N', 'O', 'P', 'A', 'R', 'S', 'T',         /* 0x10 - 0x17 */
            'U', 'V', 'Z', 'X', 'Y', 'W', '1', '2',         /* 0x18 - 0x1F */
            '3', '4', '5', '6', '7', '8', '9', '0',         /* 0x20 - 0x27 */
            10, 27, 8, 9, ' ', 0xba, '+', 0,                /* 0x28 - 0x2F */
            0xa3, 0, 0xb5, 'M', '%', 0xb3, '.', '/',        /* 0x30 - 0x37 */
            0x7a, 0, 0, 0, 0, 0, 0, 0,                      /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, '/', '*', '-', '+',                 /* 0x50 - 0x57 */
            13, '1', '2', '3', '4', '5', '6', '7',          /* 0x58 - 0x5F */
            '8', '9', '0', '.', 0, 0                        /* 0x60 - 0x65 */
            /* All the rest are unused, and will be 0. */
        },
        {
            /* "Alt" shifted values */
            0, 0, 0, 0, 0xe4, 0, 0xa9, 0,                   /* 0x00 - 0x07 */
            0xa4, 0, 0, 0, 0xee, 0xfc, 0xef, 0,             /* 0x08 - 0x0F */
            0xbf, 0xf1, 0xbd, 0xf4, 0xe6, 0xea, 0xdf, 0,    /* 0x10 - 0x17 */
            0xfb, 0, 0xe2, 0xbb, 0xfc, 0xab, 0, 0,          /* 0x18 - 0x1F */
            '#', '{', '[', '|', 0, '\\', '^', '@',          /* 0x20 - 0x27 */
            10, 27, 8, 9, ' ', ']', '}', '~',               /* 0x28 - 0x2F */
            0, 0, 0, 0xf6, 0, 0xb9, 0xd7, 0xf7,             /* 0x30 - 0x37 */
            0xa1, 0, 0, 0, 0, 0, 0, 0,                      /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, '/', '*', '-', '+',                 /* 0x50 - 0x57 */
            13, '1', '2', '3', '4', '5', '6', '7',          /* 0x58 - 0x5F */
            '8', '9', '0', '.', 0, 0                        /* 0x60 - 0x65 */
            /* All the rest are unused, and will be 0. */
        }
    },
    {
        /* Italian/QWERTY keyboard, probably. This one needs to be confirmed
           still. */
        { },
        { },
        { }
    },
    {
        /* ES (Spanish QWERTY) keyboard */
        /* The hex values in the tables are the ISO-8859-15 (Euro revision)
           representation of the Spanish special chars. */
        {
            /* Base values */
            /* 0xa1: '¡', 0xba: 'º', 0xb4: '´', 0xe7: 'ç',
               0xf1: 'ñ' */
            0, 0, 0, 0, 'a', 'b', 'c', 'd',                 /* 0x00 - 0x07 */
            'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',         /* 0x08 - 0x0F */
            'm', 'n', 'o', 'p', 'q', 'r', 's', 't',         /* 0x10 - 0x17 */
            'u', 'v', 'w', 'x', 'y', 'z', '1', '2',         /* 0x18 - 0x1F */
            '3', '4', '5', '6', '7', '8', '9', '0',         /* 0x20 - 0x27 */
            10, 27, 8, 9, ' ', '\'', 0xa1, '`',             /* 0x28 - 0x2F */
            '+', 0, 0xe7, 0xf1, 0xb4, 0xba, ',', '.',       /* 0x30 - 0x37 */
            '-', 0, 0, 0, 0, 0, 0, 0,                       /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, '/', '*', '-', '+',                 /* 0x50 - 0x57 */
            13, '1', '2', '3', '4', '5', '6', '7',          /* 0x58 - 0x5F */
            '8', '9', '0', '.', '<', 0, 0, 0,               /* 0x60 - 0x65 */
            /* All the rest are unused, and will be 0. */
        },
        {
             /* Shifted values */
             /* 0xaa: 'ª', 0xb7: '·', 0xbf: '¿', 0xc7: 'Ç',
                0xd1: 'Ñ', 0xa8: '¨' */
            0, 0, 0, 0, 'A', 'B', 'C', 'D',                 /* 0x00 - 0x07 */
            'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',         /* 0x08 - 0x0F */
            'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',         /* 0x10 - 0x17 */
            'U', 'V', 'W', 'X', 'Y', 'Z', '!', '"',         /* 0x18 - 0x1F */
            0xb7, '$', '%', '&', '/', '(', ')', '=',        /* 0x20 - 0x27 */
            10, 27, 8, 9, ' ', '?', 0xbf, '^',              /* 0x28 - 0x2F */
            '*', 0, 0xc7, 0xd1, 0xa8, 0xaa, ';', ':',       /* 0x30 - 0x37 */
            '_', 0, 0, 0, 0, 0, 0, 0,                       /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, '/', '*', '-', '+',                 /* 0x50 - 0x57 */
            13, '1', '2', '3', '4', '5', '6', '7',          /* 0x58 - 0x5F */
            '8', '9', '0', '.', '>', 0, 0, 0,               /* 0x60 - 0x65 */
            /* All the rest are unused, and will be 0. */
        },
        {
            /* "Alt" shifted values */
            /* 0xa4: '€', 0xac: '¬' */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x00 - 0x07 */
            0xa4, 0, 0, 0, 0, 0, 0, 0,                      /* 0x08 - 0x0F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x10 - 0x17 */
            0, 0, 0, 0, 0, 0, '|', '@',                     /* 0x18 - 0x1F */
            '#', 0, 0, 0xac, 0, 0, 0, 0,                    /* 0x20 - 0x27 */
            0, 0, 0, 0, 0, 0, 0, '[',                       /* 0x28 - 0x2F */
            ']', 0, '}', 0, '{', '\\', 0, 0,                /* 0x30 - 0x37 */
            '-', 0, 0, 0, 0, 0, 0, 0,                       /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x50 - 0x57 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x58 - 0x5F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x60 - 0x65 */
            /* All the rest are unused, and will be 0. */
        }

    }
};

/* The keyboard queue (global for now) */
static volatile int kbd_queue_active = 1;
static volatile int kbd_queue_tail = 0, kbd_queue_head = 0;
static volatile uint16  kbd_queue[KBD_QUEUE_SIZE];

/* Turn keyboard queueing on or off. This is mainly useful if you want
   to use the keys for a game where individual keypresses don't mean
   as much as having the keys up or down. Setting this state to
   a new value will clear the queue. */
void kbd_set_queue(int active) {
    if(kbd_queue_active != active) {
        kbd_queue_head = kbd_queue_tail = 0;
    }

    kbd_queue_active = active;
}

/* Take a key scancode, encode it appropriately, and place it on the
   keyboard queue. At the moment we assume no key overflows. */
static int kbd_enqueue(kbd_state_t *state, uint8_t keycode, uint32_t mods) {
    static const char keymap_noshift[] = {
        /*0*/   0, 0, 0, 0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
        'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z',
        /*1e*/  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
        /*28*/  13, 27, 8, 9, 32, '-', '=', '[', ']', '\\', 0, ';', '\'',
        /*35*/  '`', ',', '.', '/', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*46*/  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*53*/  0, '/', '*', '-', '+', 13, '1', '2', '3', '4', '5', '6',
        /*5f*/  '7', '8', '9', '0', '.', 0
    };
    static const char keymap_shift[] = {
        /*0*/   0, 0, 0, 0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
        'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
        /*1e*/  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
        /*28*/  13, 27, 8, 9, 32, '_', '+', '{', '}', '|', 0, ':', '"',
        /*35*/  '~', '<', '>', '?', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*46*/  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*53*/  0, '/', '*', '-', '+', 13, '1', '2', '3', '4', '5', '6',
        /*5f*/  '7', '8', '9', '0', '.', 0
    };
    uint16_t ascii;

    kbd_state_private_t *state_private = (kbd_state_private_t *)state;

    /* Don't bother with bad keycodes. */
    if(keycode <= 1)
        return 0;

    /* Queue the key up on the device-specific queue. */
    if(state_private->queue_len < KBD_QUEUE_SIZE) {
        state_private->key_queue[state_private->queue_head] = keycode | (mods << 8);
        state_private->queue_head = (state_private->queue_head + 1) & (KBD_QUEUE_SIZE - 1);
        ++state_private->queue_len;
    }

    /* If queueing is turned off, don't bother with the global queue. */
    if(!kbd_queue_active)
        return 0;

    /* Figure out its key queue value */
    if(keycode <= 0x64) {
        if(state->modifiers.raw & KBD_MOD_SHIFT)
            ascii = keymap_shift[keycode];
        else
            ascii = keymap_noshift[keycode];
    }

    if(ascii == 0)
        ascii = ((uint16)keycode) << 8;

    /* Ok... now do the enqueue to the global queue */
    kbd_queue[kbd_queue_head] = ascii;
    kbd_queue_head = (kbd_queue_head + 1) & (KBD_QUEUE_SIZE - 1);

    return 0;
}

/* Take a key off the key queue, or return -1 if there is none waiting */
int kbd_get_key(void) {
    int rv;

    /* If queueing isn't active, there won't be anything to get */
    if(!kbd_queue_active)
        return -1;

    /* Check available */
    if(kbd_queue_head == kbd_queue_tail)
        return -1;

    rv = kbd_queue[kbd_queue_tail];
    kbd_queue_tail = (kbd_queue_tail + 1) & (KBD_QUEUE_SIZE - 1);

    return rv;
}

kbd_state_t *kbd_get_state(maple_device_t *device) {
    if(!device)
        return NULL;

    if(!device->status_valid)
        return NULL;

    if(!(device->info.functions & MAPLE_FUNC_KEYBOARD))
        return NULL;

    return (kbd_state_t *)device->status;
}


char kbd_key_to_ascii(kbd_key_t key, kbd_region_t region, kbd_mods_t mods, kbd_leds_t leds) {
    char ascii = '\0';

    if(mods.ralt || (mods.lctrl && mods.lalt))
        ascii = keymaps[region - 1].alt[key];
    else if((mods.raw & KBD_MOD_SHIFT) || leds.caps_lock)
        ascii = keymaps[region - 1].shifted[key];
    else
        ascii = keymaps[region - 1].base[key];

    return ascii;
}

/* Take a key off of a specific key queue. */
int kbd_queue_pop(maple_device_t *dev, bool to_ascii) {
    kbd_state_private_t *state_private = (kbd_state_private_t *)dev->status;
    uint32_t rv;
    kbd_mods_t mods;
    kbd_leds_t leds;
    char ascii;

    const int irqs = irq_disable();

    if(!state_private->queue_len) {
        irq_restore(irqs);
        return -1;
    }

    rv = state_private->key_queue[state_private->queue_tail];
    state_private->queue_tail = (state_private->queue_tail + 1) & (KBD_QUEUE_SIZE - 1);
    --state_private->queue_len;

    irq_restore(irqs);

    if(!to_ascii)
        return (int)rv;

    mods.raw = FIELD_GET(rv, 0xff00);
    leds.raw = FIELD_GET(rv, 0xff0000);
    rv &= 0xff;

    if((ascii = kbd_key_to_ascii(rv, state_private->base.region, mods, leds))) {
        return ascii;
    } else {
        return (int)(rv << 8);
    }
}

/* Update the keyboard status; this will handle debounce handling as well as
   queueing keypresses for later usage. The key press queue uses 16-bit
   words so that we can store "special" keys as such. */
static void kbd_check_poll(maple_frame_t *frm, kbd_cond_t *cond) {
    kbd_state_t *state = (kbd_state_t *)frm->dev->status;
    kbd_state_private_t *state_private = (kbd_state_private_t *)state;

    /* If the modifier keys have changed, end the key repeating. */
    if(state->modifiers.raw != cond->modifiers.raw) {
        state_private->repeater.key = KBD_KEY_NONE;
        state_private->repeater.timeout = 0;
    }

    /* Update modifiers and LEDs */
    state->modifiers = cond->modifiers;
    state->leds = cond->leds;

    const uint32_t mods = cond->modifiers.raw | (cond->leds.raw << 8);

    /* Update all key states */
    for(unsigned int k = 0; k < KBD_MAX_KEYS; ++k) {
        state->key_states[k].raw = (state->key_states[k].raw << 1) & KEY_STATE_MASK;
    }

    /* Process all pressed keys */
    for(unsigned int p = 0; p < KBD_MAX_PRESSED_KEYS; ++p) {

        /* Once we get to a 'none', the rest will be 'none' */
        if(cond->keys[p] == KBD_KEY_NONE) {
            /* This could be used to indicate how many keys are pressed by setting it to ~i or i+1
                or similar. This could be useful, but would make it a weird exception. */
            /* If the first key in the key array is none, there are no non-modifer keys pressed at all. */
            if(!p)
                state->key_states[KBD_KEY_NONE].is_down = true;
            break;
        }
        /* Between None and A are error indicators. This would be a good place to do... something. If an error occurs the whole array will be error.*/
        else if(cond->keys[p] > KBD_KEY_NONE && cond->keys[p] < KBD_KEY_A) {
            state->key_states[cond->keys[p]].is_down = true;
            break;
        }
        /* The rest of the keys are treated normally */
        else {
            state->key_states[cond->keys[p]].is_down = true;
            state_private->repeater.key = cond->keys[p];
        }
    }

    for(unsigned k = KBD_KEY_A; k < KBD_MAX_KEYS; ++k) {
        switch(state->key_states[k].value) {
            case KEY_STATE_CHANGED_DOWN:
                kbd_enqueue(state, k, mods);

                if(k == state_private->repeater.key && repeat_timing.start) {
                    state_private->repeater.key = k;
                    state_private->repeater.timeout = timer_ms_gettime64() + repeat_timing.start;
                }

                if(event_handler.cb)
                    event_handler.cb(frm->dev, k, state->key_states[k],
                                     cond->modifiers, cond->leds, event_handler.ud);
                break;

            case KEY_STATE_HELD_DOWN:
                if(k == state_private->repeater.key && repeat_timing.start) {
                    const uint64_t time = timer_ms_gettime64();
                    /* We have passed the prescribed amount of time, and will repeat the key */
                    if(time >= state_private->repeater.timeout) {
                        kbd_enqueue(state, k, mods);
                        state_private->repeater.timeout = time + repeat_timing.interval;
                    }
                }
                break;

            case KEY_STATE_CHANGED_UP:
                if(event_handler.cb)
                    event_handler.cb(frm->dev, k, state->key_states[k],
                                     cond->modifiers, cond->leds, event_handler.ud);
                break;

            case KEY_STATE_HELD_UP:
                break;

            default:
                assert_msg(0, "invalid key keys array detected");
                break;
        }
    }
}

static void kbd_reply(maple_state_t *st, maple_frame_t *frm) {
    (void)st;

    maple_response_t *resp;
    uint32 *respbuf;

    /* Unlock the frame (it's ok, we're in an IRQ) */
    maple_frame_unlock(frm);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frm->recv_buf;

    if(resp->response != MAPLE_RESPONSE_DATATRF)
        return;

    respbuf = (uint32 *)resp->data;

    if(respbuf[0] != MAPLE_FUNC_KEYBOARD)
        return;

    if(!frm->dev)
        return;

    /* Verify the size of the frame */
    assert(sizeof(kbd_cond_t) == ((resp->data_len - 1) * sizeof(uint32_t)));

    /* Update the status area from the response */
    frm->dev->status_valid = 1;
    kbd_check_poll(frm, (kbd_cond_t *)(respbuf + 1));
}

static int kbd_poll_intern(maple_device_t *dev) {
    uint32 * send_buf;

    if(maple_frame_lock(&dev->frame) < 0)
        return 0;

    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_KEYBOARD;
    dev->frame.cmd = MAPLE_COMMAND_GETCOND;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 1;
    dev->frame.callback = kbd_reply;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    return 0;
}

static void kbd_periodic(maple_driver_t *drv) {
    maple_driver_foreach(drv, kbd_poll_intern);
}

static int kbd_attach(maple_driver_t *drv, maple_device_t *dev) {
    kbd_state_t *state = (kbd_state_t *)dev->status;
    kbd_state_private_t *state_private = (kbd_state_private_t *)state;

    int d = 0;

    (void)drv;
    /* Maple functions are enumerated, from MSB, to determine which functions
       are on each device. The only one above the keyboard function is lightgun.
       Only if it is ALSO a lightgun, will the keyboard function be second. */
    if(dev->info.functions & MAPLE_FUNC_LIGHTGUN)
        d = 1;

    /* Retrieve the region data */
    state->region = dev->info.function_data[d] & 0xFF;

    /* Unrecognized keyboards will appear as US keyboards... */
    if(!state->region || state->region > KBD_NUM_KEYMAPS) {
        fprintf(stderr, "Unknown Keyboard region: %u\n", state->region);
        state->region = KBD_REGION_US;
    }

    /* Make sure all the queue variables are set up properly... */
    state_private->queue_tail = state_private->queue_head = state_private->queue_len = 0;

    /* Make sure all the key repeat variables are set up properly too */
    state_private->repeater.key = KBD_KEY_NONE;
    state_private->repeater.timeout = 0;

    return 0;
}

/* Device driver struct */
static maple_driver_t kbd_drv = {
    .functions =  MAPLE_FUNC_KEYBOARD,
    .name = "Keyboard Driver",
    .periodic = kbd_periodic,
    .status_size = sizeof(kbd_state_t),
    .attach = kbd_attach,
    .detach = NULL
};

/* Add the keyboard to the driver chain */
void kbd_init(void) {
    maple_driver_reg(&kbd_drv);
}

void kbd_shutdown(void) {
    maple_driver_unreg(&kbd_drv);
}
