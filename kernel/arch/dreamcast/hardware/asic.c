/* KallistiOS ##version##

   asic.c
   Copyright (c)2000,2001,2002,2003 Megan Potter
*/

/*
   This module contains low-level ASIC handling. Right now this is just for
   ASIC interrupts, but it will eventually include DMA as well.

   The DC's System ASIC is integrated with the 3D chip and serves as the
   Grand Central Station for the interaction of all the various peripherals
   (which really means that it allows the SH-4 to control all of them =).
   The basic block diagram looks like this:

   +-----------+    +--------+    +-----------------+
   | 16MB Ram  |    |        |----| 8MB Texture Ram |
   +-----------+    | System |    +-----------------+
      |             |  ASIC  |    +--------------------+  +-------------+
      +-------------+        +-+--+    AICA SPU        |--+ 2MB SPU RAM |
      |A            | PVR2DC | |  +-------------------++  +-------------+
   +-------+        |        | |C +-----------------+ |
   | SH-4  |        |        | \--+ Expansion Port  | |
   +-------+        +---+----+    +-----------------+ |
                        |B        +------------+      |D
                        +---------+   GD-Rom   +------/
                        |         +------------+
                        |         +----------------------+
                        \---------+ 2MB ROM + 256K Flash |
                                  +----------------------+

   A: Main system bus -- connects the SH-4 to the ASIC and its main RAM
   B: "G1" bus -- connects the ASIC to the GD-Rom and the ROM/Flash
   C: "G2" bus -- connects the ASIC to the SPU and Expansion port
   D: Not entirely verified connection for streaming audio data

   All buses can simultaneously transmit data via PIO and/or DMA. This
   is where the ridiculous bandwidth figures come from that you see in
   marketing literature. In reality, each bus isn't terribly fast on
   its own -- they tend to have high bandwidth but high latency.

   The "G2" bus is notoriously flaky. Specifically, one should ensure
   to write the proper data size for the peripheral you are accessing
   (32-bits for SPU, 8-bits for 8-bit peripherals, etc). Every 8
   32-bit words written to the SPU must be followed by a g2_fifo_wait().
   Additionally, if SPU or Expansion Port DMA is being used, only one
   of these may proceed at once and any PIO access _must_ pause the
   DMA and disable interrupts. Any other treatment may cause serious
   data corruption between the ASIC and the G2 peripherals.

   For more information on all of this see:

   http://www.segatech.com/technical/dcblock/index.html
   http://mc.pp.se/dc/

 */

/* Small interrupt listing (from the two Marcus's =)

  691x -> irq 13
  692x -> irq 11
  693x -> irq 9

  69x0
    bit 2   render complete
        3   scanline 1
        4   scanline 2
        5   vsync
        7   opaque polys accepted
        8   opaque modifiers accepted
        9   translucent polys accepted
        10  translucent modifiers accepted
        12  maple dma complete
        13  maple error(?)
        14  gd-rom dma complete
        15  aica dma complete
        16  external dma 1 complete
        17  external dma 2 complete
        18  ??? dma complete
        21  punch-thru polys accepted

  69x4
    bit 0   gd-rom command status
        1   AICA
        2       modem?
        3       expansion port (PCI bridge)

  69x8
    bit 2   out of primitive memory
        3   out of matrix memory
        12  gd-rom dma illegal address
        13  gd-rom dma overrun

 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <arch/irq.h>
#include <dc/asic.h>
#include <arch/spinlock.h>
#include <kos/genwait.h>
#include <kos/regfield.h>
#include <kos/worker_thread.h>

/* XXX These based on g1ata.c and pvr.h and should be replaced by a standardized method */
#define IN32(addr)         (* ( (volatile uint32_t *)(addr) ) )
#define OUT32(addr, data)  IN32(addr) = (data)

/* The set of asic regs are spaced by 0x10 with 0x4 between each sub reg */
#define ASIC_EVT_REG_ADDR(irq, sub) (ASIC_IRQD_A + ((irq) * 0x10) + ((sub) * 0x4))

#define ASIC_EVT_REGS 3
#define ASIC_EVT_REG_HNDS 32

struct asic_thdata {
    asic_evt_handler hdl;
    uint32_t source;
    kthread_worker_t *worker;
    void *data;
    void (*ack_and_mask)(uint16_t);
    void (*unmask)(uint16_t);
};

/* Exception table -- this table matches each potential G2 event to a function
   pointer. If the pointer is null, then nothing happens. Otherwise, the
   function will handle the exception. */
static asic_evt_handler_entry_t
asic_evt_handlers[ASIC_EVT_REGS][ASIC_EVT_REG_HNDS];

/* Set a handler, or remove a handler */
asic_evt_handler_entry_t asic_evt_set_handler(uint16_t code, asic_evt_handler hnd, void *data) {
    uint8_t evtreg, evt;
    asic_evt_handler_entry_t old;

    evtreg = (code >> 8) & 0xff;
    evt = code & 0xff;

    assert((evtreg < ASIC_EVT_REGS) && (evt < ASIC_EVT_REG_HNDS));

    old = asic_evt_handlers[evtreg][evt];
    asic_evt_handlers[evtreg][evt] = (asic_evt_handler_entry_t){ hnd, data };
    return old;
}

/* The ASIC event handler; this is called from the global IRQ handler
   to handle external IRQ 9. */
static void handler_irq9(irq_t source, irq_context_t *context, void *data) {
    const asic_evt_handler_entry_t (*const handlers)[ASIC_EVT_REG_HNDS] = data;
    const asic_evt_handler_entry_t *entry;
    uint8_t reg, i;

    (void)source;
    (void)context;

    /* Go through each event register and look for pending events */
    for(reg = 0; reg < ASIC_EVT_REGS; reg++) {
        /* Read the event mask and clear pending */
        uint32_t mask = IN32(ASIC_ACK_A + (reg * 0x4));
        OUT32(ASIC_ACK_A + (reg * 0x4), mask);

        /* Short circuit going through the table if none on this reg */
        if(mask == 0) continue;

        /* Search for relevant handlers */
        for(i = 0; i < ASIC_EVT_REG_HNDS; i++) {
            entry = &handlers[reg][i];

            if((mask & BIT(i)) && entry->hdl != NULL)
                entry->hdl((reg << 8) | i, entry->data);
        }
    }
}

/* Disable all G2 events */
void asic_evt_disable_all(void) {
    uint8_t irq, sub;

    for(irq = 0; irq < ASIC_IRQ_MAX; irq++) {
        for(sub = 0; sub < ASIC_EVT_REGS; sub++) {
            OUT32(ASIC_EVT_REG_ADDR(irq, sub), 0);
        }
    }
}

/* Disable a particular G2 event */
void asic_evt_disable(uint16_t code, uint8_t irqlevel) {
    assert(irqlevel < ASIC_IRQ_MAX);

    uint8_t evtreg, evt;

    evtreg = (code >> 8) & 0xff;
    evt = code & 0xff;

    uint32_t addr = ASIC_EVT_REG_ADDR(irqlevel, evtreg);
    uint32_t val = IN32(addr);
    OUT32(addr, val & ~BIT(evt));
}

/* Enable a particular G2 event */
void asic_evt_enable(uint16_t code, uint8_t irqlevel) {
    assert(irqlevel < ASIC_IRQ_MAX);

    uint8_t evtreg, evt;

    evtreg = (code >> 8) & 0xff;
    evt = code & 0xff;

    uint32_t addr = ASIC_EVT_REG_ADDR(irqlevel, evtreg);
    uint32_t val = IN32(addr);
    OUT32(addr, val | BIT(evt));
}

/* Initialize events */
static void asic_evt_init(void) {
    /* Clear any pending interrupts and disable all events */
    asic_evt_disable_all();
    OUT32(ASIC_ACK_A, 0xffffffff);
    OUT32(ASIC_ACK_B, 0xffffffff);
    OUT32(ASIC_ACK_C, 0xffffffff);

    /* Clear out the event table */
    memset(asic_evt_handlers, 0, sizeof(asic_evt_handlers));

    /* Hook IRQ9,B,D */
    irq_set_handler(EXC_IRQ9, handler_irq9, asic_evt_handlers);
    irq_set_handler(EXC_IRQB, handler_irq9, asic_evt_handlers);
    irq_set_handler(EXC_IRQD, handler_irq9, asic_evt_handlers);
}

/* Shutdown events */
static void asic_evt_shutdown(void) {
    /* Disable all events */
    asic_evt_disable_all();

    /* Unhook handlers */
    irq_set_handler(EXC_IRQ9, NULL, NULL);
    irq_set_handler(EXC_IRQB, NULL, NULL);
    irq_set_handler(EXC_IRQD, NULL, NULL);
}

/* Init routine */
void asic_init(void) {
    asic_evt_init();
}

void asic_shutdown(void) {
    asic_evt_shutdown();
}

static void asic_threaded_irq(void *data) {
    struct asic_thdata *thdata = data;

    thdata->hdl(thdata->source, thdata->data);

    if(thdata->unmask)
        thdata->unmask(thdata->source);
}

static void asic_thirq_dispatch(uint32_t source, void *data) {
    struct asic_thdata *thdata = data;

    if(thdata->ack_and_mask)
        thdata->ack_and_mask(source);

    thdata->source = source;

    thd_worker_wakeup(thdata->worker);
}

int asic_evt_request_threaded_handler(uint16_t code, asic_evt_handler hnd,
                                      void *data,
                                      void (*ack_and_mask)(uint16_t),
                                      void (*unmask)(uint16_t))
{
    struct asic_thdata *thdata;
    uint32_t flags;
    kthread_t *thd;

    thdata = malloc(sizeof(*thdata));
    if(!thdata)
        return -1; /* TODO: What return code? */

    thdata->hdl = hnd;
    thdata->data = data;
    thdata->ack_and_mask = ack_and_mask;
    thdata->unmask = unmask;

    flags = irq_disable();

    thdata->worker = thd_worker_create(asic_threaded_irq, thdata);
    if(!thdata->worker) {
        irq_restore(flags);
        free(thdata);
        return -1; /* TODO: What return code? */
    }

    /* Set a reasonable name to ID the thread */
    thd = thd_worker_get_thread(thdata->worker);
    snprintf(thd->label, KTHREAD_LABEL_SIZE,
             "Threaded IRQ code: 0x%x evt: 0x%.4x",
             ((code >> 16) & 0xf), (code & 0xffff));

    /* Highest priority */
    //thd_set_prio(thd, 0);

    asic_evt_set_handler(code, asic_thirq_dispatch, thdata);

    irq_restore(flags);

    return 0;
}

void asic_evt_remove_handler(uint16_t code)
{
    asic_evt_handler_entry_t entry;
    struct asic_thdata *thdata;
    uint8_t evtreg, evt;

    evtreg = (code >> 8) & 0xff;
    evt = code & 0xff;

    entry = asic_evt_handlers[evtreg][evt];
    asic_evt_set_handler(code, NULL, NULL);

    if(entry.hdl == asic_thirq_dispatch) {
        thdata = entry.data;

        thd_worker_destroy(thdata->worker);
        free(thdata);
    }
}
