#include <format>
#include <nbytes.h>
#include <vector>

#include <gtest/gtest.h>

TEST(basic, swap_bytes16) {
  std::vector<char> input = {1, 2, 3, 4, 5, 6, 7, 8};
  nbytes::SwapBytes16(input.data(), input.size());
  std::vector<char> expected = {2, 1, 4, 3, 6, 5, 8, 7};
  EXPECT_EQ(input, expected);
  SUCCEED();
}

TEST(basic, swap_bytes32) {
  std::vector<char> input = {1, 2, 3, 4, 5, 6, 7, 8};
  nbytes::SwapBytes32(input.data(), input.size());
  std::vector<char> expected = {4, 3, 2, 1, 8, 7, 6, 5};
  EXPECT_EQ(input, expected);
  SUCCEED();
}

TEST(basic, swap_bytes64) {
  std::vector<char> input = {1, 2, 3, 4, 5, 6, 7, 8};
  nbytes::SwapBytes64(input.data(), input.size());
  std::vector<char> expected = {8, 7, 6, 5, 4, 3, 2, 1};
  EXPECT_EQ(input, expected);
  SUCCEED();
}

TEST(base64, decode_two_byte_ignores_non_latin1) {
  // U+4E2D has the low byte 0x2D ('-'); it must not decode as a base64 digit.
  const char16_t input[] = u"QUJD中REVG";
  char out[16] = {};
  size_t n = nbytes::Base64Decode(out, sizeof(out), input, 9);
  EXPECT_EQ(std::string(out, n), "ABCDEF");
}

TEST(base64, decode_two_byte_does_not_stop_on_low_byte_padding) {
  // U+013D has the low byte 0x3D ('='); it must not end the decoding.
  const char16_t input[] = u"QUJDĽREVG";
  char out[16] = {};
  size_t n = nbytes::Base64Decode(out, sizeof(out), input, 9);
  EXPECT_EQ(std::string(out, n), "ABCDEF");
}

TEST(base64, decode_two_byte_all_non_latin1_is_empty) {
  // Four U+4E2D units form a full group in the fast path; the output must
  // still be empty rather than the bytes of "----".
  const char16_t input[] = u"中中中中";
  char out[16] = {};
  size_t n = nbytes::Base64Decode(out, sizeof(out), input, 4);
  EXPECT_EQ(n, 0u);
}

TEST(hex, decode_two_byte_rejects_non_latin1) {
  // U+0141 has the low byte 0x41 ('A'); it is not a hex digit.
  const char16_t input[] = u"ŁŁ";
  char out[4] = {};
  size_t n = nbytes::HexDecode(out, sizeof(out), input, 2);
  EXPECT_EQ(n, 0u);
}

namespace {
int RefUnhex(unsigned char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

std::string RefHexDecode(const std::string &src, size_t len) {
  std::string out;
  for (size_t i = 0; i < len && i * 2 + 1 < src.size(); ++i) {
    int a = RefUnhex(src[i * 2]);
    int b = RefUnhex(src[i * 2 + 1]);
    if (a < 0 || b < 0) break;
    out.push_back(static_cast<char>((a << 4) | b));
  }
  return out;
}

std::string HexDecodeToString(const std::string &src, size_t len) {
  std::vector<char> out(len + 1);
  size_t n = nbytes::HexDecode(out.data(), len, src.data(), src.size());
  return std::string(out.data(), n);
}

std::string RandomHex(size_t bytes, uint32_t &seed, bool upper) {
  static const char lower_digits[] = "0123456789abcdef";
  static const char upper_digits[] = "0123456789ABCDEF";
  const char *digits = upper ? upper_digits : lower_digits;
  std::string s;
  for (size_t i = 0; i < bytes * 2; ++i) {
    seed = seed * 1103515245 + 12345;
    s.push_back(digits[(seed >> 16) & 15]);
  }
  return s;
}
}  // namespace

TEST(hex, decode_matches_reference_for_all_lengths) {
  uint32_t seed = 42;
  for (size_t bytes = 0; bytes <= 160; ++bytes) {
    for (bool upper : {false, true}) {
      std::string src = RandomHex(bytes, seed, upper);
      EXPECT_EQ(HexDecodeToString(src, bytes), RefHexDecode(src, bytes))
          << "bytes=" << bytes << " upper=" << upper;
    }
  }
}

TEST(hex, decode_stops_at_every_invalid_byte_in_every_position) {
  uint32_t seed = 7;
  const std::string base = RandomHex(48, seed, false);
  for (size_t pos = 0; pos < base.size(); ++pos) {
    for (int v = 0; v < 256; ++v) {
      std::string src = base;
      src[pos] = static_cast<char>(v);
      ASSERT_EQ(HexDecodeToString(src, 48), RefHexDecode(src, 48))
          << "pos=" << pos << " byte=" << v;
    }
  }
}

TEST(hex, decode_respects_output_length_and_odd_input) {
  uint32_t seed = 3;
  const std::string src = RandomHex(64, seed, false);
  for (size_t len = 0; len <= 64; ++len) {
    EXPECT_EQ(HexDecodeToString(src, len), RefHexDecode(src, len));
  }
  for (size_t cut = 0; cut <= src.size(); ++cut) {
    std::string odd = src.substr(0, cut);
    EXPECT_EQ(HexDecodeToString(odd, 64), RefHexDecode(odd, 64))
        << "cut=" << cut;
  }
}
