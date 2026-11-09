#include "nbytes.h"
#include <cmath>
#include <cstddef>
#include <string.h>

#if defined(__ARM_NEON) || defined(_M_ARM64)
#include <arm_neon.h>
#define NBYTES_HEX_NEON 1
#elif defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#define NBYTES_HEX_SSE2 1
#endif

namespace nbytes {

// ============================================================================
// Byte Swapping

namespace {
// These are defined by <sys/byteorder.h> or <netinet/in.h> on some systems.
// To avoid warnings, undefine them before redefining them.
#ifdef BSWAP_2
#undef BSWAP_2
#endif
#ifdef BSWAP_4
#undef BSWAP_4
#endif
#ifdef BSWAP_8
#undef BSWAP_8
#endif

#if defined(_MSC_VER)
#include <intrin.h>
#define BSWAP_2(x) _byteswap_ushort(x)
#define BSWAP_4(x) _byteswap_ulong(x)
#define BSWAP_8(x) _byteswap_uint64(x)
#else
#define BSWAP_2(x) ((x) << 8) | ((x) >> 8)
#define BSWAP_4(x)                                                       \
  (((x) & 0xFF) << 24) | (((x) & 0xFF00) << 8) | (((x) >> 8) & 0xFF00) | \
      (((x) >> 24) & 0xFF)
#define BSWAP_8(x)                            \
  (((x) & 0xFF00000000000000ull) >> 56) |     \
      (((x) & 0x00FF000000000000ull) >> 40) | \
      (((x) & 0x0000FF0000000000ull) >> 24) | \
      (((x) & 0x000000FF00000000ull) >> 8) |  \
      (((x) & 0x00000000FF000000ull) << 8) |  \
      (((x) & 0x0000000000FF0000ull) << 24) | \
      (((x) & 0x000000000000FF00ull) << 40) | \
      (((x) & 0x00000000000000FFull) << 56)
#endif
}  // namespace

bool SwapBytes16(char *data, size_t nbytes) {
  if (nbytes % sizeof(uint16_t) != 0) return false;

#if defined(_MSC_VER)
  if (AlignUp(data, sizeof(uint16_t)) == data) {
    // MSVC has no strict aliasing, and is able to highly optimize this case.
    uint16_t *data16 = reinterpret_cast<uint16_t *>(data);
    size_t len16 = nbytes / sizeof(uint16_t);
    for (size_t i = 0; i < len16; i++) {
      data16[i] = BSWAP_2(data16[i]);
    }
    return true;
  }
#endif

  uint16_t temp;
  for (size_t i = 0; i < nbytes; i += sizeof(temp)) {
    memcpy(&temp, &data[i], sizeof(temp));
    temp = BSWAP_2(temp);
    memcpy(&data[i], &temp, sizeof(temp));
  }

  return true;
}

bool SwapBytes32(char *data, size_t nbytes) {
  if (nbytes % sizeof(uint32_t) != 0) return false;

#if defined(_MSC_VER)
  // MSVC has no strict aliasing, and is able to highly optimize this case.
  if (AlignUp(data, sizeof(uint32_t)) == data) {
    uint32_t *data32 = reinterpret_cast<uint32_t *>(data);
    size_t len32 = nbytes / sizeof(uint32_t);
    for (size_t i = 0; i < len32; i++) {
      data32[i] = BSWAP_4(data32[i]);
    }
    return true;
  }
#endif

  uint32_t temp;
  for (size_t i = 0; i < nbytes; i += sizeof(temp)) {
    memcpy(&temp, &data[i], sizeof(temp));
    temp = BSWAP_4(temp);
    memcpy(&data[i], &temp, sizeof(temp));
  }

  return true;
}

bool SwapBytes64(char *data, size_t nbytes) {
  if (nbytes % sizeof(uint64_t) != 0) return false;

#if defined(_MSC_VER)
  if (AlignUp(data, sizeof(uint64_t)) == data) {
    // MSVC has no strict aliasing, and is able to highly optimize this case.
    uint64_t *data64 = reinterpret_cast<uint64_t *>(data);
    size_t len64 = nbytes / sizeof(uint64_t);
    for (size_t i = 0; i < len64; i++) {
      data64[i] = BSWAP_8(data64[i]);
    }
    return true;
  }
#endif

  uint64_t temp;
  for (size_t i = 0; i < nbytes; i += sizeof(temp)) {
    memcpy(&temp, &data[i], sizeof(temp));
    temp = BSWAP_8(temp);
    memcpy(&data[i], &temp, sizeof(temp));
  }

  return true;
}

// ============================================================================
// Base64 (legacy)

// supports regular and URL-safe base64
const int8_t unbase64_table[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -1, -1, -2, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, 62, -1, 62, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1,
    63, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
    43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1};

// ============================================================================
// Hex

const int8_t unhex_table[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,
    9,  -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1};

#if defined(NBYTES_HEX_NEON)
namespace {
inline bool HexDecodeBlock(const char *src, char *dst) {
  const uint8x16x2_t c = vld2q_u8(reinterpret_cast<const uint8_t *>(src));
  uint8x16_t nib[2];
  uint8x16_t valid = vdupq_n_u8(0xFF);
  for (int k = 0; k < 2; k++) {
    const uint8x16_t d = vsubq_u8(c.val[k], vdupq_n_u8('0'));
    const uint8x16_t l =
        vsubq_u8(vorrq_u8(c.val[k], vdupq_n_u8(0x20)), vdupq_n_u8('a'));
    const uint8x16_t is_digit = vcltq_u8(d, vdupq_n_u8(10));
    const uint8x16_t is_alpha = vcltq_u8(l, vdupq_n_u8(6));
    valid = vandq_u8(valid, vorrq_u8(is_digit, is_alpha));
    nib[k] = vbslq_u8(is_digit, d, vaddq_u8(l, vdupq_n_u8(10)));
  }
  if (vminvq_u8(valid) == 0) return false;
  vst1q_u8(reinterpret_cast<uint8_t *>(dst),
           vorrq_u8(vshlq_n_u8(nib[0], 4), nib[1]));
  return true;
}
}  // namespace
#elif defined(NBYTES_HEX_SSE2)
namespace {
inline __m128i HexNibbles(__m128i c, int *mask) {
  const __m128i d = _mm_sub_epi8(c, _mm_set1_epi8('0'));
  const __m128i l =
      _mm_sub_epi8(_mm_or_si128(c, _mm_set1_epi8(0x20)), _mm_set1_epi8('a'));
  const __m128i is_digit = _mm_cmpeq_epi8(_mm_min_epu8(d, _mm_set1_epi8(9)), d);
  const __m128i is_alpha = _mm_cmpeq_epi8(_mm_min_epu8(l, _mm_set1_epi8(5)), l);
  *mask = _mm_movemask_epi8(_mm_or_si128(is_digit, is_alpha));
  return _mm_or_si128(
      _mm_and_si128(is_digit, d),
      _mm_andnot_si128(is_digit, _mm_add_epi8(l, _mm_set1_epi8(10))));
}

inline __m128i HexPack(__m128i n) {
  const __m128i hi = _mm_and_si128(n, _mm_set1_epi16(0x00FF));
  const __m128i lo = _mm_srli_epi16(n, 8);
  return _mm_or_si128(_mm_slli_epi16(hi, 4), lo);
}

inline bool HexDecodeBlock(const char *src, char *dst) {
  int m0, m1;
  const __m128i n0 =
      HexNibbles(_mm_loadu_si128(reinterpret_cast<const __m128i *>(src)), &m0);
  const __m128i n1 = HexNibbles(
      _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 16)), &m1);
  if ((m0 & m1) != 0xFFFF) return false;
  _mm_storeu_si128(reinterpret_cast<__m128i *>(dst),
                   _mm_packus_epi16(HexPack(n0), HexPack(n1)));
  return true;
}
}  // namespace
#endif

size_t HexDecodeBlocks(char *buf, size_t len, const char *src, size_t srcLen) {
  size_t i = 0;
#if defined(NBYTES_HEX_NEON) || defined(NBYTES_HEX_SSE2)
  const size_t n = std::min(len, srcLen / 2);
  for (; i + 16 <= n; i += 16) {
    if (!HexDecodeBlock(src + i * 2, buf + i)) break;
  }
#endif
  return i;
}

inline constexpr char nibble(uint8_t x) {
  uint8_t add = (x >= 10) ? ('a' - 10) : '0';
  return x + add;
}

size_t HexEncode(const char *src, size_t slen, char *dst, size_t dlen) {
  // We know how much we'll write, just make sure that there's space.
  NBYTES_ASSERT_TRUE(dlen >= MultiplyWithOverflowCheck<size_t>(slen, 2u) &&
                     "not enough space provided for hex encode");

  dlen = slen * 2;
  for (size_t i = 0, k = 0; k < dlen; i += 1, k += 2) {
    uint8_t val = static_cast<uint8_t>(src[i]);
    dst[k + 0] = nibble(val >> 4);
    dst[k + 1] = nibble(val & 15);
  }

  return dlen;
}

std::string HexEncode(const char *src, size_t slen) {
  size_t dlen = slen * 2;
  std::string dst(dlen, '\0');
  HexEncode(src, slen, dst.data(), dlen);
  return dst;
}

// ============================================================================

void ForceAsciiSlow(const char *src, char *dst, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    dst[i] = src[i] & 0x7f;
  }
}

void ForceAscii(const char *src, char *dst, size_t len) {
  if (len < 16) {
    ForceAsciiSlow(src, dst, len);
    return;
  }

  const unsigned bytes_per_word = sizeof(uintptr_t);
  const unsigned align_mask = bytes_per_word - 1;
  const unsigned src_unalign = reinterpret_cast<uintptr_t>(src) & align_mask;
  const unsigned dst_unalign = reinterpret_cast<uintptr_t>(dst) & align_mask;

  if (src_unalign > 0) {
    if (src_unalign == dst_unalign) {
      const unsigned unalign = bytes_per_word - src_unalign;
      ForceAsciiSlow(src, dst, unalign);
      src += unalign;
      dst += unalign;
      len -= unalign;
    } else {
      ForceAsciiSlow(src, dst, len);
      return;
    }
  }

#if defined(_WIN64) || defined(_LP64)
  const uintptr_t mask = ~0x8080808080808080ll;
#else
  const uintptr_t mask = ~0x80808080l;
#endif

  const uintptr_t *srcw = reinterpret_cast<const uintptr_t *>(src);
  uintptr_t *dstw = reinterpret_cast<uintptr_t *>(dst);

  for (size_t i = 0, n = len / bytes_per_word; i < n; ++i) {
    dstw[i] = srcw[i] & mask;
  }

  const unsigned remainder = len & align_mask;
  if (remainder > 0) {
    const size_t offset = len - remainder;
    ForceAsciiSlow(src + offset, dst + offset, remainder);
  }
}

}  // namespace nbytes
