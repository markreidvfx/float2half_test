#include "maratyszcza_sse2.h"
#include <stdint.h>
#include <immintrin.h>

// https://www.corsix.org/content/converting-fp32-to-fp16
// https://github.com/Maratyszcza/FP16/blob/0a92994d729ff76a58f692d3028ca1b64b145d91/include/fp16/fp16.h#L223-L247

#define DEBUG_VERIFY 0

#if DEBUG_VERIFY
#include <assert.h>
#include <stdio.h>
// verify
static inline uint16_t to_f16_hw(float v)
{
    uint16_t result[8] = {0};
    __m128 ps =_mm_set1_ps(v);
    __m128i ph = _mm_cvtps_ph(ps, _MM_FROUND_TO_NEAREST_INT);

    _mm_storeu_si128((__m128i*)result, ph);

    return result[0];
}
#endif

static inline __m128i blendv_sse2(__m128i a, __m128i b, __m128i mask)
{
    return _mm_xor_si128(_mm_and_si128(_mm_xor_si128(a, b), mask), a);
}

// drop in replacement for hardware instruction
static inline __m128i cvtps_ph_sse2(__m128 a, int imm8 /*unused always _MM_FROUND_TO_NEAREST_INT*/)
{
    __m128i x = _mm_castps_si128(a);

    __m128i x_sgn = _mm_and_si128(x, _mm_set1_epi32(0x80000000u));
    __m128i x_exp = _mm_and_si128(x, _mm_set1_epi32(0x7f800000u));

    // sse2 doesn't have _mm_max_epu32, but _mm_max_ps works
    __m128i exp_max = _mm_set1_epi32(0x38800000u);
    x_exp = _mm_castps_si128(_mm_max_ps(_mm_castsi128_ps(x_exp), _mm_castsi128_ps(exp_max))); // max(e, -14)
    x_exp = _mm_add_epi32(x_exp, _mm_set1_epi32(15u << 23)); // e += 15
    x = _mm_and_si128(x, _mm_set1_epi32(0x7fffffffu)); // Discard sign

    __m128 f = _mm_castsi128_ps(x);
    __m128 magicf = _mm_castsi128_ps(x_exp);

    // If 15 < e then inf, otherwise e += 2
    f = _mm_mul_ps(_mm_mul_ps(f, _mm_set1_ps(0x1.0p+112f)), _mm_set1_ps(0x1.0p-110f));
    f = _mm_add_ps(f, magicf);

    __m128i u = _mm_castps_si128(f);

    __m128i h_exp = _mm_and_si128(_mm_srli_epi32(u, 13), _mm_set1_epi32(0x7c00u));
    __m128i h_sig = _mm_and_si128(u, _mm_set1_epi32(0x0fffu));

    // blend in nan values only if present
    __m128i nan_mask = _mm_cmpgt_epi32(x, _mm_set1_epi32(0x7f800000u));
    if (_mm_movemask_epi8(nan_mask)) {
        __m128i nan = _mm_and_si128(_mm_srli_epi32(x, 13), _mm_set1_epi32(0x03FFu));
        nan = _mm_or_si128(_mm_set1_epi32(0x0200u), nan);
        h_sig = blendv_sse2(h_sig, nan, nan_mask);
    }

    __m128i ph = _mm_add_epi32(_mm_srli_epi32(x_sgn, 16),_mm_add_epi32(h_exp, h_sig));

    // pack u16 values into lower 8 bytes
    ph = _mm_shufflehi_epi16(ph, (1 << 6 | 1 << 4 | 2 << 2 | 0 << 0));
    ph = _mm_shufflelo_epi16(ph, (1 << 6 | 1 << 4 | 2 << 2 | 0 << 0));
    return _mm_shuffle_epi32(ph, (3 << 6 | 3 << 4 | 2 << 2 | 0 << 0));
}


static inline uint16_t to_f16(float v)
{
    uint16_t result[8] = {0};
    __m128 ps =_mm_set1_ps(v);
    __m128i ph = cvtps_ph_sse2(ps, _MM_FROUND_TO_NEAREST_INT);

    _mm_storeu_si128((__m128i*)result, ph);

    return result[0];
}


uint16_t f32_to_f16_maratyszcza_sse2(float f)
{
    return to_f16(f);
}

void f32_to_f16_buffer_maratyszcza_sse2(uint32_t *data, uint16_t *result, int data_size)
{
    int size = data_size / 4 * 4;
    int remainder = data_size - size;

    uint32_t *base_src = data;
    uint16_t *base_dst = result;

    for (int i = 0; i < size; i+=4) {
        __m128 ps = _mm_loadu_ps((float*)data);
        __m128i ph = cvtps_ph_sse2(ps, _MM_FROUND_TO_NEAREST_INT);
        _mm_storeu_si64((__m128i*)result, ph);

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
         _mm_storeu_si64((__m128i*)&out_buf[0], ph);

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
