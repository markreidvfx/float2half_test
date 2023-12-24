
#include <stdint.h>

uint16_t f32_to_f16_hw(float f);
float f16_to_f32_hw(uint16_t f);

void f32_to_f16_buffer_hw(uint32_t *data, uint16_t *result, int data_size);
void f16_to_f32_buffer_hw(uint16_t *data, uint32_t *result, int data_size);