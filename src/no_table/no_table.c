#include "no_table.h"

typedef union {
        uint32_t i;
        float    f;
} int_float;

static inline uint16_t rounded(uint32_t value, int g, int s)
{
    return value + (g & (s | value));
}

static inline uint16_t float2half_full(uint32_t f)
{

    uint16_t sign = (f >> 16) & 0x8000;
    f &= 0x7FFFFFFF;

     // inf or nan
    if(f >= 0x7F800000) {
        if (f > 0x7F800000)
            return sign | 0x7C00 | (0x0200 | ((f >> 13) & 0x03FF));
        return sign | 0x7C00;
    }

    // too large, round to infinity
    if(f >= 0x47800000)
        return sign | 0x7C00;

    // exponent large enough to result in a normal number, round and return
    if(f >= 0x38800000)
        return rounded(sign | (((f>>23)-112)<<10) | ((f>>13)&0x03FF), (f>>12)&1, (f&0x0FFF)!=0);

    // denormal
    if(f >= 0x33000000){
        int i = 125 - (f>>23);
        f = (f & 0x007FFFFF) | 0x00800000;
        return rounded(sign | (f >> (i+1)), (f>>i)&1, (f & ((1 << i) - 1 ))!=0);
    }
    // zero or -zero
    return sign;
}


uint16_t f32_to_f16_no_table(float f)
{
    int_float value;
    value.f = f;
    return float2half_full(value.i);
}

void f32_to_f16_buffer_no_table(uint32_t *data, uint16_t *result, int data_size)
{
    int_float value;
    for (int i =0; i < data_size; i++) {
        value.i = data[i];
        result[i] = float2half_full(value.i);
    }
}
