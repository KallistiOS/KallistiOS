/* KallistiOS ##version##

   kernel/arch/dreamcast/hardware/ubc.c
   Copyright (C) 2024 Falco Girgis
*/

#include <dc/ubc.h>

#include <arch/memory.h>
#include <arch/irq.h>

#include <kos/dbglog.h>

#include <string.h>
#include <assert.h>

/* Macros for accessing SFRs common to both channels by index */
#define BAR(o)  (*((vuint32 *)(uintptr_t)(SH4_REG_UBC_BARA  + (unsigned)(o) * 0xc))) /* Address */
#define BASR(o) (*((vuint8  *)(uintptr_t)(SH4_REG_UBC_BASRA + (unsigned)(o) * 0x4))) /* ASID */
#define BAMR(o) (*((vuint8  *)(uintptr_t)(SH4_REG_UBC_BAMRA + (unsigned)(o) * 0xc))) /* Address Mask */
#define BBR(o)  (*((vuint16 *)(uintptr_t)(SH4_REG_UBC_BBRA  + (unsigned)(o) * 0xc))) /* Bus Cycle */

/* Macros for accessing individual, channel-specific SFRs */
#define BARA  (BAR(ubc_channel_a))              /**< Break Address A */
#define BASRA (BASR(ubc_channel_a))             /**< Break ASID A */
#define BAMRA (BAMR(ubc_channel_a))             /**< Break Address Mask A */
#define BBRA  (BBR(ubc_channel_a))              /**< Break Bus Cycle A */
#define BARB  (BAR(ubc_channel_b))              /**< Break Address B */
#define BASRB (BASR(ubc_channel_b))             /**< Break ASID B */
#define BAMRB (BAMR(ubc_channel_b))             /**< Break Address Mask B */
#define BBRB  (BBR(ubc_channel_b))              /**< Break Bus Cycle B */
#define BDRB  (*((vuint32 *)SH4_REG_UBC_BDRB))  /**< Break Data B */
#define BDMRB (*((vuint32 *)SH4_REG_UBC_BDMRB)) /**< Break Data Mask B */
#define BRCR  (*((vuint16 *)SH4_REG_UBC_BRCR))  /**< Break Control */

/* BASR Fields */
#define BASM            (1 << 2)             /* No ASID (1), use ASID (0) */
#define BAM_BIT_HIGH    3                    /* High bit number for address mask */
#define BAM_BITS        3                    /* Number of bits in address mask */
#define BAM_HIGH        (1 << BAM_BIT_HIGH)  /* Mask for high bit of BAM */
#define BAM_LOW         0x3                  /* Mask for low bits of BAM */
#define BAM             (BAM_HIGH | BAM_LOW) /* Mask for all bits of BAM */

/* BBR Fields */
#define ID_BIT          4                  /* Bit position for access cycle field */
#define ID              (3 << ID_BIT)      /* Bit mask for access cycle field */
#define RW_BIT          2                  /* Bit position for R/W field */
#define RW              (3 << RW_BIT)      /* Bit mask for R/W field */
#define SZ_BIT_HIGH     6                  /* High bit for size field */
#define SZ_BITS         3                  /* Number of bits in size field */
#define SZ_HIGH         (1 << SZ_BIT_HIGH) /* Mask for high bit in size field */
#define SZ_LOW          0x3                /* Mask for low bits in size field */
#define SZ              (SZ_HIGH | SZ_LOW) /* Mask for all bits in size field */

/* BRCR Fields */
#define CMFA    (1 << 15) /* Set when break condition A is met (not cleared) */
#define CMFB    (1 << 14) /* Set when break condition B is met (not cleared) */
#define PCBA    (1 << 10) /* Channel A instruction break after (1) or before (0) execution */
#define DBEB    (1 << 7)  /* Include Data Bus condition for channel B */
#define PCBB    (1 << 6)  /* Channel B instruction break after (1) or before (0) execution */
#define SEQ     (1 << 3)  /* A, B are independent (0), or sequential [A->B] (1) */
#define UBDE    (1 << 0)  /* Use debug function in DBR register rather than IRQ handler */

/* Channel types */
typedef enum ubc_channel {
    ubc_channel_a,      
    ubc_channel_b,     
    ubc_channel_count
} ubc_channel_t;

/* Internal state data for each channel. */
static struct ubc_channel_state {
    const ubc_breakpoint_t *bp;
    ubc_break_func_t        cb;
    void                   *ud;
} channel_state[ubc_channel_count] = { 0 };

/* Translates ubc_access_t to BBR.ID field format. */
inline static uint8_t __pure get_access_mask(ubc_access_t access) {
    switch(access) {
    case ubc_access_either:
        return 0x3;
    default:
        return access;
    }
}

/* Translates ubc_rw_t to BBR.RW field format. */
inline static uint8_t __pure get_rw_mask(ubc_rw_t rw) {
    switch(rw) {
    case ubc_rw_either:
        return 0x3;
    default:
        return rw;
    }
}

/* Translates ubc_address_mask_t to BASR.BAM field format. */
inline static uint8_t __pure get_address_mask(ubc_address_mask_t addr_mask) {
    switch(addr_mask) {
        case ubc_address_mask_all:
            return 3;
        case ubc_address_mask_16:
            return 4;
        case ubc_address_mask_20:
            return 5;
        default:
            return addr_mask;
    }
}

/* Stalls the pipeline while the UBC refreshes after changing its config. */
static void ubc_wait(void) {
    __asm__ __volatile__("nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n");
}

/* Sets the state data and configures the UBC registers for the breakpoint on
   the given channel. */
static void enable_breakpoint(ubc_channel_t           ch,
                              const ubc_breakpoint_t *bp,
                              ubc_break_func_t        cb,
                              void                   *ud) {
    /* Set state variables. */
    channel_state[ch].bp = bp;
    channel_state[ch].cb = cb;
    channel_state[ch].ud = ud;

    /* Configure Registers. */

    /* Address */
    BAR(ch) = (uintptr_t)bp->address;

    /* Address mask */
    const uint8_t address_mask = get_address_mask(bp->address_mask);

    BAMR(ch) = ((address_mask << (BAM_BIT_HIGH - (BAM_BITS - 1))) & BAM_HIGH) |
                (address_mask & BAM_LOW);

    /* ASID */
    if(bp->asid.enabled) {
        /* ASID value */
        BASR(ch) = bp->asid.value;
        /* ASID enable */
        BAMR(ch) &= ~BASM;
    }
    else {
        /* ASID disable */
        BAMR(ch) |= BASM;
    }

    /* Data (channel B only) */
    if(bp->operand.data.enabled) {
        /* Data value */
        BDRB = bp->operand.data.value;
        /* Data mask */
        BDMRB = bp->operand.data.mask;
        /* Data enable */
        BRCR |= DBEB;
    }
    else {
        /* Data disable */
        BRCR &= ~DBEB;
    }

    /* Instruction */
    if(bp->instruction.break_before)
        /* Instruction break before */
        BRCR &= (ch == ubc_channel_a)? (~PCBA) : (~PCBB);
    else
        /* Instruction break after */
        BRCR |= (ch == ubc_channel_a)? PCBA : PCBB;

    /* Set Access Type */
    BBR(ch) |= get_access_mask(bp->access) << ID_BIT;
    /* Read/Write type */
    BBR(ch) |= get_rw_mask(bp->operand.rw) << RW_BIT;
    /* Size type */
    BBR(ch) |= ((bp->operand.size << (SZ_BIT_HIGH - (SZ_BITS - 1))) & SZ_HIGH)
            |   (bp->operand.size & SZ_LOW);

    /* Wait for UBC configuration to refresh. */
    ubc_wait();
}

/* Public entry-point for adding a breakpoint. */
bool ubc_add_breakpoint(const ubc_breakpoint_t *bp,
                        ubc_break_func_t       callback,
                        void                   *user_data) {
    /* Check if we're dealing with a combined sequential breakpoint */
    if(bp->next) {
        /* Ensure we only have a sequence of 2, without leading data break. */
        if(bp->next->next || bp->operand.data.enabled)
            return false;

        /* Ensure we have both channels free */
        if(channel_state[ubc_channel_a].bp || channel_state[ubc_channel_b].bp)
            return false;

        enable_breakpoint(ubc_channel_a, bp, callback, user_data);
        enable_breakpoint(ubc_channel_b, bp->next, callback, user_data);

        /* Configure both channels to run sequentially*/
        BRCR |= SEQ;
    }
    /* Handle single-channel */ 
    else {
        /* Check if we require channel B */
        if(bp->operand.data.enabled) {
            /* Check if the channel is free */
            if(channel_state[ubc_channel_b].bp)
                return false;

            enable_breakpoint(ubc_channel_b, bp, callback, user_data);
        } 
        /* We can take either channel */ 
        else {
            /* Take whichever channel is free. */
            if(!channel_state[ubc_channel_a].bp)
                enable_breakpoint(ubc_channel_a, bp, callback, user_data);
            else if(!channel_state[ubc_channel_b].bp)
                enable_breakpoint(ubc_channel_b, bp, callback, user_data);
            else
                return false;
        }

        /* Configure both channels to run independently. */
        BRCR &= ~SEQ;
    }

    /* Wait for UBC config to refresh. */
    ubc_wait();

    return true;
}

/* Clears the register config and static state data for the given channel. */
static void disable_breakpoint(ubc_channel_t ch) {
    /* Clear UBC conditions for the given channel. */
    BBR(ch)  = 0;
    BAMR(ch) = 0;
    BASR(ch) = 0;
    BAR(ch)  = 0;

    /* Wait for UBC config to refresh. */
    ubc_wait();

    /* Check whether we have a breakpoint without data-comparison on channel B,
       which should get migrated to channel A, in case a data watchpoint gets
       added later, so it can use channel B. */
    if(ch == ubc_channel_a &&
       channel_state[ubc_channel_b].bp &&
       !channel_state[ubc_channel_b].bp->operand.data.enabled) {

        /* Copy channel B breakpoint to channel A. */
        enable_breakpoint(ubc_channel_a,
                          channel_state[ubc_channel_b].bp,
                          channel_state[ubc_channel_b].cb,
                          channel_state[ubc_channel_b].ud);

        /* Disable original breakpoint on channel B. */
        disable_breakpoint(ubc_channel_b);
    }
    else { /* Clear our state for the given channel. */
        memset(&channel_state[ch], 0, sizeof(struct ubc_channel_state));
    }
}

/* Finds the channel for a given breakpoint and disables it. */
bool ubc_remove_breakpoint(const ubc_breakpoint_t *bp) {
    /* Disabling a sequential breakpoint pair */
    if(bp->next) {
        if(channel_state[ubc_channel_a].bp == bp &&
           channel_state[ubc_channel_b].bp == bp->next) {
            /* Clear both channels */
            disable_breakpoint(ubc_channel_b);
            disable_breakpoint(ubc_channel_a);

            /* Return success, we found it. */
            return true;
        }
    }
    /* Disable single, non-sequential breakpoint */
    else {
        /* Search each channel */
        for(unsigned ch = 0; ch < ubc_channel_count; ++ch) {
            /* Check if it has the given breakpoint */
            if(channel_state[ch].bp == bp) {
                disable_breakpoint(ch);

                /* Return success, we found it. */
                return true;
            }
        }
    }

    /* We never found your breakpoint! */
    return false;
}

/* Disables all breakpoints. */
void ubc_clear_breakpoints(void) {
    disable_breakpoint(ubc_channel_b);
    disable_breakpoint(ubc_channel_a);
}

/* Entry-point for UBC-related interrupt handling. */
static void handle_exception(irq_t code, irq_context_t *irq_ctx, void *data) {
    struct ubc_channel_state *state = data;
    bool serviced = false;

    (void)code;

    /* Check if channel B's condition is active. */
    if(BRCR & CMFB) {
        bool disable = false;

        /* Invoke the user's callback if there is one. */
        if(state[ubc_channel_b].cb)
            disable = state[ubc_channel_b].cb(
                            state[ubc_channel_b].bp,
                            irq_ctx,
                            state[ubc_channel_b].ud);

        /* Check whether the breakpoint should disable itself. */
        if(disable) {
            disable_breakpoint(ubc_channel_b);

            /* Disable the previous breakpoint too if it's a sequence. */
            if(BRCR & SEQ)
                disable_breakpoint(ubc_channel_a);
        }

        serviced = true;
    }

    /* Check if channel A's condition is active. */
    if(BRCR & CMFA) {
        /* Only proceed if channel A is not part of a sequence, in which case
           it should already be handled above by channel B's logic. */
        if(!(BRCR & SEQ)) {
            bool disable = false;

            /* Invoke the user's callback if there is one. */
            if(channel_state[ubc_channel_a].cb)
                disable = channel_state[ubc_channel_a].cb(
                                channel_state[ubc_channel_a].bp,
                                irq_ctx,
                                channel_state[ubc_channel_a].ud);

            /* Check whether the breakpoint should disable itself. */
            if(disable)
                disable_breakpoint(ubc_channel_a);
        }

        serviced = true;
    }

    /* Check whether we have an unhandled break request. */
    if(!serviced)
        dbglog(DBG_WARNING, "Unhandled UBC break request!\n");

    /* Reset condition flags for both channels. */
    BRCR &= ~(CMFA | CMFB);

    /* Wait for UBC configuration change to take effect. */
    ubc_wait();
}

/* UBC initialization routine called upon KOS init. */
void ubc_init(void) {
    /* Clear any accidental remaining breakpoints. */
    ubc_clear_breakpoints();

    /* Clear the control register and wait for it to take effect. */
    BRCR = 0;
    ubc_wait();

    /* Install our exception handler for the UBC exception types. */
    irq_set_handler(EXC_USER_BREAK_PRE, handle_exception, channel_state);
    irq_set_handler(EXC_USER_BREAK_POST, handle_exception, channel_state);
}

/* UBC shutdown routine called when exiting KOS. */
void ubc_shutdown(void) {
    /* Clear any remaining breakpoints. */
    ubc_clear_breakpoints();

    /* Clear the control register and wait for it to take effect. */
    BRCR = 0;
    ubc_wait();

    /* Uninstall our exception handler from the UBC exception types. */
    irq_set_handler(EXC_USER_BREAK_PRE, NULL, NULL);
    irq_set_handler(EXC_USER_BREAK_POST, NULL, NULL);
}

