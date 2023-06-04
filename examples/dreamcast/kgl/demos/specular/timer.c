/* 
   KallistiOS 2.0.0

   timer.c
   (C) 2013 Josh Pearson
*/

#include "timer.h"

static uint32 s, ms;
static uint64 msec;

/* Get current hardware timing using arch/timer.h */
unsigned int GetTime() {
    timer_ms_gettime(&s, &ms);
    msec = (((uint64)s) * ((uint64)1000)) + ((uint64)ms);
    return (unsigned int)msec;
}


