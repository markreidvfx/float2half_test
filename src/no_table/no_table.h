
#include <stdint.h>

uint16_t f32_to_f16_no_table(float f);
void f32_to_f16_buffer_no_table(uint32_t *data, uint16_t *result, int data_size);