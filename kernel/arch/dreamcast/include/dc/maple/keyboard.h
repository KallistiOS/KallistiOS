/* KallistiOS ##version##

   dc/maple/keyboard.h
   Copyright (C) 2000-2002 Jordan DeLong and Megan Potter
   Copyright (C) 2012 Lawrence Sebald
   Copyright (C) 2025 Falco Girgis

*/

/** \file    dc/maple/keyboard.h
    \brief   Definitions for using the keyboard device.
    \ingroup kbd

    This file contains the definitions needed to access the Maple keyboard
    device. Obviously, this corresponds to the MAPLE_FUNC_KEYBOARD function
    code.

    \author Jordan DeLong
    \author Megan Potter
    \author Lawrence Sebald
    \author Falco Girgis
*/

#ifndef __DC_MAPLE_KEYBOARD_H
#define __DC_MAPLE_KEYBOARD_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <dc/maple.h>
#include <kos/regfield.h>

#include <stdbool.h>
#include <stdint.h>

/** \defgroup kbd   Keyboard
    \brief          Driver for the Dreamcast's Keyboard Input Device
    \ingroup        peripherals
*/

/** \defgroup kbd_status_grp    Device Status
    \brief                      Types relating to overall keyboard state

    Types and API functions revolving around individual constituents of
    the overall keyboard state. These values can either be retrieved manually
    with \ref kbd_polling.

    @{
*/

/** \defgroup   kbd_mods_grp    Modifier Keys
    \brief                      Types associated with keyboard modifier keys
    \ingroup                    kbd_status_grp

    Modifier keys are represented by the kbd_mods_t union type. Each key state
    can be accessed by:
        1. Directly using a convenience bit field.
        2. Bitwise `AND` of kbd_mods_t::raw with one of the \ref kbd_mods_flags.

    @{
*/

/** \defgroup   kbd_mods_flags  Flags
    \brief                      Keyboard modifier key flags

    These are the various modifiers that can be pressed on the keyboard. Their
    current state is stored within kbd_cond_t::modifiers.

    \sa kbd_mods_t::raw

    @{
*/
/* Single-Key Modifiers */
#define KBD_MOD_LCTRL       BIT(0)    /**< \brief Left Control key */
#define KBD_MOD_LSHIFT      BIT(1)    /**< \brief Left Shift key */
#define KBD_MOD_LALT        BIT(2)    /**< \brief Left alternate key */
#define KBD_MOD_S1          BIT(3)    /**< \brief S1 key */
#define KBD_MOD_RCTRL       BIT(4)    /**< \brief Right Control key */
#define KBD_MOD_RSHIFT      BIT(5)    /**< \brief Right Shift key */
#define KBD_MOD_RALT        BIT(6)    /**< \brief Right Alternate key */
#define KBD_MOD_S2          BIT(7)    /**< \brief S2 key */

/* Multi-Key Modifiers */
/** \brief Either Control key */
#define KBD_MOD_CTRL        (KBD_MOD_LCTRL | KBD_MOD_RCTRL)
/** \brief Either Shift key */
#define KBD_MOD_SHIFT       (KBD_MOD_LSHIFT | KBD_MOD_RSHIFT)
/** \brief Either Alternate key */
#define KBD_MOD_ALT         (KBD_MOD_LALT | KBD_MOD_RALT)
/** @} */

/** \brief Modifier Keys

    Convenience union containing the state of all keyboard modifier keys.

    \sa kbd_mods_flags, kbd_state_t::modifiers
*/
typedef union kbd_mods {
    /** \brief Convenience Bitfields */
    struct {
        uint8_t lctrl   : 1;    /**< \brief Left Control key */
        uint8_t lshift  : 1;    /**< \brief Left Shift key */
        uint8_t lalt    : 1;    /**< \brief Left Alternate key */
        uint8_t s1      : 1;    /**< \brief S1 key */
        uint8_t rctrl   : 1;    /**< \brief Right Control key */
        uint8_t rshift  : 1;    /**< \brief Right Shift key */
        uint8_t ralt    : 1;    /**< \brief Right Alternate key */
        uint8_t s2      : 1;    /**< \brief S2 key */
    };
    uint8_t raw;    /**< \brief Packed 8-bit unsigned integer of bitflags */
} kbd_mods_t;

/** @} */

/** \defgroup   kbd_leds_grp    LEDs
    \brief                      Types associated with keyboard LEDs
    \ingroup                    kbd

    LEDs are represented by the kbd_leds_t union type. Each individual LED
    can be accessed by:
        1. Directly using a convenience bit field.
        2. Bitwise `AND` of kbd_leds_t::raw with one of the \ref kbd_led_flags.

    @{
*/

/** \defgroup   kbd_led_flags   Flags
    \brief                      Keyboard LED flags

    These are the LEDs that can be turned on and off on the keyboard. This list
    may not be exhaustive. Think of these sort of like an extension of the
    modifiers list.

    \sa kbd_leds_t::raw

    @{
*/
#define KBD_LED_NUMLOCK     BIT(0)    /**< \brief Num Lock LED */
#define KBD_LED_CAPSLOCK    BIT(1)    /**< \brief Caps Lock LED */
#define KBD_LED_SCRLOCK     BIT(2)    /**< \brief Scroll Lock LED */
#define KBD_LED_UNKNOWN1    BIT(3)    /**< \brief Unknown LED 1 */
#define KBD_LED_UNKNOWN2    BIT(4)    /**< \brief Unknown LED 2 */
#define KBD_LED_KANA        BIT(5)    /**< \brief Kana LED */
#define KBD_LED_POWER       BIT(6)    /**< \brief Power LED */
#define KBD_LED_SHIFT       BIT(7)    /**< \brief Shift LED */
/** @} */

/** \brief Keyboard LEDs

    Union containing the state of all keyboard LEDs.

    \sa kbd_led_flags, kbd_cond_t::leds
*/
typedef union kbd_leds {
    /** \brief Convenience Bitfields */
    struct {
        uint8_t num_lock    : 1;    /**< \brief Num Lock LED */
        uint8_t caps_lock   : 1;    /**< \brief Caps Lock LED */
        uint8_t scroll_lock : 1;    /**< \brief Scroll Lock LED */
        uint8_t unknown1    : 1;    /**< \brief Unknown LED 1 */
        uint8_t unknown2    : 1;    /**< \brief Unknown LED 2 */
        uint8_t kana        : 1;    /**< \brief Kana LED */
        uint8_t power       : 1;    /**< \brief Power LED */
        uint8_t shift       : 1;    /**< \brief Shift LED */
    };
    uint8_t raw;    /**< \brief Packed 8-bit unsigned integer of bitflags */
} kbd_leds_t;

/** @} */

/** \brief Raw Keyboard Key Identifiers
    \ingroup                kbd

    This is the list of keys that are on the keyboard that may be pressed. The
    keyboard returns keys in this format.

    \note
    These are the raw keycodes returned by the US keyboard, and thus only cover
    the keys on US keyboards (even though they can be used with other keyboards).
*/
typedef enum __packed kbd_key {
    KBD_KEY_NONE         = 0x00, /**< \brief No key */
    KBD_KEY_ERROR        = 0x01, /**< \brief ERROR_ROLLOVER */
    KBD_KEY_ERR2         = 0x02, /**< \brief Unknown error */
    KBD_KEY_ERR3         = 0x03, /**< \brief Unknown error */
    KBD_KEY_A            = 0x04, /**< \brief A key */
    KBD_KEY_B            = 0x05, /**< \brief B key */
    KBD_KEY_C            = 0x06, /**< \brief C key */
    KBD_KEY_D            = 0x07, /**< \brief D key */
    KBD_KEY_E            = 0x08, /**< \brief E key */
    KBD_KEY_F            = 0x09, /**< \brief F key */
    KBD_KEY_G            = 0x0a, /**< \brief G key */
    KBD_KEY_H            = 0x0b, /**< \brief H key */
    KBD_KEY_I            = 0x0c, /**< \brief I key */
    KBD_KEY_J            = 0x0d, /**< \brief J key */
    KBD_KEY_K            = 0x0e, /**< \brief K key */
    KBD_KEY_L            = 0x0f, /**< \brief L key */
    KBD_KEY_M            = 0x10, /**< \brief M key */
    KBD_KEY_N            = 0x11, /**< \brief N key */
    KBD_KEY_O            = 0x12, /**< \brief O key */
    KBD_KEY_P            = 0x13, /**< \brief P key */
    KBD_KEY_Q            = 0x14, /**< \brief Q key */
    KBD_KEY_R            = 0x15, /**< \brief R key */
    KBD_KEY_S            = 0x16, /**< \brief S key */
    KBD_KEY_T            = 0x17, /**< \brief T key */
    KBD_KEY_U            = 0x18, /**< \brief U key */
    KBD_KEY_V            = 0x19, /**< \brief V key */
    KBD_KEY_W            = 0x1a, /**< \brief W key */
    KBD_KEY_X            = 0x1b, /**< \brief X key */
    KBD_KEY_Y            = 0x1c, /**< \brief Y key */
    KBD_KEY_Z            = 0x1d, /**< \brief Z key */
    KBD_KEY_1            = 0x1e, /**< \brief 1 key */
    KBD_KEY_2            = 0x1f, /**< \brief 2 key */
    KBD_KEY_3            = 0x20, /**< \brief 3 key */
    KBD_KEY_4            = 0x21, /**< \brief 4 key */
    KBD_KEY_5            = 0x22, /**< \brief 5 key */
    KBD_KEY_6            = 0x23, /**< \brief 6 key */
    KBD_KEY_7            = 0x24, /**< \brief 7 key */
    KBD_KEY_8            = 0x25, /**< \brief 8 key */
    KBD_KEY_9            = 0x26, /**< \brief 9 key */
    KBD_KEY_0            = 0x27, /**< \brief 0 key */
    KBD_KEY_ENTER        = 0x28, /**< \brief Enter key */
    KBD_KEY_ESCAPE       = 0x29, /**< \brief Escape key */
    KBD_KEY_BACKSPACE    = 0x2a, /**< \brief Backspace key */
    KBD_KEY_TAB          = 0x2b, /**< \brief Tab key */
    KBD_KEY_SPACE        = 0x2c, /**< \brief Space key */
    KBD_KEY_MINUS        = 0x2d, /**< \brief Minus key */
    KBD_KEY_PLUS         = 0x2e, /**< \brief Plus key */
    KBD_KEY_LBRACKET     = 0x2f, /**< \brief [ key */
    KBD_KEY_RBRACKET     = 0x30, /**< \brief ] key */
    KBD_KEY_BACKSLASH    = 0x31, /**< \brief \ key */
    KBD_KEY_SEMICOLON    = 0x33, /**< \brief ; key */
    KBD_KEY_QUOTE        = 0x34, /**< \brief " key */
    KBD_KEY_TILDE        = 0x35, /**< \brief ~ key */
    KBD_KEY_COMMA        = 0x36, /**< \brief , key */
    KBD_KEY_PERIOD       = 0x37, /**< \brief . key */
    KBD_KEY_SLASH        = 0x38, /**< \brief Slash key */
    KBD_KEY_CAPSLOCK     = 0x39, /**< \brief Caps Lock key */
    KBD_KEY_F1           = 0x3a, /**< \brief F1 key */
    KBD_KEY_F2           = 0x3b, /**< \brief F2 key */
    KBD_KEY_F3           = 0x3c, /**< \brief F3 key */
    KBD_KEY_F4           = 0x3d, /**< \brief F4 key */
    KBD_KEY_F5           = 0x3e, /**< \brief F5 key */
    KBD_KEY_F6           = 0x3f, /**< \brief F6 key */
    KBD_KEY_F7           = 0x40, /**< \brief F7 key */
    KBD_KEY_F8           = 0x41, /**< \brief F8 key */
    KBD_KEY_F9           = 0x42, /**< \brief F9 key */
    KBD_KEY_F10          = 0x43, /**< \brief F10 key */
    KBD_KEY_F11          = 0x44, /**< \brief F11 key */
    KBD_KEY_F12          = 0x45, /**< \brief F12 key */
    KBD_KEY_PRINT        = 0x46, /**< \brief Print Screen key */
    KBD_KEY_SCRLOCK      = 0x47, /**< \brief Scroll Lock key */
    KBD_KEY_PAUSE        = 0x48, /**< \brief Pause key */
    KBD_KEY_INSERT       = 0x49, /**< \brief Insert key */
    KBD_KEY_HOME         = 0x4a, /**< \brief Home key */
    KBD_KEY_PGUP         = 0x4b, /**< \brief Page Up key */
    KBD_KEY_DEL          = 0x4c, /**< \brief Delete key */
    KBD_KEY_END          = 0x4d, /**< \brief End key */
    KBD_KEY_PGDOWN       = 0x4e, /**< \brief Page Down key */
    KBD_KEY_RIGHT        = 0x4f, /**< \brief Right Arrow key */
    KBD_KEY_LEFT         = 0x50, /**< \brief Left Arrow key */
    KBD_KEY_DOWN         = 0x51, /**< \brief Down Arrow key */
    KBD_KEY_UP           = 0x52, /**< \brief Up Arrow key */
    KBD_KEY_PAD_NUMLOCK  = 0x53, /**< \brief Keypad Numlock key */
    KBD_KEY_PAD_DIVIDE   = 0x54, /**< \brief Keypad Divide key */
    KBD_KEY_PAD_MULTIPLY = 0x55, /**< \brief Keypad Multiply key */
    KBD_KEY_PAD_MINUS    = 0x56, /**< \brief Keypad Minus key */
    KBD_KEY_PAD_PLUS     = 0x57, /**< \brief Keypad Plus key */
    KBD_KEY_PAD_ENTER    = 0x58, /**< \brief Keypad Enter key */
    KBD_KEY_PAD_1        = 0x59, /**< \brief Keypad 1 key */
    KBD_KEY_PAD_2        = 0x5a, /**< \brief Keypad 2 key */
    KBD_KEY_PAD_3        = 0x5b, /**< \brief Keypad 3 key */
    KBD_KEY_PAD_4        = 0x5c, /**< \brief Keypad 4 key */
    KBD_KEY_PAD_5        = 0x5d, /**< \brief Keypad 5 key */
    KBD_KEY_PAD_6        = 0x5e, /**< \brief Keypad 6 key */
    KBD_KEY_PAD_7        = 0x5f, /**< \brief Keypad 7 key */
    KBD_KEY_PAD_8        = 0x60, /**< \brief Keypad 8 key */
    KBD_KEY_PAD_9        = 0x61, /**< \brief Keypad 9 key */
    KBD_KEY_PAD_0        = 0x62, /**< \brief Keypad 0 key */
    KBD_KEY_PAD_PERIOD   = 0x63, /**< \brief Keypad Period key */
    KBD_KEY_S3           = 0x65  /**< \brief S3 key */
} kbd_key_t;

/** \brief      Region Codes for the Dreamcast keyboard
    \ingroup    kbd

    This is the list of possible values for kbd_state_t::region.
*/
typedef enum kbd_region {
    KBD_REGION_JP = 1, /**< \brief Japanese keyboard */
    KBD_REGION_US = 2, /**< \brief US keyboard */
    KBD_REGION_UK = 3, /**< \brief UK keyboard */
    KBD_REGION_DE = 4, /**< \brief German keyboard */
    KBD_REGION_FR = 5, /**< \brief French keyboard */
    KBD_REGION_IT = 6, /**< \brief Italian keyboard (not supported yet) */
    KBD_REGION_ES = 7  /**< \brief Spanish keyboard */
} kbd_region_t;

/** \defgroup   key_states  Key States
    \brief                  States each key can be in.
    \ingroup                kbd

    These are the different 'states' each key can be in. They are stored in
    kbd_state_t->matrix, and manipulated/checked by kbd_check_poll.

    none-> pressed or none
    was pressed-> pressed or none
    pressed-> was_pressed
    @{
*/
#define KEY_STATE_NONE        0
#define KEY_STATE_WAS_PRESSED 1
#define KEY_STATE_PRESSED     2
/** @} */

/** \brief   Maximum number of keys the DC can read simultaneously.
    \ingroup kbd
    This is a hardware constant. The define prevents the magic number '6' from appearing.
**/
#define MAX_PRESSED_KEYS 6

/** \brief   Maximum number of keys a DC keyboard can have.
    \ingroup kbd
    This is a hardware constant. The define prevents the magic number '256' from appearing.
**/
#define KBD_MAX_KEYS 256

/* Short-term compatibility helper. */
static const int MAX_KBD_KEYS   __depr("Please use KBD_MAX_KEYS.") = KBD_MAX_KEYS;

/** \brief   Keyboard keymap.
    \ingroup kbd

    This structure represents a mapping from raw key values to ASCII values, if
    appropriate. This handles base values as well as shifted ("shift" and "Alt"
    keys) values.

    \headerfile dc/maple/keyboard.h
*/
typedef struct kbd_keymap {
    uint8_t base[KBD_MAX_KEYS];
    uint8_t shifted[KBD_MAX_KEYS];
    uint8_t alt[KBD_MAX_KEYS];
} kbd_keymap_t;

/** \brief   Keyboard raw condition structure.
    \ingroup kbd

    This structure is what the keyboard responds with as its current status.

    \headerfile dc/maple/keyboard.h
*/
typedef struct {
    uint8_t modifiers;    /**< \brief Bitmask of set modifiers. */
    uint8_t leds;         /**< \brief Bitmask of set LEDs */
    uint8_t keys[MAX_PRESSED_KEYS];      /**< \brief Key codes for currently pressed keys. */
} kbd_cond_t;

/** \brief   Keyboard status structure.
    \ingroup kbd

    This structure holds information about the current status of the keyboard
    device. This is what maple_dev_status() will return.

    \headerfile dc/maple/keyboard.h
*/
typedef struct kbd_state {
    /** \brief  The latest raw condition of the keyboard. */
    kbd_cond_t cond;

    /** \brief  Key array.

        This array lists the state of all possible keys on the keyboard. It can
        be used for key repeat and debouncing. This will be non-zero if the key
        is currently being pressed.

        \see    kbd_keys
    */
    uint8_t matrix[KBD_MAX_KEYS];

    /** \brief  Modifier key status. */
    int shift_keys;

    /** \brief  Keyboard type/region. */
    kbd_region_t region;
} kbd_state_t;

/** \defgroup kbd_input     Querying for Input
    \brief                  Various methods for checking keyboard input

    There are 2 different ways to check for input with the keyboard API:

    Mechanism             | Description
    ----------------------|--------------------------------------------
    \ref kbd_polling      |Manual checks each key state every frame
    \ref kbd_queue        |Monitor for new key press events every frame

    @{
*/

/** \defgroup kbd_polling   State Polling
    \brief                  Frame-based polling for keyboard input

    One method of checking for key input is to simply poll
    kbd_state_t::matrix for the desired key states each frame.

    First, lets grab a pointer to the kbd_state_t:

        kbd_state_t *kbd = kbd_get_state(device);

    Then let's "move" every frame an arrow key is held down:

        if(kbd->matrix[KBD_KEY_LEFT] == KEY_STATE_PRESSED)
            printf("Moving left!\n");
        if(kbd->matrix[KBD_KEY_RIGHT] == KEY_STATE_PRESSED)
            printf("Moving right!\n");
        if(kbd->matrix[KBD_KEY_UP] == KEY_STATE_PRESSED)
            printf("Moving up!\n");
        if(kbd->matrix[KBD_KEY_DOWN] == KEY_STATE_PRESSED)
            printf("Moving down!\n");

    Finally, let's charge an "attack" incrementing the charge for each
    frame that the key is held and resetting when the key is released:

        if(kbd->matrix[KBD_KEY_SPACE] == KEY_STATE_PRESSED)
            charge++;
        if(kbd->matrix[KBD_KEY_SPACE] == KEY_STATE_WAS_PRESSED) {
            printf("Releasing a charged attack of %i!\n", charge);
            charge = 0;
        }

    @{
*/

/** \brief Retrieves the keyboard state from a maple device

    Accessor method for safely retrieving a kbd_state_t from a maple_device_t
    of a `MAPLE_FUNC_KEYBOARD` type. This function also checks for whether
    the given device is actually a keyboard and for whether it is currently
    valid.

    \param  device          Handle corresponding to a `MAPLE_FUNC_KEYBOARD`
                            device.

    \retval kbd_state_t*    A pointer to the internal keyboard state on success.
    \retval NULL            On failure.

*/
kbd_state_t *kbd_get_state(maple_device_t *device);

/** @} */

/** \defgroup kbd_queue    Queue Monitoring
    \brief                 Monitor queue for key press events.

    \par Popping from the Queue
    One method of checking for key input is to use the internal key press
    queue. This is most frequently used when keyboard input is used within
    a text processing context, which is only concerned with individual key
    press events, rather than the frame-by-frame state.

    \par
    We simply pop keys off of the queue in a loop, until the queue is empty:

        int k;

        while((k = kbd_queue_pop(device, 1)) != KBD_QUEUE_END)
            printf("Key pressed: %c!\n", (char)k);

    \par Repeated Presses
    As with a text processor, a key which has been held down for a duration of
    time will generate periodic key press events which will be pushed onto the
    queue.

    @{
*/

/** \brief   Size of a keyboard queue.

    Each keyboard queue will hold this many elements. Once the queue fills, no
    new elements will be placed on the queue. As long as you check the queue
    relatively frequently, the default of 16 should be plenty.

    \note   This <strong>MUST</strong> be a power of two.
*/
#define KBD_QUEUE_SIZE 16

/** \brief Delimiter value for kbd_queue_pop()

    Value returned from kbd_queue_pop() when there are no more keys in the
    queue.

    \sa kbd_queue_pop()
*/
#define KBD_QUEUE_END     -1

/** \brief Configures held key auto-repeat intervals

    This function is used to configure the specific timing behavior for how the
    internal queue treats a key which is being held down. Giving non-zero values
    for both parameters will cause the held key to be re-enqueued every
    \p interval milliseconds after it has been held for the initial \p start time
    in milliseconds.

    Specifying a value of zero for the two parameters disables this repeating key
    behavior.

    \note
    By default, the \p start time is 600ms while the repeating \p interval is 20ms.

    \param  start       The duration after which the held key starts to
                        register as repeated key presses (or zero to disable
                        this behavior).
    \param  interval    The duration between subsequent key repeats after the
                        initial \p start time has elapsed.

    \sa kbd_queue_pop()
*/
void kbd_set_repeat_timing(uint16_t start, uint16_t interval);

/** \brief   Pop a key off a specific keyboard's queue.

    This function pops the front element off of the specified keyboard queue,
    and returns the value of that key to the caller.

    If the \p xlat parameter is true and the key represents an ISO-8859-1
    character, that is the value that will be returned from this function.
    If the key cannot be converted into a valid ISO-8859-1 character the
    raw key code, shifted up by 8 bits, will be returned.

    If the \p xlat parameter is false, the lower 8 bits of the returned
    value will be the raw key code. The next 8 bits will be the modifier
    keys that were down when the key was pressed (kbd_mods_t). The next
    8 bits will be the lock key/LED statuses (kbd_leds_t).

    \param  dev             The keyboard device to read from.
    \param  xlat            Set to true to do key translation. Otherwise,
                            you'll simply get the raw key value. Raw key values
                            are not mapped at all, so you are responsible for
                            figuring out what it is by the region.

    \return                 The value at the front of the queue, or KBD_QUEUE_END
                            if there are no keys in the queue.
*/
int kbd_queue_pop(maple_device_t *dev, bool xlat);

/** \brief   Activate or deactivate global key queueing.
    \deprecated

    This function will turn the internal keyboard queueing on or off. Note that
    there is only one queue for the whole system, no matter how many keyboards
    are attached, and the queue is of fairly limited length. Turning queueing
    off is useful (for instance) in a game where individual keypresses don't
    mean as much as having the keys up or down does.

    You can clear the queue (without popping all the keys off) by setting the
    active value to a different value than it was.

    The queue is by default on, unless you turn it off.

    \param  active          Set to non-zero to activate the queue.
    \note                   The global queue does not account for non-US
                            keyboard layouts and is deprecated. Please use the
                            individual queues instead for future code.
*/
void kbd_set_queue(int active) __deprecated;

/** \brief   Pop a key off the global keyboard queue.
    \deprecated

    This function pops the front off of the keyboard queue, and returns the
    value to the caller. The value returned will be the ASCII value of the key
    pressed (accounting for the shift keys being pressed).

    If a key does not have an ASCII value associated with it, the raw key code
    will be returned, shifted up by 8 bits.

    \return                 The value at the front of the queue, or KBD_QUEUE_END
                            if there are no keys in the queue or queueing is off.
    \note                   This function does not account for non-US keyboard
                            layouts properly (for compatibility with old code),
                            and is deprecated. Use the individual keyboard
                            queues instead to properly account for non-US
                            keyboards.
    \see                    kbd_queue_pop()
*/
int kbd_get_key(void) __deprecated;

/** @} */

/** @} */

/* \cond */
/* Init / Shutdown */
void kbd_init(void);
void kbd_shutdown(void);
/* \endcond */

__END_DECLS

#endif  /* __DC_MAPLE_KEYBOARD_H */
