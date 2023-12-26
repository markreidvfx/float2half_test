#include <stdint.h>

float f16_to_f32_ryg_sse2(uint16_t h);
void f16_to_f32_buffer_ryg_sse2(uint16_t *data, uint32_t *result, int data_size);