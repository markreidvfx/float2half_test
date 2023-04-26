#include <stdint.h>

void init_table();

uint16_t f32_to_f16_table(float f);
void f32_to_f16_buffer_table(uint32_t *data, uint16_t *result, int data_size);