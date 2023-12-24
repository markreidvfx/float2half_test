#include <stdint.h>
#include <stddef.h>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

typedef union {
        uint32_t u;
        float    f;
} int_float;

void randomize_buffer_u32(uint32_t *data, size_t size, int real_only);