#include "ryg_sse2.h"
#include <immintrin.h>

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