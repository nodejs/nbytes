#include <format>
#include <nbytes.h>
#include <vector>

#include <gtest/gtest.h>

TEST(basic, swap_bytes16) {
  std::vector<char> input = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  nbytes::SwapBytes16(input.data(), input.size());
  std::vector<char> expected = {2, 1, 4, 3, 6, 5, 8, 7, 10, 9, 12, 11, 14, 13, 16, 15};
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
