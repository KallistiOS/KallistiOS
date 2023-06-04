/* 
   KallistiOS 2.0.0

   timer.h
   (C) 2013 Josh Pearson
*/

#ifndef DC_TIMER_H
#define DC_TIMER_H

/* Timer for frame-rate management ********************************************/
#include <time.h>
#include <arch/timer.h>

/* Get current hardware timing using arch/timer.h */
unsigned int GetTime();

#endif
