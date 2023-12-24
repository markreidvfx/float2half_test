#include <stdint.h>

uint16_t f32_to_f16_ryg(float f);
float f16_to_f32_ryg(uint16_t h);

void f32_to_f16_buffer_ryg(uint32_t *data, uint16_t *result, int data_size);
void f16_to_f32_buffer_ryg(uint16_t *data, uint32_t *result, int data_size);