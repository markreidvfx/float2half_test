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
#include "imath/imath.h"

#if defined(ARCH_X86)
#include "x86_cpu_info.h"
#include "ryg_sse2/ryg_sse2.h"
#include "emmintrin.h"
#endif

#include "half2float_table.h"

static void f16_to_f32_buffer_static_table(uint16_t *data, uint32_t *result, int data_size)
{
    for (int i =0; i < data_size; i++) {
        result[i] = f16_to_f32_static_table[data[i]];
    }
}

static float f16_to_f32_static_table_func(uint16_t h)
{
    int_float v;
    v.u = f16_to_f32_static_table[h];
    return v.f;
}

typedef struct F16Test {
    const char *name;
    float (*f16_to_f32)(uint16_t h);
    void (*f16_to_f32_buffer)(uint16_t *data, uint32_t *result, int data_size);
} F16Test;

const static F16Test f16_tests[] =
{
    {"hardware",            f16_to_f32_hw,                 f16_to_f32_buffer_hw },
    {"static_table",        f16_to_f32_static_table_func,  f16_to_f32_buffer_static_table },
    {"table",               f16_to_f32_table,              f16_to_f32_buffer_table },
    {"imath",               f16_to_f32_imath,              f16_to_f32_buffer_imath },
    {"ryg",                 f16_to_f32_ryg,                f16_to_f32_buffer_ryg },
#if defined(ARCH_X86)
    {"ryg_sse2",            f16_to_f32_ryg_sse2,           f16_to_f32_buffer_ryg_sse2 },
#endif
};

#define TEST_COUNT ARRAY_SIZE(f16_tests)

#define USE_VALIDATE 1

#if USE_VALIDATE
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
#else
int validate(uint16_t *src, uint32_t *result, size_t size) { return 0;}
#endif

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
    printf("%-20s : %f %f %f secs\n", name, min_value, average, max_value); \
    fprintf(f, "%s,%f,%f,%f\n", name, min_value, average, max_value)


int main(int argc, char *argv[])
{
    int_float a;
    int_float b;
    uint64_t freq, start;
    double elapse;
    double average;
    double min_value;
    double max_value;
    uint16_t *ptr;
    int first = 0;

    FILE *f = NULL;
    char *csv_path = NULL;

    if (argc > 1)
        csv_path = argv[1];
    else
        csv_path = "half2float_result.csv";

    f = fopen(csv_path,"wb");
    if (!f) {
        printf("unable to open csv file: %s'\n", csv_path);
        return -1;
    }

#if defined(ARCH_X86)
    // print cpu name and check for f16c instruction
    CPUInfo info = {0};
    get_cpu_info(&info);
    printf("CPU: %s %s %s\n", CPU_ARCH, info.name, info.extensions);
    fprintf(f, "%s,%s,%s\n", CPU_ARCH, info.name, info.extensions);

    if (!(info.flags & X86_CPU_FLAG_F16C)) {
        first = 1;
        printf("** CPU does not support f16c instruction**\n");
    }
#else
    printf("CPU: %s %s\n", CPU_ARCH, get_cpu_model_name());
    fprintf(f, "%s,%s\n", CPU_ARCH, get_cpu_model_name());
#endif

    printf("%s %s\n", get_platform_name(), COMPILER_NAME);
    fprintf(f, "%s,%s\n", get_platform_name(), COMPILER_NAME);
    printf("csv file: %s\n", csv_path);

    init_tables();

    printf("\n%-20s:\n", "name");
    for (size_t j = first; j < TEST_COUNT; j++) {
        freq = get_timer_frequency();
        start = get_timer();
        if (!f16_tests[j].f16_to_f32)
            continue;

        for (int i = 0; i <= UINT16_MAX; i++) {
            a.u = f16_to_f32_static_table[i];
            b.f = f16_tests[j].f16_to_f32(i);

            if (a.u != b.u) {
                printf("%s : %05d 0x%08X != 0x%08X %f %f\n", f16_tests[j].name, i, a.u, b.u, a.f, b.f);
                // printf("0x%08X\n", a.i - b.i);
            }

        }
        elapse = (double)((get_timer() - start)) / (double)freq;
        printf("%-20s: checked accuracy in %f secs\n", f16_tests[j].name, elapse);
    }

    uint16_t *data = (uint16_t*) malloc(sizeof(uint16_t) * BUFFER_SIZE * TEST_RUNS);
    uint32_t *result = (uint32_t*) malloc(sizeof(uint32_t) * BUFFER_SIZE * TEST_RUNS);

    srand(time(NULL));
    printf("\r\nruns: %d, buffer size: %d, random f16 <= HALF_MAX\n\n", TEST_RUNS, BUFFER_SIZE);
    randomize_buffer_u16(data, BUFFER_SIZE * TEST_RUNS, 1);

    printf("%-20s :      min      avg     max\n", "name");
    fprintf(f, "\nperf_test,runs: %d buffer size: %d %s\n%s,%s,%s,%s\n", TEST_RUNS, BUFFER_SIZE,"random f16 <= HALF_MAX", "name", "min", "avg", "max");
    for (size_t i = first; i < TEST_COUNT; i++) {
        TIME_FUNC(f16_tests[i].name, f16_tests[i].f16_to_f32_buffer, BUFFER_SIZE, TEST_RUNS);
    }
    fflush(stdout);

    srand(time(NULL));
    printf("\r\nruns: %d, buffer size: %d, random f16 full +inf+nan\n\n", TEST_RUNS, BUFFER_SIZE);
    randomize_buffer_u16(data, BUFFER_SIZE * TEST_RUNS, 0);

    printf("%-20s :      min      avg     max\n", "name");
    fprintf(f, "\nperf_test,runs: %d buffer size: %d %s\n%s,%s,%s,%s\n", TEST_RUNS, BUFFER_SIZE,"random f16 full +inf+nan", "name", "min", "avg", "max");
    for (size_t i = first; i < TEST_COUNT; i++) {
        TIME_FUNC(f16_tests[i].name, f16_tests[i].f16_to_f32_buffer, BUFFER_SIZE, TEST_RUNS);
    }
    fflush(stdout);
    fprintf(f, "\n");
    fclose(f);
    return 0;
}