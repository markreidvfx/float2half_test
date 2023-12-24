#include <stdint.h>

void init_tables();

uint16_t f32_to_f16_table(float f);
float f16_to_f32_table(uint16_t h);

void f32_to_f16_buffer_table(uint32_t *data, uint16_t *result, int data_size);
void f16_to_f32_buffer_table(uint16_t *data, uint32_t *result, int data_size);