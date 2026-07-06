/* KallistiOS ##version##

   dc/maple/mie.h
   Copyright (C) 2026 Ruslan Rostovtsev

   NAOMI and NAOMI 2 MIE-JVS bridge Maple device driver.
*/

/** \file    dc/maple/mie.h
    \brief   Definitions for the NAOMI MIE-JVS bridge Maple device.
    \ingroup mie

    This file contains the public API for the MIE (Maple I/O Emulator) driver
    for NAOMI and NAOMI 2, which bridges the Maple bus to a JVS I/O board.

    \author Ruslan Rostovtsev
*/

#ifndef __DC_MAPLE_MIE_H
#define __DC_MAPLE_MIE_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <kos/regfield.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

/** \defgroup mie MIE
    \brief    NAOMI and NAOMI 2 MIE-JVS bridge Maple device
    \ingroup  peripherals

    The MIE (Maple I/O Emulator) is a Z80-based bridge that connects the Maple
    bus to a JVS I/O board. It normally occupies Maple port A, unit 0. Player
    inputs from the JVS board are mapped to a standard Dreamcast controller
    state structure (\ref cont_state_t), as well as the original native JVS
    inputs structure (\ref mie_jvs_inputs_t).

    Port A can be switched between JVS mode and a regular Dreamcast Maple port.
    The driver detects which mode is active and either handles JVS input or
    yields port A to the standard Maple autodetect (controller, VMU, etc.).

    \author Ruslan Rostovtsev

    @{
*/

/** \defgroup mie_jvs_inputs JVS Inputs
    \brief    Native JVS input state structures and masks
    \ingroup  mie

    Decoded JVS switch, analog, coin, and panel state from the I/O board.
    Fetch the latest values by casting maple_dev_status() on an MIE device
    to \ref mie_state_t and reading the \p jvs member.
*/

/** \defgroup mie_jvs_limits Limits
    \brief    JVS channel and buffer size constants
    \ingroup  mie_jvs_inputs

    @{
*/
#define MIE_JVS_PLAYER_SLOTS    2       /**< \brief Max player switch banks. */
#define MIE_JVS_ANALOG_CHANNELS 8       /**< \brief Number of JVS analog channels. */
#define MIE_JVS_ANALOG_RAW_MASK (GENMASK(6, 0) | GENMASK(15, 8))
#define MIE_JVS_COIN_SLOTS      2       /**< \brief Max JVS coin slots. */
#define MIE_JVS_PANEL_DIP_COUNT 4       /**< \brief Number of cabinet DIP switches. */
#define MIE_JVS_OUTPUT_COUNT    24      /**< \brief Max JVS general purpose outputs. */
#define MIE_EEPROM_SIZE         128     /**< \brief MIE on-board EEPROM size in bytes. */
#define MIE_ID_SIZE             64      /**< \brief MIE board identifier buffer size. */
/** @} */

/** \defgroup mie_jvs_masks Input Masks
    \brief    Bit masks for mie_jvs_callback() and native switch fields
    \ingroup  mie_jvs_inputs

    For player inputs, use \p MIE_JVS_*_BIT with \p MIE_JVS_IN_P1 or
    \p MIE_JVS_IN_P2. First-byte switches are \p start, \p service, directions,
    and push switches 1–2; second-byte switches (\p sw9 through \p sw16) are
    push switches 3–10.

    For \p MIE_JVS_IN_SYSTEM, use \p MIE_JVS_SYS_*_BIT.
    For the cabinet panel, use \p MIE_JVS_PANEL_DIP*_BIT
    or \p MIE_JVS_PANEL_*_BIT with the matching \ref mie_jvs_input_t value.

    @{
*/
#define MIE_JVS_SW1_BIT            BIT(8)  /**< \brief Push switch 2. */
#define MIE_JVS_SW2_BIT            BIT(9)  /**< \brief Push switch 1. */
#define MIE_JVS_RIGHT_BIT          BIT(10) /**< \brief Right switch (JVS SW3). */
#define MIE_JVS_LEFT_BIT           BIT(11) /**< \brief Left switch (JVS SW4). */
#define MIE_JVS_DOWN_BIT           BIT(12) /**< \brief Down switch (JVS SW5). */
#define MIE_JVS_UP_BIT             BIT(13) /**< \brief Up switch (JVS SW6). */
#define MIE_JVS_SERVICE_BIT        BIT(14) /**< \brief Service switch (JVS SW7). */
#define MIE_JVS_START_BIT          BIT(15) /**< \brief Start switch (JVS SW8). */
#define MIE_JVS_SW9_BIT            BIT(0)  /**< \brief Push switch 3. */
#define MIE_JVS_SW10_BIT           BIT(1)  /**< \brief Push switch 4. */
#define MIE_JVS_SW11_BIT           BIT(2)  /**< \brief Push switch 5. */
#define MIE_JVS_SW12_BIT           BIT(3)  /**< \brief Push switch 6. */
#define MIE_JVS_SW13_BIT           BIT(4)  /**< \brief Push switch 7. */
#define MIE_JVS_SW14_BIT           BIT(5)  /**< \brief Push switch 8. */
#define MIE_JVS_SW15_BIT           BIT(6)  /**< \brief Push switch 9. */
#define MIE_JVS_SW16_BIT           BIT(7)  /**< \brief Push switch 10. */
#define MIE_JVS_COIN_INSERT_BIT    BIT(0)  /**< \brief Coin slot insert edge for MIE_JVS_IN_COIN*. */
#define MIE_JVS_PANEL_DIP1_BIT     BIT(0)  /**< \brief Panel DIP switch 1. */
#define MIE_JVS_PANEL_DIP2_BIT     BIT(1)  /**< \brief Panel DIP switch 2. */
#define MIE_JVS_PANEL_DIP3_BIT     BIT(2)  /**< \brief Panel DIP switch 3. */
#define MIE_JVS_PANEL_DIP4_BIT     BIT(3)  /**< \brief Panel DIP switch 4. */
#define MIE_JVS_PANEL_TEST_BIT     BIT(0)  /**< \brief Panel test switch (PSW1). */
#define MIE_JVS_PANEL_SERVICE_BIT  BIT(1)  /**< \brief Panel service switch (PSW2). */
#define MIE_JVS_SYS_TILT3_BIT      BIT(4)  /**< \brief Tilt switch 3. */
#define MIE_JVS_SYS_TILT2_BIT      BIT(5)  /**< \brief Tilt switch 2. */
#define MIE_JVS_SYS_TILT1_BIT      BIT(6)  /**< \brief Tilt switch 1. */
#define MIE_JVS_SYS_TEST_BIT       BIT(7)  /**< \brief Test switch. */
/** @} */

/** \defgroup mie_jvs_coin_status Coin Status Flags
    \brief    COININP status bits in the upper word
    \ingroup  mie_jvs_inputs

    @{
*/
#define MIE_JVS_COIN_STUFF           0x4000  /**< \brief Coin chute stuffed. */
#define MIE_JVS_COIN_COUNTER_BREAK   0x8000  /**< \brief Coin counter fault. */
#define MIE_JVS_COIN_BUSY            (MIE_JVS_COIN_STUFF | MIE_JVS_COIN_COUNTER_BREAK)
#define MIE_JVS_COIN_DEC_LIMIT       0x1000  /**< \brief Auto COINDEC when meter exceeds this. */
/** @} */

/** \brief   Parsed JVS coin slot state.
    \ingroup mie_jvs_inputs

    Updated from JVS COININP (0x21) during periodic polling. Use
    mie_read_coin_input() to copy the latest cached slots; that function does
    not issue a separate bus transaction.

    Each slot is a 16-bit meter value in JVS wire order. Status flags
    \ref MIE_JVS_COIN_STUFF and \ref MIE_JVS_COIN_COUNTER_BREAK occupy bits
    14..15 (\ref MIE_JVS_COIN_BUSY). When either flag is set the driver keeps
    the previous \p count and sets mie_jvs_inputs_t::coin_fault for that frame.

    On a normal reading, \p count is \p raw with status bits cleared
    (\c raw & ~MIE_JVS_COIN_BUSY). Coin inserts are detected as positive
    deltas against the previous \p count.
*/
typedef struct mie_jvs_coin_slot {
    uint16_t raw;       /**< \brief Raw 16-bit COININP word. */
    uint16_t count;     /**< \brief Last good meter (status bits cleared). */
} mie_jvs_coin_slot_t;

/** \brief   One player JVS switch bank.
    \ingroup mie_jvs_inputs

    A 1 bit indicates that the corresponding switch is active.
    See \ref mie_jvs_masks for the usual arcade assignment of each switch.
*/
typedef struct mie_jvs_player_sw {
    union {
        uint16_t raw;             /**< \brief Raw 16-bit JVS switch word. */
        struct {
            uint16_t sw9: 1;      /**< \brief Push switch 3 value. */
            uint16_t sw10: 1;     /**< \brief Push switch 4 value. */
            uint16_t sw11: 1;     /**< \brief Push switch 5 value. */
            uint16_t sw12: 1;     /**< \brief Push switch 6 value. */
            uint16_t sw13: 1;     /**< \brief Push switch 7 value. */
            uint16_t sw14: 1;     /**< \brief Push switch 8 value. */
            uint16_t sw15: 1;     /**< \brief Push switch 9 value. */
            uint16_t sw16: 1;     /**< \brief Push switch 10 value. */
            uint16_t sw1: 1;      /**< \brief Push switch 2 value. */
            uint16_t sw2: 1;      /**< \brief Push switch 1 value. */
            uint16_t right: 1;    /**< \brief Right switch (JVS SW3). */
            uint16_t left: 1;     /**< \brief Left switch (JVS SW4). */
            uint16_t down: 1;     /**< \brief Down switch (JVS SW5). */
            uint16_t up: 1;       /**< \brief Up switch (JVS SW6). */
            uint16_t service: 1;  /**< \brief Service switch (JVS SW7). */
            uint16_t start: 1;    /**< \brief Start switch (JVS SW8). */
        };
    };
} mie_jvs_player_sw_t;

/** \brief   Panel DIP switch bank.
    \ingroup mie_jvs_inputs
*/
typedef struct mie_jvs_panel_dip {
    union {
        uint8_t raw;        /**< \brief Raw DIP switch byte. */
        struct {
            uint8_t sw1: 1; /**< \brief DIP switch 1 value. */
            uint8_t sw2: 1; /**< \brief DIP switch 2 value. */
            uint8_t sw3: 1; /**< \brief DIP switch 3 value. */
            uint8_t sw4: 1; /**< \brief DIP switch 4 value. */
            uint8_t: 4;
        };
    };
} mie_jvs_panel_dip_t;

/** \brief   JVS SWINP system switch byte.
    \ingroup mie_jvs_inputs

    A set bit means the switch is active. See \ref mie_jvs_masks for
    \p MIE_JVS_SYS_*_BIT values.
*/
typedef struct mie_jvs_system {
    union {
        uint8_t raw;            /**< \brief Raw JVS system switch byte. */
        struct {
            uint8_t: 4;
            uint8_t tilt3: 1;   /**< \brief Tilt switch 3. */
            uint8_t tilt2: 1;   /**< \brief Tilt switch 2. */
            uint8_t tilt1: 1;   /**< \brief Tilt switch 1. */
            uint8_t test: 1;    /**< \brief Test switch. */
        };
    };
} mie_jvs_system_t;

/** \brief   Cabinet panel switches.
    \ingroup mie_jvs_inputs
*/
typedef struct mie_jvs_panel {
    mie_jvs_panel_dip_t dip;    /**< \brief Cabinet DIP switches. */
    union {
        uint8_t raw;            /**< \brief Raw panel service switch byte. */
        struct {
            uint8_t test: 1;    /**< \brief Panel test switch (PSW1). */
            uint8_t service: 1; /**< \brief Panel service switch (PSW2). */
            uint8_t: 6;
        };
    } psw;
} mie_jvs_panel_t;

/** \brief   Decoded JVS input state.
    \ingroup mie_jvs_inputs

    Full native JVS input snapshot for two players, the cabinet panel, analog
    channels, and coin slots. Analog channel names follow the default racing
    layout; use the \p analog array for generic access.

    \sa mie_state_t
*/
typedef struct mie_jvs_inputs {
    mie_jvs_system_t system;    /**< \brief JVS SWINP system switch byte. */
    mie_jvs_panel_t panel;      /**< \brief Cabinet panel switches. */
    mie_jvs_player_sw_t p1;     /**< \brief Player 1 switch bank. */
    mie_jvs_player_sw_t p2;     /**< \brief Player 2 switch bank. */
    union {
        uint16_t analog[MIE_JVS_ANALOG_CHANNELS]; /**< \brief Raw analog channel values. */
        struct {
            uint16_t wheel;     /**< \brief Steering wheel analog channel. */
            uint16_t accel;     /**< \brief Accelerator pedal analog channel. */
            uint16_t brake;     /**< \brief Brake pedal analog channel. */
            uint16_t ch3;       /**< \brief Analog channel 3. */
            uint16_t ch4;       /**< \brief Analog channel 4. */
            uint16_t ch5;       /**< \brief Analog channel 5. */
            uint16_t ch6;       /**< \brief Analog channel 6. */
            uint16_t ch7;       /**< \brief Analog channel 7. */
        };
    };
    mie_jvs_coin_slot_t coin[MIE_JVS_COIN_SLOTS]; /**< \brief Coin slot meters. */
    uint8_t coin_pulse; /**< \brief One-shot insert flags (bit N = slot N). */
    uint8_t coin_fault; /**< \brief Fault flags (bit N = slot N, raw & BUSY). */
} mie_jvs_inputs_t;

/** \brief   MIE device status.
    \ingroup mie

    Contains the latest decoded player inputs from the JVS board.
    Cast maple_dev_status() on an MIE device to obtain this structure.

    \headerfile dc/maple/mie.h
    \sa mie_jvs_inputs, maple_dev_status
*/
typedef struct mie_state {
    cont_state_t cont;          /**< \brief Mapped Dreamcast controller state. */
    mie_jvs_inputs_t jvs;       /**< \brief Native JVS input state. */
} mie_state_t;

/** \defgroup mie_mapping Controller Mapping
    \brief    JVS to Dreamcast controller translation
    \ingroup  mie

    During polling the driver fills \ref mie_state_t::cont from native JVS
    inputs using the active mapper. Pass NULL to mie_set_cont_map() to restore
    the built-in default layout below.

    Default JVS to \ref cont_state_t mapping:

    - p1.start / p2.start -> CONT_START
    - p1.up / p2.up -> CONT_DPAD_UP
    - p1.down / p2.down -> CONT_DPAD_DOWN
    - p1.left / p2.left -> CONT_DPAD_LEFT
    - p1.right / p2.right -> CONT_DPAD_RIGHT
    - p1.sw16 / p2.sw16 -> CONT_A
    - p1.sw2 / p2.sw2 -> CONT_B
    - p1.sw1 / p2.sw1 -> CONT_X
    - p1.sw14 / p2.sw14 -> CONT_Y
    - p1.service / p2.service -> CONT_Z
    - panel.psw.test -> CONT_C
    - panel.psw.service -> CONT_D
    - wheel -> joyx (-128 to 127)
    - accel -> rtrig (0 to 255)
    - brake -> ltrig (0 to 255)

    The default mapper skips wheel and pedals when wheel and accel raw values
    differ by no more than 0x500.
*/

/** \brief   JVS inputs to Dreamcast controller mapper.
    \ingroup mie_mapping

    Installed with mie_set_cont_map(). Receives decoded native JVS state and
    fills a \ref cont_state_t compatible structure for use with the standard
    controller API.

    \param  jvs             Decoded JVS input state.
    \param  cont            Controller state to fill.

    \sa mie_set_cont_map
*/
typedef void (*mie_cont_map_fn_t)(const mie_jvs_inputs_t *jvs,
                                  cont_state_t *cont);

/** \defgroup mie_callbacks Callbacks
    \brief    Automatic input notification callbacks
    \ingroup  mie

    Callbacks are invoked on a dedicated worker thread when the requested input
    transitions from released to pressed. Pass NULL as the callback pointer to
    uninstall a registration with the same key.
*/

/** \brief   Mapped controller button callback type.
    \ingroup mie_callbacks

    Called when the mapped \ref cont_state_t buttons matching the registered
    mask become pressed.

    \param  btns            Current mapped button bitmask (\ref controller_input_masks).

    \sa mie_btn_callback
*/
typedef void (*mie_btn_callback_t)(uint32_t btns);

/** \brief   Native JVS input source for mie_jvs_callback().
    \ingroup mie_callbacks
*/
typedef enum mie_jvs_input {
    MIE_JVS_IN_SYSTEM,      /**< \brief JVS SWINP system switch byte. */
    MIE_JVS_IN_P1,          /**< \brief Player 1 switch bank. */
    MIE_JVS_IN_P2,          /**< \brief Player 2 switch bank. */
    MIE_JVS_IN_PANEL_DIP,   /**< \brief Cabinet DIP switches. */
    MIE_JVS_IN_PANEL_PSW,   /**< \brief Cabinet panel service switches. */
    MIE_JVS_IN_COIN1,       /**< \brief Coin slot 1 (COIN SW 1) meter insert. */
    MIE_JVS_IN_COIN2        /**< \brief Coin slot 2 (COIN SW 2) meter insert. */
} mie_jvs_input_t;

/** \brief   Native JVS input callback type.
    \ingroup mie_callbacks

    \param  input           Input source that triggered the callback.
    \param  mask            Active bits from the matched mask.

    \sa mie_jvs_callback, mie_jvs_masks
*/
typedef void (*mie_jvs_callback_t)(mie_jvs_input_t input, uint32_t mask);

/** \defgroup mie_port0 Port A Mode
    \brief    NAOMI port A wiring detection
    \ingroup  mie
*/

/** \defgroup mie_analog Analog Calibration
    \brief    JVS wheel and pedal calibration
    \ingroup  mie

    The driver maps the JVS wheel and pedal channels into \ref mie_state_t::cont
    (\p joyx, \p rtrig, \p ltrig). With a valid calibration installed the values
    are normalized (\p joyx -128 to 127, triggers 0 to 255); otherwise a built-in
    legacy mapping is used. While an interactive calibration session is running,
    the raw channel values are passed through to \p cont unchanged.

    Use mie_analog_calib_start() and mie_analog_calib_capture() to run the
    interactive flow, then persist the result obtained with
    mie_analog_calib_get() and restore it later with mie_analog_calib_set().

    For raw \ref mie_jvs_inputs_t values use mie_analog_norm_wheel(),
    mie_analog_norm_accel(), and mie_analog_norm_brake().
*/

/** \brief   Per-axis JVS analog calibration data.
    \ingroup mie_analog

    For pedals only \p min and \p max are used. For the wheel, \p center must lie
    strictly between \p min and \p max.
*/
typedef struct mie_analog_axis_calib {
    uint16_t min;       /**< \brief Raw value at the minimum position. */
    uint16_t center;    /**< \brief Raw center value (wheel only). */
    uint16_t max;       /**< \brief Raw value at the maximum position. */
} mie_analog_axis_calib_t;

/** \brief   Wheel and pedal calibration set.
    \ingroup mie_analog
*/
typedef struct mie_analog_calib {
    mie_analog_axis_calib_t wheel;      /**< \brief Steering wheel calibration. */
    mie_analog_axis_calib_t accel;      /**< \brief Accelerator pedal calibration. */
    mie_analog_axis_calib_t brake;      /**< \brief Brake pedal calibration. */
} mie_analog_calib_t;

/** \brief   Interactive calibration step.
    \ingroup mie_analog

    Returned by mie_analog_calib_current() to drive the UI prompts. Each step
    is confirmed with mie_analog_calib_capture().
*/
typedef enum mie_analog_calib_step {
    MIE_ANALOG_CALIB_IDLE = 0,      /**< \brief No session running. */
    MIE_ANALOG_CALIB_WHEEL,         /**< \brief Turn the wheel fully both ways. */
    MIE_ANALOG_CALIB_WHEEL_CENTER,  /**< \brief Hold the wheel centered. */
    MIE_ANALOG_CALIB_ACCEL,         /**< \brief Release then fully press accelerator. */
    MIE_ANALOG_CALIB_BRAKE          /**< \brief Release then fully press brake. */
} mie_analog_calib_step_t;

/** \brief   NAOMI port A wiring mode.
    \ingroup mie_port0

    Port A can be wired to the MIE/JVS bridge or switched to a regular
    Dreamcast Maple port. On retail Dreamcast, mie_port0_mode() always returns
    \p MIE_PORT0_MAPLE.
*/
typedef enum mie_port0_mode {
    MIE_PORT0_UNKNOWN = 0,      /**< \brief Probing not finished yet. */
    MIE_PORT0_JVS,              /**< \brief Port A is MIE/JVS bridge. */
    MIE_PORT0_MAPLE             /**< \brief Port A is standard Maple autodetect. */
} mie_port0_mode_t;

/** \brief   Query NAOMI port A wiring mode.
    \ingroup mie_port0

    Returns the mode selected after MIE probing during maple_wait_scan().
    When \p MIE_PORT0_MAPLE is active, port A is handled by the standard Maple
    autodetect path instead of this driver.

    \return                 Current port A wiring mode.
*/
mie_port0_mode_t mie_port0_mode(void);

/** \defgroup mie_coin Coin Control
    \brief    JVS coin slot meter access and commands
    \ingroup  mie
*/

/** \brief   Read cached coin slot meter.
    \ingroup mie_coin

    Returns the parsed cumulative count for the given coin slot from the latest
    driver poll. Does not perform a synchronous bus transaction.

    \param  slot            Coin slot index (0 = first slot).
    \return                 Coin meter count, or 0 for an invalid slot.

    \sa mie_read_coin_input, mie_jvs_coin_slot_t
*/
uint16_t mie_get_coin_meter(uint8_t slot);

/** \brief   Copy cached coin slot state from the last poll.
    \ingroup mie_coin

    Copies the parsed per-slot data already obtained by the periodic input
    poll. Does not send a standalone JVS COININP (0x21) command.

    \param  out             Per-slot parsed data (up to \p MIE_JVS_COIN_SLOTS).
    \param  max_slots       Number of coin slots to copy (1 or 2).
    \retval true            Arguments valid and JVS initialized.
    \retval false           Invalid arguments or JVS not initialized.

    \sa mie_get_coin_meter, mie_jvs_coin_slot_t, mie_jvs_inputs_t::coin_fault
*/
bool mie_read_coin_input(mie_jvs_coin_slot_t *out, int max_slots);

/** \brief   Send JVS COINDEC (0x30) for a coin slot.
    \ingroup mie_coin

    Subtracts credits from the specified coin slot meter on the JVS board.

    \param  slot            Coin slot index (0 = first slot).
    \param  amount          Credits to subtract.
    \param  block           \c true to wait for completion; \c false to queue only.
    \retval true            Command completed or queued successfully.
    \retval false           Invalid arguments, JVS not initialized, busy, or I/O failed.
*/
bool mie_coin_decrease(uint8_t slot, uint16_t amount, bool block);

/** \brief   Send JVS COINADD (0x35) for a coin slot.
    \ingroup mie_coin

    Adds credits to the specified coin slot meter on the JVS board.

    \param  slot            Coin slot index (0 = first slot).
    \param  amount          Credits to add.
    \param  block           \c true to wait for completion; \c false to queue only.
    \retval true            Command completed or queued successfully.
    \retval false           Invalid arguments, JVS not initialized, busy, or I/O failed.
*/
bool mie_coin_add(uint8_t slot, uint16_t amount, bool block);

/** \defgroup mie_output General Purpose Output
    \brief    JVS general purpose output (lamps, solenoids)
    \ingroup  mie

    The JVS I/O board exposes up to \ref MIE_JVS_OUTPUT_COUNT general purpose
    output lines, driven with the JVS general-purpose output command (0x32). They
    are used for cabinet lamps, start button lights, solenoids, and similar
    actuators.

    Output state is a \c uint32_t bitmask in JVS wire order: output \p n is
    bit \c (31 - n). The driver keeps a cached copy so individual lines can be
    toggled with mie_jvs_set_output() without affecting the others.

    @{
*/

/** \brief Bit mask for JVS output index \p index (output 0 = bit 31). */
#define MIE_JVS_OUTPUT_MASK(index) BIT(31 - (index))

/** \brief All \ref MIE_JVS_OUTPUT_COUNT outputs enabled. */
#define MIE_JVS_OUTPUT_ALL GENMASK(31, 32 - MIE_JVS_OUTPUT_COUNT)

/** \brief Bitmask for the first \p count JVS outputs (output 0 = bit 31). */
#define MIE_JVS_OUTPUT_MASK_COUNT(count) GENMASK(31, 32 - (count))

/** \brief   Number of JVS outputs reported by the I/O board.
    \return                 Output count from the last function check (0x14),
                            or \ref MIE_JVS_OUTPUT_COUNT if unknown.

    \sa mie_jvs_set_outputs
*/
uint8_t mie_jvs_driver_outputs(void);

/** \brief   Set all JVS general purpose outputs at once.
    \param  outputs         Output bitmask in JVS wire order.
    \param  block           \c true to wait for completion; \c false to queue only.
    \retval true            Command completed or queued successfully.
    \retval false           JVS not initialized, busy, or I/O failed.

    \sa mie_jvs_set_output, mie_jvs_get_outputs
*/
bool mie_jvs_set_outputs(uint32_t outputs, bool block);

/** \brief   Set a single JVS general purpose output.

    \param  index           Output index (0 to \ref MIE_JVS_OUTPUT_COUNT - 1).
    \param  on              \c true to turn the output on; \c false to turn it off.
    \param  block           \c true to wait for completion; \c false to queue only.
    \retval true            Command completed or queued successfully.
    \retval false           Invalid index, JVS not initialized, busy, or I/O failed.

    \sa mie_jvs_set_outputs, mie_jvs_get_outputs
*/
bool mie_jvs_set_output(uint8_t index, bool on, bool block);

/** \brief   Get the cached JVS general purpose output state.
    \return                 Last applied output bitmask in JVS wire order.

    \sa mie_jvs_set_outputs
*/
uint32_t mie_jvs_get_outputs(void);

/** @} */

/** \brief   Set an automatic mapped button press callback.
    \ingroup mie_callbacks

    Registers a callback invoked when the mapped controller buttons in \p btns
    transition from not-all-pressed to all-pressed. The callback runs on a
    worker thread, not in the Maple IRQ context.

    \param  btns            Button mask to match (\ref controller_input_masks).
    \param  cb              Callback to invoke, or NULL to uninstall callbacks
                            registered for the same \p btns value.
    \retval 0               On success.
    \retval -1              On allocation or thread creation failure.

    \sa mie_btn_callback_t, mie_jvs_callback
*/
int mie_btn_callback(uint32_t btns, mie_btn_callback_t cb);

/** \brief   Set an automatic native JVS input callback.
    \ingroup mie_callbacks

    Registers a callback invoked when the selected native input source has all
    bits in \p mask transition from not-all-active to all-active. The callback
    runs on a worker thread, not in the Maple IRQ context.

    For \p MIE_JVS_IN_P1, \p MIE_JVS_IN_P2, \p MIE_JVS_IN_SYSTEM,
    \p MIE_JVS_IN_PANEL_DIP, and \p MIE_JVS_IN_PANEL_PSW, \p mask uses the
    matching \ref mie_jvs_masks constants.
    For \p MIE_JVS_IN_COIN1 and \p MIE_JVS_IN_COIN2, use \p MIE_JVS_COIN_INSERT_BIT.
    These fire when the JVS COININP meter for that slot increases after a physical
    coin insert on COIN SW 1/2. The I/O board updates the meter automatically;
    do not call mie_coin_add() again for the same insert.

    \param  input           Native JVS input source to watch.
    \param  mask            Bit mask within that source.
    \param  cb              Callback to invoke, or NULL to uninstall callbacks
                            registered for the same \p input and \p mask pair.
    \retval 0               On success.
    \retval -1              On allocation or thread creation failure.

    \sa mie_jvs_callback_t, mie_btn_callback
*/
int mie_jvs_callback(mie_jvs_input_t input, uint32_t mask,
                     mie_jvs_callback_t cb);

/** \brief   Install a custom JVS-to-controller mapper.
    \ingroup mie_mapping

    Replaces the built-in default mapping applied during polling. Pass NULL to
    restore the default mapper documented in \ref mie_mapping.

    \param  fn              Mapper function, or NULL for the built-in default.

    \sa mie_cont_map_fn_t, mie_state_t
*/
void mie_set_cont_map(mie_cont_map_fn_t fn);

/** \defgroup mie_device Device Access
    \brief    MIE board identification, EEPROM, and firmware init
    \ingroup  mie
*/

/** \brief   Read the MIE board identifier string.
    \ingroup mie_device

    Sends the MIE GET ID command and copies the response into \p dst.

    \param  dst             Destination buffer (\p MIE_ID_SIZE bytes).
    \return                 \p dst on success, NULL on failure.

    \sa MIE_ID_SIZE
*/
char *mie_get_id(char *dst);

/** \brief   Read the MIE on-board EEPROM.
    \ingroup mie_device

    Performs a synchronous EEPROM fetch over the MIE IO protocol.

    \param  dst             Destination buffer (\p MIE_EEPROM_SIZE bytes).
    \retval true            Read succeeded.
    \retval false           Invalid buffer or I/O failed.

    \sa MIE_EEPROM_SIZE, mie_set_eeprom
*/
bool mie_get_eeprom(void *dst);

/** \brief   Write the MIE on-board EEPROM.
    \ingroup mie_device

    Performs a synchronous EEPROM write over the MIE IO protocol. Data is
    written in 16-byte chunks with pacing between transfers.

    \param  eeprom          Source buffer (\p MIE_EEPROM_SIZE bytes).
    \retval true            Write succeeded.
    \retval false           Invalid buffer or I/O failed.

    \sa MIE_EEPROM_SIZE, mie_get_eeprom
*/
bool mie_set_eeprom(uint8_t *eeprom);

/** \brief   Compute Sega NAOMI EEPROM CRC-16.
    \ingroup mie_device

    CRC-16-CCITT variant with seed \c 0xDEBDEB00 and an extra round over a
    trailing \c 0x00 byte. Used by MIE EEPROM and NAOMI backup SRAM
    bookkeeping blocks.

    \param  buf             Data to checksum.
    \param  size            Length in bytes.
    \return                 CRC-16 value.

    \sa mie_eeprom_fix_crc
*/
uint16_t mie_eeprom_crc16(const uint8_t *buf, size_t size);

/** \brief   Recompute NAOMI EEPROM CRC fields in a 128-byte buffer.
    \ingroup mie_device

    Updates the duplicated CRC16 headers for the system-settings block
    (bytes \c 0–\c 17) and the game-settings block (bytes \c 36–\c 43).

    \param  eeprom          Buffer to update (\p MIE_EEPROM_SIZE bytes).

    \sa mie_eeprom_crc16, mie_get_eeprom, mie_set_eeprom
*/
void mie_eeprom_fix_crc(uint8_t *eeprom);

/** \brief   Get the active driver calibration.
    \ingroup mie_analog

    \return                 Pointer to the current calibration. Use
                            mie_analog_calib_valid() to check whether it is usable.
*/
const mie_analog_calib_t *mie_analog_calib_get(void);

/** \brief   Install a calibration set.
    \ingroup mie_analog

    \param  calib           Calibration to copy into the driver. Invalid ranges
                            fall back to the legacy mapping.
*/
void mie_analog_calib_set(const mie_analog_calib_t *calib);

/** \brief   Check whether the active calibration has usable ranges.
    \ingroup mie_analog

    \retval true            Wheel and pedal ranges are valid.
    \retval false           At least one axis range is invalid.
*/
bool mie_analog_calib_valid(void);

/** \brief   Clear the active calibration and any running session.
    \ingroup mie_analog
*/
void mie_analog_calib_reset(void);

/** \brief   Start the interactive calibration session.
    \ingroup mie_analog

    Begins at \ref MIE_ANALOG_CALIB_WHEEL. While a session runs the driver tracks
    channel extremes automatically and feeds raw values to \ref mie_state_t::cont.
*/
void mie_analog_calib_start(void);

/** \brief   Cancel a running calibration session.
    \ingroup mie_analog

    The previously active calibration is left untouched.
*/
void mie_analog_calib_cancel(void);

/** \brief   Check whether a calibration session is running.
    \ingroup mie_analog

    \retval true            A session is active.
    \retval false           No session is running.
*/
bool mie_analog_calib_active(void);

/** \brief   Query the current calibration step.
    \ingroup mie_analog

    \return                 The active \ref mie_analog_calib_step_t.
*/
mie_analog_calib_step_t mie_analog_calib_current(void);

/** \brief   Confirm the current step and advance the session.
    \ingroup mie_analog

    For the wheel and pedal range steps the tracked extremes are kept. For the
    center step the latest raw wheel value is stored. On the final step the
    result is validated and, when valid, installed as the active calibration.

    \retval true            The session finished. Inspect mie_analog_calib_get()
                            for the result (unchanged if invalid).
    \retval false           Advanced to the next step.
*/
bool mie_analog_calib_capture(void);

/** \brief   Normalize a raw JVS wheel value.
    \ingroup mie_analog

    Uses the active calibration when valid, otherwise the built-in legacy
    mapping. Does not pass raw values through during a calibration session;
    use \ref mie_state_t::jvs or \ref mie_state_t::cont for live capture.

    \return                 Normalized wheel position (-128 to 127).
*/
int mie_analog_norm_wheel(uint16_t raw);

/** \brief   Normalize a raw JVS accelerator value.
    \ingroup mie_analog

    \return                 Normalized pedal position (0 to 255).
*/
int mie_analog_norm_accel(uint16_t raw);

/** \brief   Normalize a raw JVS brake value.
    \ingroup mie_analog

    \return                 Normalized pedal position (0 to 255).
*/
int mie_analog_norm_brake(uint16_t raw);

/** \brief   Initialize MIE/JVS, uploading Z80 firmware when needed.
    \ingroup mie_device

    Registers the driver if needed, probes whether the Z80 bridge is already
    active (for example after the original NAOMI BIOS or DreamShell),
    and uploads \p fw_path when JVS communication is not yet available.

    Intended for early boot on NAOMI hardware before the normal Maple scan
    completes JVS setup. Returns false on retail Dreamcast.

    \param  fw_path         Path to mie_z80.bin firmware image.
    \retval true            Z80/JVS initialization succeeded.
    \retval false           Invalid path, not NAOMI hardware, or init failed.

    \sa mie_init_scan
*/
bool mie_init_fw(const char *fw_path);

/* \cond */
void mie_init(void);
void mie_shutdown(void);
void mie_init_scan(void);
void mie_scan_complete(void);
/* \endcond */

/** @} */

__END_DECLS

#endif
