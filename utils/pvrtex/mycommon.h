#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define ARR_SIZE(array)	(sizeof(array) / sizeof(array[0]))

static inline bool IsPow2(uint32_t val) {
	return ((val - 1) & val) == 0;
}
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define CLAMP(small,num,big) (MAX((small),MIN((num),(big))))

#define ROUND_UP_POW2(val, pow_of_2_amt) (((val) + ((pow_of_2_amt)-1)) & ~((pow_of_2_amt)-1))

static inline float lerp(float ratio, float a, float b) {
	return ratio * b + (a - ratio * a);
}

#if 1
#define SAFE_FREE(ptr) \
	if (*(ptr) != NULL) { free(*(ptr)); *(ptr) = NULL; }
#define SMART_ALLOC(ptr, size) \
	do { SAFE_FREE(ptr); *ptr = calloc(size, 1); } while(0)
#else
static void SafeFree(void **ptr) {
	if (*ptr != NULL) {
		free(*ptr);
		*ptr = NULL;
	}
}
#endif

extern unsigned RoundUpPow2(unsigned val);
extern unsigned RoundDownPow2(unsigned val);
unsigned RoundNearest(unsigned val, unsigned round);
int SelectNearest(int down, int val, int up);
void ErrorExit(const char *fmt, ...);
