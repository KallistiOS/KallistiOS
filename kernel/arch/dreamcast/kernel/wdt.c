/* KallistiOS ##version##

   arch/dreamcast/kernel/wdt.c
   Copyright (C) 2023 Falco Girgis
*/

/* This file contains the implementation of the SH4 watchdog timer driver. */

#include <arch/wdt.h>
#include <arch/irq.h>
#include <kos/regfield.h>

/* Macros for accessing WDT registers */
#define WDT(o, t)       (*((volatile t *)(WDT_BASE + (o))))
#define WDT_READ(o)     (WDT(o, uint8_t))
#define WDT_WRITE(o, v) (WDT(o, uint16_t) = ((o##_HIGH << 8) | ((v) & 0xff)))

/* Constants for WDT register access */
#define WDT_BASE        0xffc00008  /* Base address for WDT registers */
#define WTCNT_HIGH      0x5a        /* High byte value when writing to WTCNT */
#define WTCSR_HIGH      0xa5        /* Low byte value when writing to WTCSR */

/* Offsets of WDT registers */
#define WTCNT           0x0 /* Watchdog Timer Counter */ 
#define WTCSR           0x4 /* Watchdog Timer Control/Status */

/* WDT Control/Status Register flags */
#define WTCSR_TME       7   /* Timer Enable */
#define WTCSR_WTIT      6   /* Timer Mode Select */
#define WTCSR_RSTS      5   /* Reset Select */
#define WTCSR_WOVF      4   /* Watchdog Timer Overflow Flag */
#define WTCSR_IOVF      3   /* Interval Timer Overflow Flag */
#define WTCSR_CKS2      2   /* Clock Select (3 bits) */
#define WTCSR_CKS1      1
#define WTCSR_CKS0      0

/* Default values for interval timer mode */
#define WDT_CLK_DEFAULT WDT_CLK_DIV_32  /* Interval timer mode clock divider */
#define WDT_INT_DEFAULT 41              /* Interval timer mode period (us) */

/* Interval timer state data */
static void *user_data = NULL;
static wdt_callback callback = NULL;
static uint32_t us_interval = 0;
static uint32_t us_elapsed = 0;

/* Interval timer mode interrupt handler */
static void wdt_isr(irq_t src, irq_context_t *cxt, void *data) {
    (void)src;
    (void)cxt;
    (void)data;

    /* Update elapsed time */
    us_elapsed += WDT_INT_DEFAULT;
    
    /* Invoke user callback when enough time has elapsed */
    if(us_elapsed >= us_interval) { 
        callback(user_data);
        us_elapsed -= us_interval;
    }

    /* Reset interval timer overflow flag */
    WDT_WRITE(WTCSR, WDT_READ(WTCSR) & ~BIT(WTCSR_IOVF));
}

/* Enables the WDT in interval timer mode */
void wdt_enable_timer(uint8_t initial_count,
                      uint32_t micro_seconds,
                      uint8_t irq_prio,
                      wdt_callback callback_,
                      void *user_data_) {
    /* Initial WTCSR register configuration */
    const uint8_t wtcsr = WDT_CLK_DEFAULT;

    /* Stop WDT, enable interval timer mode, set clock divider */
    WDT_WRITE(WTCSR, wtcsr);

    /* Store user callback data for later */
    callback = callback_;
    user_data = user_data_;
    us_elapsed = 0;
    us_interval = micro_seconds;

    /* Register our interrupt handler */
    irq_set_handler(EXC_WDT_ITI, wdt_isr, NULL);

    /* Unmask the WDTIT interrupt, giving it a new priority */
    irq_set_priority(IRQ_SRC_WDT, irq_prio);

    /* Initialize WDT counter to starting value */
    WDT_WRITE(WTCNT, initial_count);

    /* Write same configuration plus the enable bit to start the WDT */
    WDT_WRITE(WTCSR, wtcsr | BIT(WTCSR_TME));
}

/* Enables the WDT in watchdog mode */
void wdt_enable_watchdog(uint8_t initial_count,
                         WDT_CLK_DIV clk_config,
                         WDT_RST reset_select) {
    /* Initial WTCSR register configuration */
    const uint8_t wtcsr = BIT(WTCSR_WTIT) |
                          (reset_select << WTCSR_RSTS) | 
                          clk_config;

    /* Stop WDT, enable watchdog mode, set reset type, set clock divider */
    WDT_WRITE(WTCSR, wtcsr);
    
    /* Initialize WDT counter to starting value */
    WDT_WRITE(WTCNT, initial_count);
    
    /* Write same configuration plus the enable bit to start the WDT */
    WDT_WRITE(WTCSR, wtcsr | BIT(WTCSR_TME));
}

/* Set the value of the WTCNT register */
void wdt_set_counter(uint8_t count) {
    WDT_WRITE(WTCNT, count);
}

/* Returns the value of the WTCNT register */
uint8_t wdt_get_counter(void) {
    return WDT_READ(WTCNT);
}

/* Resets the WTCNT register to 0 */
void wdt_pet(void) {
    wdt_set_counter(0);
}

/* Disables the WDT */
void wdt_disable(void) {
    /* Stop the WDT */
    WDT_WRITE(WTCSR, WDT_READ(WTCSR) & ~BIT(WTCSR_TME));

    /* Mask the WDTIT interrupt */
    irq_set_priority(IRQ_SRC_WDT, IRQ_PRIO_MASKED);

    /* Unregister our interrupt handler */
    irq_set_handler(EXC_WDT_ITI, NULL, NULL);

    /* Reset the WDT counter */
    wdt_pet();
}

/* Returns whether the WDT is enabled */
int wdt_is_enabled(void) {
    return WDT_READ(WTCSR) & BIT(WTCSR_TME);
}
