/* KallistiOS ##version##

   arch/xbox/kernel/gdt.c
   Copyright (C) 2026 The KallistiOS Contributors
*/

#include "gdt.h"

#include <arch/arch.h>
#include <arch/stack.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define X86_GDT_ENTRY_COUNT 4U
#define X86_SELECTOR_CODE   0x08U
#define X86_SELECTOR_DATA   0x10U
#define X86_SELECTOR_TSS    0x18U

typedef struct __attribute__((packed)) x86_descriptor_register {
    uint16_t limit;
    uint32_t base;
} x86_descriptor_register_t;

typedef struct __attribute__((packed)) x86_tss {
    uint16_t previous_task, reserved0;
    uint32_t esp0;
    uint16_t ss0, reserved1;
    uint32_t esp1;
    uint16_t ss1, reserved2;
    uint32_t esp2;
    uint16_t ss2, reserved3;
    uint32_t cr3, eip, eflags;
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint16_t es, reserved4;
    uint16_t cs, reserved5;
    uint16_t ss, reserved6;
    uint16_t ds, reserved7;
    uint16_t fs, reserved8;
    uint16_t gs, reserved9;
    uint16_t ldt, reserved10;
    uint16_t trap;
    uint16_t iomap_base;
} x86_tss_t;

_Static_assert(sizeof(x86_descriptor_register_t) == 6,
               "IA-32 descriptor register image must be 6 bytes");
_Static_assert(sizeof(x86_tss_t) == 104, "IA-32 TSS must be 104 bytes");
_Static_assert(offsetof(x86_tss_t, esp0) == 4, "TSS ESP0 offset mismatch");
_Static_assert(offsetof(x86_tss_t, ss0) == 8, "TSS SS0 offset mismatch");
_Static_assert(offsetof(x86_tss_t, iomap_base) == 102,
               "TSS I/O-map offset mismatch");

extern void xbox_gdt_activate(const x86_descriptor_register_t *gdtr,
                              uint16_t fs, uint16_t gs, uint16_t tr);

static uint64_t kos_gdt[X86_GDT_ENTRY_COUNT] __attribute__((aligned(16)));
static x86_tss_t kos_tss __attribute__((aligned(16)));
static x86_descriptor_register_t loader_gdtr;
static uint16_t loader_fs_selector;
static uint16_t loader_gs_selector;
static uint16_t loader_task_selector;
static uint32_t loader_task_descriptor_high;
static bool gdt_initialized;

static uint64_t gdt_segment(uint32_t base, uint32_t limit,
                            uint8_t access, uint8_t flags) {
    return ((uint64_t)(limit & 0x0000ffffU)) |
           ((uint64_t)(base & 0x00ffffffU) << 16) |
           ((uint64_t)access << 40) |
           ((uint64_t)((limit >> 16) & 0x0fU) << 48) |
           ((uint64_t)(flags & 0x0fU) << 52) |
           ((uint64_t)(base >> 24) << 56);
}

int xbox_gdt_init(void) {
    x86_descriptor_register_t gdtr;
    const volatile uint32_t *loader_gdt_words;
    uint32_t task_descriptor_high;
    unsigned int task_descriptor_offset;
    unsigned int task_type;
    uint16_t cs, ds, es, ss, fs, gs, tr;

    if(gdt_initialized)
        return -1;

    __asm__ volatile("sgdt %0" : "=m"(loader_gdtr));
    __asm__ volatile("movw %%cs, %0" : "=rm"(cs));
    __asm__ volatile("movw %%ds, %0" : "=rm"(ds));
    __asm__ volatile("movw %%es, %0" : "=rm"(es));
    __asm__ volatile("movw %%ss, %0" : "=rm"(ss));
    __asm__ volatile("movw %%fs, %0" : "=rm"(fs));
    __asm__ volatile("movw %%gs, %0" : "=rm"(gs));
    __asm__ volatile("str %0" : "=rm"(tr));

    /*
     * XBE entry unavoidably begins on the Xbox kernel's descriptor state.
     * KOS only depends on the conventional flat ring-0 code and data
     * selectors needed by xbox_gdt_activate(). FS, GS, and TR belong to the
     * loader and are captured exactly rather than imposing a kernel-specific
     * selector layout on other loaders.
     */
    if(cs != X86_SELECTOR_CODE || ds != X86_SELECTOR_DATA ||
       es != X86_SELECTOR_DATA || ss != X86_SELECTOR_DATA ||
       loader_gdtr.limit < X86_SELECTOR_DATA + 7U)
        arch_panic("Unsupported Xbox loader GDT handoff");

    if(tr) {
        /*
         * STR may include selector RPL bits, but its table-indicator bit must
         * name the active GDT. Validate the complete eight-byte descriptor
         * before shutdown ever clears its busy bit in loader-owned memory.
         */
        task_descriptor_offset = tr & ~7U;
        if((tr & 4U) ||
           task_descriptor_offset + 7U > loader_gdtr.limit)
            arch_panic("Unsupported Xbox loader task selector");

        loader_gdt_words =
            (const volatile uint32_t *)(uintptr_t)loader_gdtr.base;
        task_descriptor_high =
            loader_gdt_words[task_descriptor_offset / 4U + 1U];
        task_type = (task_descriptor_high >> 8) & 0x0fU;
        if((task_descriptor_high & 0x00001000U) ||
           !(task_descriptor_high & 0x00008000U) ||
           (task_type != 0x03U && task_type != 0x0bU))
            arch_panic("Unsupported Xbox loader task descriptor");

        loader_task_descriptor_high = task_descriptor_high;
    }
    else {
        loader_task_descriptor_high = 0;
    }

    memset(kos_gdt, 0, sizeof(kos_gdt));
    memset(&kos_tss, 0, sizeof(kos_tss));

    /* KOS owns its flat ring-0 code/data descriptors. Set the accessed bit up
       front so the CPU never needs to write it as a side effect. */
    kos_gdt[1] = gdt_segment(0, 0x000fffffU, 0x9bU, 0x0cU);
    kos_gdt[2] = gdt_segment(0, 0x000fffffU, 0x93U, 0x0cU);

    kos_tss.esp0 = (uint32_t)(uintptr_t)arch_stack_top;
    kos_tss.ss0 = X86_SELECTOR_DATA;
    kos_tss.iomap_base = sizeof(kos_tss);
    kos_gdt[3] = gdt_segment((uint32_t)(uintptr_t)&kos_tss,
                             sizeof(kos_tss) - 1U, 0x89U, 0);

    gdtr.limit = sizeof(kos_gdt) - 1U;
    gdtr.base = (uint32_t)(uintptr_t)kos_gdt;
    loader_fs_selector = fs;
    loader_gs_selector = gs;
    loader_task_selector = tr;
    xbox_gdt_activate(&gdtr, 0, 0, X86_SELECTOR_TSS);
    gdt_initialized = true;
    return 0;
}

void xbox_gdt_shutdown(void) {
    volatile uint32_t *loader_gdt_words = NULL;
    unsigned int task_descriptor_high_word = 0;

    if(!gdt_initialized)
        return;

    /*
     * LTR marked the KOS TSS busy while the loader's prior TSS descriptor
     * remained busy. Temporarily mark the loader descriptor available so LTR
     * in xbox_gdt_activate() can restore it; the CPU sets it busy again,
     * returning the descriptor to its original state.
     */
    if(loader_task_selector) {
        loader_gdt_words = (volatile uint32_t *)(uintptr_t)loader_gdtr.base;
        task_descriptor_high_word =
            (loader_task_selector & ~7U) / 4U + 1U;
        loader_gdt_words[task_descriptor_high_word] =
            loader_task_descriptor_high & ~0x00000200U;
    }

    xbox_gdt_activate(&loader_gdtr, loader_fs_selector, loader_gs_selector,
                      loader_task_selector);

    /* LTR marks the restored TSS busy. Put back the loader's exact original
       descriptor image rather than retaining any incidental memory changes. */
    if(loader_task_selector)
        loader_gdt_words[task_descriptor_high_word] =
            loader_task_descriptor_high;

    gdt_initialized = false;
}
