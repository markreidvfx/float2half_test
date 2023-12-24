
#include "hardware.h"

typedef union {
        uint32_t i;
        float    f;
} int_float;

#if defined(__aarch64__) || defined(__arm__)
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

#if defined(__aarch64__) || defined(__arm__)
void f32_to_f16_buffer_hw(uint32_t *data, uint16_t *result, int data_size)
{
    int_float value;

    for (int i =0; i < data_size; i++) {
        value.i = data[i];
        result[i] = to_f16(value.f);
    }
}




#else
void f32_to_f16_buffer_hw(uint32_t *data, uint16_t *result, int data_size)
{
    int size = data_size / 4 * 4;
    int remainder = data_size - size;

    uint32_t *base_src = data;
    uint16_t *base_dst = result;

    for (int i = 0; i < size; i+=4) {
        __m128 ps = _mm_loadu_ps((float*)data);
        __m128i ph = _mm_cvtps_ph(ps, _MM_FROUND_TO_NEAREST_INT);
        _mm_storel_epi64((__m128i*)result, ph);

        data += 4;
        result += 4;
    }

    if (remainder) {
        uint32_t in_buf[4] = {0};
        uint16_t out_buf[4] = {0};
        for (int i = 0; i < remainder; i++) {
            in_buf[i] = data[i];
        }

        __m128 ps = _mm_loadu_ps((float*)&in_buf[0]);
        __m128i ph = _mm_cvtps_ph(ps, _MM_FROUND_TO_NEAREST_INT);
         _mm_storel_epi64((__m128i*)out_buf, ph);

        for (int i = 0; i < remainder; i++) {
            result[i] = out_buf[i];
        }
    }

# if 0
    // verify against single
    for (int i = 0; i < data_size; i++) {
        union { uint32_t u; float f; } f;
        f.u = base_src[i];

        uint16_t r = to_f16(f.f);
        uint16_t t = base_dst[i];

        if (r != t) {
            printf("cvt:  %d 0x%08x 0x%04x != 0x%04x\n", i, f.u, r, t);
            assert(r == t);
        }
    }
#endif
}

#endif

void f16_to_f32_buffer_hw(uint16_t *data, uint32_t *result, int data_size)
{
    int_float value;

    for (int i =0; i < data_size; i++) {
        value.f = f16_to_f32_hw(data[i]);
        result[i] = value.i;
    }
}