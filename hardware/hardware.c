
#include "hardware.h"

typedef union {
        uint32_t i;
        float    f;
} int_float;

#ifdef USE_ARM
static inline uint16_t to_f16(float v)
{
    union {
        _Float16 f;
        uint16_t i;
    } u;
    u.f = v;
    return u.i;
}

static inline uint32_t to_f32(uint16_t v)
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

static inline uint16_t to_f16(float v)
{
    uint16_t result[8] = {0};
    __m128 ps =_mm_set1_ps(v);
    __m128i ph = _mm_cvtps_ph(ps, _MM_FROUND_TO_NEAREST_INT);

    _mm_storeu_si128((__m128i*)result, ph);

    return result[0];
}

static inline uint32_t to_f32(uint16_t v)
{
    uint32_t result[4] = {0};
    uint16_t src[8] = {v, v, v, v, v, v, v, v};

    __m128i ph = _mm_loadu_si128((__m128i*)&src);
    __m128 ps = _mm_cvtph_ps(ph);

    _mm_storeu_si128((__m128i*)result, _mm_castps_si128(ps));

    return result[0];
}
#endif


uint16_t f32_to_f16_hw(float f)
{
    return to_f16(f);
}

float f16_to_f32_hw(uint16_t f)
{
    int_float value;
    value.i = to_f32(f);
    return value.f;
}

void f32_to_f16_buffer_hw(uint32_t *data, uint16_t *result, int data_size)
{
    int_float value;

    for (int i =0; i < data_size; i++) {
        value.i = data[i];
        result[i] = to_f16(value.f);
    }
}