#include "cpython.h"
#include <math.h>
#include <stdio.h>
#include <assert.h>

// https://www.corsix.org/content/converting-fp32-to-fp16
// https://github.com/python/cpython/blob/5f7d68e48de19c5c3a241d7126fc2af227c2f74a/Objects/floatobject.c#L2071-L2173

static inline int PyFloat_Pack2(double x) {
  uint16_t sign, bits;
  int e;

  if (x == 0.0) {
    sign = (copysign(1.0, x) == -1.0);
    e = 0;
    bits = 0;
  } else if (isinf(x)) {
    sign = (x < 0.0);
    e = 0x1f;
    bits = 0;
  } else if (isnan(x)) {
    // There are 2046 distinct half-precision NaNs (1022 signaling
    // and 1024 quiet), but there are only two quiet NaNs that don't
    // arise by quieting a signaling NaN; we get those by setting
    // the topmost bit of the fraction field and clearing all other
    // fraction bits. We choose the one with the appropriate sign.
    sign = (copysign(1.0, x) == -1.0);
    e = 0x1f;
    bits = 0x200;
  } else {
    sign = (x < 0.0);
    if (sign) {
      x = -x;
    }

    double f = frexp(x, &e);
    // Normalize f to be in the range [1.0, 2.0)
    f *= 2.0;
    e--;

    if (e >= 16) {
      goto Overflow;
    } else if (e < -25) {
      // |x| < 2**-25. Underflow to zero.
      f = 0.0;
      e = 0;
    } else if (e < -14) {
      // |x| < 2**-14. Gradual underflow
      f = ldexp(f, 14 + e);
      e = 0;
    } else /* if (!(e == 0 && f == 0.0)) */ {
      e += 15;
      f -= 1.0; // Get rid of leading 1
    }

    f *= 1024.0; // 2**10
    // Round to even
    bits = (uint16_t)f; // Note the truncation
    assert(bits < 1024);
    assert(e < 31);
    if ((f - bits > 0.5) || ((f - bits == 0.5) && (bits % 2 == 1))) {
      if (++bits == 1024) {
        // The carry propagated out of a string of 10 1 bits.
        bits = 0;
        if (++e == 31) {
          goto Overflow;
        }
      }
    }
  }

  return (sign << 15) | (e << 10) | bits;
Overflow:
#if 0
  PyErr_SetString(PyExc_OverflowError,
                  "float too large to pack with e format");
#endif
    // printf("float too large to pack with e format\n");
  return -1;
}


typedef union {
        uint32_t i;
        float    f;
} int_float;

uint16_t f32_to_f16_cpython(float f)
{
    return (uint16_t)PyFloat_Pack2(f);
}

void f32_to_f16_buffer_cpython(uint32_t *data, uint16_t *result, int data_size)
{
    int_float value;
    for (int i =0; i < data_size; i++) {
        value.i = data[i];
        result[i] = PyFloat_Pack2(value.f);
    }
}
