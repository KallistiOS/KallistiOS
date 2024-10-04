/* KallistiOS ##version##
   fenv.c
   Copyright (C) 2024 Falco Girgis
*/

#include <fenv.h>
#include <float.h>

#define FPSCR            __builtin_sh_get_fpscr()
#define FPSCR_SET(value) __builtin_sh_set_fpscr((value))

static fenv_t __fe_dfl_env = 0;
const fenv_t *_fe_dfl_env = &__fe_dfl_env;

static femode_t __fe_dfl_mode = 0;
const femode_t *_fe_dfl_mode = &__fe_dfl_mode;

#define FENV_MODE_MASK  0x3c0000
#define FENV_MODE_SHIFT 18

int fegetmode(femode_t *mode) {
    *mode = (FPSCR & FENV_MODE_MASK) >> FENV_MODE_SHIFT;
    return 0;
}

int fesetmode(const femode_t *mode) {
    FPSCR_SET((*mode << FENV_MODE_SHIFT) & FENV_MODE_MASK);
    return 0;
}

#define FENV_EXCEPT_MASK  0x1f
#define FENV_ENABLE_SHIFT 7
#define FENV_FLAG_SHIFT 2

/* GNU Extensions */
int feenableexcept(int excepts) {
    FPSCR_SET(FPSCR | ((excepts & FENV_EXCEPT_MASK) << FENV_ENABLE_SHIFT));
    return 0;
}

int fedisableexcept(int excepts) {
    FPSCR_SET(FPSCR & ~((excepts & FENV_EXCEPT_MASK) << FENV_ENABLE_SHIFT));
    return 0;
}

int fegetexcept(void) {
    return (FPSCR >> FENV_FLAG_SHIFT) & FENV_EXCEPT_MASK;
}

int feclearexcept(int excepts) {
    excepts &= FE_ALL_ACCEPT;

    FPSCR_SET(FPSCR & ~((excepts & FENV_EXCEPT_MASK) << FENV_FLAG_SHIFT));
    return 0;
}

int fetestexcept(int excepts) {
    excepts &= FE_ALL_ACCEPT;

    return (FPSCR & (excepts << FENV_FLAG_SHIFT));
}

int feraiseexcept(int excepts) {
    if(!excepts)
        return 0;

    if(excepts & FE_INEXACT) {
        volatile double a = 1.0;
        volatile double b = 3.0;
        volatile double c = a / b;
        (void)c;
    }

    if(excepts & FE_UNDERFLOW) {
        volatile double a = DBL_MIN;
        volatile double b = 10.0;
        volatile double c = a / b;
        (void)c;
    }

    if(excepts & FE_OVERFLOW) {
        volatile double a = DBL_MAX;
        volatile double b = 10.0;
        volatile double c = a * b;
        (void)c;
    }

    if(excepts & FE_DIVBYZERO) {
        volatile double a = 1.0;
        volatile double b = 0.0;
        volatile double c = a / b;
        (void)c;
    }

    FPSCR_SET(FPSCR | ((excepts & FE_ALL_ACCEPT) << FENV_FLAG_SHIFT));
    return 0;
}

int fegetexceptflag(fexcept_t *flagp, int excepts) {
    excepts &= FE_ALL_ACCEPT;
    *flagp = (FPSCR >> FENV_FLAG_SHIFT) & excepts;
    return 0;
}

int fesetexceptflag(const fexcept_t *flagp, int excepts) {
    int fpscr = FPSCR;
    fpscr &= ~((excepts & FE_ALL_ACCEPT) << FENV_FLAG_SHIFT);
    fpscr |= (*flagp & excepts & FE_ALL_ACCEPT) << FENV_FLAG_SHIFT;
    FPSCR_SET(fpscr);
    return 0;
}

#define FENV_RM_MASK 0x3

int fegetround(void) {
    return FPSCR & FENV_RM_MASK;
}

int fesetround(int round) {
    if(round & ~0x1)
        return 1;

    int fpscr = FPSCR;
    fpscr &= ~FENV_RM_MASK;
    fpscr |= round;
    FPSCR_SET(fpscr);

    return 0;
}

int fegetenv(fenv_t *envp) {
    *envp = (FPSCR >> FENV_ENABLE_SHIFT) & FENV_EXCEPT_MASK;
    return 0;
}

int feholdexcept(fenv_t *envp) {
    fegetenv(envp);
    fedisableexcept(FE_ALL_ACCEPT);
    return 0;
}

int fesetenv(const fenv_t *envp) {
    int fpscr = FPSCR;
    fpscr &= ~(FENV_EXCEPT_MASK << FENV_ENABLE_SHIFT);
    fpscr |= ((*envp & FENV_EXCEPT_MASK) << FENV_ENABLE_SHIFT);
    FPSCR_SET(fpscr);
    return 0;
}

int feupdateenv(const fenv_t *envp) {
    int excepts = fegetexcept();
    fesetenv(envp);
    return feraiseexcept(excepts);
}