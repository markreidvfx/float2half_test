#include <stdint.h>

uint16_t f32_to_f16_maratyszcza_sse2(float f);
void f32_to_f16_buffer_maratyszcza_sse2(uint32_t *data, uint16_t *result, int data_size);