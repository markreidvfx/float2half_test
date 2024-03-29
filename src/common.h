#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stddef.h>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

#define TEST_RUNS 50
#define BUFFER_SIZE (1920*1080*4)

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef union {
        uint32_t u;
        float    f;
} int_float;

void randomize_buffer_u32(uint32_t *data, size_t size, int real_only);
void randomize_buffer_u16(uint16_t *data, size_t size, int real_only);

#endif // COMMON_H