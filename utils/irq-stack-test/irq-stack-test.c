/* KallistiOS ##version##
   Copyright (C) 2026 Joseph Black
*/
#include <arch/irq.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef __DREAMCAST__
#include <kos.h>
#include <stdatomic.h>

static volatile unsigned interrupted_atomics;
static irq_cb_t previous_handler;

static void observe_irq(irq_t code, irq_context_t *context, void *data) {
    (void)data;
    if(context->r[15] >= UINT32_MAX - 127u) {
        uint32_t saved_sp = context->r[15];
        uint32_t saved_pc = context->pc;
        assert(irq_context_stack_pointer(context) == context->r[1]);
        assert(context->r[15] == saved_sp && context->pc == saved_pc);
        ++interrupted_atomics;
    }
    if(previous_handler.hdl)
        previous_handler.hdl(code, context, previous_handler.data);
}

static void stress(void) {
    atomic_uint value = 0;
    unsigned expected_value = 0;
    unsigned old_hz = thd_get_hz();
    previous_handler = irq_get_global_handler();
    irq_set_global_handler(observe_irq, NULL);
    thd_set_hz(1000);
    uint64_t deadline = timer_ms_gettime64() + 2000;
    do {
        for(unsigned i = 0; i < 1024; ++i) {
            atomic_fetch_add_explicit(&value, 1, memory_order_relaxed);
            ++expected_value;
            unsigned expected = expected_value;
            assert(atomic_compare_exchange_strong(&value, &expected,
                                                  expected_value + 1));
            ++expected_value;
        }
    } while(timer_ms_gettime64() < deadline);
    thd_set_hz(old_hz);
    irq_set_global_handler(previous_handler.hdl, previous_handler.data);
    assert(atomic_load(&value) == expected_value);
    printf("irq-stack-test: operations=%u interrupted-atomics=%u\n",
           expected_value, interrupted_atomics);
    /* Some emulators execute atomics as indivisible host operations. Zero
       hits is not evidence that interrupt-inside-gUSA handling was tested. */
}
#endif

int main(void) {
    irq_context_t context, before;
    memset(&context, 0, sizeof(context));
    context.r[1] = UINT32_C(0x8cffff80);
    context.r[0] = UINT32_C(0x8c010100);
    for(unsigned size = 1; size <= 128; ++size) {
        context.r[15] = 0u - size;
        for(unsigned at_end = 0; at_end < 2; ++at_end) {
            context.pc = context.r[0] - (at_end ? 0 : 2);
            memcpy(&before, &context, sizeof(before));
#if defined(__SH_ATOMIC_MODEL_SOFT_GUSA__) && __SH_ATOMIC_MODEL_SOFT_GUSA__
            assert(irq_context_stack_pointer(&context) == context.r[1]);
#else
            assert(irq_context_stack_pointer(&context) == context.r[15]);
#endif
            assert(!memcmp(&context, &before, sizeof(context)));
            assert(CONTEXT_SP(context) == 0u - size);
        }
    }
    const uint32_t ordinary[] = {0, 1, 128, UINT32_MAX - 128,
                                0x8cffff80, 0xacffff80};
    for(size_t i = 0; i < sizeof(ordinary) / sizeof(ordinary[0]); ++i) {
        context.r[15] = ordinary[i];
        assert(irq_context_stack_pointer(&context) == ordinary[i]);
    }
    /* A bogus preserved address must still reach the caller's bounds check. */
    context.r[15] = (uint32_t)-8;
    context.r[1] = 0;
#if defined(__SH_ATOMIC_MODEL_SOFT_GUSA__) && __SH_ATOMIC_MODEL_SOFT_GUSA__
    assert(irq_context_stack_pointer(&context) == 0);
#endif
#ifdef __DREAMCAST__
    stress();
#endif
    puts("irq-stack-test: PASS");
    return 0;
}
