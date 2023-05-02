#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <float.h>
#include <time.h>
#include <math.h>
#include <inttypes.h>
#include <assert.h>

#include "hardware/hardware.h"
#include "table/table.h"
#include "table_round/table_round.h"
#include "no_table/no_table.h"
#include "cpython/cpython.h"
#include "numpy/numpy.h"
#include "imath/imath.h"
#include "tursa/tursa.h"
#include "ryg/ryg.h"
#include "maratyszcza/maratyszcza.h"
#include "maratyszcza_nanfix/maratyszcza_nanfix.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#if defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(_M_IX86)
#define ARCH_X86
#include "maratyszcza_sse2/maratyszcza_sse2.h"
#include "x86_cpu_info.h"
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

typedef struct F16Test {
    const char *name;
    uint16_t (*f32_to_f16)(float v);
    void (*f32_to_f16_buffer)(uint32_t *data, uint16_t *result, int data_size);
} F16Test;

const static F16Test f16_tests[] =
{
    {"hardware",            f32_to_f16_hw,                 f32_to_f16_buffer_hw },
    {"table no rounding",   f32_to_f16_table,              f32_to_f16_buffer_table },
    {"table rounding",      f32_to_f16_table_round,        f32_to_f16_buffer_table_round },
    {"no table",            f32_to_f16_no_table,           f32_to_f16_buffer_no_table },
    {"imath half",          f32_to_f16_imath,              f32_to_f16_buffer_imath },
    {"cpython",             f32_to_f16_cpython,            f32_to_f16_buffer_cpython },
    {"numpy",               f32_to_f16_numpy,              f32_to_f16_buffer_numpy },
    {"tursa",               f32_to_f16_tursa,              f32_to_f16_buffer_tursa },
    {"ryg",                 f32_to_f16_ryg,                f32_to_f16_buffer_ryg },
    {"maratyszcza",         f32_to_f16_maratyszcza,        f32_to_f16_buffer_maratyszcza },
    {"maratyszcza nan fix", f32_to_f16_maratyszcza_nanfix, f32_to_f16_buffer_maratyszcza_nanfix },
#if defined(ARCH_X86)
    {"maratyszcza sse2",    f32_to_f16_maratyszcza_sse2,   f32_to_f16_buffer_maratyszcza_sse2 }
#endif
};

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define TEST_COUNT ARRAY_SIZE(f16_tests)

#define PRINT_ERROR_RESULT(name, count, total) \
    printf("%-20s : %g%% \n", name,  100.0 - (100.0 * count/(double)total))

void test_hardware_accuracy()
{
    int_float value;

    uint32_t full_error[TEST_COUNT] = {0};
    uint32_t half_error[TEST_COUNT] = {0};
    uint32_t nan_exact_error[TEST_COUNT] = {0};
    uint32_t nan_error[TEST_COUNT] = {0};

    uint32_t inf_error[TEST_COUNT] = {0};

    uint32_t nan_total = 0;
    uint32_t inf_total = 0;
    uint32_t half_total = 0;

    // test every possible float32 value
    for (uint64_t i = 0; i <= UINT32_MAX; i++) {
        value.i = (uint32_t)i;

        uint16_t r0 = f32_to_f16_hw(value.f);

        for (int j = 1; j < TEST_COUNT; j++) {
            uint16_t r1 = f16_tests[j].f32_to_f16(value.f);

            // check if value exactly matches hardware
            int e = (r0 != r1);
            full_error[j] += e;

            if (isnan(value.f)) {
                // float v = f16_to_f32_hw(r0);
                // assert(isnan(v));
                if (j == 1)
                    nan_total++;

                nan_exact_error[j] += e;

                // check if the converted value is a NaN
                // even if it doesn't match hardware exactly

                // float t = f16_to_f32_hw(r1);
                // int is_a_nan = ((r1 & 0x7FFF) > 0x7C00);
                // assert(is_a_nan == isnan(t));

                nan_error[j] += !((r1 & 0x7FFF) > 0x7C00);

            } else if ((value.i & 0x7FFFFFFF) > 0x477fefff) {
                // value should be +inf/-inf

                // float v = f16_to_f32_hw(r0);
                // assert(isinf(v));

                if (j == 1)
                    inf_total++;

                inf_error[j] += e;

            } else {
                // value can be mapped to a f16 normal number or denormal

                // float v = f16_to_f32_hw(r0);
                // assert(!(isinf(v) || isnan(v)));

                if (j == 1)
                    half_total++;

                half_error[j] += e;

            }
        }

        if ((i % 0x10000000 ) == 0){
            printf("\r %4.1f%%", 100.0 * i/(double)UINT32_MAX);
            fflush(stdout);
        }
    }

    printf("\rnormal/denormal value matches hardware, out of %u:\n", half_total);
    for (int i = 1; i < TEST_COUNT; i++) {
        PRINT_ERROR_RESULT(f16_tests[i].name, half_error[i], half_total);
    }

    printf("\nnan value exactly matches hardware, out of %u:\n", nan_total);
    for (int i = 1; i < TEST_COUNT; i++) {
        PRINT_ERROR_RESULT(f16_tests[i].name, nan_exact_error[i], nan_total);
    }

    printf("\nnan is a nan value but might not match hardware, out of %u:\n", nan_total);
    for (int i = 1; i < TEST_COUNT; i++) {
        PRINT_ERROR_RESULT(f16_tests[i].name, nan_error[i], nan_total);
    }

    printf("\n+/-inf value matches hardware, out of %u:\n", inf_total);
    for (int i = 1; i < TEST_COUNT; i++) {
        PRINT_ERROR_RESULT(f16_tests[i].name, inf_error[i], inf_total);
    }

    printf("\ntotal exact hardware match:\n");
    for (int i = 1; i < TEST_COUNT; i++) {
        PRINT_ERROR_RESULT(f16_tests[i].name, full_error[i], UINT32_MAX);
    }
}

uint32_t rand_uint32(void)
{
  return  ((0x7fff & rand()) << 30) | ((0x7fff & rand()) << 15) | (0x7fff & rand());
}

int rand_uint32_real()
{
    int_float v;
    for (;;) {
        v.i = rand_uint32();

        if ((v.i &= 0x7FFFFFFF) <= 0x477fefff) {
            // uint16_t r0 = f32_to_f16_hw(v.f);
            // float f = f16_to_f32_hw(r0);
            // assert(!(isnan(f) || isinf(f)));
            return v.i;
        }
    }
}

void randomize_buffer(uint32_t *data, size_t size, int real_only)
{
    // fill up buffer with random data
    for (int i =0; i < size; i++) {
        if (real_only)
            data[i] = rand_uint32_real();
        else
            data[i] = rand_uint32();

        // printf("0x%08x\n", data[i]);
        if ((i % 20000000) == 0){
            printf("\rrandomizing buffers: %4.1f%%", 100.0 * i/(double)size);
            fflush(stdout);
        }
    }
    printf("\r");
}

#define TIME_FUNC(name, func, buffer_size, runs)                          \
    min_value = INFINITY;                                                 \
    max_value = -INFINITY;                                                \
    average = 0.0;                                                        \
    ptr = data;                                                           \
    for (int j = 0; j < runs; j++) {                                      \
        start = get_timer();                                              \
        func(ptr, result, buffer_size);                                   \
        elapse = (double)((get_timer() - start)) / (double)freq;          \
        min_value = MIN(min_value, elapse);                               \
        max_value = MAX(max_value, elapse);                               \
        average += elapse * 1.0 / (double)runs;                           \
        ptr += buffer_size;                                               \
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
    uint32_t *ptr;
    int has_hardware_f16 = 1;
    int first = 0;

#if defined(ARCH_X86)
    // print cpu name and check for f16c instruction
    CPUInfo info = {0};
    get_cpu_info(&info);
    printf("CPU: %s %s\n", info.name, info.extensions);
    has_hardware_f16 = info.flags & X86_CPU_FLAG_F16C;
    if (!has_hardware_f16) {
        printf("** CPU does not support f16c instruction, skipping some tests **\n");
        first = 1;
    }
#endif

    init_table();
    init_table_round();

#define TEST_RUNS 50
#define BUFFER_SIZE (1920*1080*4)

#if 1
    // init_test_data
    uint32_t *data = (uint32_t*) malloc(sizeof(uint32_t) * BUFFER_SIZE * TEST_RUNS);
    uint16_t *result = (uint16_t*) malloc(sizeof(uint16_t) * BUFFER_SIZE * TEST_RUNS);

    if (!data || !result) {
        printf("malloc error");
        return -1;
    }

    srand(time(NULL));
    printf("\r\nruns: %d, buffer size: %d, random f32 <= HALF_MAX\n\n", TEST_RUNS, BUFFER_SIZE);
    randomize_buffer(data, BUFFER_SIZE * TEST_RUNS, 1);

    printf("%-20s :      min      avg     max\n", "name");
    for (int i = first; i < TEST_COUNT; i++) {
        TIME_FUNC(f16_tests[i].name, f16_tests[i].f32_to_f16_buffer, BUFFER_SIZE, TEST_RUNS);
    }

    fflush(stdout);

    srand(time(NULL));
    printf("\r\nruns: %d, buffer size: %d, random f32 full +inf+nan\n\n", TEST_RUNS, BUFFER_SIZE);
    randomize_buffer(data, BUFFER_SIZE * TEST_RUNS, 0);

    printf("%-20s :      min      avg     max\n", "name");
    for (int i = first; i < TEST_COUNT; i++) {
        TIME_FUNC(f16_tests[i].name, f16_tests[i].f32_to_f16_buffer, BUFFER_SIZE, TEST_RUNS);
    }

    fflush(stdout);
    free(data);
    free(result);
#endif


#if 1
    if (has_hardware_f16) {
        printf("\nchecking accuracy against hardware\n\n");
        test_hardware_accuracy();
    } else {
        printf("\nskipping hardware accuracy test\n");
    }
#endif
}