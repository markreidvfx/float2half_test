#include "maratyszcza_nanfix.h"

// https://www.corsix.org/content/converting-fp32-to-fp16
// https://github.com/Maratyszcza/FP16/blob/0a92994d729ff76a58f692d3028ca1b64b145d91/include/fp16/fp16.h#L223-L247

static inline uint16_t fp16_ieee_from_fp32_value(uint32_t x) {
  uint32_t x_sgn = x & 0x80000000u;
  uint32_t x_exp = x & 0x7f800000u;
  x_exp = (x_exp < 0x38800000u) ? 0x38800000u : x_exp; // max(e, -14)
  x_exp += 15u << 23; // e += 15
  x &= 0x7fffffffu; // Discard sign

  union { uint32_t u; float f; } f, magic;
  f.u = x;
  magic.u = x_exp;

  // If 15 < e then inf, otherwise e += 2
  f.f = (f.f * 0x1.0p+112f) * 0x1.0p-110f;

  // If we were in the x_exp >= 0x38800000u case:
  // Replace f's exponent with that of x_exp, and discard 13 bits of
  // f's significand, leaving the remaining bits in the low bits of
  // f, relying on FP addition being round-to-nearest-even. If f's
  // significand was previously `a.bcdefghijk`, then it'll become
  // `1.000000000000abcdefghijk`, from which `bcdefghijk` will become
  // the FP16 mantissa, and `a` will add onto the exponent. Note that
  // rounding might add one to all this.
  // If we were in the x_exp < 0x38800000u case:
  // Replace f's exponent with the minimum FP16 exponent, discarding
  // however many bits are required to make that happen, leaving
  // whatever is left in the low bits.
  f.f += magic.f;

  uint32_t h_exp = (f.u >> 13) & 0x7c00u; // low 5 bits of exponent
  uint32_t h_sig = f.u & 0x0fffu; // low 12 bits (10 are mantissa)
  h_sig = (x > 0x7f800000u) ? 0x0200u | ((x >> 13) & 0x03FF) : h_sig; // any NaN -> qNaN
  return (x_sgn >> 16) + h_exp + h_sig;
}

typedef union {
        uint32_t i;
        float    f;
} int_float;

uint16_t f32_to_f16_maratyszcza_nanfix(float f)
{
    int_float value;
    value.f = f;
    return fp16_ieee_from_fp32_value(value.i);
}

void f32_to_f16_buffer_maratyszcza_nanfix(uint32_t *data, uint16_t *result, int data_size)
{
    int_float value;
    for (int i =0; i < data_size; i++) {
        value.i = data[i];
        result[i] = fp16_ieee_from_fp32_value(value.i);
    }
}
