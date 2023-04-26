#include "imath.h"
#include "imath_half.h"

uint16_t f32_to_f16_imath(float f)
{
    return imath_float_to_half(f);
}

void f32_to_f16_buffer_imath(uint32_t *data, uint16_t *result, int data_size)
{
    imath_half_uif_t value;
    for (int i =0; i < data_size; i++) {
        value.i = data[i];
        result[i] = imath_float_to_half(value.f);
    }
}