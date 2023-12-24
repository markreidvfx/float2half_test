#include "table.h"

typedef union {
        uint32_t i;
        float    f;
} int_float;

typedef struct Float2HalfTables {
    uint16_t basetable[512];
    uint8_t shifttable[512];
} Float2HalfTables;

typedef struct Half2FloatTables {
    uint32_t mantissatable[3072];
    uint32_t exponenttable[64];
    uint16_t offsettable[64];
} Half2FloatTables;

static Float2HalfTables f2h_table;
static Half2FloatTables h2f_table;

static void init_float2half_tables(Float2HalfTables *t)
{
    for (int i = 0; i < 256; i++) {
        int e = i - 127;

        if (e < -24) { // Very small numbers map to zero
            t->basetable[i|0x000]  = 0x0000;
            t->basetable[i|0x100]  = 0x8000;
            t->shifttable[i|0x000] = 24;
            t->shifttable[i|0x100] = 24;
        } else if (e < -14) { // Small numbers map to denorms
            t->basetable[i|0x000] = (0x0400>>(-e-14));
            t->basetable[i|0x100] = (0x0400>>(-e-14)) | 0x8000;
            t->shifttable[i|0x000] = -e-1;
            t->shifttable[i|0x100] = -e-1;
        } else if (e <= 15) { // Normal numbers just lose precision
            t->basetable[i|0x000] = ((e + 15) << 10);
            t->basetable[i|0x100] = ((e + 15) << 10) | 0x8000;
            t->shifttable[i|0x000] = 13;
            t->shifttable[i|0x100] = 13;
        } else if (e < 128) { // Large numbers map to Infinity
            t->basetable[i|0x000]  = 0x7C00;
            t->basetable[i|0x100]  = 0xFC00;
            t->shifttable[i|0x000] = 24;
            t->shifttable[i|0x100] = 24;
        } else { // Infinity and NaN's stay Infinity and NaN's
            t->basetable[i|0x000]  = 0x7C00;
            t->basetable[i|0x100]  = 0xFC00;
            t->shifttable[i|0x000] = 13;
            t->shifttable[i|0x100] = 13;
        }
    }
}

static inline uint16_t to_f16(uint32_t f)
{
    uint16_t h;
    const Float2HalfTables *t = &f2h_table;

    int i = (f >> 23) & 0x01FF;
    h = t->basetable[i] + ((f & 0x007FFFFF) >> t->shifttable[i]);
    return h;
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

static uint32_t convertmantissa(uint32_t i)
{
    int32_t m = i << 13; // Zero pad mantissa bits
    int32_t e = 0; // Zero exponent

    while (!(m & 0x00800000)) { // While not normalized
        e -= 0x00800000; // Decrement exponent (1<<23)
        m <<= 1; // Shift mantissa
    }

    m &= ~0x00800000; // Clear leading 1 bit
    e +=  0x38800000; // Adjust bias ((127-14)<<23)

    return m | e; // Return combined number
}

void init_half2float_tables(Half2FloatTables *t)
{
    t->mantissatable[0] = 0;
    for (int i = 1; i < 1024; i++)
        t->mantissatable[i] = convertmantissa(i);
    for (int i = 1024; i < 2048; i++)
        t->mantissatable[i] = 0x38000000UL + ((i - 1024) << 13UL);
    for (int i = 2048; i < 3072; i++)
        t->mantissatable[i] = t->mantissatable[i - 1024] | 0x400000UL;
    t->mantissatable[2048] = t->mantissatable[1024];

    t->exponenttable[0] = 0;
    for (int i = 1; i < 31; i++)
        t->exponenttable[i] = i << 23;
    for (int i = 33; i < 63; i++)
        t->exponenttable[i] = 0x80000000UL + ((i - 32) << 23UL);
    t->exponenttable[31]= 0x47800000UL;
    t->exponenttable[32]= 0x80000000UL;
    t->exponenttable[63]= 0xC7800000UL;

    t->offsettable[0] = 0;
    for (int i = 1; i < 64; i++)
        t->offsettable[i] = 1024;
    t->offsettable[31] = 2048;
    t->offsettable[32] = 0;
    t->offsettable[63] = 2048;
}

static inline uint32_t to_f32(uint16_t h)
{
    const Half2FloatTables *t = &h2f_table;
    uint32_t v;
    v = t->mantissatable[t->offsettable[h >> 10] + (h & 0x3ff)] + t->exponenttable[h >> 10];
    return v;
}

float f16_to_f32_table(uint16_t h)
{
    int_float value;
    value.i = to_f32(h);
    return value.f;
}

void f16_to_f32_buffer_table(uint16_t *data, uint32_t *result, int data_size)
{
    for (int i =0; i < data_size; i++) {
        result[i] = to_f32(data[i]);
    }
}

void init_tables()
{
    init_float2half_tables(&f2h_table);
    init_half2float_tables(&h2f_table);
}