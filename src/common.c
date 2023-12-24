#include "common.h"
#include <stdio.h>
#include <stdlib.h>

uint32_t rand_uint32(void)
{
  return  ((0x7fff & rand()) << 30) | ((0x7fff & rand()) << 15) | (0x7fff & rand());
}

int rand_uint32_real()
{
    int_float v;
    for (;;) {
        v.u = rand_uint32();

        if ((v.u &= 0x7FFFFFFF) <= 0x477fefff) {
            // uint16_t r0 = f32_to_f16_hw(v.f);
            // float f = f16_to_f32_hw(r0);
            // assert(!(isnan(f) || isinf(f)));
            return v.u;
        }
    }
}

void randomize_buffer_u32(uint32_t *data, size_t size, int real_only)
{
    // fill up buffer with random data
    for (size_t i =0; i < size; i++) {
        if (real_only)
            data[i] = rand_uint32_real();
        else
            data[i] = rand_uint32();

        // printf("0x%08x\n", data[i]);
        if ((i % 20000000) == 0){
            printf("\rrandomizing buffers: %4.1f%%", 100.0 * i/(double)size);
            fflush(stdout);
        }
    }
    printf("\r");
}