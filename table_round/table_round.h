#include <stdint.h>

void init_table_round();

uint16_t f32_to_f16_table_round(float f);
void f32_to_f16_buffer_table_round(uint32_t *data, uint16_t *result, int data_size);