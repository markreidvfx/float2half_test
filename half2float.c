#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <float.h>
#include <time.h>

#ifdef USE_ARM
static uint32_t to_f32(uint16_t v)
{
    union {
        float f;
        uint32_t i;
    }a;
    union {
        _Float16 f;
        uint16_t i;
    } b;

    b.i = v;
    a.f = b.f;
    return a.i;
}
#else
#include <immintrin.h>
static uint32_t to_f32(uint16_t v)
{
    uint32_t result[4] = {};
    uint16_t src[8] = {v, v, v, v, v, v, v, v};

    __m128i ph = _mm_loadu_si128((__m128i*)&src);
    __m128 ps = _mm_cvtph_ps(ph);

    _mm_storeu_si128((__m128i*)result, _mm_castps_si128(ps));

    return result[0];
}
#endif

typedef struct Half2FloatTables {
    uint32_t mantissatable[3072];
    uint32_t exponenttable[64];
    uint16_t offsettable[64];
} Half2FloatTables;

static Half2FloatTables h2f_table;

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

void ff_init_half2float_tables(Half2FloatTables *t)
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


static inline uint32_t table_half2float(uint16_t h, const Half2FloatTables *t)
{
    uint32_t f;
    f = t->mantissatable[t->offsettable[h >> 10] + (h & 0x3ff)] + t->exponenttable[h >> 10];
    return f;
}

int main(int argc, char *argv[])
{

    union {
        float f;
        uint32_t i;
    }a;

    union {
        float f;
        uint32_t i;
    }b;

    ff_init_half2float_tables(&h2f_table);

    for (int i = 0; i < UINT16_MAX; i++) {
        a.i = to_f32(i);
        b.i = table_half2float(i, &h2f_table);

        if (a.i != b.i) {
            printf("%05d 0x%08X != 0x%08X %f\n", i, a.i, b.i, a.f);
        }

    }

    return 0;
}