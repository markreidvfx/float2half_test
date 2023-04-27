
#include "tursa.h"
// https://www.corsix.org/content/converting-fp32-to-fp16
// https://github.com/nomovok-opensource/wrath/blob/3a8d8ee92f845ed96e95b3736a2a9deef4ac5e4c/src/3rd_party/ieeehalfprecision/ieeehalfprecision.c#L118-L156

uint16_t tursa_floatbits_to_halfbits(uint32_t x) {
  uint32_t xs = x & 0x80000000u; // Pick off sign bit
  uint32_t xe = x & 0x7f800000u; // Pick off exponent bits
  uint32_t xm = x & 0x007fffffu; // Pick off mantissa bits
  uint16_t hs = (uint16_t)(xs >> 16); // Sign bit
  if (xe == 0) { // Denormal will underflow, return a signed zero
    return hs;
  }
  if (xe == 0x7f800000u) { // Inf or NaN
    if (xm == 0) { // If mantissa is zero ...
      return (uint16_t)(hs | 0x7c00u); // Signed Inf
    } else {
      return (uint16_t)0xfe00u; // NaN, only 1st mantissa bit set
    }
  }
  // Normalized number
  int hes = ((int)(xe >> 23)) - 127 + 15; // Exponent unbias the single, then bias the halfp
  if (hes >= 0x1f) { // Overflow
    return (uint16_t)(hs | 0x7c00u); // Signed Inf
  } else if (hes <= 0) { // Underflow
    uint16_t hm;
    if ((14 - hes) > 24) { // Mantissa shifted all the way off & no rounding possibility
      hm = (uint16_t)0u; // Set mantissa to zero
    } else {
      xm |= 0x00800000u; // Add the hidden leading bit
      hm = (uint16_t)(xm >> (14 - hes)); // Mantissa
      if ((xm >> (13 - hes)) & 1u) { // Check for rounding
        hm += (uint16_t)1u; // Round, might overflow into exp bit, but this is OK
      }
    }
    return (hs | hm); // Combine sign bit and mantissa bits, biased exponent is zero
  } else {
    uint16_t he = (uint16_t)(hes << 10); // Exponent
    uint16_t hm = (uint16_t)(xm >> 13); // Mantissa
    if (xm & 0x1000u) { // Check for rounding
      return (hs | he | hm) + (uint16_t)1u; // Round, might overflow to inf, this is OK
    } else {
      return (hs | he | hm); // No rounding
    }
  }
}

typedef union {
        uint32_t i;
        float    f;
} int_float;

uint16_t f32_to_f16_tursa(float f)
{
    int_float value;
    value.f = f;
    return tursa_floatbits_to_halfbits(value.i);
}

void f32_to_f16_buffer_tursa(uint32_t *data, uint16_t *result, int data_size)
{
    int_float value;
    for (int i =0; i < data_size; i++) {
        value.i = data[i];
        result[i] = tursa_floatbits_to_halfbits(value.i);
    }
}
