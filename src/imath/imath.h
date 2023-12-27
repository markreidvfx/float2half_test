
#include <stdint.h>

uint16_t f32_to_f16_imath(float f);
float f16_to_f32_imath(uint16_t h);

void f32_to_f16_buffer_imath(uint32_t *data, uint16_t *result, int data_size);
void f16_to_f32_buffer_imath(uint16_t *data, uint32_t *result, int data_size);