/* 
   KallistiOS 2.0.0

   timer.c
   (C) 2013 Josh Pearson
*/

#include "timer.h"

static uint32_t s, ms;
static uint64_t msec;

/* Get current hardware timing using arch/timer.h */
unsigned int GetTime(void) {
    timer_ms_gettime(&s, &ms);
    msec = (((uint64_t)s) * ((uint64_t)1000)) + ((uint64_t)ms);
    return (unsigned int)msec;
}


