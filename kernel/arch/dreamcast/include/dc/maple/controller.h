/* KallistiOS ##version##

   dc/maple/controller.h
   (C)2000-2002 Jordan DeLong, Megan Potter

   Thanks to Marcus Comstedt for information on the controller.
*/

/** \file   dc/maple/controller.h
    \brief  Definitions for using the controller device.

    This file contains the definitions needed to access the Maple controller
    device. Obviously, this corresponds to the MAPLE_FUNC_CONTROLLER function
    code.

    \author Jordan DeLong
    \author Megan Potter
*/

#ifndef __DC_MAPLE_CONTROLLER_H
#define __DC_MAPLE_CONTROLLER_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/** \defgroup   controller_buttons  Controller button codes

    This is the set of all buttons that are valid for a maple bus controller
    device. The names should make it obvious what the button is.

    @{
*/
#define CONT_C          (1<<0)
#define CONT_B          (1<<1)
#define CONT_A          (1<<2)
#define CONT_START      (1<<3)
#define CONT_DPAD_UP        (1<<4)
#define CONT_DPAD_DOWN      (1<<5)
#define CONT_DPAD_LEFT      (1<<6)
#define CONT_DPAD_RIGHT     (1<<7)
#define CONT_Z          (1<<8)
#define CONT_Y          (1<<9)
#define CONT_X          (1<<10)
#define CONT_D          (1<<11)
#define CONT_DPAD2_UP       (1<<12)
#define CONT_DPAD2_DOWN     (1<<13)
#define CONT_DPAD2_LEFT     (1<<14)
#define CONT_DPAD2_RIGHT    (1<<15)
/** @} */

/* Raw controller condition structure */
typedef struct {
    uint16 buttons;         /* buttons bitfield */
    uint8 rtrig;            /* right trigger */
    uint8 ltrig;            /* left trigger */
    uint8 joyx;         /* joystick X */
    uint8 joyy;         /* joystick Y */
    uint8 joy2x;            /* second joystick X */
    uint8 joy2y;            /* second joystick Y */
} cont_cond_t;

/* Our more civilized structure. Note that there are some fairly
   significant differences in value interpretations here between
   this struct and the old one:

   - "buttons" is zero based: a 1-bit means the button is PRESSED.
   - "joyx", "joyy", "joy2x", and "joy2y" are all zero based: zero
     means they are in the center, negative values are up/left,
     and positive values are down/right.

   Note also that this is the struct you will get back if you call
   maple_dev_status(), NOT a cont_cond_t. However, cont_get_cond() will
   return the old compatible struct for now. */

/** \brief  Controller status structure.

    This structure contains information about the status of the controller
    device and can be fetched with maple_dev_status().

    A 1 bit in the buttons bitfield indicates that a button is pressed, and the
    joyx, joyy, joy2x, joy2 values are all 0 based (0 is centered).

    \headerfile dc/maple/controller.h
*/
typedef struct {
    /** \brief  Buttons bitfield.
        \see    controller_buttons
    */
    uint32  buttons;

    /** \brief  Left trigger value. */
    int ltrig;

    /** \brief  Right trigger value. */
    int rtrig;

    /** \brief  Main joystick x-axis value. */
    int joyx;

    /** \brief  Main joystick y-axis value. */
    int joyy;

    /** \brief  Secondary joystick x-axis value (if applicable). */
    int joy2x;

    /** \brief  Secondary joystick y-axis value (if applicable). */
    int joy2y;
} cont_state_t;

/* \cond */
/* Init / Shutdown */
int cont_init(void);
void cont_shutdown(void);
/* \endcond */

/** \brief  Controller automatic callback type.

    Functions of this type can be set with cont_btn_callback() to respond
    automatically to the specified set of buttons being pressed. This can be
    used, for instance, to implement the standard A+B+X+Y+Start method of ending
    the program running.
*/
typedef void (*cont_btn_callback_t)(uint8 addr, uint32 btns);

/** \brief  Set an automatic button press callback.

    This function sets a callback function to be called when the specified
    controller has the set of buttons given pressed. Only one callback
    is stored globally, so if this function is called multiple times,
    subsequent calls overwrite the effects of previous ones.

    \param  addr            The controller to listen on. This value can be
                            obtained by using maple_addr(). The value 0 can
                            be used to listen on all controllers.
    \param  btns            The buttons bitmask to match.
    \param  cb              The callback to call when the buttons are pressed.
*/
void cont_btn_callback(uint8 addr, uint32 btns, cont_btn_callback_t cb);

/** \defgroup   controller_caps Controller capability bits.

    These bits will be set in the function_data for the controller's deviceinfo
    if the controller supports the corresponding feature.

    @{
*/
#define CONT_CAPABILITY_C               (1<<24)
#define CONT_CAPABILITY_B               (1<<25)
#define CONT_CAPABILITY_A               (1<<26)
#define CONT_CAPABILITY_START           (1<<27)
#define CONT_CAPABILITY_DPAD_UP         (1<<28)
#define CONT_CAPABILITY_DPAD_DOWN       (1<<29)
#define CONT_CAPABILITY_DPAD_LEFT       (1<<30)
#define CONT_CAPABILITY_DPAD_RIGHT      (1<<31)
#define CONT_CAPABILITY_Z               (1<<16)
#define CONT_CAPABILITY_Y               (1<<17)
#define CONT_CAPABILITY_X               (1<<18)
#define CONT_CAPABILITY_D               (1<<19)
#define CONT_CAPABILITY_DPAD2_UP        (1<<20)
#define CONT_CAPABILITY_DPAD2_DOWN      (1<<21)
#define CONT_CAPABILITY_DPAD2_LEFT      (1<<22)
#define CONT_CAPABILITY_DPAD2_RIGHT     (1<<23)
#define CONT_CAPABILITY_RTRIG           (1<<8)
#define CONT_CAPABILITY_LTRIG           (1<<9)
#define CONT_CAPABILITY_ANALOG_X        (1<<10)
#define CONT_CAPABILITY_ANALOG_Y        (1<<11)
#define CONT_CAPABILITY_ANALOG2_X       (1<<12)
#define CONT_CAPABILITY_ANALOG2_Y       (1<<13)
/** @} */

__END_DECLS

#endif  /* __DC_MAPLE_CONTROLLER_H */

