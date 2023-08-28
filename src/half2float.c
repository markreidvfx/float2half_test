#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <float.h>
#include <time.h>

#include "platform_info.h"
#include "hardware/hardware.h"

#if defined(ARCH_X86)
#include "x86_cpu_info.h"
#include "emmintrin.h"
#endif

#include "half2float_table.h"


typedef union
{
    uint32_t u;
    float f;
} int_float;


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

// https://fgiesen.wordpress.com/2012/03/28/half-to-float-done-quic/
static float half_to_float_ryg(uint16_t h)
{
    static const int_float magic = { (254 - 15) << 23 };
    static const int_float was_infnan = { (127 + 16) << 23 };
    int_float o;

    o.u = (h & 0x7fff) << 13;     // exponent/mantissa bits
    o.f *= magic.f;               // exponent adjust
    if (o.f >= was_infnan.f) {    // make sure Inf/NaN survive
        if (o.u == was_infnan.u)
            // was ininity
            o.u |= 0x00FF << 23;
        else {
            // was nan, match hardware nan
            o.u |= 0x01FF << 22;
        }
    }
    o.u |= (h & 0x8000) << 16;    // sign bit
    return o.f;
}

#if defined(ARCH_X86)

static inline __m128i sse2_blendv(__m128i a, __m128i b, __m128i mask)
{
    return _mm_xor_si128(_mm_and_si128(_mm_xor_si128(a, b), mask), a);
}
static inline __m128 sse2_cvtph_ps(__m128i a)
{
    __m128 magic      = _mm_castsi128_ps(_mm_set1_epi32((254 - 15) << 23));
    __m128 was_infnan = _mm_castsi128_ps(_mm_set1_epi32((127 + 16) << 23));
    __m128 sign;
    __m128 o;

    // the values to unpack are in the lower 64 bits
    // | 0 1 | 2 3 | 4 5 | 6 7 | 8 9 | 10 11 | 12 13 | 14 15
    // | 0 1 | 0 1 | 2 3 | 2 3 | 4 5 |  4  5 | 6   7 | 6   7
    a = _mm_unpacklo_epi16(a, a);

    // extract sign
    sign = _mm_castsi128_ps(_mm_slli_epi32(_mm_and_si128(a, _mm_set1_epi32(0x8000)), 16));

    // extract exponent/mantissa bits
    o = _mm_castsi128_ps(_mm_slli_epi32(_mm_and_si128(a, _mm_set1_epi32(0x7fff)), 13));

    // magic multiply
    o = _mm_mul_ps(o, magic);

    // blend in inf/nan values only if present
    __m128i mask = _mm_castps_si128(_mm_cmpge_ps(o, was_infnan));
    if (_mm_movemask_epi8(mask)) {
        __m128i ou =  _mm_castps_si128(o);
        __m128i ou_nan = _mm_or_si128(ou, _mm_set1_epi32( 0x01FF << 22));
        __m128i ou_inf = _mm_or_si128(ou, _mm_set1_epi32( 0x00FF << 23));

        // blend in nans
        ou = sse2_blendv(ou, ou_nan, mask);

        // blend in infinities
        mask = _mm_cmpeq_epi32( _mm_castps_si128(o), _mm_castps_si128(was_infnan));
        o  = _mm_castsi128_ps(sse2_blendv(ou, ou_inf, mask));
    }

    return  _mm_or_ps(o, sign);
}

#endif

int main(int argc, char *argv[])
{

#if defined(ARCH_X86)
    // print cpu name and check for f16c instruction
    CPUInfo info = {0};
    get_cpu_info(&info);
    printf("CPU: %s %s %s\n", CPU_ARCH, info.name, info.extensions);
    if (!(info.flags & X86_CPU_FLAG_F16C)) {
        printf("** CPU does not support f16c instruction**\n");
    }
#else
    printf("CPU: %s %s\n", CPU_ARCH, get_cpu_model_name());
#endif

    int_float a;
    int_float b;
    uint64_t freq, start;
    double elapse;

    ff_init_half2float_tables(&h2f_table);

    freq = get_timer_frequency();
    start = get_timer();

    for (int i = 0; i <= UINT16_MAX-4; i++) {
        a.u = f16_to_f32_table[i];
        b.u = table_half2float(i, &h2f_table);

        if (a.u != b.u) {
            printf("%05d 0x%08X != 0x%08X %f %f\n", i, a.u, b.u, a.f, b.f);
            // printf("0x%08X\n", a.i - b.i);
        }

    }
    elapse = (double)((get_timer() - start)) / (double)freq;
    printf("table_half2float complete in %f secs\n", elapse);


    freq = get_timer_frequency();
    start = get_timer();

    for (int i = 0; i <= UINT16_MAX-4; i++) {
        a.u = f16_to_f32_table[i];
        b.f = half_to_float_ryg(i);

        if (a.u != b.u) {
            printf("%05d 0x%08X != 0x%08X %f %f\n", i, a.u, b.u, a.f, b.f);
            // printf("0x%08X\n", a.i - b.i);
        }

    }
    elapse = (double)((get_timer() - start)) / (double)freq;
    printf("half_to_float_ryg complete in %f secs\n", elapse);

#if defined(ARCH_X86)
    freq = get_timer_frequency();
    start = get_timer();

        // a.f = f16_to_f32_hw(i);
    for (int i = 0; i <= UINT16_MAX-4; i++) {
        // a.u = f16_to_f32_table[i];
        float result[4];
        __m128i v = _mm_set_epi16(0, 1, 2, 3, i+3, i+2, i+1, i);
        __m128 r = sse2_cvtph_ps(v);
        _mm_storeu_ps(result, r);

        for (int j = 0; j < 4; j++) {
            b.f = result[j];
            a.u = f16_to_f32_table[i+j];
            if (a.u != b.u) {
                printf("%05d 0x%08X != 0x%08X %f %f\n", i, a.u, b.u, a.f, b.f);
                // printf("0x%08X\n", a.i - b.i);
            }
        }
    }
    elapse = (double)((get_timer() - start)) / (double)freq;
    printf("sse2_cvtph_ps complete in %f secs\n", elapse);
#endif

    return 0;
}