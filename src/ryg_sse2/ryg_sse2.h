#include <stdint.h>

uint16_t f32_to_f16_ryg_sse2(float f);
float f16_to_f32_ryg_sse2(uint16_t h);

void f16_to_f32_buffer_ryg_sse2(uint16_t *data, uint32_t *result, int data_size);
void f32_to_f16_buffer_ryg_sse2(uint32_t *data, uint16_t *result, int data_size);