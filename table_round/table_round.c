#include "table_round.h"

typedef union {
        uint32_t i;
        float    f;
} int_float;

typedef struct Float2HalfTables {
    uint16_t basetable[512];
    uint8_t shifttable[512]; // first bit used to indicate rounding is needed
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
    uint16_t round = t->shifttable[i] & 1;

    // bit shifting can lose nan, ensures nan stays nan
    uint16_t keep_nan = ((f & 0x7FFFFFFF) > 0x7F800000) << 9;
    // guard bit (most significant discarded bit)
    uint16_t g = ((f | 0x00800000) >> (shift - 1)) & round;
    // sticky bit (all but the most significant discarded bits)
    uint16_t s = (f << (33 - shift)) != 0;

    h = t->basetable[i] + ((f & 0x007FFFFF) >> shift);

    // round to nearest, ties to even
    h += (g & (s | h));

    // or 0x0200 if nan
    h |= keep_nan;
    return h;
}


void init_table_round()
{
    init_float2half_tables(&f2h_table);
}

uint16_t f32_to_f16_table_round(float f)
{
    int_float value;
    value.f = f;
    return to_f16(value.i);
}

void f32_to_f16_buffer_table_round(uint32_t *data, uint16_t *result, int data_size)
{
    int_float value;
    for (int i =0; i < data_size; i++) {
        value.i = data[i];
        result[i] = to_f16(value.i);
    }
}