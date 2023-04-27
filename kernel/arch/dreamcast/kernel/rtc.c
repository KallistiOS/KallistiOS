/* KallistiOS ##version##

   rtc.c
   (c)2001 Megan Potter
   (c)2023 Falco Girgis
*/

/* Real Time Clock support

   The functions in here return various info about the real-world time and
   date stored in the machine. The general process here is to retrieve
   the date/time value and then use the other functions to interpret it.

   rtc_get_time() should return a UNIX-epoch time stamp, and then the normal
   BSD library functions can be used to interpret that time stamp.

   For the Dreamcast, the RTC is a 32-bit seconds counter located at
   0xa0710000 and 0xa0710004 (each 32-bits long). 0000 contains the high
   16 bits and 0004 contains the low 16 bits. The epoch of this counter is
   January 1, 1950, 00:00. So we just grab that value and add about
   20 years to it.

 */

#include <arch/rtc.h>
#include <arch/timer.h>
#include <dc/g2bus.h>

#define RTC_CONTROL_ADDR        0xa0710008  /* SFR with write-enable bit */
#define RTC_SECONDS_HIGH_ADDR   0xa0710000  /* Upper 16 bits of timestamp */
#define RTC_SECONDS_LOW_ADDR    0xa0710004  /* Lower 16 bits of timestamp */
#define RTC_UNIX_EPOCH_DELTA    631152000   /* Twenty years in seconds */
#define RTC_RETRY_COUNT         3           /* # of times to repeat on bad access */

/* The boot time; we'll save this in rtc_init() */
static time_t boot_time = 0;

/* Returns the date/time value as a UNIX epoch time stamp */
time_t rtc_unix_secs(void) {
    uint32 rtcold, rtcnew;
    int i;

    /* Try several times to make sure we don't read one value, then the
       clock increments itself, then we read the second value. This
       algorithm is from NetBSD. */
    rtcold = 0;

    for(;;) {
        for(i = 0; i < RTC_RETRY_COUNT; i++) {
            rtcnew = ((g2_read_32(RTC_SECONDS_HIGH_ADDR) & 0xffff) << 16) | 
                      (g2_read_32(RTC_SECONDS_LOW_ADDR) & 0xffff);

            if(rtcnew != rtcold)
                break;
        }

        if(i < 3)
            rtcold = rtcnew;
        else
            break;
    }

    /* Subtract out 20 years */
    rtcnew = rtcnew - RTC_UNIX_EPOCH_DELTA;

    return rtcnew;
}

/* Sets the date/time value from a UNIX epoch time stamp, 
   returning 0 for success or -1 for failure. */
int rtc_set_unix_secs(time_t secs) {
    int result = 0;
    uint32_t rtcnew;
    int i;

    /* Adjust by 20 years to get to the expected RTC time. */
    const uint32_t adjusted = secs + RTC_UNIX_EPOCH_DELTA;

    /* Enable writing by setting LSB of control */
    g2_write_32(RTC_CONTROL_ADDR, 0x1);

    /* Try 3 times to ensure we didn't write a value then have 
       the clock increment itself before the next. */
    for(i = 0; i < RTC_RETRY_COUNT; i++) { 
        /* Write the least-significant 16-bits first, because 
           writing to the high 16-bits will lock RTC writes. */
        g2_write_32(RTC_SECONDS_LOW_ADDR, (adjusted) & 0xffff);
        g2_write_32(RTC_SECONDS_HIGH_ADDR, (adjusted >> 16) & 0xffff);

        /* Read the time back again, to ensure it was written properly. */
        rtcnew = ((g2_read_32(RTC_SECONDS_HIGH_ADDR) & 0xffff) << 16) | 
                  (g2_read_32(RTC_SECONDS_LOW_ADDR) & 0xffff);

        if(rtcnew == adjusted)
            break;
    }

    /* Signify failure if the fetched time never matched the
       time we attempted to set. */
    if(i == RTC_RETRY_COUNT)
        result = -1;

    /* We have to update the boot time now as well, subtracting
       the amount of time that has elapsed since boot from the 
       new time we've just set. */
    uint32 s, ms;
    timer_ms_gettime(&s, &ms);
    boot_time = rtcnew - RTC_UNIX_EPOCH_DELTA - s;

    return result;
}

/* Returns the date/time that the system was booted as a UNIX epoch time
   stamp. Adding this to the value from timer_ms_gettime() will
   produce a current timestamp without needing the trip over the G2 BUS. */
time_t rtc_boot_time(void) {
    return boot_time;
}

int rtc_init(void) {
    boot_time = rtc_unix_secs();
    return 0;
}

void rtc_shutdown(void) {
}
