#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "common.h"

#include "platform_info.h"

#include <float.h>
#include <math.h>
#include <time.h>
#include <inttypes.h>

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

#if defined(ARCH_X86)
#include "maratyszcza_sse2/maratyszcza_sse2.h"
#include "x86_cpu_info.h"
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


#define TEST_COUNT ARRAY_SIZE(f16_tests)

#define PRINT_ERROR_RESULT(name, count, total) \
    printf("%-20s : %g%% \n", name,  100.0 - (100.0 * count/(double)total)); \
    fprintf(f, "%s,%f,%f\n", name, (double)count, (double)total)

void test_hardware_accuracy(FILE *f, int has_hardware_f16)
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

    if (!has_hardware_f16) {
        printf("** cpu has no f16c instruction, verifying against f32_to_f16_no_table instead **\n\n");
    }

    // test every possible float32 value
    for (uint64_t i = 0; i <= UINT32_MAX; i++) {
        value.u = (uint32_t)i;

        uint16_t r0;
        if (has_hardware_f16)
            r0 = f32_to_f16_hw(value.f);
        else
            r0 = f32_to_f16_no_table(value.f);

        for (size_t j = 1; j < TEST_COUNT; j++) {
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

            } else if ((value.u & 0x7FFFFFFF) > 0x477fefff) {
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

    printf("\rnormal and denormal value matches hardware, out of %u:\n", half_total);
    fprintf(f, "\nerror_test,normal and denormal value matches hardware\nname,error,total\n");

    for (size_t i = 1; i < TEST_COUNT; i++) {
        PRINT_ERROR_RESULT(f16_tests[i].name, half_error[i], half_total);
    }

    printf("\nnan value exactly matches hardware, out of %u:\n", nan_total);
    fprintf(f, "\nerror_test,nan value exactly matches hardware\nname,error,total\n");

    for (size_t i = 1; i < TEST_COUNT; i++) {
        PRINT_ERROR_RESULT(f16_tests[i].name, nan_exact_error[i], nan_total);
    }

    printf("\nnan is a nan value but might not match hardware, out of %u:\n", nan_total);
    fprintf(f, "\nerror_test,nan is a nan value but might not match hardware\nname,error,total\n");

    for (size_t i = 1; i < TEST_COUNT; i++) {
        PRINT_ERROR_RESULT(f16_tests[i].name, nan_error[i], nan_total);
    }

    printf("\n+/-inf value matches hardware, out of %u:\n", inf_total);
    fprintf(f, "\nerror_test,+/-inf value matches hardware\nname,error,total\n");

    for (size_t i = 1; i < TEST_COUNT; i++) {
        PRINT_ERROR_RESULT(f16_tests[i].name, inf_error[i], inf_total);
    }

    printf("\ntotal exact hardware match:\n");
    fprintf(f, "\nerror_test,total exact hardware match\nname,error,total\n");

    for (size_t i = 1; i < TEST_COUNT; i++) {
        PRINT_ERROR_RESULT(f16_tests[i].name, full_error[i], UINT32_MAX);
    }
}

#define TIME_FUNC(name, func, buffer_size, runs)                            \
    min_value = INFINITY;                                                   \
    max_value = -INFINITY;                                                  \
    average = 0.0;                                                          \
    ptr = data;                                                             \
    for (size_t j = 0; j < runs; j++) {                                        \
        start = get_timer();                                                \
        func(ptr, result, buffer_size);                                     \
        elapse = (double)((get_timer() - start)) / (double)freq;            \
        min_value = MIN(min_value, elapse);                                 \
        max_value = MAX(max_value, elapse);                                 \
        average += elapse * 1.0 / (double)runs;                             \
        ptr += buffer_size;                                                 \
    }                                                                       \
                                                                            \
    printf("%-20s : %f %f %f secs\n", name, min_value, average, max_value); \
    fprintf(f, "%s,%f,%f,%f\n", name, min_value, average, max_value)

int main(int argc, char *argv[])
{
    uint64_t freq = get_timer_frequency();
    uint64_t start;
    double elapse;
    double average;
    double min_value;
    double max_value;
    uint32_t *ptr;
    int has_hardware_f16 = 1;
    int first = 0;

    FILE *f = NULL;
    char *csv_path = NULL;

    if (argc > 1)
        csv_path = argv[1];
    else
        csv_path = "float2half_result.csv";

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

    has_hardware_f16 = info.flags & X86_CPU_FLAG_F16C;
    if (!has_hardware_f16) {
        printf("** CPU does not support f16c instruction, skipping some tests **\n");
        first = 1;
    }
#else
    printf("CPU: %s %s\n", CPU_ARCH, get_cpu_model_name());
    if (f)
        fprintf(f, "%s,%s\n", CPU_ARCH, get_cpu_model_name());
#endif

    printf("%s %s\n", get_platform_name(), COMPILER_NAME);
    fprintf(f, "%s,%s\n", get_platform_name(), COMPILER_NAME);
    printf("csv file: %s\n", csv_path);

    init_tables();
    init_table_round();


#if 1
    // init_test_data
    uint32_t *data = (uint32_t*) malloc(sizeof(uint32_t) * BUFFER_SIZE * TEST_RUNS);
    uint16_t *result = (uint16_t*) malloc(sizeof(uint16_t) * BUFFER_SIZE * TEST_RUNS);

    if (!data || !result) {
        printf("malloc error\n");
        return -1;
    }

    srand(time(NULL));
    printf("\r\nruns: %d, buffer size: %d, random f32 <= HALF_MAX\n\n", TEST_RUNS, BUFFER_SIZE);
    randomize_buffer_u32(data, BUFFER_SIZE * TEST_RUNS, 1);

    printf("%-20s :      min      avg     max\n", "name");
    fprintf(f, "\nperf_test,runs: %d buffer size: %d %s,\n%s,%s,%s,%s\n", TEST_RUNS, BUFFER_SIZE,"random f32 <= HALF_MAX", "name", "min", "avg", "max");
    for (size_t i = first; i < TEST_COUNT; i++) {
        TIME_FUNC(f16_tests[i].name, f16_tests[i].f32_to_f16_buffer, BUFFER_SIZE, TEST_RUNS);
    }

    fflush(stdout);

    srand(time(NULL));
    printf("\r\nruns: %d, buffer size: %d, random f32 full +inf+nan\n\n", TEST_RUNS, BUFFER_SIZE);
    randomize_buffer_u32(data, BUFFER_SIZE * TEST_RUNS, 0);


    printf("%-20s :      min      avg     max\n", "name");
    fprintf(f, "\nperf_test,runs: %d buffer size: %d %s,\n%s,%s,%s,%s\n", TEST_RUNS, BUFFER_SIZE,"random f32 full +inf+nan", "name", "min", "avg", "max");
    for (size_t i = first; i < TEST_COUNT; i++) {
        TIME_FUNC(f16_tests[i].name, f16_tests[i].f32_to_f16_buffer, BUFFER_SIZE, TEST_RUNS);
    }

    fflush(stdout);
    free(data);
    free(result);
#endif


#if 1

    printf("\nchecking hardware accuracy\n\n");
    start = get_timer();
    test_hardware_accuracy(f, has_hardware_f16);
    elapse = (double)((get_timer() - start)) / (double)freq;
    printf("\nhardware check in %f secs\n",  elapse);


#endif
    fprintf(f, "\n");
    fclose(f);

}