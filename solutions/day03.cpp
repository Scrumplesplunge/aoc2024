#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <cstring>
#include <print>

namespace aoc2024 {
namespace {

// Either consumes a prefix of `input` matching `required` and returns true, or
// returns false without modifying `input`.
bool ConsumePrefix(std::string_view& input, std::string_view prefix) {
  if (!input.starts_with(prefix)) return false;
  input.remove_prefix(prefix.size());
  return true;
}

// Either consumes a single character of `input` matching `c` and returns true,
// or returns false without modifying `input`.
bool ConsumeChar(std::string_view& input, char c) {
  if (input.empty() || input[0] != c) return false;
  input.remove_prefix(1);
  return true;
}

// Either consumes an int at the prefix of `input` and returns true, or returns
// false without modifying `input`.
bool ConsumeInt(std::string_view& input, int& value) {
  const auto [end, error] = std::from_chars(
      input.data(), input.data() + input.size(), value);
  if (error != std::errc()) return false;
  input.remove_prefix(end - input.data());
  return true;
}

}  // namespace

Task<void> Day03(tcp::Socket& socket) {
  char buffer[20000];
  std::string_view input(co_await socket.Read(buffer));
  assert(input.size() < 20000);  // If we hit 20k then we may have truncated.

  bool enable = true;
  int part1 = 0;
  int part2 = 0;
  while (true) {
    // Skip to the next possible starting position.
    const auto i = input.find_first_of("dm");
    if (i == input.npos) break;  // No more matches.
    input.remove_prefix(i);
    if (ConsumePrefix(input, "do()")) {
      enable = true;
    } else if (ConsumePrefix(input, "don't()")) {
      enable = false;
    } else if (int a, b; ConsumePrefix(input, "mul(") && ConsumeInt(input, a) &&
                         ConsumeChar(input, ',') && ConsumeInt(input, b) &&
                         ConsumeChar(input, ')')) {
      part1 += a * b;
      if (enable) part2 += a * b;
    } else {
      // No match at the starting position. Search again starting after it.
      input.remove_prefix(1);
    }
  }

  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
