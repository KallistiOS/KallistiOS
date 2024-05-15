#include <arch/timer.h>

int spinlock_is_locked() {
    return 0;
}

void arch_sleep(int x) {
    return;
}

void irq_restore(int x) {
    return;
}

void irq_enable(void) { return; }
void irq_force_return(void) { return; }
int irq_inside_int(void) { return 0; }
int irq_set_global_handler(irq_handler hnd) { return 0; }
irq_handler irq_get_handler(irq_t source) { return NULL; }
int irq_set_handler(irq_t source, irq_handler hnd) { return 0; }
irq_context_t* irq_get_context(void) { return NULL; }
irq_handler irq_get_global_handler(void) { return NULL; }

int irq_disable() {
    return 0;
}

void irq_create_context(irq_context_t *context, uint32 stack_pointer,
                        uint32 routine, uint32 *args, int usermode) {
    return;
}

void irq_set_context(irq_context_t * c) {
    return;
}

uint64 timer_ms_gettime64() {
    return 0;
}

void timer_ms_gettime(uint32* secs, uint32* msecs) {
    *secs = 0;
    *msecs = 0;
}

uint64 timer_us_gettime64() {
    return 0;
}

timer_primary_callback_t timer_primary_set_callback(timer_primary_callback_t callback) {
    return NULL;
}

void timer_primary_wakeup(uint32 millis) {
}

void timer_spin_sleep(int x) {
    return;
}

void timer_primary_enable() {
    return;
}

int thd_block_now(irq_context_t * c) {
    return 0;
}


/* Called to shut down the system */
void arch_exit() {
}

/* Called from syscall to reboot the system */
void arch_reboot() {
}

void arch_abort() {
}

void arch_exec_at(const void* image, uint32 length, uint32 address) {};
void arch_exec(const void* image, uint32 length) {}

void arch_stk_trace(int n) {}
void arch_stk_trace_at(uint32 fp, int n) {}

void arch_panic(const char* str) {
}


void icache_flush_range(uint32 start, uint32 count) { return; }
void dcache_inval_range(uint32 start, uint32 count) { return; }
void dcache_flush_range(uint32 start, uint32 count) { return; }
void dcache_purge_range(uint32 start, uint32 count) { return; }
void* dcache_pref_range(uint32 start, uint32 count) { return NULL; }
