#include "mycommon.h"

unsigned RoundUpPow2(unsigned val) {
	val--;
	val |= val >> 1;
	val |= val >> 2;
	val |= val >> 4;
	val |= val >> 8;
	val |= val >> 16;
	return val+1;
}
unsigned RoundDownPow2(unsigned val) {
	if (IsPow2(val))
		return val;
	val--;
	val |= val >> 1;
	val |= val >> 2;
	val |= val >> 4;
	val |= val >> 8;
	val |= val >> 16;
	return (val+1)>>1;
}

int SelectNearest(int down, int val, int up) {
	return (abs(val - up) < abs(val - down)) ? up : down;
}

unsigned RoundNearest(unsigned val, unsigned round) {
	val = val + round/2;
	val /= round;
	return val * round;
}
