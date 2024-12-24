/* KallistiOS ##version##

   arch/dreamcast/kernel/irq.c
   Copyright (C) 2000-2001 Megan Potter
   Copyright (C) 2024 Paul Cercueil
   Copyright (C) 2024 Falco Girgis
   Copyright (C) 2024 Andy Barajas
*/

/* This module contains low-level handling for IRQs and related exceptions. */

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <arch/arch.h>
#include <arch/types.h>
#include <arch/irq.h>
#include <arch/timer.h>
#include <arch/stack.h>
#include <kos/dbgio.h>
#include <kos/thread.h>
#include <kos/library.h>

/* Macros for accessing related registers. */
#define TRA    ( *((volatile uint32_t *)(0xff000020)) ) /* TRAPA Exception Register */
#define EXPEVT ( *((volatile uint32_t *)(0xff000024)) ) /* Exception Event Register */
#define INTEVT ( *((volatile uint32_t *)(0xff000028)) ) /* Interrupt Event Register */

/* IRQ handler closure */
struct irq_cb {
    irq_handler hdl;
    void       *data;
};

/* TRAPA handler closure */
struct trapa_cb {
    trapa_handler hdl;
    void         *data;
};

/* Individual exception handlers */
static struct irq_cb   irq_handlers[0x100];
/* TRAPA exception handlers */
static struct trapa_cb trapa_handlers[0x100];

/* Global exception handler -- hook this if you want to get each and every
   exception; you might get more than you bargained for, but it can be useful. */
static struct irq_cb   global_irq_handler;

/* Default IRQ context location */
static irq_context_t    irq_context_default;

/* Are we inside an interrupt? */
static int inside_int;
int irq_inside_int(void) {
    return inside_int;
}

/* Set a handler, or remove a handler */
int irq_set_handler(irq_t code, irq_handler hnd, void *data) {
    /* Make sure they don't do something crackheaded */
    if(code >= 0x1000 || (code & 0x000f))
        return -1;

    code >>= 4;
    irq_handlers[code] = (struct irq_cb){ hnd, data };

    return 0;
}

/* Get the address of the current handler */
irq_handler irq_get_handler(irq_t code) {
    /* Make sure they don't do something crackheaded */
    if(code >= 0x1000 || (code & 0x000f))
        return NULL;

    code >>= 4;

    return irq_handlers[code].hdl;
}

/* Set a global handler */
int irq_set_global_handler(irq_handler hnd, void *data) {
    global_irq_handler.hdl = hnd;
    global_irq_handler.data = data;
    return 0;
}

/* Get the global exception handler */
irq_handler irq_get_global_handler(void) {
    return global_irq_handler.hdl;
}

/* Set or remove a trapa handler */
int trapa_set_handler(trapa_t code, trapa_handler hnd, void *data) {
    trapa_handlers[code] = (struct trapa_cb){ hnd, data };
    return 0;
}

/* Get a particular trapa handler */
trapa_handler trapa_get_handler(trapa_t code, void **data) {
    if(data)
        *data = trapa_handlers[code].data;

    return trapa_handlers[code].hdl;
}

/* Get a string description of the exception */
static char *irq_exception_string(irq_t evt) {
    switch(evt) {
        case EXC_ILLEGAL_INSTR:
            return "Illegal instruction";
        case EXC_SLOT_ILLEGAL_INSTR:
            return "Slot illegal instruction";
        case EXC_GENERAL_FPU:
            return "General FPU exception";
        case EXC_SLOT_FPU:
            return "Slot FPU exception";
        case EXC_DATA_ADDRESS_READ:
            return "Data address error (read)";
        case EXC_DATA_ADDRESS_WRITE:
            return "Data address error (write)";
        case EXC_DTLB_MISS_READ:  /* or EXC_ITLB_MISS */
            return "Instruction or Data(read) TLB miss";  
        case EXC_DTLB_MISS_WRITE:  
            return "Data(write) TLB miss";
        case EXC_DTLB_PV_READ:  /* or EXC_ITLB_PV */
            return "Instruction or Data(read) TLB protection violation";  
        case EXC_DTLB_PV_WRITE:
            return "Data TLB protection violation (write)";
        case EXC_FPU:
            return "FPU exception";
        case EXC_INITIAL_PAGE_WRITE:  
            return "Initial page write exception";  
        case EXC_TRAPA:  
            return "Unconditional trap (trapa)"; 
        case EXC_USER_BREAK_POST:  /* or EXC_USER_BREAK_PRE */
            return "User break";  
        default:  
            return "Unknown exception";
    }
}

/* Print a kernel panic reg dump */
extern irq_context_t *irq_srt_addr;
static void irq_dump_regs(int code, irq_t evt) {
    uint32_t fp;
    uint32_t *regs = irq_srt_addr->r;
    bool valid_pc;
    bool valid_pr;

    dbglog(DBG_DEAD, "Unhandled exception: PC %08lx, code %d, evt %04x\n",
           irq_srt_addr->pc, code, (uint16_t)evt);
    dbglog(DBG_DEAD, " R0-R7: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
           regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7]);
    dbglog(DBG_DEAD, " R8-R15: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
           regs[8], regs[9], regs[10], regs[11], regs[12], regs[13], regs[14], regs[15]);
    dbglog(DBG_DEAD, " SR %08lx PR %08lx\n", irq_srt_addr->sr, irq_srt_addr->pr);
    fp = regs[14];
    arch_stk_trace_at(fp, 0);
    
    if(code == 1) {
        dbglog(DBG_DEAD, "\nEncountered %s. ", irq_exception_string(evt)); 
        
        valid_pc = arch_valid_text_address(irq_srt_addr->pc);
        valid_pr = arch_valid_text_address(irq_srt_addr->pr);
        /* Construct template message only if either PC/PR address is valid */
        if(valid_pc || valid_pr) {
            dbglog(DBG_DEAD, "Use this template terminal command to help"
                " diagnose:\n\n\t$KOS_ADDR2LINE -f -C -i -e prog.elf");
            
            if(valid_pc)
                dbglog(DBG_DEAD, " %08lx", irq_srt_addr->pc);

            if(valid_pr)
                dbglog(DBG_DEAD, " %08lx", irq_srt_addr->pr);

#ifdef FRAME_POINTERS
            while(fp != 0xffffffff) {
                /* Validate the function pointer (fp) */
                if((fp & 3) || (fp < 0x8c000000) || (fp > _arch_mem_top))
                    break;

                /* Get the return address from the function pointer */
                fp = arch_fptr_ret_addr(fp);

                /* Validate the return address */
                if(!arch_valid_text_address(fp))
                    break;

                dbglog(DBG_DEAD, " %08lx", fp);
                fp = arch_fptr_next(fp);
            }
#endif
        }

        dbglog(DBG_DEAD, "\n");
    }
}

/* The C-level routine that processes context switching and other
   types of interrupts. NOTE: We are running on the stack of the process
   that was interrupted! */
volatile uint32_t jiffies = 0;
void irq_handle_exception(int code) {
    const struct irq_cb *hnd;
    uint32_t evt = 0;
    int handled = 0;

    switch(code) {
        /* If it's a code 0, well, we shouldn't be here. */
        case 0:
            arch_panic("spurious RESET exception");
            break;

        /* If it's a code 1 or 2, grab the event from expevt. */
        case 1:
        case 2:
            evt = EXPEVT;
            break;

        /* If it's a code 3, grab the event from intevt. */
        case 3:
            evt = INTEVT;
            break;
    }

    if(inside_int) {
        hnd = &irq_handlers[EXC_DOUBLE_FAULT >> 4];
        if(hnd->hdl != NULL)
            hnd->hdl(EXC_DOUBLE_FAULT, irq_srt_addr, hnd->data);
        else
            irq_dump_regs(code, evt);

        thd_pslist(dbgio_printf);
        // library_print_list(dbgio_printf);
        arch_panic("double fault");
    }

    /* Reveal this info about the int to inside_int for better 
       diagnostics returns if we try to do something in the int. */
    inside_int = ((code&0xf)<<16) | (evt&0xffff);

    /* If there's a global handler, call it */
    if(global_irq_handler.hdl) {
        global_irq_handler.hdl(evt, irq_srt_addr, global_irq_handler.data);
        handled = 1;
    }

    /* dbgio_printf("got int %04x %04x\n", code, evt); */

    /* If it's a timer interrupt, clear the status */
    if(evt >= EXC_TMU0_TUNI0 && evt <= EXC_TMU2_TUNI2) {
        if(evt == EXC_TMU0_TUNI0) {
            timer_clear(TMU0);
        }
        else if(evt == EXC_TMU1_TUNI1) {
            timer_clear(TMU1);
        }
        else {
            timer_clear(TMU2);
        }

        handled = 1;
    }

    /* If there's a handler, call it */
    {
        hnd = &irq_handlers[evt >> 4];
        if(hnd->hdl != NULL) {
            hnd->hdl(evt, irq_srt_addr, hnd->data);
            handled = 1;
        }
    }

    if(!handled) {
        hnd = &irq_handlers[EXC_UNHANDLED_EXC >> 4];
        if(hnd->hdl != NULL)
            hnd->hdl(evt, irq_srt_addr, hnd->data);
        else
            irq_dump_regs(code, evt);

        arch_panic("unhandled IRQ/Exception");
    }

    irq_disable();
    inside_int = 0;
}

void irq_handle_trapa(irq_t code, irq_context_t *context, void *data) {
    const struct irq_cb *hnd, *handlers = data;
    uint32_t vec;

    (void)code;

    /* Get the trapa vector */
    vec = TRA >> 2;

    /* Check for handler and call if present */
    hnd = &handlers[vec];

    if(hnd->hdl)
        hnd->hdl(vec, context, hnd->data);
}

extern void irq_vma_table(void);

/* Switches register banks; call this outside of exception handling
   (but make sure interrupts are off!!) to change where registers will
   go to, or call it inside an exception handler to switch contexts.
   Make sure you have at least REG_BYTE_CNT bytes available. DO NOT
   ALLOW ANY INTERRUPTS TO HAPPEN UNTIL THIS HAS BEEN CALLED AT
   LEAST ONCE! */
void irq_set_context(irq_context_t *regbank) {
    irq_srt_addr = regbank;
}

/* Return the current IRQ context */
irq_context_t *irq_get_context(void) {
    return irq_srt_addr;
}

/* Fill a newly allocated context block for usage with supervisor/kernel
   or user mode. The given parameters will be passed to the called routine (up
   to the architecture maximum). */
void irq_create_context(irq_context_t *context, uint32_t stkpntr,
                        uint32_t routine, const uint32_t *args, bool usermode) {
    /* Clear out all registers. */
    memset(context, 0, sizeof(irq_context_t));

    /* Setup the program frame */
    context->pc = (uint32_t)routine;
    context->sr = 0x40000000;   /* note: need to handle IMASK */
    context->r[15] = stkpntr;
    context->r[14] = 0xffffffff;

    /* Copy up to four args */
    context->r[4] = args[0];
    context->r[5] = args[1];
    context->r[6] = args[2];
    context->r[7] = args[3];

    /* Handle user mode */
    if(usermode) {
        context->sr &= ~0x40000000;
        context->r[15] &= ~0xf0000000;
    }
}

/* Default timer handler (until threads can take over) */
static void irq_def_timer(irq_t src, irq_context_t *context, void *data) {
    (void)src;
    (void)context;
    (void)data;
}

/* Default FPU exception handler (can't seem to turn these off) */
static void irq_def_fpu(irq_t src, irq_context_t *context, void *data) {
    (void)src;
    (void)data;
    context->pc += 2;
}

/* Pre-init SR and VBR */
static uint32_t pre_sr, pre_vbr;

/* Have we been initialized? */
static bool initted = false;

/* Init routine */
int irq_init(void) {
    assert(!initted);

    /* Save SR and VBR */
    __asm__("stc    sr,r0\n"
            "mov.l  r0,%0" : : "m"(pre_sr));
    __asm__("stc    vbr,r0\n"
            "mov.l  r0,%0" : : "m"(pre_vbr));

    /* Make sure interrupts are disabled */
    irq_disable();

    /* Blank the exception handler tables */
    memset(irq_handlers, 0, sizeof(irq_handlers));
    memset(trapa_handlers, 0, sizeof(trapa_handlers));
    memset(&global_irq_handler, 0, sizeof(global_irq_handler));

    /* Default to not in an interrupt */
    inside_int = 0;

    /* Set a default timer handler */
    irq_set_handler(EXC_TMU0_TUNI0, irq_def_timer, (void *)0);

    /* Set a trapa handler */
    irq_set_handler(EXC_TRAPA, irq_handle_trapa, trapa_handlers);

    /* Set a default FPU exception handler */
    irq_set_handler(EXC_FPU, irq_def_fpu, NULL);

    /* Set a default context (will be superseded if threads are
       enabled later) */
    irq_set_context(&irq_context_default);

    /* Set VBR to our exception table above, but don't enable
       exceptions and IRQs yet. */
    __asm__("	! Set VBR\n"
            "	mov.l _vbr_addr,r0\n"
            "	ldc	  r0,vbr\n"
            "	bra   _after_vbr\n"
            "	nop\n"
            "	.align 2\n"
            "_vbr_addr:\n"
            "	.long _irq_vma_table\n"
            "_after_vbr:\n");

    initted = true;

    return 0;
}

void irq_shutdown(void) {
    if(!initted)
        return;

    /* Restore SR and VBR */
    __asm__("mov.l  %0,r0\n"
            "ldc    r0,sr" : : "m"(pre_sr));
    __asm__("mov.l  %0,r0\n"
            "ldc    r0,vbr" : : "m"(pre_vbr));

    initted = false;
}