#include "ryg.h"

// https://www.corsix.org/content/converting-fp32-to-fp16
// https://gist.github.com/rygorous/2156668

static inline uint16_t float_to_half_fast3_rtne(uint32_t x) {
  uint32_t x_sgn = x & 0x80000000u;
  x ^= x_sgn;

  uint16_t o;
  if (x >= 0x47800000u) { // result is Inf or NaN
    o = (x > 0x7f800000u) ? 0x7e00  // NaN->qNaN
                          : 0x7c00; // and Inf->Inf
  } else { // (De)normalized number or zero
    if (x < 0x38800000u) { // resulting FP16 is subnormal or zero
      // use a magic value to align our 10 mantissa bits at
      // the bottom of the float. as long as FP addition
      // is round-to-nearest-even this just works.
      union { uint32_t u; float f; } f, denorm_magic;
      f.u = x;
      denorm_magic.u = ((127u - 14u) + (23u - 10u)) << 23;

      f.f += denorm_magic.f;

      // and one integer subtract of the bias later, we have our
      // final float!
      o = f.u - denorm_magic.u;
    } else {
      uint32_t mant_odd = (x >> 13) & 1; // resulting mantissa is odd

      // update exponent, rounding bias part 1
      x += ((15u - 127u) << 23) + 0xfff;
      // rounding bias part 2
      x += mant_odd;
      // take the bits!
      o = x >> 13;
    }
  }
  return (x_sgn >> 16) | o;
}


typedef union {
        uint32_t u;
        float    f;
} int_float;

uint16_t f32_to_f16_ryg(float f)
{
    int_float value;
    value.f = f;
    return float_to_half_fast3_rtne(value.u);
}

void f32_to_f16_buffer_ryg(uint32_t *data, uint16_t *result, int data_size)
{
    int_float value;
    for (int i =0; i < data_size; i++) {
        value.u = data[i];
        result[i] = float_to_half_fast3_rtne(value.u);
    }
}


// https://fgiesen.wordpress.com/2012/03/28/half-to-float-done-quic/
static inline float half_to_float_ryg(uint16_t h)
{
    static const int_float magic = { (254u - 15u) << 23 };
    static const int_float was_infnan = { (127u + 16u) << 23 };
    int_float o;

    o.u = (h & 0x7fff) << 13;     // exponent/mantissa bits
    o.f *= magic.f;               // exponent adjust
    if (o.f >= was_infnan.f) {    // make sure Inf/NaN survive
        if (o.u == was_infnan.u)
            // was ininity
            o.u |= 0x00FF << 23;
        else {
            // was nan, match hardware nan
            o.u |= 0x01FF << 22;
        }
    }
    o.u |= (h & 0x8000) << 16;    // sign bit
    return o.f;
}

float f16_to_f32_ryg(uint16_t h)
{
  return half_to_float_ryg(h);
}

void f16_to_f32_buffer_ryg(uint16_t *data, uint32_t *result, int data_size)
{
    int_float value;
    for (int i =0; i < data_size; i++) {
        value.f = half_to_float_ryg(data[i]);
        result[i] = value.u;
    }
}
