#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <float.h>
#include <time.h>
#include <math.h>
#include <inttypes.h>

#include "imath_half.h"
#include "hardware/hardware.h"
#include "table/table.h"
#include "table_round/table_round.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#if defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(_M_IX86)
#define ARCH_X86
#endif

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
#ifdef ARCH_X86
#include <cpuid.h>
#endif

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

static void print_cpu_name()
{
#if defined(ARCH_X86)
    char CPU[65] = {0};
    for(int i = 0; i < 3; ++i)
    {
#if _WIN32
        __cpuid((int *)(CPU + 16*i), 0x80000002 + i);
#else
        __get_cpuid(0x80000002 + i,
                    (int unsigned *)(CPU + 16*i),
                    (int unsigned *)(CPU + 16*i + 4),
                    (int unsigned *)(CPU + 16*i + 8),
                    (int unsigned *)(CPU + 16*i + 12));
#endif
    }
    printf("CPU: %s\n", CPU);
#endif
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
        r0 = f32_to_f16_hw(value.f);

        r1 = f32_to_f16_table(value.f);
        errors[0] += (r0 != r1);

        r1 = f32_to_f16_table_round(value.f);
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

#define TEST_RUNS 24
#define BUFFER_SIZE (1920*1080*4)

void randomize_buffer(uint32_t *data)
{
    // fill up buffer with random data
    for (int i =0; i < BUFFER_SIZE; i++) {
        data[i] = rand_uint64();
    }
}

#define TIME_FUNC(name, func)                                             \
    min_value = INFINITY;                                                 \
    max_value = -INFINITY;                                                \
    average = 0.0;                                                        \
    srand(time(NULL));                                                    \
    for (int i = 0; i < TEST_RUNS; i++) {                                 \
        randomize_buffer(data);                                           \
        start = get_timer();                                              \
        func;                                                             \
        elapse = (double)((get_timer() - start)) / (double)freq;          \
        min_value = MIN(min_value, elapse);                               \
        max_value = MAX(max_value, elapse);                               \
        average += elapse * 1.0 / (double)TEST_RUNS;                      \
    }                                                                     \
                                                                          \
    printf("%-20s : %f %f %f secs\n", name, min_value, average, max_value)


int main(int argc, char *argv[])
{
    uint16_t r0, r1;
    int_float value;
    uint64_t freq = get_timer_frequency();
    uint64_t start;
    double elapse;
    double average;
    double min_value;
    double max_value;

    init_table();
    init_table_round();

    // init_test_data
    uint32_t *data = (uint32_t*) malloc(sizeof(uint32_t) * BUFFER_SIZE);
    uint16_t *result = (uint16_t*) malloc(sizeof(uint16_t) * BUFFER_SIZE);

    if (!data || !result) {
        printf("malloc error");
        return 0;
    }

    print_cpu_name();

    printf("runs: %d, buffer size: %d\n", TEST_RUNS, BUFFER_SIZE);
    printf("%-20s :      min      avg     max\n", " ");

    TIME_FUNC("hardware",          f32_to_f16_buffer_hw(data, result, BUFFER_SIZE));
    TIME_FUNC("table no rounding", f32_to_f16_buffer_table(data, result, BUFFER_SIZE));
    TIME_FUNC("table rounding",    f32_to_f16_buffer_table_round(data, result, BUFFER_SIZE));
    TIME_FUNC("no table",          test_float2half_full_perf(data, result, BUFFER_SIZE));
    TIME_FUNC("imath half",        test_imath_float_to_half_perf(data, result, BUFFER_SIZE));

    free(data);
    free(result);
    fflush(stdout);
#if 1
    printf("\nchecking accuracy\n");
    test_hardware_accuracy();
#endif
}