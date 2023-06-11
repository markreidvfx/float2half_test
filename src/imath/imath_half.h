//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.
//

//
// Primary original authors:
//     Florian Kainz <kainz@ilm.com>
//     Rod Bogart <rgb@ilm.com>
//

#ifndef IMATH_HALF_H_
#define IMATH_HALF_H_

/// @file half.h
/// The half type is a 16-bit floating number, compatible with the
/// IEEE 754-2008 binary16 type.
///
/// **Representation of a 32-bit float:**
///
/// We assume that a float, f, is an IEEE 754 single-precision
/// floating point number, whose bits are arranged as follows:
///
///     31 (msb)
///     |
///     | 30     23
///     | |      |
///     | |      | 22                    0 (lsb)
///     | |      | |                     |
///     X XXXXXXXX XXXXXXXXXXXXXXXXXXXXXXX
///
///     s e        m
///
/// S is the sign-bit, e is the exponent and m is the significand.
///
/// If e is between 1 and 254, f is a normalized number:
///
///             s    e-127
///     f = (-1)  * 2      * 1.m
///
/// If e is 0, and m is not zero, f is a denormalized number:
///
///             s    -126
///     f = (-1)  * 2      * 0.m
///
/// If e and m are both zero, f is zero:
///
///     f = 0.0
///
/// If e is 255, f is an "infinity" or "not a number" (NAN),
/// depending on whether m is zero or not.
///
/// Examples:
///
///     0 00000000 00000000000000000000000 = 0.0
///     0 01111110 00000000000000000000000 = 0.5
///     0 01111111 00000000000000000000000 = 1.0
///     0 10000000 00000000000000000000000 = 2.0
///     0 10000000 10000000000000000000000 = 3.0
///     1 10000101 11110000010000000000000 = -124.0625
///     0 11111111 00000000000000000000000 = +infinity
///     1 11111111 00000000000000000000000 = -infinity
///     0 11111111 10000000000000000000000 = NAN
///     1 11111111 11111111111111111111111 = NAN
///
/// **Representation of a 16-bit half:**
///
/// Here is the bit-layout for a half number, h:
///
///     15 (msb)
///     |
///     | 14  10
///     | |   |
///     | |   | 9        0 (lsb)
///     | |   | |        |
///     X XXXXX XXXXXXXXXX
///
///     s e     m
///
/// S is the sign-bit, e is the exponent and m is the significand.
///
/// If e is between 1 and 30, h is a normalized number:
///
///             s    e-15
///     h = (-1)  * 2     * 1.m
///
/// If e is 0, and m is not zero, h is a denormalized number:
///
///             S    -14
///     h = (-1)  * 2     * 0.m
///
/// If e and m are both zero, h is zero:
///
///     h = 0.0
///
/// If e is 31, h is an "infinity" or "not a number" (NAN),
/// depending on whether m is zero or not.
///
/// Examples:
///
///     0 00000 0000000000 = 0.0
///     0 01110 0000000000 = 0.5
///     0 01111 0000000000 = 1.0
///     0 10000 0000000000 = 2.0
///     0 10000 1000000000 = 3.0
///     1 10101 1111000001 = -124.0625
///     0 11111 0000000000 = +infinity
///     1 11111 0000000000 = -infinity
///     0 11111 1000000000 = NAN
///     1 11111 1111111111 = NAN
///
/// **Conversion via Lookup Table:**
///
/// Converting from half to float is performed by default using a
/// lookup table. There are only 65,536 different half numbers; each
/// of these numbers has been converted and stored in a table pointed
/// to by the ``imath_half_to_float_table`` pointer.
///
/// Prior to Imath v3.1, conversion from float to half was
/// accomplished with the help of an exponent look table, but this is
/// now replaced with explicit bit shifting.
///
/// **Conversion via Hardware:**
///
/// For Imath v3.1, the conversion routines have been extended to use
/// F16C SSE instructions whenever present and enabled by compiler
/// flags.
///
/// **Conversion via Bit-Shifting**
///
/// If F16C SSE instructions are not available, conversion can be
/// accomplished by a bit-shifting algorithm. For half-to-float
/// conversion, this is generally slower than the lookup table, but it
/// may be preferable when memory limits preclude storing of the
/// 65,536-entry lookup table.
///
/// The lookup table symbol is included in the compilation even if
/// ``IMATH_HALF_USE_LOOKUP_TABLE`` is false, because application code
/// using the exported ``half.h`` may choose to enable the use of the table.
///
/// An implementation can eliminate the table from compilation by
/// defining the ``IMATH_HALF_NO_LOOKUP_TABLE`` preprocessor symbol.
/// Simply add:
///
///     #define IMATH_HALF_NO_LOOKUP_TABLE
///
/// before including ``half.h``, or define the symbol on the compile
/// command line.
///
/// Furthermore, an implementation wishing to receive ``FE_OVERFLOW``
/// and ``FE_UNDERFLOW`` floating point exceptions when converting
/// float to half by the bit-shift algorithm can define the
/// preprocessor symbol ``IMATH_HALF_ENABLE_FP_EXCEPTIONS`` prior to
/// including ``half.h``:
///
///     #define IMATH_HALF_ENABLE_FP_EXCEPTIONS
///
/// **Conversion Performance Comparison:**
///
/// Testing on a Core i9, the timings are approximately:
///
/// half to float
/// - table: 0.71 ns / call
/// - no table: 1.06 ns / call
/// - f16c: 0.45 ns / call
///
/// float-to-half:
/// - original: 5.2 ns / call
/// - no exp table + opt: 1.27 ns / call
/// - f16c: 0.45 ns / call
///
/// **Note:** the timing above depends on the distribution of the
/// floats in question.
///

#include <stdint.h>
#include <stdio.h>


//-------------------------------------------------------------------------
// Limits
//
// Visual C++ will complain if HALF_DENORM_MIN, HALF_NRM_MIN etc. are not float
// constants, but at least one other compiler (gcc 2.96) produces incorrect
// results if they are.
//-------------------------------------------------------------------------

#define IMATH_UNLIKELY(a) a

#if (defined _WIN32 || defined _WIN64) && defined _MSC_VER

/// Smallest positive denormalized half
#    define HALF_DENORM_MIN 5.96046448e-08f
/// Smallest positive normalized half
#    define HALF_NRM_MIN 6.10351562e-05f
/// Smallest positive normalized half
#    define HALF_MIN 6.10351562e-05f
/// Largest positive half
#    define HALF_MAX 65504.0f
/// Smallest positive e for which ``half(1.0 + e) != half(1.0)``
#    define HALF_EPSILON 0.00097656f
#else
/// Smallest positive denormalized half
#    define HALF_DENORM_MIN 5.96046448e-08
/// Smallest positive normalized half
#    define HALF_NRM_MIN 6.10351562e-05
/// Smallest positive normalized half
#    define HALF_MIN 6.10351562e-05f
/// Largest positive half
#    define HALF_MAX 65504.0
/// Smallest positive e for which ``half(1.0 + e) != half(1.0)``
#    define HALF_EPSILON 0.00097656
#endif

/// Number of digits in mantissa (significand + hidden leading 1)
#define HALF_MANT_DIG 11
/// Number of base 10 digits that can be represented without change:
///
/// ``floor( (HALF_MANT_DIG - 1) * log10(2) ) => 3.01... -> 3``
#define HALF_DIG 3
/// Number of base-10 digits that are necessary to uniquely represent
/// all distinct values:
///
/// ``ceil(HALF_MANT_DIG * log10(2) + 1) => 4.31... -> 5``
#define HALF_DECIMAL_DIG 5
/// Base of the exponent
#define HALF_RADIX 2
/// Minimum negative integer such that ``HALF_RADIX`` raised to the power
/// of one less than that integer is a normalized half
#define HALF_DENORM_MIN_EXP -13
/// Maximum positive integer such that ``HALF_RADIX`` raised to the power
/// of one less than that integer is a normalized half
#define HALF_MAX_EXP 16
/// Minimum positive integer such that 10 raised to that power is a
/// normalized half
#define HALF_DENORM_MIN_10_EXP -4
/// Maximum positive integer such that 10 raised to that power is a
/// normalized half
#define HALF_MAX_10_EXP 4

/// a type for both C-only programs and C++ to use the same utilities
typedef union imath_half_uif
{
    uint32_t i;
    float    f;
} imath_half_uif_t;

/// a type for both C-only programs and C++ to use the same utilities
typedef uint16_t imath_half_bits_t;

static inline imath_half_bits_t
imath_float_to_half (float f)
{
    imath_half_uif_t  v;
    imath_half_bits_t ret;
    uint32_t          e, m, ui, r, shift;

    v.f = f;

    ui  = (v.i & ~0x80000000);
    ret = ((v.i >> 16) & 0x8000);

    // exponent large enough to result in a normal number, round and return
    if (ui >= 0x38800000)
    {
        // inf or nan
        if (IMATH_UNLIKELY (ui >= 0x7f800000))
        {
            ret |= 0x7c00;
            if (ui == 0x7f800000) return ret;
            m = (ui & 0x7fffff) >> 13;
            // make sure we have at least one bit after shift to preserve nan-ness
            return ret | (uint16_t) m | (uint16_t) (m == 0);
        }

        // too large, round to infinity
        if (IMATH_UNLIKELY (ui > 0x477fefff))
        {
#    ifdef IMATH_HALF_ENABLE_FP_EXCEPTIONS
            feraiseexcept (FE_OVERFLOW);
#    endif
            return ret | 0x7c00;
        }

        ui -= 0x38000000;
        ui = ((ui + 0x00000fff + ((ui >> 13) & 1)) >> 13);
        return ret | (uint16_t) ui;
    }

    // zero or flush to 0
    if (ui < 0x33000001)
    {
#    ifdef IMATH_HALF_ENABLE_FP_EXCEPTIONS
        if (ui == 0) return ret;
        feraiseexcept (FE_UNDERFLOW);
#    endif
        return ret;
    }

    // produce a denormalized half
    e     = (ui >> 23);
    shift = 0x7e - e;
    m     = 0x800000 | (ui & 0x7fffff);
    r     = m << (32 - shift);
    ret |= (m >> shift);
    if (r > 0x80000000 || (r == 0x80000000 && (ret & 0x1) != 0)) ++ret;
    return ret;
}

#endif // IMATH_HALF_H_