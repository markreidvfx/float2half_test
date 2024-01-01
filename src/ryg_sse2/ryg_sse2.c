#include "ryg_sse2.h"
#include <immintrin.h>


static inline __m128 sse2_cvtph_ps(__m128i a)
{
    __m128 magic      = _mm_castsi128_ps(_mm_set1_epi32((254 - 15) << 23));
    __m128 was_infnan = _mm_castsi128_ps(_mm_set1_epi32((127 + 16) << 23));
    __m128i sign, nan_mask, inf_mask, ou, ou_nan, ou_inf;
    __m128 o;
    // the values to unpack are in the lower 64 bits
    // | 0 1 | 2 3 | 4 5 | 6 7 | 8 9 | 10 11 | 12 13 | 14 15
    // | 0 1 | 0 1 | 2 3 | 2 3 | 4 5 |  4  5 | 6   7 | 6   7
    a = _mm_unpacklo_epi16(a, a);

    // extract sign
    sign = _mm_slli_epi32(_mm_and_si128(a, _mm_set1_epi32(0x8000)), 16);

    // extract exponent/mantissa bits
    o = _mm_castsi128_ps(_mm_slli_epi32(_mm_and_si128(a, _mm_set1_epi32(0x7fff)), 13));

    // magic multiply
    o = _mm_mul_ps(o, magic);

    ou = _mm_castps_si128(o);
    nan_mask = _mm_castps_si128(_mm_cmpgt_ps(o, was_infnan));
    inf_mask = _mm_cmpeq_epi32(ou, _mm_castps_si128(was_infnan));

    ou_nan = _mm_and_si128(nan_mask, _mm_set1_epi32(0x01FF << 22));
    ou_inf = _mm_and_si128(inf_mask, _mm_set1_epi32(0x00FF << 23));

    return  _mm_castsi128_ps(_mm_or_si128(ou, _mm_or_si128(sign, _mm_or_si128(ou_nan, ou_inf))));
}

float f16_to_f32_ryg_sse2(uint16_t v)
{
    float result[4];
    __m128 ps = sse2_cvtph_ps(_mm_set1_epi16(v));
    _mm_storeu_ps(result, ps);
    return result[0];
}

void f16_to_f32_buffer_ryg_sse2(uint16_t *data, uint32_t *result, int data_size)
{
    int size = data_size / 4 * 4;
    int remainder = data_size - size;

    for (int i = 0; i < size; i+=4) {
        __m128i ph = _mm_loadl_epi64((const __m128i*)data);
        __m128 p = sse2_cvtph_ps(ph);
        _mm_storeu_si128((__m128i*)result, _mm_castps_si128(p));

        data += 4;
        result += 4;
    }

    if (remainder) {
        uint16_t in_buf[4] = {0};
        uint32_t out_buf[4] = {0};
        for (int i = 0; i < remainder; i++) {
            in_buf[i] = data[i];
        }

        __m128i ph = _mm_loadl_epi64((const __m128i*)&in_buf[0]);
        __m128 p = sse2_cvtph_ps(ph);
        _mm_storeu_si128((__m128i*)&out_buf[0], _mm_castps_si128(p));

        for (int i = 0; i < remainder; i++) {
            result[i] = out_buf[i];
        }
    }
}

static inline __m128i blendv_sse2(__m128i a, __m128i b, __m128i mask)
{
    return _mm_xor_si128(_mm_and_si128(_mm_xor_si128(a, b), mask), a);
}

// drop in replacement for hardware instruction
static inline __m128i cvtps_ph_sse2(__m128 a, int imm8 /*unused always _MM_FROUND_TO_NEAREST_INT*/)
{
    __m128i x_sgn = _mm_and_si128(_mm_castps_si128(a), _mm_set1_epi32(0x80000000u));
    __m128i x =  _mm_andnot_si128(x_sgn, _mm_castps_si128(a));
    x_sgn = _mm_srli_epi32(x_sgn, 16);

    __m128i infnan_mask = _mm_cmpgt_epi32(x, _mm_set1_epi32(0x47800000u - 1));
    __m128i nan_mask = _mm_cmpgt_epi32(x, _mm_set1_epi32(0x7f800000u));

    __m128i nan = _mm_and_si128(_mm_srli_epi32(x, 13), _mm_set1_epi32(0x03FFu));
    nan = _mm_or_si128(nan, _mm_set1_epi32(0x7e00u));
    nan = _mm_and_si128(nan, nan_mask);

    __m128i inf = _mm_set1_epi32(0x7c00);
    __m128i infnan = _mm_or_si128(inf, nan);

    __m128i subnormal_mask =  _mm_cmplt_epi32(x, _mm_set1_epi32(0x38800000u));
    __m128i denorm_magic = _mm_set1_epi32(((127u - 14u) + (23u - 10u)) << 23);

    __m128i denorm = _mm_castps_si128(_mm_add_ps(_mm_castsi128_ps(denorm_magic), _mm_castsi128_ps(x)));
    denorm = _mm_sub_epi32(denorm, denorm_magic);

    __m128i mant_odd =_mm_and_si128(_mm_srli_epi32(x, 13), _mm_set1_epi32(1));
    x = _mm_add_epi32(x, _mm_set1_epi32(((15u - 127u) << 23) + 0xfffu));
    x = _mm_add_epi32(x, mant_odd);
    x = _mm_srli_epi32(x , 13);

    x = blendv_sse2(x, denorm, subnormal_mask);
    x = blendv_sse2(x, infnan, infnan_mask);

    x = _mm_or_si128(x_sgn, x);

        // pack u16 values into lower 8 bytes
    x = _mm_shufflehi_epi16(x, (1 << 6 | 1 << 4 | 2 << 2 | 0 << 0));
    x = _mm_shufflelo_epi16(x, (1 << 6 | 1 << 4 | 2 << 2 | 0 << 0));
    return _mm_shuffle_epi32(x, (1 << 6 | 1 << 4 | 2 << 2 | 0 << 0));
}

static inline uint16_t to_f16(float v)
{
    uint16_t result[8] = {0};
    __m128 ps =_mm_set1_ps(v);
    __m128i ph = cvtps_ph_sse2(ps, _MM_FROUND_TO_NEAREST_INT);

    _mm_storeu_si128((__m128i*)result, ph);

    return result[0];
}

uint16_t f32_to_f16_ryg_sse2(float f)
{
    return to_f16(f);
}


void f32_to_f16_buffer_ryg_sse2(uint32_t *data, uint16_t *result, int data_size)
{
    int size = data_size / 4 * 4;
    int remainder = data_size - size;

#if DEBUG_VERIFY
    uint32_t *base_src = data;
    uint16_t *base_dst = result;
#endif

    for (int i = 0; i < size; i+=4) {
        __m128 ps = _mm_loadu_ps((float*)data);
        __m128i ph = cvtps_ph_sse2(ps, _MM_FROUND_TO_NEAREST_INT);
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
        __m128i ph = cvtps_ph_sse2(ps, _MM_FROUND_TO_NEAREST_INT);
         _mm_storel_epi64((__m128i*)&out_buf[0], ph);

        for (int i = 0; i < remainder; i++) {
            result[i] = out_buf[i];
        }
    }

# if DEBUG_VERIFY
    // verify against hardware
    for (int i = 0; i < data_size; i++) {
        union { uint32_t u; float f; } f;
        f.u = base_src[i];

        uint16_t r = to_f16_hw(f.f);
        uint16_t t = to_f16(f.f);

        if (r != t){
            printf("to_f16:  %d 0x%08x 0x%04x != 0x%04x\n", i, f.u, r, t);
            assert(r == t);
        }

        t = base_dst[i];
        if (r != t) {
            printf("cvt:  %d 0x%08x 0x%04x != 0x%04x\n", i, f.u, r, t);
            assert(r == t);
        }
    }
#endif

}
