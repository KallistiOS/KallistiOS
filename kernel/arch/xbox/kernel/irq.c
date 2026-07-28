/* KallistiOS ##version##

   arch/xbox/kernel/irq.c
   Copyright (C) 2026 The KallistiOS Contributors
*/

#include <arch/arch.h>
#include <arch/irq.h>

#include <kos/dbgio.h>
#include <kos/thread.h>

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "gdt.h"
#include "pic.h"

#define X86_EXCEPTION_COUNT 32U
#define X86_VECTOR_COUNT 256U
#define X86_EFLAGS_RESERVED 0x00000002U
#define X86_EFLAGS_IF       0x00000200U
#define X86_IDT_INTERRUPT_GATE 0x8eU
#define X86_IDT_USER_INTERRUPT_GATE 0xeeU

_Static_assert(sizeof(irq_context_t) == REG_BYTE_CNT,
               "REG_BYTE_CNT must equal sizeof(irq_context_t)");
_Static_assert(_Alignof(irq_context_t) == 16,
               "irq_context_t must be 16-byte aligned");
_Static_assert(offsetof(irq_context_t, edi) == 0x00, "context offset mismatch");
_Static_assert(offsetof(irq_context_t, esi) == 0x04, "context offset mismatch");
_Static_assert(offsetof(irq_context_t, ebp) == 0x08, "context offset mismatch");
_Static_assert(offsetof(irq_context_t, esp) == 0x0c, "context offset mismatch");
_Static_assert(offsetof(irq_context_t, ebx) == 0x10, "context offset mismatch");
_Static_assert(offsetof(irq_context_t, edx) == 0x14, "context offset mismatch");
_Static_assert(offsetof(irq_context_t, ecx) == 0x18, "context offset mismatch");
_Static_assert(offsetof(irq_context_t, eax) == 0x1c, "context offset mismatch");
_Static_assert(offsetof(irq_context_t, eip) == 0x20, "context offset mismatch");
_Static_assert(offsetof(irq_context_t, eflags) == 0x24,
               "context offset mismatch");
_Static_assert(offsetof(irq_context_t, cs) == 0x28, "context offset mismatch");
_Static_assert(offsetof(irq_context_t, ds) == 0x2c, "context offset mismatch");
_Static_assert(offsetof(irq_context_t, es) == 0x30, "context offset mismatch");
_Static_assert(offsetof(irq_context_t, fs) == 0x34, "context offset mismatch");
_Static_assert(offsetof(irq_context_t, gs) == 0x38, "context offset mismatch");
_Static_assert(offsetof(irq_context_t, ss) == 0x3c, "context offset mismatch");
_Static_assert(offsetof(irq_context_t, fxstate) == 0x40,
               "FXSAVE area must begin at offset 0x40");
_Static_assert(offsetof(irq_context_t, fxstate) % 16 == 0,
               "FXSAVE area must be 16-byte aligned");

int inside_int;

typedef struct __attribute__((packed)) x86_idt_gate {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t reserved;
    uint8_t attributes;
    uint16_t offset_high;
} x86_idt_gate_t;

typedef struct __attribute__((packed)) x86_descriptor_register {
    uint16_t limit;
    uint32_t base;
} x86_descriptor_register_t;

_Static_assert(sizeof(x86_idt_gate_t) == 8, "IA-32 IDT gate must be 8 bytes");
_Static_assert(sizeof(x86_descriptor_register_t) == 6,
               "IA-32 descriptor register image must be 6 bytes");

extern void (*irq_stubs[X86_VECTOR_COUNT])(void);

static x86_idt_gate_t irq_idt[X86_VECTOR_COUNT]
    __attribute__((aligned(16)));
static x86_descriptor_register_t irq_loader_idtr;
static irq_cb_t irq_handlers[256];
static irq_cb_t irq_global_handler;
static irq_context_t irq_bootstrap_context;
static irq_context_t *irq_context_current;
static bool irq_initialized;

static char *irq_append_str(char *output, const char *text) {
    while(*text)
        *output++ = *text++;

    return output;
}

static char *irq_append_hex(char *output, const char *label, uint32_t value) {
    static const char digits[] = "0123456789abcdef";
    unsigned int digit;

    output = irq_append_str(output, label);
    *output++ = '0';
    *output++ = 'x';

    for(digit = 0; digit < 8U; ++digit)
        *output++ = digits[(value >> (28U - digit * 4U)) & 0x0fU];

    return output;
}

static __noreturn void irq_unhandled_exception(uint32_t vector,
                                                uint32_t error_code,
                                                const irq_context_t *context) {
    char report[384];
    char *output = report;
    uint32_t fault_address = 0;

    if(vector == EXC_PAGE_FAULT)
        __asm__ volatile("mov %%cr2, %0" : "=r"(fault_address));

    output = irq_append_str(output, "KOS Xbox unhandled CPU exception\n");
    output = irq_append_hex(output, "  vector=", vector);
    output = irq_append_hex(output, " error=", error_code);
    output = irq_append_str(output, "\n");
    output = irq_append_hex(output, "  eip=", context->eip);
    output = irq_append_hex(output, " esp=", context->esp);
    output = irq_append_hex(output, " ebp=", context->ebp);
    output = irq_append_str(output, "\n");
    output = irq_append_hex(output, "  eax=", context->eax);
    output = irq_append_hex(output, " ebx=", context->ebx);
    output = irq_append_hex(output, " ecx=", context->ecx);
    output = irq_append_hex(output, " edx=", context->edx);
    output = irq_append_str(output, "\n");
    output = irq_append_hex(output, "  edi=", context->edi);
    output = irq_append_hex(output, " esi=", context->esi);
    output = irq_append_hex(output, " eflags=", context->eflags);
    output = irq_append_str(output, "\n");
    if(context->eip >= 0x00010000U &&
       context->eip <= _arch_mem_top - 2U * sizeof(uint32_t)) {
        const uint32_t *code = (const uint32_t *)(uintptr_t)context->eip;

        output = irq_append_hex(output, "  code0=", code[0]);
        output = irq_append_hex(output, " code1=", code[1]);
        output = irq_append_str(output, "\n");
    }
    if(thd_current) {
        output = irq_append_hex(output, "  stack_lo=",
                                (uintptr_t)thd_current->stack);
        output = irq_append_hex(output, " stack_hi=",
                                (uintptr_t)thd_current->stack +
                                thd_current->stack_size);
        output = irq_append_str(output, "\n");
    }
    if(vector == EXC_PAGE_FAULT) {
        output = irq_append_hex(output, "  cr2=", fault_address);
        output = irq_append_str(output, "\n");
    }
    *output = '\0';

    dbgio_write_str(report);

    arch_panic("Unhandled Xbox CPU exception");
}

static void irq_set_gate(unsigned int vector, uintptr_t entry,
                         uint16_t selector, uint8_t attributes) {
    x86_idt_gate_t *gate = &irq_idt[vector];

    gate->offset_low = (uint16_t)entry;
    gate->selector = selector;
    gate->reserved = 0;
    gate->attributes = attributes;
    gate->offset_high = (uint16_t)(entry >> 16);
}

static __noreturn void thread_context_returned(void) {
    arch_panic("Xbox thread entry returned unexpectedly");
}

void arch_irq_create_context(irq_context_t *context,
                             uintptr_t stack_pointer,
                             uintptr_t routine,
                             const uintptr_t *args) {
    uintptr_t *entry_stack;
    uint16_t selector;

    memset(context, 0, sizeof(*context));

    /*
     * thd_init() represents the already-running bootstrap thread by passing a
     * NULL user routine in args[0]. Its stack is live: constructing a normal
     * new-thread frame at stack_pointer would overwrite arch_main()'s active
     * callers near arch_stack_top. This placeholder can never be restored
     * before either interrupt entry or thd_block_now() captures the real live
     * context, so give stack validation a valid value without touching memory.
     */
    if(!args[0]) {
        context->esp = (uint32_t)stack_pointer;
        context->eip = (uint32_t)(uintptr_t)thread_context_returned;
        return;
    }

    /* At i386 function entry ESP is 12 modulo 16. Reserve five words so four
       cdecl arguments plus a synthetic return address fit below aligned top. */
    stack_pointer &= ~(uintptr_t)0x0f;
    entry_stack = (uintptr_t *)(stack_pointer - 5U * sizeof(uintptr_t));
    entry_stack[0] = (uintptr_t)thread_context_returned;
    entry_stack[1] = args[0];
    entry_stack[2] = args[1];
    entry_stack[3] = args[2];
    entry_stack[4] = args[3];

    context->esp = (uint32_t)(uintptr_t)entry_stack;
    context->eip = (uint32_t)routine;
    /* KOS owns the IDT, PIC, and PIT before any created thread can run, so
       normal thread contexts begin with maskable interrupts enabled. */
    context->eflags = X86_EFLAGS_RESERVED | X86_EFLAGS_IF;

    __asm__ volatile("movw %%cs, %0" : "=rm"(selector));
    context->cs = selector;
    __asm__ volatile("movw %%ds, %0" : "=rm"(selector));
    context->ds = selector;
    __asm__ volatile("movw %%es, %0" : "=rm"(selector));
    context->es = selector;
    __asm__ volatile("movw %%fs, %0" : "=rm"(selector));
    context->fs = selector;
    __asm__ volatile("movw %%gs, %0" : "=rm"(selector));
    context->gs = selector;
    __asm__ volatile("movw %%ss, %0" : "=rm"(selector));
    context->ss = selector;

    /* Construct a clean, valid FXSAVE image without disturbing the creator's
       live x87/SSE state. FCW=0x037f and MXCSR=0x1f80 are architectural
       defaults with floating-point exceptions masked. */
    *(uint16_t *)&context->fxstate[0] = 0x037f;
    *(uint32_t *)&context->fxstate[24] = 0x00001f80;
    *(uint32_t *)&context->fxstate[28] = 0x0000ffff;
}

void arch_irq_set_context(irq_context_t *context) {
    irq_context_current = context;
}

irq_context_t *arch_irq_get_context(void) {
    return irq_context_current;
}

int arch_irq_set_handler(irq_t code, irq_hdl_t handler, void *data) {
    unsigned int vector = (unsigned int)code;

    if(vector >= 256U)
        return -1;

    irq_disable_scoped();
    irq_handlers[vector] = (irq_cb_t) { handler, data };
    return 0;
}

irq_cb_t arch_irq_get_handler(irq_t code) {
    unsigned int vector = (unsigned int)code;

    if(vector >= 256U)
        return (irq_cb_t) { NULL, NULL };

    irq_disable_scoped();
    return irq_handlers[vector];
}

int arch_irq_set_global_handler(irq_hdl_t handler, void *data) {
    irq_disable_scoped();
    irq_global_handler = (irq_cb_t) { handler, data };
    return 0;
}

irq_cb_t arch_irq_get_global_handler(void) {
    irq_disable_scoped();
    return irq_global_handler;
}

/* Called only by irqentry.S with maskable interrupts disabled. */
void arch_irq_dispatch(uint32_t vector, irq_context_t *context,
                       uint32_t error_code) {
    irq_cb_t callback;
    bool handled = false;

    if(inside_int > 1)
        arch_panic("Nested Xbox exception");
    if(vector >= X86_VECTOR_COUNT)
        arch_panic("Xbox interrupt vector outside installed IDT");

    callback = irq_global_handler;
    if(callback.hdl) {
        callback.hdl((irq_t)vector, context, callback.data);
        handled = true;
    }

    callback = irq_handlers[vector];
    if(callback.hdl) {
        callback.hdl((irq_t)vector, context, callback.data);
        handled = true;
    }

    if(!handled && vector < X86_EXCEPTION_COUNT) {
        callback = irq_handlers[EXC_UNHANDLED_EXC];
        if(callback.hdl)
            callback.hdl((irq_t)vector, context, callback.data);
        else
            irq_unhandled_exception(vector, error_code, context);
    }

    if(vector >= XBOX_PIC_MASTER_VECTOR &&
       vector < XBOX_PIC_MASTER_VECTOR + XBOX_PIC_IRQ_COUNT)
        xbox_pic_eoi(vector);
}

int irq_init(void) {
    x86_descriptor_register_t idtr;
    uintptr_t entry;
    uint16_t code_selector;
    unsigned int vector;

    if(irq_initialized)
        return -1;

    arch_irq_disable();
    if(xbox_gdt_init() != 0)
        return -1;
    memset(irq_handlers, 0, sizeof(irq_handlers));
    memset(&irq_global_handler, 0, sizeof(irq_global_handler));
    memset(&irq_bootstrap_context, 0, sizeof(irq_bootstrap_context));
    inside_int = 0;

    __asm__ volatile("sidt %0" : "=m"(irq_loader_idtr));
    __asm__ volatile("movw %%cs, %0" : "=rm"(code_selector));

    for(vector = 0; vector < X86_VECTOR_COUNT; ++vector) {
        entry = (uintptr_t)irq_stubs[vector];
        irq_set_gate(vector, entry, code_selector,
                     (vector == EXC_BREAKPOINT || vector == EXC_OVERFLOW)
                         ? X86_IDT_USER_INTERRUPT_GATE
                         : X86_IDT_INTERRUPT_GATE);
    }

    idtr.limit = sizeof(irq_idt) - 1U;
    idtr.base = (uint32_t)(uintptr_t)irq_idt;
    irq_context_current = &irq_bootstrap_context;
    __asm__ volatile("lidt %0" : : "m"(idtr) : "memory");
    if(xbox_pic_init() != 0) {
        __asm__ volatile("lidt %0" : : "m"(irq_loader_idtr) : "memory");
        xbox_gdt_shutdown();
        irq_context_current = NULL;
        return -1;
    }
    irq_initialized = true;
    return 0;
}

void irq_shutdown(void) {
    if(!irq_initialized)
        return;

    arch_irq_disable();
    xbox_pic_shutdown();
    __asm__ volatile("lidt %0" : : "m"(irq_loader_idtr) : "memory");
    xbox_gdt_shutdown();
    irq_context_current = NULL;
    inside_int = 0;
    irq_initialized = false;
}
