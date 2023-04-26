#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <float.h>
#include <time.h>
#include <inttypes.h>

#include "imath_half.h"

typedef union {
        uint32_t i;
        float    f;
} int_float;

#if _WIN32
#include <windows.h>
#include <intrin.h>
#define strdup _strdup

static uint64_t get_timer_frequency()
{
    LARGE_INTEGER Result;
    QueryPerformanceFrequency(&Result);
    return Result.QuadPart;
}
static uint64_t get_timer(void)
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result.QuadPart;
}
#else
#include <time.h>
#include <unistd.h>
static uint64_t get_timer_frequency()
{
    uint64_t Result = 1000000000ull;
    return Result;
}
static uint64_t get_timer(void)
{
    struct timespec Spec;
    clock_gettime(CLOCK_MONOTONIC, &Spec);
    uint64_t Result = ((uint64_t)Spec.tv_sec * 1000000000ull) + (uint64_t)Spec.tv_nsec;
    return Result;
}
#endif

#if _MSC_VER
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__ ((noinline))
#endif


#ifdef USE_ARM
static uint16_t to_f16(float v)
{
    union {
        _Float16 f;
        uint16_t i;
    } u;
    u.f = v;
    return u.i;
}
#else
#include <immintrin.h>
static inline uint16_t to_f16(float v)
{
    uint16_t result[8] = {0};
    __m128 ps =_mm_set1_ps(v);
    __m128i ph = _mm_cvtps_ph(ps, _MM_FROUND_TO_NEAREST_INT);

    _mm_storeu_si128((__m128i*)result, ph);

    return result[0];
}
#endif

typedef struct Float2HalfTables {
    uint16_t basetable[512];
    uint8_t shifttable[512]; // first bit used to indicate rounding is needed
} Float2HalfTables;

static Float2HalfTables f2h_table;

void init_float2half_tables(Float2HalfTables *t)
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

static inline uint16_t table_float2half_no_round(uint32_t f)
{
    uint16_t h;
    const Float2HalfTables *t = &f2h_table;

    int i = (f >> 23) & 0x01FF;
    int shift = t->shifttable[i] >> 1;
    h = t->basetable[i] + ((f & 0x007FFFFF) >> shift);
    return h;
}

static inline uint16_t table_float2half_round(uint32_t f)
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

static inline uint16_t rounded(uint32_t value, int g, int s)
{
    return value + (g & (s | value));
}

static inline uint16_t float2half_full(uint32_t f)
{

    uint16_t sign = (f >> 16) & 0x8000;
    f &= 0x7FFFFFFF;

     // inf or nan
    if(f >= 0x7F800000) {
        if (f > 0x7F800000)
            return sign | 0x7C00 | (0x0200 | ((f >> 13) & 0x03FF));
        return sign | 0x7C00;
    }

    // too large, round to infinity
    if(f >= 0x47800000)
        return sign | 0x7C00;

    // exponent large enough to result in a normal number, round and return
    if(f >= 0x38800000)
        return rounded(sign | (((f>>23)-112)<<10) | ((f>>13)&0x03FF), (f>>12)&1, (f&0x0FFF)!=0);

    // denormal
    if(f >= 0x33000000){
        int i = 125 - (f>>23);
        f = (f & 0x007FFFFF) | 0x00800000;
        return rounded(sign | (f >> (i+1)), (f>>i)&1, (f & ((1 << i) - 1 ))!=0);
    }
    // zero or -zero
    return sign;
}

uint64_t rand_uint64(void) {
  uint64_t r = 0;
  for (int i=0; i<64; i += 15 /*30*/) {
    r = r*((uint64_t)RAND_MAX + 1) + rand();
  }
  return r;
}

void NOINLINE test_hardware_perf(uint32_t *data, uint16_t *result, int data_size)
{
    int_float value;
    for (int i =0; i < data_size; i++) {
        value.i = data[i];
        result[i] = to_f16(value.f);
    }
}

void NOINLINE test_table_perf(uint32_t *data, uint16_t *result, int data_size)
{
    int_float value;
    for (int i =0; i < data_size; i++) {
        value.i = data[i];
        result[i] = table_float2half_no_round(value.i);
    }
}

void NOINLINE test_table_rounding_perf(uint32_t *data, uint16_t *result, int data_size)
{
    int_float value;
    for (int i =0; i < data_size; i++) {
        value.i = data[i];
        result[i] = table_float2half_round(value.i);
    }
}

void NOINLINE test_float2half_full_perf(uint32_t *data, uint16_t *result, int data_size)
{
    int_float value;
    for (int i =0; i < data_size; i++) {
        value.i = data[i];
        result[i] = float2half_full(value.i);
    }
}

void NOINLINE test_imath_float_to_half_perf(uint32_t *data, uint16_t *result, int data_size)
{
    int_float value;
    for (int i =0; i < data_size; i++) {
        value.i = data[i];
        result[i] = imath_float_to_half(value.f);
    }
}

#define PRINT_ERROR_RESULT(name, count) \
    printf("%-20s : %f%% err\n", name,   100.0 * count/(double)UINT32_MAX)

void test_hardware_accuracy()
{
    int_float value;
    uint16_t r0, r1;

    uint32_t errors[4] = {0};

    for (uint64_t i =0; i <= UINT32_MAX; i++) {
        // test every possible float value
        value.i = (uint32_t)i;
        r0 = to_f16(value.f);

        r1 = table_float2half_no_round(value.i);
        errors[0] += (r0 != r1);

        r1 = table_float2half_round(value.i);
        errors[1] += (r0 != r1);

        r1 = float2half_full(value.i);
        errors[2] += (r0 != r1);

        r1 = imath_float_to_half(value.f);
        errors[3] += (r0 != r1);
    }

    PRINT_ERROR_RESULT("table no rounding", errors[0]);
    PRINT_ERROR_RESULT("table rounding",    errors[1]);
    PRINT_ERROR_RESULT("no table",          errors[2]);
    PRINT_ERROR_RESULT("imath half",        errors[3]);

}

#define TIME_FUNC(name, func)                                   \
    start = get_timer();                                        \
    func;                                                       \
    elapse = (double)((get_timer() - start)) / (double)freq;    \
    printf("%-20s : %f secs\n", name, elapse)

#define TEST_SIZE 10000000

int main(int argc, char *argv[])
{
    uint16_t r0, r1;
    int_float value;
    uint64_t freq = get_timer_frequency();
    uint64_t start;
    double elapse;

    init_float2half_tables(&f2h_table);

    // init_test_data
    uint32_t *data = (uint32_t*) malloc(sizeof(uint32_t) * TEST_SIZE);
    uint16_t *result = (uint16_t*) malloc(sizeof(uint16_t) * TEST_SIZE);

    if (!data || !result) {
        printf("malloc error");
        return 0;
    }

    // fill up buffer with random data
    srand(time(NULL));
    for (int i =0; i < TEST_SIZE; i++) {
        data[i] = rand_uint64();
    }

    printf("runs: %d\n", TEST_SIZE);
    TIME_FUNC("hardware",          test_hardware_perf(data, result, TEST_SIZE));
    TIME_FUNC("table no rounding", test_table_perf(data, result, TEST_SIZE));
    TIME_FUNC("table rounding",    test_table_rounding_perf(data, result, TEST_SIZE));
    TIME_FUNC("no table",          test_float2half_full_perf(data, result, TEST_SIZE));
    TIME_FUNC("imath half",        test_imath_float_to_half_perf(data, result, TEST_SIZE));

    free(data);
    free(result);
    fflush(stdout);

    printf("\nchecking accuracy\n");
    test_hardware_accuracy();
}