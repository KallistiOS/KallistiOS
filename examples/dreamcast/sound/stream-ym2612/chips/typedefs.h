#include <stdint.h>

#define INLINE static inline


typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint32_t offs_t;
typedef int32_t stream_sample_t;
typedef int32_t INT32;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef stream_sample_t FMSAMPLE;
typedef void (*FM_TIMERHANDLER)(void*, int, int, int);
typedef void (*FM_IRQHANDLER)(void*, int);

#ifndef _have_ssg_callbacks
#define _have_ssg_callbacks
typedef struct {
    void (*set_clock)(void*, int);
    void (*write)(void*, int, int);
    int (*read)(void*);
    void (*reset)(void*);
} ssg_callbacks;
#endif
