#ifndef INTRINSICS_H
#define INTRINSICS_H

#include <immintrin.h>
#include <QtGlobal>

union f4vector
{
    float f[4];
    __m128 v;
};

union ui4vector
{
    quint32 i[4];
    __m128i v;
};

union i4vector
{
    qint32 i[4];
    __m128i v;
};

union ui8vector
{
    quint16 i[8];
    __m128i v;
};

union i8vector
{
    qint16 i[8];
    __m128i v;
};

union ui16vector
{
    quint8 i[16];
    __m128i v;
};

union i16vector
{
    qint8 i[16];
    __m128i v;
};

typedef float a_float_vec[ 4 ] __attribute__((__aligned__(16)));
typedef qint32 a_int32_vec[ 4 ] __attribute__((__aligned__(16)));
typedef qint16 a_int16_vec[ 8 ] __attribute__((__aligned__(16)));
typedef char a16char __attribute__((__aligned__(16)));

#if !defined (__SSE4_2__) || !defined (__SSE4_1__)
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__)) _mm_min_epi32(const __m128i &a, const __m128i &b)
{
    __m128i a8;
    __m128i b8;
    __m128i min8;
    a8 = _mm_packs_epi32(a, a);
    b8 = _mm_packs_epi32(b, b);
    min8 = _mm_min_epi16(a8, b8);
    return _mm_unpackhi_epi16(min8, _mm_setzero_si128());
}

extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__)) _mm_max_epi32(const __m128i &a, const __m128i &b)
{
    __m128i a8;
    __m128i b8;
    __m128i max8;
    a8 = _mm_packs_epi32(a, a);
    b8 = _mm_packs_epi32(b, b);
    max8 = _mm_max_epi16(a8, b8);
    return _mm_unpackhi_epi16(max8, _mm_setzero_si128());
}

extern __inline __attribute__((__gnu_inline__, __always_inline__, __artificial__))  __m128i _mm_mullo_epi322(const __m128i &a, const __m128i &b)
{
    __m128i tmp1 = _mm_mul_epu32(a, b);
    __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(a, 4), _mm_srli_si128(b, 4));
    return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0, 0, 2, 0)), _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0, 0, 2, 0)));
}

#endif

extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__)) _mm_sel_si128(const __m128i &a, const __m128i &b, const __m128i &mask)
{
    // (((b ^ a) & mask)^a)
    return _mm_xor_si128(a, _mm_and_si128(mask, _mm_xor_si128(b, a)));
}

extern __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__)) _mm_sel_ps(const __m128 &a, const __m128 &b, const __m128 &mask)
{
    // (((b ^ a) & mask)^a)
    return _mm_xor_ps(a, _mm_and_ps(mask, _mm_xor_ps(b, a)));
}

#endif // INTRINSICS_H
