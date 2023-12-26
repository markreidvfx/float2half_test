#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "hardware/hardware.h"

static uint32_t rand_u32()
{
  return  ((0x7fff & rand()) << 30) | ((0x7fff & rand()) << 15) | (0x7fff & rand());
}

static uint16_t rand_u16()
{
    uint32_t v = ((0x7fff & rand()) << 15) | (0x7fff & rand());
    return (uint16_t)(0xFFFF & v);
}

static uint16_t rand_u16_real()
{
    for (;;) {
        uint16_t v = rand_u16();
        if ((v & 0x7FFF) <= 0x7BFF) {

            // float f = f16_to_f32_hw(v);
            // assert(!(isnan(f) || isinf(f)));
            return v;
        }
    }
}

static int rand_u32_real()
{
    int_float v;
    for (;;) {
        v.u = rand_u32();

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
            data[i] = rand_u32_real();
        else
            data[i] = rand_u32();

        // printf("0x%08x\n", data[i]);
        if ((i % 20000000) == 0){
            printf("\rrandomizing buffers: %4.1f%%", 100.0 * i/(double)size);
            fflush(stdout);
        }
    }
    printf("\r");
}

void randomize_buffer_u16(uint16_t *data, size_t size, int real_only)
{
    // fill up buffer with random data
    for (size_t i =0; i < size; i++) {
        if (real_only)
            data[i] = rand_u16_real();
        else
            data[i] = rand_u16();

        if ((i % 20000000) == 0){
            printf("\rrandomizing buffers: %4.1f%%", 100.0 * i/(double)size);
            fflush(stdout);
        }
    }
    printf("\r");
}