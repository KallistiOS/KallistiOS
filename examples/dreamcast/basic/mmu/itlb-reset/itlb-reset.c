/* KallistiOS ##version##
   Copyright (C) 2026 Joseph Black

   Standalone ITLB-array reset probe. Runs from direct P1/P2 addresses and
   does not dereference translated memory. Do not embed in a live MMU user.
*/
#include <kos.h>
#include <arch/mmu.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

KOS_INIT_FLAGS(INIT_DEFAULT);

/* Explicit emulator-only limitation; hardware validation defaults to all
   three arrays. Never silently treat a failed attribute readback as a pass. */
#ifndef ITLB_RESET_VERIFY_DATA2
#define ITLB_RESET_VERIFY_DATA2 1
#endif
#if ITLB_RESET_VERIFY_DATA2 != 0 && ITLB_RESET_VERIFY_DATA2 != 1
#error ITLB_RESET_VERIFY_DATA2 must be 0 or 1
#endif
extern void mmu_reset_itlb(void);
extern void tlb_probe_write(uint32_t address, uint32_t value);
extern uint32_t tlb_probe_read(uint32_t address);

#define IADDR 0xf2000000u
#define IDATA1 0xf3000000u
#define IDATA2 0xf3800000u
#define UADDR 0xf6003f00u
#define UDATA1 0xf7003f00u

static bool run_case(unsigned valid_mask) {
    irq_mask_t irq = irq_disable();
    volatile uint32_t *const pteh = (volatile uint32_t *)0xff000000;
    uint32_t saved_pteh = *pteh;
    const uint32_t ubases[] = {UADDR, UDATA1};
    uint32_t before_u[2];
    unsigned failures = 0;
    /* Observe, but do not modify, the reserved SQ UTLB entry. */
    for(unsigned i = 0; i < 2; ++i)
        before_u[i] = tlb_probe_read(ubases[i]);
    if(!(before_u[0] & 0x100u)) failures |= 128;

    for(unsigned i = 0; i < 4; ++i) {
        uint32_t v = ((valid_mask >> i) & 1u) << 8;
        uint32_t addr = 0x10000000u + i * 4096 + i + 1 + v;
        uint32_t data = 0x0c100058u + i * 4096 + v;
        tlb_probe_write(IDATA2 + i * 256, 8 + i);
        tlb_probe_write(IDATA1 + i * 256, data);
        tlb_probe_write(IADDR + i * 256, addr);
        if((tlb_probe_read(IADDR + i * 256) & 0xfffffdffu) != addr) failures |= 1;
        if((tlb_probe_read(IDATA1 + i * 256) & 0x1ffffddau) != data) failures |= 2;
#if ITLB_RESET_VERIFY_DATA2
        if((tlb_probe_read(IDATA2 + i * 256) & 15u) != 8 + i) failures |= 4;
#endif
    }
    mmu_reset_itlb();
    for(unsigned i = 0; i < 4; ++i) {
        uint32_t a = tlb_probe_read(IADDR + i * 256) & 0xfffffdffu;
        uint32_t d = tlb_probe_read(IDATA1 + i * 256) & 0x1ffffddau;
        if(a) failures |= 8;
        if(d) failures |= 16;
#if ITLB_RESET_VERIFY_DATA2
        if(tlb_probe_read(IDATA2 + i * 256) & 15u) failures |= 32;
#endif
    }
    if(*pteh != saved_pteh) failures |= 64;
    const uint32_t umasks[] = {0xffffffffu, 0x1ffffdffu};
    for(unsigned i = 0; i < 2; ++i)
        if(((tlb_probe_read(ubases[i]) ^ before_u[i]) & umasks[i]) != 0) failures |= 128;
    /* Restore the non-test state and clear every ITLB slot even on failure. */
    for(unsigned i = 0; i < 4; ++i) {
        tlb_probe_write(IADDR + i * 256, 0);
        tlb_probe_write(IDATA1 + i * 256, 0);
        tlb_probe_write(IDATA2 + i * 256, 0);
    }
    irq_restore(irq);
    bool ok = failures == 0;
    if(!ok)
        printf("ITLB-RESET: FAIL valid-mask=%u fields=%u\n", valid_mask, failures);
    return ok;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    bool enabled_here = !mmu_enabled();
    if(enabled_here)
        mmu_init_basic();
    bool ok = true;
    for(unsigned mask = 0; mask < 16; ++mask)
        ok &= run_case(mask);
    if(enabled_here)
        mmu_shutdown_basic();
    if(ok)
        printf("ITLB-RESET: PASS cases=16 entries=4 arrays=%u utlb=1 pteh=1 data2=%u\n",
               2u + ITLB_RESET_VERIFY_DATA2, ITLB_RESET_VERIFY_DATA2);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
