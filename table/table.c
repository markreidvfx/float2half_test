#include "table.h"

typedef union {
        uint32_t i;
        float    f;
} int_float;

typedef struct Float2HalfTables {
    uint16_t basetable[512];
    uint8_t shifttable[512];
} Float2HalfTables;

static Float2HalfTables f2h_table;

static void init_float2half_tables(Float2HalfTables *t)
{
    for (int i = 0; i < 256; i++) {
        int e = i - 127;

        if (e < -24) { // Very small numbers map to zero
            t->basetable[i|0x000]  = 0x0000;
            t->basetable[i|0x100]  = 0x8000;
            t->shifttable[i|0x000] = (24 << 1) | (e == -25 ? 1 : 0);
            t->shifttable[i|0x100] = (24 << 1) | (e == -25 ? 1 : 0);
        } else if (e < -14) { // Small numbers map to denorms
            t->basetable[i|0x000] = (0x0400>>(-e-14));
            t->basetable[i|0x100] = (0x0400>>(-e-14)) | 0x8000;
            t->shifttable[i|0x000] = ((-e-1) << 1) | 1;
            t->shifttable[i|0x100] = ((-e-1) << 1) | 1;
        } else if (e <= 15) { // Normal numbers just lose precision
            t->basetable[i|0x000] = ((e + 15) << 10);
            t->basetable[i|0x100] = ((e + 15) << 10) | 0x8000;
            t->shifttable[i|0x000] = (13 << 1) | 1;
            t->shifttable[i|0x100] = (13 << 1) | 1;
        } else if (e < 128) { // Large numbers map to Infinity
            t->basetable[i|0x000]  = 0x7C00;
            t->basetable[i|0x100]  = 0xFC00;
            t->shifttable[i|0x000] = 24 << 1;
            t->shifttable[i|0x100] = 24 << 1;
        } else { // Infinity and NaN's stay Infinity and NaN's
            t->basetable[i|0x000]  = 0x7C00;
            t->basetable[i|0x100]  = 0xFC00;
            t->shifttable[i|0x000] = 13 << 1;
            t->shifttable[i|0x100] = 13 << 1;
        }
    }
}

static inline uint16_t to_f16(uint32_t f)
{
    uint16_t h;
    const Float2HalfTables *t = &f2h_table;

    int i = (f >> 23) & 0x01FF;
    int shift = t->shifttable[i] >> 1;
    h = t->basetable[i] + ((f & 0x007FFFFF) >> shift);
    return h;
}


void init_table()
{
    init_float2half_tables(&f2h_table);
}

uint16_t f32_to_f16_table(float f)
{
    int_float value;
    value.f = f;
    return to_f16(value.i);
}

void f32_to_f16_buffer_table(uint32_t *data, uint16_t *result, int data_size)
{
    int_float value;
    for (int i =0; i < data_size; i++) {
        value.i = data[i];
        result[i] = to_f16(value.i);
    }
}