#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <float.h>
#include <time.h>
#include <math.h>

#include "common.h"
#include "platform_info.h"
#include "hardware/hardware.h"
#include "table/table.h"
#include "ryg/ryg.h"

#if defined(ARCH_X86)
#include "x86_cpu_info.h"
#include "emmintrin.h"
#endif

#include "half2float_table.h"

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

void f16_to_f32_buffer_static_table(uint16_t *data, uint32_t *result, int data_size)
{
    for (int i =0; i < data_size; i++) {
        result[i] = f16_to_f32_static_table[data[i]];
    }
}


typedef struct F16Test {
    const char *name;
    float (*f16_to_f32)(uint16_t h);
    void (*f16_to_f32_buffer)(uint16_t *data, uint32_t *result, int data_size);
} F16Test;

const static F16Test f16_tests[] =
{
    {"hardware",            f16_to_f32_hw,                 f16_to_f32_buffer_hw },
    {"static_table",        NULL,                          f16_to_f32_buffer_static_table },
    {"table",               f16_to_f32_table,              f16_to_f32_buffer_table },
    {"ryg",                 f16_to_f32_ryg,                f16_to_f32_buffer_ryg },
};

#define TEST_COUNT ARRAY_SIZE(f16_tests)

int validate(uint16_t *src, uint32_t *result, size_t size)
{
    for (size_t i = 0;  i < size; i++) {
        uint32_t t = f16_to_f32_static_table[src[i]];
        if (result[i] != t) {
            printf("0x%08X != 0x%08X\n", result[i], t);
            return 1;
        }
    }
    return 0;
}

#define TIME_FUNC(name, func, buffer_size, runs)                            \
    min_value = INFINITY;                                                   \
    max_value = -INFINITY;                                                  \
    average = 0.0;                                                          \
    ptr = data;                                                             \
    for (size_t j = 0; j < runs; j++) {                                     \
        start = get_timer();                                                \
        func(ptr, result, buffer_size);                                     \
        elapse = (double)((get_timer() - start)) / (double)freq;            \
        min_value = MIN(min_value, elapse);                                 \
        max_value = MAX(max_value, elapse);                                 \
        average += elapse * 1.0 / (double)runs;                             \
        assert(!validate(ptr, result, buffer_size));                        \
        ptr += buffer_size;                                                 \
    }                                                                       \
                                                                            \
    printf("%-20s : %f %f %f secs\n", name, min_value, average, max_value);

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
    double average;
    double min_value;
    double max_value;
    uint16_t *ptr;

    init_tables();

    freq = get_timer_frequency();
    start = get_timer();

    for (int i = 0; i <= UINT16_MAX-4; i++) {
        a.u = f16_to_f32_static_table[i];
        b.f = f16_to_f32_table(i);

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
        a.u = f16_to_f32_static_table[i];
        b.f = f16_to_f32_ryg(i);

        // printf("0x%04x %f\n", i &  0x7FFF, a.f);

        if (a.u != b.u) {
            printf("%05d 0x%08X != 0x%08X %f %f\n", i, a.u, b.u, a.f, b.f);
            // printf("0x%08X\n", a.i - b.i);
        }

    }
    elapse = (double)((get_timer() - start)) / (double)freq;
    printf("half_to_float_ryg complete in %f secs\n", elapse);

    uint16_t *data = (uint16_t*) malloc(sizeof(uint16_t) * BUFFER_SIZE * TEST_RUNS);
    uint32_t *result = (uint32_t*) malloc(sizeof(uint32_t) * BUFFER_SIZE * TEST_RUNS);

    srand(time(NULL));
    printf("\r\nruns: %d, buffer size: %d, random f16 <= HALF_MAX\n\n", TEST_RUNS, BUFFER_SIZE);
    randomize_buffer_u16(data, BUFFER_SIZE * TEST_RUNS, 1);

    printf("%-20s :      min      avg     max\n", "name");
    for (size_t i = 0; i < TEST_COUNT; i++) {
        TIME_FUNC(f16_tests[i].name, f16_tests[i].f16_to_f32_buffer, BUFFER_SIZE, TEST_RUNS);
    }
    fflush(stdout);

    srand(time(NULL));
    printf("\r\nruns: %d, buffer size: %d, random f16 full +inf+nan\n\n", TEST_RUNS, BUFFER_SIZE);
    randomize_buffer_u16(data, BUFFER_SIZE * TEST_RUNS, 0);

    printf("%-20s :      min      avg     max\n", "name");
    for (size_t i = 0; i < TEST_COUNT; i++) {
        TIME_FUNC(f16_tests[i].name, f16_tests[i].f16_to_f32_buffer, BUFFER_SIZE, TEST_RUNS);
    }

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