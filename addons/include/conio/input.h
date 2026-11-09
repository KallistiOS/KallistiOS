/* KallistiOS ##version##

 input.h

 (c)2002 Megan Potter

 Adapted from Kosh, (c)2000 Jordan DeLong
*/

#ifndef __CONIO_INPUT_H
#define __CONIO_INPUT_H

/* size of the input buffer */
#define CONIO_INPUT_BUFFSIZE    256

/* functions */
void conio_input_frame(void);
void conio_input_init(void);
void conio_input_shutdown(void);

typedef void (*conio_input_callback_t)(const char *str);
void conio_input_callback(conio_input_callback_t cb);

/* Default conio input system: call with block > 0 to wait for the user
   to type something; the output will be placed in dst, which should
   be at least dstcnt bytes large. Block, if non-zero, is the maximum number
   of milliseconds to block before giving up (in which case we abort and
   return -1); if block < 0, we wait infinitely. Returns 0 on success,
   -1 on failure. */
int conio_input_getline(int block, char *dst, int dstcnt);

#endif
